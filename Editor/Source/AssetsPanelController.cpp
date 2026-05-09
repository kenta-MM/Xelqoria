#include "AssetsPanelController.h"

#include <algorithm>
#include <array>
#include <CommCtrl.h>
#include <cstdio>
#include <iterator>
#include <objbase.h>
#include <ShObjIdl.h>
#include <Shellapi.h>
#include <system_error>
#include <utility>
#include <Windows.h>
#include <wrl/client.h>

#include "EditorStringUtils.h"

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr int NameColumnIndex = 0;
        constexpr int ModifiedTimeColumnIndex = 1;
        constexpr int TypeColumnIndex = 2;
        constexpr int SizeColumnIndex = 3;
        constexpr int DragPreviewWidth = 220;
        constexpr int DragPreviewHeight = 64;
        constexpr int DragPreviewImageSize = 48;
        constexpr int DragPreviewCursorOffsetX = 14;
        constexpr int DragPreviewCursorOffsetY = 18;

        /// <summary>
        /// ディレクトリエントリをフォルダ優先、名前順で並べる。
        /// </summary>
        /// <param name="lhs">比較する左辺。</param>
        /// <param name="rhs">比較する右辺。</param>
        /// <returns>左辺を先に表示する場合は true。</returns>
        [[nodiscard]] bool CompareDirectoryEntry(
            const std::filesystem::directory_entry& lhs,
            const std::filesystem::directory_entry& rhs)
        {
            std::error_code lhsErrorCode;
            std::error_code rhsErrorCode;
            const bool lhsIsDirectory = lhs.is_directory(lhsErrorCode);
            const bool rhsIsDirectory = rhs.is_directory(rhsErrorCode);
            if (lhsIsDirectory != rhsIsDirectory)
            {
                return lhsIsDirectory;
            }

            return lhs.path().filename().wstring() < rhs.path().filename().wstring();
        }

        /// <summary>
        /// ファイルパスの表示名を取得する。
        /// </summary>
        /// <param name="path">表示対象パス。</param>
        /// <returns>ファイル名。空の場合はパス文字列。</returns>
        [[nodiscard]] std::wstring GetDisplayName(const std::filesystem::path& path)
        {
            const std::wstring fileName = path.filename().wstring();
            if (false == fileName.empty())
            {
                return fileName;
            }

            return path.wstring();
        }

        /// <summary>
        /// FILETIME を表示用のローカル日時へ変換する。
        /// </summary>
        /// <param name="fileTime">変換する FILETIME。</param>
        /// <returns>日時表示文字列。</returns>
        [[nodiscard]] std::wstring FormatFileTime(const FILETIME& fileTime)
        {
            FILETIME localFileTime{};
            if (FALSE == FileTimeToLocalFileTime(&fileTime, &localFileTime))
            {
                return {};
            }

            SYSTEMTIME systemTime{};
            if (FALSE == FileTimeToSystemTime(&localFileTime, &systemTime))
            {
                return {};
            }

            wchar_t text[64]{};
            std::swprintf(
                text,
                std::size(text),
                L"%04u/%02u/%02u %02u:%02u",
                static_cast<unsigned>(systemTime.wYear),
                static_cast<unsigned>(systemTime.wMonth),
                static_cast<unsigned>(systemTime.wDay),
                static_cast<unsigned>(systemTime.wHour),
                static_cast<unsigned>(systemTime.wMinute));
            return text;
        }

        /// <summary>
        /// ファイルサイズを表示文字列へ変換する。
        /// </summary>
        /// <param name="fileSize">バイト単位のサイズ。</param>
        /// <returns>サイズ表示文字列。</returns>
        [[nodiscard]] std::wstring FormatFileSize(unsigned long long fileSize)
        {
            wchar_t text[64]{};
            if (fileSize < 1024ull)
            {
                std::swprintf(text, std::size(text), L"%llu B", fileSize);
                return text;
            }

            const unsigned long long kibibytes = (fileSize + 1023ull) / 1024ull;
            if (kibibytes < 1024ull)
            {
                std::swprintf(text, std::size(text), L"%llu KB", kibibytes);
                return text;
            }

            const unsigned long long mebibytes = (kibibytes + 1023ull) / 1024ull;
            std::swprintf(text, std::size(text), L"%llu MB", mebibytes);
            return text;
        }

        /// <summary>
        /// システムアイコン番号を取得する。
        /// </summary>
        /// <param name="path">対象パス。</param>
        /// <param name="fileAttributes">ファイル属性。</param>
        /// <param name="useFileAttributes">属性のみで判定する場合は true。</param>
        /// <returns>システムイメージリスト上のアイコン番号。</returns>
        [[nodiscard]] int GetSystemIconIndex(
            const std::filesystem::path& path,
            DWORD fileAttributes,
            bool useFileAttributes)
        {
            SHFILEINFOW fileInfo{};
            UINT flags = SHGFI_SYSICONINDEX | SHGFI_SMALLICON;
            if (useFileAttributes)
            {
                flags |= SHGFI_USEFILEATTRIBUTES;
            }

            SHGetFileInfoW(
                path.c_str(),
                fileAttributes,
                &fileInfo,
                sizeof(fileInfo),
                flags);
            return fileInfo.iIcon;
        }

        /// <summary>
        /// フォルダ用のシステムアイコン番号を取得する。
        /// </summary>
        /// <returns>システムイメージリスト上のフォルダアイコン番号。</returns>
        [[nodiscard]] int GetFolderIconIndex()
        {
            return GetSystemIconIndex(
                std::filesystem::path(L"folder"),
                FILE_ATTRIBUTE_DIRECTORY,
                true);
        }

        /// <summary>
        /// Windows の種類表示名を取得する。
        /// </summary>
        /// <param name="path">対象パス。</param>
        /// <param name="fileAttributes">ファイル属性。</param>
        /// <param name="useFileAttributes">属性のみで判定する場合は true。</param>
        /// <returns>種類表示名。</returns>
        [[nodiscard]] std::wstring GetTypeName(
            const std::filesystem::path& path,
            DWORD fileAttributes,
            bool useFileAttributes)
        {
            SHFILEINFOW fileInfo{};
            UINT flags = SHGFI_TYPENAME;
            if (useFileAttributes)
            {
                flags |= SHGFI_USEFILEATTRIBUTES;
            }

            if (0 == SHGetFileInfoW(
                    path.c_str(),
                    fileAttributes,
                    &fileInfo,
                    sizeof(fileInfo),
                    flags))
            {
                return {};
            }

            return fileInfo.szTypeName;
        }

        /// <summary>
        /// ファイルシステム項目から表示データを構築する。
        /// </summary>
        /// <param name="entry">対象ディレクトリエントリ。</param>
        /// <returns>表示データ。</returns>
        [[nodiscard]] AssetListEntry BuildEntry(const std::filesystem::directory_entry& entry)
        {
            AssetListEntry result{};
            result.path = entry.path();
            result.displayName = GetDisplayName(entry.path());

            WIN32_FILE_ATTRIBUTE_DATA attributeData{};
            if (FALSE == GetFileAttributesExW(entry.path().c_str(), GetFileExInfoStandard, &attributeData))
            {
                result.typeName = L"Unknown";
                return result;
            }

            result.isDirectory = 0 != (attributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
            result.modifiedTimeText = FormatFileTime(attributeData.ftLastWriteTime);
            result.iconIndex = result.isDirectory
                ? GetFolderIconIndex()
                : GetSystemIconIndex(entry.path(), attributeData.dwFileAttributes, false);
            result.typeName = result.isDirectory
                ? L"ファイル フォルダー"
                : GetTypeName(entry.path(), attributeData.dwFileAttributes, false);
            if (false == result.isDirectory)
            {
                const unsigned long long fileSize =
                    (static_cast<unsigned long long>(attributeData.nFileSizeHigh) << 32)
                    | static_cast<unsigned long long>(attributeData.nFileSizeLow);
                result.sizeText = FormatFileSize(fileSize);
            }

            return result;
        }

        /// <summary>
        /// Assets の右クリックメニューから実行されたコマンド ID を表す。
        /// </summary>
        constexpr UINT_PTR CreateSpriteMenuCommandId = 1;

        /// <summary>
        /// Assets 項目の削除メニューから実行されたコマンド ID を表す。
        /// </summary>
        constexpr UINT_PTR DeleteEntryMenuCommandId = 2;
    }

    void AssetsPanelController::Bind(const EditorShell& shell)
    {
        m_assetsListView = shell.GetAssetsListView();
        m_assetsSummaryLabel = shell.GetAssetsSummaryLabel();
        InitializeListView();
    }

    void AssetsPanelController::Refresh(const std::optional<EditorProjectInfo>& projectInfo)
    {
        if (false == projectInfo.has_value())
        {
            m_assetsRootDirectory.clear();
            m_currentDirectory.clear();
            m_visibleEntries.clear();
            m_selectedFilePath.clear();
            m_selectedSpriteAssetId = {};
            m_draggingSpriteAssetId = {};
            m_draggingTextureAssetId = {};
            m_draggingImagePath.clear();
            m_canPlaceDraggingAssetInScene = false;
            m_createSpriteRequested = false;
            m_createSpriteTargetDirectory.clear();
            EndDragImage();
            RefreshListView();
            RefreshSummaryLabel();
            return;
        }

        const std::filesystem::path rootDirectory = projectInfo->projectFilePath.parent_path();
        if (rootDirectory != m_assetsRootDirectory)
        {
            m_assetsRootDirectory = rootDirectory;
            m_currentDirectory = rootDirectory;
            m_selectedFilePath.clear();
            m_lastClickTick = 0;
            m_lastClickedIndex = -1;
        }

        RebuildVisibleEntries();
        RefreshListView();
        RefreshSummaryLabel();
    }

    void AssetsPanelController::SyncSelection()
    {
        SyncSelectedPathFromListView();
    }

    bool AssetsPanelController::HandleNotify(LPARAM notifyParameter)
    {
        if (0 == notifyParameter || nullptr == m_assetsListView)
        {
            return false;
        }

        const NMHDR* notifyHeader = reinterpret_cast<NMHDR*>(notifyParameter);
        if (notifyHeader->hwndFrom != m_assetsListView)
        {
            return false;
        }

        if (notifyHeader->code == NM_DBLCLK)
        {
            const NMITEMACTIVATE* itemActivate = reinterpret_cast<NMITEMACTIVATE*>(notifyParameter);
            if (0 <= itemActivate->iItem)
            {
                return TryOpenEntry(static_cast<std::size_t>(itemActivate->iItem));
            }

            return false;
        }

        if (notifyHeader->code == NM_RCLICK)
        {
            const NMITEMACTIVATE* itemActivate = reinterpret_cast<NMITEMACTIVATE*>(notifyParameter);
            POINT menuPoint{};
            GetCursorPos(&menuPoint);

            const int nameLabelIndex = HitTestListViewNameLabel(menuPoint);
            if (0 <= nameLabelIndex)
            {
                return ShowEntryContextMenu(static_cast<std::size_t>(nameLabelIndex), menuPoint);
            }

            std::filesystem::path createSpriteTargetDirectory = m_assetsRootDirectory;
            if (0 <= itemActivate->iItem)
            {
                const std::size_t entryIndex = static_cast<std::size_t>(itemActivate->iItem);
                if (entryIndex >= m_visibleEntries.size())
                {
                    return false;
                }

                const AssetListEntry& entry = m_visibleEntries[entryIndex];
                if (false == entry.isDirectory || entry.isParentLink)
                {
                    return false;
                }

                createSpriteTargetDirectory = entry.path;
            }

            if (true == createSpriteTargetDirectory.empty())
            {
                return false;
            }

            return ShowCreateSpriteContextMenu(createSpriteTargetDirectory, menuPoint);
        }

        if (notifyHeader->code == LVN_KEYDOWN)
        {
            const NMLVKEYDOWN* keyDown = reinterpret_cast<NMLVKEYDOWN*>(notifyParameter);
            if (keyDown->wVKey == VK_RETURN)
            {
                return TryOpenSelectedEntry();
            }
        }

        if (notifyHeader->code == LVN_ITEMCHANGED)
        {
            const NMLISTVIEW* listView = reinterpret_cast<NMLISTVIEW*>(notifyParameter);
            const bool becameSelected = 0 != (listView->uChanged & LVIF_STATE)
                && 0 == (listView->uOldState & LVIS_SELECTED)
                && 0 != (listView->uNewState & LVIS_SELECTED);
            if (becameSelected)
            {
                SyncSelectedPathFromListView();
            }
        }

        return false;
    }

    void AssetsPanelController::UpdateDragState(const Core::InputSnapshot& inputSnapshot)
    {
        if (nullptr == m_assetsListView)
        {
            return;
        }

        if (inputSnapshot.WasKeyPressed(VK_RETURN) && GetFocus() == m_assetsListView)
        {
            (void)TryOpenSelectedEntry();
            return;
        }

        m_assetDragReleasedThisFrame = false;
        if (false == inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left))
        {
            if (m_isAssetDragActive)
            {
                m_assetDragReleasedThisFrame = true;
            }

            EndDragImage();
            m_isAssetDragActive = false;
            return;
        }

        if (m_isAssetDragActive)
        {
            MoveDragImage(inputSnapshot.GetCursorScreenPoint());
        }

        if (false == inputSnapshot.WasMouseButtonPressed(Core::MouseButton::Left))
        {
            return;
        }

        const int hitIndex = HitTestListView(inputSnapshot.GetCursorScreenPoint());
        if (hitIndex < 0)
        {
            return;
        }

        ListView_SetItemState(
            m_assetsListView,
            hitIndex,
            LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED);
        SetFocus(m_assetsListView);
        SyncSelectedPathFromListView();

        const AssetListEntry& hitEntry = m_visibleEntries[static_cast<std::size_t>(hitIndex)];
        if (false == hitEntry.isDirectory && IsTextureImageFile(hitEntry.path))
        {
            m_draggingImagePath = hitEntry.path;
            m_draggingTextureAssetId = BuildTextureAssetId(hitEntry.path);
            m_draggingSpriteAssetId = BuildSpriteAssetId(hitEntry.path);
            m_isAssetDragActive = false == m_draggingSpriteAssetId.IsEmpty();
            m_canPlaceDraggingAssetInScene = false;
            if (m_isAssetDragActive)
            {
                BeginDragImage(hitEntry.path, hitEntry.iconIndex, inputSnapshot.GetCursorScreenPoint());
            }
        }
        else
        {
            m_draggingImagePath.clear();
            m_draggingTextureAssetId = {};
            m_draggingSpriteAssetId = {};
            m_isAssetDragActive = false;
            m_canPlaceDraggingAssetInScene = false;
            EndDragImage();
        }

        const ULONGLONG currentTick = GetTickCount64();
        const bool isDoubleClick = hitIndex == m_lastClickedIndex
            && currentTick - m_lastClickTick <= static_cast<ULONGLONG>(GetDoubleClickTime());
        m_lastClickedIndex = hitIndex;
        m_lastClickTick = currentTick;

        if (isDoubleClick)
        {
            (void)TryOpenEntry(static_cast<std::size_t>(hitIndex));
        }
    }

    void AssetsPanelController::CompleteReleasedDrag()
    {
        m_draggingSpriteAssetId = {};
        m_draggingTextureAssetId = {};
        m_draggingImagePath.clear();
        m_canPlaceDraggingAssetInScene = false;
        EndDragImage();
        RefreshSummaryLabel();
    }

    const Core::AssetId& AssetsPanelController::GetSelectedAssetId() const
    {
        return m_selectedSpriteAssetId;
    }

    const Core::AssetId& AssetsPanelController::GetDraggingSpriteAssetId() const
    {
        return m_draggingSpriteAssetId;
    }

    const Core::AssetId& AssetsPanelController::GetDraggingTextureAssetId() const
    {
        return m_draggingTextureAssetId;
    }

    const std::filesystem::path& AssetsPanelController::GetDraggingImagePath() const
    {
        return m_draggingImagePath;
    }

    bool AssetsPanelController::IsDragActive() const
    {
        return m_isAssetDragActive;
    }

    bool AssetsPanelController::CanPlaceDraggingAssetInScene() const
    {
        return m_canPlaceDraggingAssetInScene;
    }

    bool AssetsPanelController::WasDragReleasedThisFrame() const
    {
        return m_assetDragReleasedThisFrame;
    }

    bool AssetsPanelController::HasVisibleSpriteAssets() const
    {
        for (const AssetListEntry& entry : m_visibleEntries)
        {
            if (false == entry.isDirectory && IsTextureImageFile(entry.path))
            {
                return true;
            }
        }

        return false;
    }

    bool AssetsPanelController::HasCreateSpriteRequest() const
    {
        return m_createSpriteRequested;
    }

    void AssetsPanelController::ClearCreateSpriteRequest()
    {
        m_createSpriteRequested = false;
        m_createSpriteTargetDirectory.clear();
    }

    const std::filesystem::path& AssetsPanelController::GetCreateSpriteTargetDirectory() const
    {
        return m_createSpriteTargetDirectory;
    }

    void AssetsPanelController::InitializeListView()
    {
        if (nullptr == m_assetsListView || m_listViewInitialized)
        {
            return;
        }

        SHFILEINFOW fileInfo{};
        const HIMAGELIST imageList = reinterpret_cast<HIMAGELIST>(SHGetFileInfoW(
            L"C:\\",
            FILE_ATTRIBUTE_DIRECTORY,
            &fileInfo,
            sizeof(fileInfo),
            SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES));
        if (nullptr != imageList)
        {
            ListView_SetImageList(m_assetsListView, imageList, LVSIL_SMALL);
        }

        const std::array<std::pair<const wchar_t*, int>, 4> columns{
            std::pair<const wchar_t*, int>{ L"名前", 140 },
            std::pair<const wchar_t*, int>{ L"更新日時", 120 },
            std::pair<const wchar_t*, int>{ L"種類", 120 },
            std::pair<const wchar_t*, int>{ L"サイズ", 72 }
        };

        for (std::size_t index = 0; index < columns.size(); ++index)
        {
            LVCOLUMNW column{};
            column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            column.pszText = const_cast<wchar_t*>(columns[index].first);
            column.cx = columns[index].second;
            column.iSubItem = static_cast<int>(index);
            ListView_InsertColumn(m_assetsListView, static_cast<int>(index), &column);
        }

        m_listViewInitialized = true;
    }

    void AssetsPanelController::RebuildVisibleEntries()
    {
        m_visibleEntries.clear();
        if (m_currentDirectory.empty())
        {
            return;
        }

        std::error_code errorCode;
        if (false == std::filesystem::is_directory(m_currentDirectory, errorCode) || errorCode)
        {
            return;
        }

        AppendParentEntry();
        AppendCurrentDirectoryEntries();
    }

    void AssetsPanelController::AppendParentEntry()
    {
        if (m_currentDirectory.empty() || m_currentDirectory == m_assetsRootDirectory)
        {
            return;
        }

        AssetListEntry parentEntry{};
        parentEntry.path = m_currentDirectory.parent_path();
        parentEntry.displayName = L"..";
        parentEntry.typeName = L"親フォルダー";
        parentEntry.iconIndex = GetFolderIconIndex();
        parentEntry.isDirectory = true;
        parentEntry.isParentLink = true;
        m_visibleEntries.push_back(parentEntry);
    }

    void AssetsPanelController::AppendCurrentDirectoryEntries()
    {
        std::error_code errorCode;
        std::vector<std::filesystem::directory_entry> entries{};
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_currentDirectory, errorCode))
        {
            if (errorCode)
            {
                return;
            }

            entries.push_back(entry);
        }

        std::sort(entries.begin(), entries.end(), CompareDirectoryEntry);
        for (const std::filesystem::directory_entry& entry : entries)
        {
            m_visibleEntries.push_back(BuildEntry(entry));
        }
    }

    void AssetsPanelController::RefreshListView()
    {
        if (nullptr == m_assetsListView)
        {
            return;
        }

        ListView_DeleteAllItems(m_assetsListView);
        int selectedIndex = -1;
        for (std::size_t index = 0; index < m_visibleEntries.size(); ++index)
        {
            const AssetListEntry& entry = m_visibleEntries[index];

            LVITEMW item{};
            item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            item.iItem = static_cast<int>(index);
            item.iSubItem = NameColumnIndex;
            item.pszText = const_cast<wchar_t*>(entry.displayName.c_str());
            item.iImage = entry.iconIndex;
            item.lParam = static_cast<LPARAM>(index);
            ListView_InsertItem(m_assetsListView, &item);

            ListView_SetItemText(
                m_assetsListView,
                static_cast<int>(index),
                ModifiedTimeColumnIndex,
                const_cast<wchar_t*>(entry.modifiedTimeText.c_str()));
            ListView_SetItemText(
                m_assetsListView,
                static_cast<int>(index),
                TypeColumnIndex,
                const_cast<wchar_t*>(entry.typeName.c_str()));
            ListView_SetItemText(
                m_assetsListView,
                static_cast<int>(index),
                SizeColumnIndex,
                const_cast<wchar_t*>(entry.sizeText.c_str()));

            if (false == m_selectedFilePath.empty() && entry.path == m_selectedFilePath)
            {
                selectedIndex = static_cast<int>(index);
            }
        }

        if (0 <= selectedIndex)
        {
            ListView_SetItemState(
                m_assetsListView,
                selectedIndex,
                LVIS_SELECTED | LVIS_FOCUSED,
                LVIS_SELECTED | LVIS_FOCUSED);
        }
    }

    void AssetsPanelController::SyncSelectedPathFromListView()
    {
        const int selectedIndex = GetSelectedListViewIndex();
        if (selectedIndex < 0 || static_cast<std::size_t>(selectedIndex) >= m_visibleEntries.size())
        {
            return;
        }

        const AssetListEntry& entry = m_visibleEntries[static_cast<std::size_t>(selectedIndex)];
        if (entry.isParentLink)
        {
            m_selectedFilePath.clear();
            m_selectedSpriteAssetId = {};
        }
        else
        {
            m_selectedFilePath = entry.path;
            m_selectedSpriteAssetId = IsTextureImageFile(entry.path)
                ? BuildSpriteAssetId(entry.path)
                : Core::AssetId{};
        }

        RefreshSummaryLabel();
    }

    bool AssetsPanelController::TryOpenEntry(std::size_t entryIndex)
    {
        if (entryIndex >= m_visibleEntries.size())
        {
            return false;
        }

        const AssetListEntry entry = m_visibleEntries[entryIndex];
        if (false == entry.isDirectory)
        {
            return false;
        }

        if (entry.isParentLink && m_currentDirectory == m_assetsRootDirectory)
        {
            return false;
        }

        m_currentDirectory = entry.path;
        if (m_currentDirectory.empty())
        {
            m_currentDirectory = m_assetsRootDirectory;
        }

        m_selectedFilePath.clear();
        m_lastClickedIndex = -1;
        m_lastClickTick = 0;
        RebuildVisibleEntries();
        RefreshListView();
        RefreshSummaryLabel();
        return true;
    }

    bool AssetsPanelController::TryOpenSelectedEntry()
    {
        const int selectedIndex = GetSelectedListViewIndex();
        if (selectedIndex < 0)
        {
            return false;
        }

        return TryOpenEntry(static_cast<std::size_t>(selectedIndex));
    }

    int AssetsPanelController::GetSelectedListViewIndex() const
    {
        if (nullptr == m_assetsListView)
        {
            return -1;
        }

        return ListView_GetNextItem(m_assetsListView, -1, LVNI_SELECTED);
    }

    int AssetsPanelController::HitTestListView(POINT screenPoint) const
    {
        if (nullptr == m_assetsListView)
        {
            return -1;
        }

        POINT clientPoint = screenPoint;
        ScreenToClient(m_assetsListView, &clientPoint);

        LVHITTESTINFO hitTest{};
        hitTest.pt = clientPoint;
        const int hitIndex = ListView_HitTest(m_assetsListView, &hitTest);
        if (hitIndex < 0 || 0 == (hitTest.flags & LVHT_ONITEM))
        {
            return -1;
        }

        return hitIndex;
    }

    int AssetsPanelController::HitTestListViewNameLabel(POINT screenPoint) const
    {
        if (nullptr == m_assetsListView)
        {
            return -1;
        }

        POINT clientPoint = screenPoint;
        ScreenToClient(m_assetsListView, &clientPoint);

        LVHITTESTINFO hitTest{};
        hitTest.pt = clientPoint;
        const int hitIndex = ListView_SubItemHitTest(m_assetsListView, &hitTest);
        if (hitIndex < 0
            || hitTest.iSubItem != NameColumnIndex
            || 0 == (hitTest.flags & LVHT_ONITEMLABEL))
        {
            return -1;
        }

        return hitIndex;
    }

    bool AssetsPanelController::ShowEntryContextMenu(std::size_t entryIndex, POINT screenPoint)
    {
        if (nullptr == m_assetsListView || entryIndex >= m_visibleEntries.size())
        {
            return false;
        }

        const AssetListEntry& entry = m_visibleEntries[entryIndex];
        if (entry.isParentLink)
        {
            return false;
        }

        ListView_SetItemState(
            m_assetsListView,
            static_cast<int>(entryIndex),
            LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED);
        SetFocus(m_assetsListView);
        SyncSelectedPathFromListView();

        HMENU popupMenu = CreatePopupMenu();
        if (nullptr == popupMenu)
        {
            return false;
        }

        AppendMenuW(
            popupMenu,
            MF_STRING,
            DeleteEntryMenuCommandId,
            entry.isDirectory ? L"フォルダを削除する" : L"ファイルを削除する");

        const UINT command = TrackPopupMenu(
            popupMenu,
            TPM_RETURNCMD | TPM_RIGHTBUTTON,
            screenPoint.x,
            screenPoint.y,
            0,
            m_assetsListView,
            nullptr);
        DestroyMenu(popupMenu);

        if (DeleteEntryMenuCommandId != command)
        {
            return false;
        }

        return DeleteEntry(entryIndex);
    }

    bool AssetsPanelController::ShowCreateSpriteContextMenu(
        const std::filesystem::path& targetDirectory,
        POINT screenPoint)
    {
        if (nullptr == m_assetsListView || true == targetDirectory.empty())
        {
            return false;
        }

        HMENU popupMenu = CreatePopupMenu();
        if (nullptr == popupMenu)
        {
            return false;
        }

        AppendMenuW(popupMenu, MF_STRING, CreateSpriteMenuCommandId, L"Spriteを作成");

        const UINT command = TrackPopupMenu(
            popupMenu,
            TPM_RETURNCMD | TPM_RIGHTBUTTON,
            screenPoint.x,
            screenPoint.y,
            0,
            m_assetsListView,
            nullptr);
        DestroyMenu(popupMenu);

        if (CreateSpriteMenuCommandId != command)
        {
            return false;
        }

        m_createSpriteRequested = true;
        m_createSpriteTargetDirectory = targetDirectory;
        return true;
    }

    bool AssetsPanelController::DeleteEntry(std::size_t entryIndex)
    {
        if (entryIndex >= m_visibleEntries.size())
        {
            return false;
        }

        const AssetListEntry entry = m_visibleEntries[entryIndex];
        if (entry.isParentLink || true == entry.path.empty())
        {
            return false;
        }

        std::error_code errorCode;
        if (entry.isDirectory)
        {
            std::filesystem::remove_all(entry.path, errorCode);
        }
        else
        {
            std::filesystem::remove(entry.path, errorCode);
        }

        if (errorCode)
        {
            MessageBoxW(
                m_assetsListView,
                L"削除に失敗しました。",
                L"Assets",
                MB_OK | MB_ICONERROR);
            return false;
        }

        if (m_selectedFilePath == entry.path)
        {
            m_selectedFilePath.clear();
            m_selectedSpriteAssetId = {};
        }

        if (m_draggingImagePath == entry.path)
        {
            CompleteReleasedDrag();
        }

        RebuildVisibleEntries();
        RefreshListView();
        RefreshSummaryLabel();
        return true;
    }

    void AssetsPanelController::RefreshSummaryLabel()
    {
        if (nullptr == m_assetsSummaryLabel)
        {
            return;
        }

        if (m_assetsRootDirectory.empty() || m_currentDirectory.empty())
        {
            SetWindowTextW(m_assetsSummaryLabel, L"Assets: project not opened");
            return;
        }

        std::error_code errorCode;
        const std::filesystem::path relativeCurrentPath = std::filesystem::relative(
            m_currentDirectory,
            m_assetsRootDirectory,
            errorCode);
        const std::wstring currentPathText = errorCode || relativeCurrentPath.empty() || relativeCurrentPath.wstring() == L"."
            ? std::wstring(L".")
            : relativeCurrentPath.wstring();

        wchar_t summaryText[512]{};
        if (false == m_selectedFilePath.empty())
        {
            errorCode.clear();
            const std::filesystem::path relativeSelectedPath = std::filesystem::relative(
                m_selectedFilePath,
                m_assetsRootDirectory,
                errorCode);
            const std::wstring selectedPath = errorCode
                ? m_selectedFilePath.wstring()
                : relativeSelectedPath.wstring();
            std::swprintf(
                summaryText,
                std::size(summaryText),
                L"Assets: %ls / %u items / selected %ls",
                currentPathText.c_str(),
                static_cast<unsigned>(m_visibleEntries.size()),
                selectedPath.c_str());
        }
        else
        {
            std::swprintf(
                summaryText,
                std::size(summaryText),
                L"Assets: %ls / %u items",
                currentPathText.c_str(),
                static_cast<unsigned>(m_visibleEntries.size()));
        }

        SetWindowTextW(m_assetsSummaryLabel, summaryText);
    }

    void AssetsPanelController::BeginDragImage(
        const std::filesystem::path& imagePath,
        int fallbackIconIndex,
        POINT screenPoint)
    {
        EndDragImage();

        m_dragImageList = CreateDragImageList(imagePath, fallbackIconIndex);
        BeginDragPreview(imagePath, screenPoint);
        if (nullptr == m_dragImageList)
        {
            return;
        }

        if (FALSE == ImageList_BeginDrag(m_dragImageList, 0, 24, 24))
        {
            ImageList_Destroy(m_dragImageList);
            m_dragImageList = nullptr;
            return;
        }

        if (FALSE == ImageList_DragEnter(GetDesktopWindow(), screenPoint.x, screenPoint.y))
        {
            ImageList_EndDrag();
            ImageList_Destroy(m_dragImageList);
            m_dragImageList = nullptr;
            return;
        }

        m_isDragImageVisible = true;
    }

    void AssetsPanelController::MoveDragImage(POINT screenPoint)
    {
        if (false == m_isDragImageVisible)
        {
            MoveDragPreview(screenPoint);
            return;
        }

        ImageList_DragMove(screenPoint.x, screenPoint.y);
        MoveDragPreview(screenPoint);
    }

    void AssetsPanelController::EndDragImage()
    {
        if (m_isDragImageVisible)
        {
            ImageList_DragLeave(GetDesktopWindow());
            ImageList_EndDrag();
            m_isDragImageVisible = false;
        }

        if (nullptr != m_dragImageList)
        {
            ImageList_Destroy(m_dragImageList);
            m_dragImageList = nullptr;
        }

        EndDragPreview();
    }

    void AssetsPanelController::BeginDragPreview(const std::filesystem::path& imagePath, POINT screenPoint)
    {
        EndDragPreview();

        m_dragPreviewWindow = CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
            L"Static",
            L"",
            WS_POPUP | WS_BORDER,
            screenPoint.x + DragPreviewCursorOffsetX,
            screenPoint.y + DragPreviewCursorOffsetY,
            DragPreviewWidth,
            DragPreviewHeight,
            nullptr,
            nullptr,
            GetModuleHandleW(nullptr),
            nullptr);
        if (nullptr == m_dragPreviewWindow)
        {
            return;
        }

        m_dragPreviewBitmap = CreateDragPreviewBitmap(imagePath);
        if (nullptr != m_dragPreviewBitmap)
        {
            m_dragPreviewImage = CreateWindowExW(
                0,
                L"Static",
                L"",
                WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
                6,
                6,
                DragPreviewImageSize,
                DragPreviewImageSize,
                m_dragPreviewWindow,
                nullptr,
                GetModuleHandleW(nullptr),
                nullptr);
            if (nullptr != m_dragPreviewImage)
            {
                SendMessageW(m_dragPreviewImage, STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(m_dragPreviewBitmap));
            }
        }

        if (nullptr == m_dragPreviewImage)
        {
            SHFILEINFOW fileInfo{};
            if (0 != SHGetFileInfoW(
                    imagePath.c_str(),
                    FILE_ATTRIBUTE_NORMAL,
                    &fileInfo,
                    sizeof(fileInfo),
                    SHGFI_ICON | SHGFI_LARGEICON))
            {
                m_dragPreviewIcon = fileInfo.hIcon;
                m_dragPreviewImage = CreateWindowExW(
                    0,
                    L"Static",
                    L"",
                    WS_CHILD | WS_VISIBLE | SS_ICON | SS_CENTERIMAGE,
                    6,
                    6,
                    DragPreviewImageSize,
                    DragPreviewImageSize,
                    m_dragPreviewWindow,
                    nullptr,
                    GetModuleHandleW(nullptr),
                    nullptr);
                if (nullptr != m_dragPreviewImage)
                {
                    SendMessageW(m_dragPreviewImage, STM_SETICON, reinterpret_cast<WPARAM>(m_dragPreviewIcon), 0);
                }
            }
        }

        const std::wstring fileName = imagePath.filename().wstring();
        m_dragPreviewText = CreateWindowExW(
            0,
            L"Static",
            fileName.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE | SS_ENDELLIPSIS,
            60,
            8,
            DragPreviewWidth - 68,
            DragPreviewHeight - 16,
            m_dragPreviewWindow,
            nullptr,
            GetModuleHandleW(nullptr),
            nullptr);
        if (nullptr != m_dragPreviewText)
        {
            HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            SendMessageW(m_dragPreviewText, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }

        ShowWindow(m_dragPreviewWindow, SW_SHOWNOACTIVATE);
        MoveDragPreview(screenPoint);
    }

    void AssetsPanelController::MoveDragPreview(POINT screenPoint)
    {
        if (nullptr == m_dragPreviewWindow)
        {
            return;
        }

        SetWindowPos(
            m_dragPreviewWindow,
            HWND_TOPMOST,
            screenPoint.x + DragPreviewCursorOffsetX,
            screenPoint.y + DragPreviewCursorOffsetY,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    void AssetsPanelController::EndDragPreview()
    {
        if (nullptr != m_dragPreviewWindow)
        {
            DestroyWindow(m_dragPreviewWindow);
            m_dragPreviewWindow = nullptr;
            m_dragPreviewImage = nullptr;
            m_dragPreviewText = nullptr;
        }

        if (nullptr != m_dragPreviewBitmap)
        {
            DeleteObject(m_dragPreviewBitmap);
            m_dragPreviewBitmap = nullptr;
        }

        if (nullptr != m_dragPreviewIcon)
        {
            DestroyIcon(m_dragPreviewIcon);
            m_dragPreviewIcon = nullptr;
        }
    }

    HBITMAP AssetsPanelController::CreateDragPreviewBitmap(const std::filesystem::path& imagePath) const
    {
        const HRESULT coInitializeResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        const bool shouldUninitializeCom = SUCCEEDED(coInitializeResult);

        Microsoft::WRL::ComPtr<IShellItemImageFactory> imageFactory;
        const HRESULT itemHr = SHCreateItemFromParsingName(
            imagePath.c_str(),
            nullptr,
            IID_PPV_ARGS(imageFactory.GetAddressOf()));
        HBITMAP thumbnail = nullptr;
        if (SUCCEEDED(itemHr) && imageFactory)
        {
            SIZE thumbnailSize{ DragPreviewImageSize, DragPreviewImageSize };
            (void)imageFactory->GetImage(
                thumbnailSize,
                SIIGBF_BIGGERSIZEOK,
                &thumbnail);
        }

        if (shouldUninitializeCom)
        {
            CoUninitialize();
        }

        return thumbnail;
    }

    HIMAGELIST AssetsPanelController::CreateDragImageList(
        const std::filesystem::path& imagePath,
        int fallbackIconIndex) const
    {
        constexpr int DragImageSize = 48;
        const HRESULT coInitializeResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        const bool shouldUninitializeCom = SUCCEEDED(coInitializeResult);

        HIMAGELIST imageList = ImageList_Create(
            DragImageSize,
            DragImageSize,
            ILC_COLOR32 | ILC_MASK,
            1,
            1);
        if (nullptr == imageList)
        {
            if (shouldUninitializeCom)
            {
                CoUninitialize();
            }

            return nullptr;
        }

        Microsoft::WRL::ComPtr<IShellItemImageFactory> imageFactory;
        const HRESULT itemHr = SHCreateItemFromParsingName(
            imagePath.c_str(),
            nullptr,
            IID_PPV_ARGS(imageFactory.GetAddressOf()));
        if (SUCCEEDED(itemHr) && imageFactory)
        {
            HBITMAP thumbnail = nullptr;
            SIZE thumbnailSize{ DragImageSize, DragImageSize };
            const HRESULT thumbnailHr = imageFactory->GetImage(
                thumbnailSize,
                SIIGBF_BIGGERSIZEOK,
                &thumbnail);
            if (SUCCEEDED(thumbnailHr) && nullptr != thumbnail)
            {
                ImageList_Add(imageList, thumbnail, nullptr);
                DeleteObject(thumbnail);
                if (0 < ImageList_GetImageCount(imageList))
                {
                    if (shouldUninitializeCom)
                    {
                        CoUninitialize();
                    }

                    return imageList;
                }
            }
        }

        SHFILEINFOW fileInfo{};
        HIMAGELIST systemImageList = reinterpret_cast<HIMAGELIST>(SHGetFileInfoW(
            imagePath.c_str(),
            FILE_ATTRIBUTE_NORMAL,
            &fileInfo,
            sizeof(fileInfo),
            SHGFI_SYSICONINDEX | SHGFI_LARGEICON));
        if (nullptr != systemImageList)
        {
            HICON icon = ImageList_GetIcon(systemImageList, fallbackIconIndex, ILD_NORMAL);
            if (nullptr != icon)
            {
                ImageList_AddIcon(imageList, icon);
                DestroyIcon(icon);
                if (0 < ImageList_GetImageCount(imageList))
                {
                    if (shouldUninitializeCom)
                    {
                        CoUninitialize();
                    }

                    return imageList;
                }
            }
        }

        ImageList_Destroy(imageList);
        if (shouldUninitializeCom)
        {
            CoUninitialize();
        }

        return nullptr;
    }

    bool AssetsPanelController::IsTextureImageFile(const std::filesystem::path& path)
    {
        const std::wstring extension = path.extension().wstring();
        return extension == L".png"
            || extension == L".jpg"
            || extension == L".jpeg"
            || extension == L".bmp";
    }

    Core::AssetId AssetsPanelController::BuildTextureAssetId(const std::filesystem::path& path) const
    {
        std::error_code errorCode;
        const std::filesystem::path relativePath = std::filesystem::relative(path, m_assetsRootDirectory, errorCode);
        if (errorCode)
        {
            return {};
        }

        return Core::AssetId("textures/" + ToNarrowString(relativePath.generic_wstring()));
    }

    Core::AssetId AssetsPanelController::BuildSpriteAssetId(const std::filesystem::path& path) const
    {
        std::error_code errorCode;
        const std::filesystem::path relativePath = std::filesystem::relative(path, m_assetsRootDirectory, errorCode);
        if (errorCode)
        {
            return {};
        }

        return Core::AssetId("sprites/" + ToNarrowString(relativePath.generic_wstring()));
    }
}
