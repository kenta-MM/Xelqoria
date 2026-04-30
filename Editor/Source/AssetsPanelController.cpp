#include "AssetsPanelController.h"

#include <algorithm>
#include <array>
#include <CommCtrl.h>
#include <cstdio>
#include <iterator>
#include <Shellapi.h>
#include <system_error>
#include <utility>
#include <Windows.h>

#include "EditorStringUtils.h"

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr int NameColumnIndex = 0;
        constexpr int ModifiedTimeColumnIndex = 1;
        constexpr int TypeColumnIndex = 2;
        constexpr int SizeColumnIndex = 3;

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
            if (0 <= itemActivate->iItem)
            {
                return false;
            }

            HMENU popupMenu = CreatePopupMenu();
            if (nullptr == popupMenu)
            {
                return false;
            }

            AppendMenuW(popupMenu, MF_STRING, CreateSpriteMenuCommandId, L"Spriteを作成");

            POINT menuPoint{};
            GetCursorPos(&menuPoint);
            const UINT command = TrackPopupMenu(
                popupMenu,
                TPM_RETURNCMD | TPM_RIGHTBUTTON,
                menuPoint.x,
                menuPoint.y,
                0,
                m_assetsListView,
                nullptr);
            DestroyMenu(popupMenu);

            if (CreateSpriteMenuCommandId == command)
            {
                m_createSpriteRequested = true;
                return true;
            }
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

            m_isAssetDragActive = false;
            return;
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
        }
        else
        {
            m_draggingImagePath.clear();
            m_draggingTextureAssetId = {};
            m_draggingSpriteAssetId = {};
            m_isAssetDragActive = false;
            m_canPlaceDraggingAssetInScene = false;
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
