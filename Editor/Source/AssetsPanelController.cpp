#include "AssetsPanelController.h"

#include <algorithm>
#include <array>
#include <CommCtrl.h>
#include <cstdio>
#include <cwctype>
#include <functional>
#include <iterator>
#include <objbase.h>
#include <ShObjIdl.h>
#include <Shellapi.h>
#include <system_error>
#include <utility>
#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include "EditorAssetPathUtils.h"
#include "EditorPathSecurity.h"
#include "ScriptAssetService.h"

#pragma comment(lib, "windowscodecs.lib")

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
        constexpr int AssetsIconSize = 16;
        constexpr const wchar_t* EditorIconsRelativeDirectory = L"Assets\\Editor\\Icons";

        /// <summary>
        /// COM 利用スコープの終了時に CoUninitialize を遅延実行する。
        /// </summary>
        struct ScopedComInitialization
        {
            explicit ScopedComInitialization(DWORD concurrencyModel)
            {
                const HRESULT result = CoInitializeEx(nullptr, concurrencyModel);
                shouldUninitialize = SUCCEEDED(result);
            }

            ~ScopedComInitialization()
            {
                if (shouldUninitialize)
                {
                    CoUninitialize();
                }
            }

            ScopedComInitialization(const ScopedComInitialization&) = delete;
            ScopedComInitialization& operator=(const ScopedComInitialization&) = delete;

            bool shouldUninitialize = false;
        };

        [[nodiscard]] POINT ToWin32Point(Platform::Point point)
        {
            return POINT{ static_cast<LONG>(point.x), static_cast<LONG>(point.y) };
        }

        /// <summary>
        /// 拡張子文字列を小文字へ正規化する。
        /// </summary>
        /// <param name="extension">正規化する拡張子。</param>
        /// <returns>小文字化した拡張子。</returns>
        [[nodiscard]] std::wstring NormalizeExtension(std::wstring extension)
        {
            for (wchar_t& character : extension)
            {
                character = static_cast<wchar_t>(std::towlower(character));
            }

            return extension;
        }

        /// <summary>
        /// 指定ディレクトリから Editor アイコン配置ディレクトリを親方向へ探索する。
        /// </summary>
        /// <param name="startDirectory">探索開始ディレクトリ。</param>
        /// <returns>見つかったアイコンディレクトリ。未検出時は空。</returns>
        [[nodiscard]] std::filesystem::path FindEditorIconsDirectoryFrom(const std::filesystem::path& startDirectory)
        {
            if (startDirectory.empty())
            {
                return {};
            }

            std::error_code errorCode;
            std::filesystem::path currentDirectory = std::filesystem::absolute(startDirectory, errorCode);
            if (errorCode)
            {
                currentDirectory = startDirectory;
            }

            while (false == currentDirectory.empty())
            {
                const std::filesystem::path candidate = currentDirectory / EditorIconsRelativeDirectory;
                if (std::filesystem::is_directory(candidate, errorCode)
                    && false == static_cast<bool>(errorCode))
                {
                    return candidate;
                }

                errorCode.clear();
                const std::filesystem::path parentDirectory = currentDirectory.parent_path();
                if (parentDirectory == currentDirectory)
                {
                    break;
                }

                currentDirectory = parentDirectory;
            }

            return {};
        }

        /// <summary>
        /// 利用可能な Editor アイコン配置ディレクトリを取得する。
        /// </summary>
        /// <param name="assetsRootDirectory">現在の Assets ルートディレクトリ。</param>
        /// <returns>見つかったアイコンディレクトリ。未検出時は空。</returns>
        [[nodiscard]] std::filesystem::path FindEditorIconsDirectory(const std::filesystem::path& assetsRootDirectory)
        {
            const std::filesystem::path projectIconsDirectory = assetsRootDirectory / EditorIconsRelativeDirectory;
            std::error_code errorCode;
            if (false == assetsRootDirectory.empty()
                && std::filesystem::is_directory(projectIconsDirectory, errorCode)
                && false == static_cast<bool>(errorCode))
            {
                return projectIconsDirectory;
            }

            errorCode.clear();
            const std::filesystem::path currentIconsDirectory =
                FindEditorIconsDirectoryFrom(std::filesystem::current_path(errorCode));
            if (false == currentIconsDirectory.empty())
            {
                return currentIconsDirectory;
            }

            wchar_t modulePath[MAX_PATH]{};
            if (0 != GetModuleFileNameW(nullptr, modulePath, std::size(modulePath)))
            {
                return FindEditorIconsDirectoryFrom(std::filesystem::path(modulePath).parent_path());
            }

            return {};
        }

        /// <summary>
        /// PNG ファイルを ImageList へ追加できる 32bpp HBITMAP として読み込む。
        /// </summary>
        /// <param name="iconPath">読み込む PNG パス。</param>
        /// <returns>読み込んだ HBITMAP。失敗時は nullptr。</returns>
        [[nodiscard]] HBITMAP LoadPngIconBitmap(const std::filesystem::path& iconPath)
        {
            if (false == std::filesystem::exists(iconPath))
            {
                return nullptr;
            }

            ScopedComInitialization comInitialization(COINIT_APARTMENTTHREADED);

            Microsoft::WRL::ComPtr<IWICImagingFactory> imagingFactory;
            HRESULT hr = CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(imagingFactory.GetAddressOf()));
            if (FAILED(hr))
            {
                return nullptr;
            }

            Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
            hr = imagingFactory->CreateDecoderFromFilename(
                iconPath.c_str(),
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,
                decoder.GetAddressOf());
            if (FAILED(hr))
            {
                return nullptr;
            }

            Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
            hr = decoder->GetFrame(0, frame.GetAddressOf());
            if (FAILED(hr))
            {
                return nullptr;
            }

            Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
            hr = imagingFactory->CreateBitmapScaler(scaler.GetAddressOf());
            if (FAILED(hr))
            {
                return nullptr;
            }

            hr = scaler->Initialize(
                frame.Get(),
                AssetsIconSize,
                AssetsIconSize,
                WICBitmapInterpolationModeFant);
            if (FAILED(hr))
            {
                return nullptr;
            }

            Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
            hr = imagingFactory->CreateFormatConverter(converter.GetAddressOf());
            if (FAILED(hr))
            {
                return nullptr;
            }

            hr = converter->Initialize(
                scaler.Get(),
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom);
            if (FAILED(hr))
            {
                return nullptr;
            }

            BITMAPINFO bitmapInfo{};
            bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bitmapInfo.bmiHeader.biWidth = AssetsIconSize;
            bitmapInfo.bmiHeader.biHeight = -AssetsIconSize;
            bitmapInfo.bmiHeader.biPlanes = 1;
            bitmapInfo.bmiHeader.biBitCount = 32;
            bitmapInfo.bmiHeader.biCompression = BI_RGB;

            void* bits = nullptr;
            HBITMAP bitmap = CreateDIBSection(
                nullptr,
                &bitmapInfo,
                DIB_RGB_COLORS,
                &bits,
                nullptr,
                0);
            if (nullptr == bitmap || nullptr == bits)
            {
                if (nullptr != bitmap)
                {
                    DeleteObject(bitmap);
                }

                return nullptr;
            }

            constexpr UINT stride = AssetsIconSize * 4;
            constexpr UINT bufferSize = stride * AssetsIconSize;
            hr = converter->CopyPixels(nullptr, stride, bufferSize, static_cast<BYTE*>(bits));
            if (FAILED(hr))
            {
                DeleteObject(bitmap);
                return nullptr;
            }

            return bitmap;
        }

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

        void CombineHash(std::uint64_t& seed, std::uint64_t value)
        {
            seed ^= value + 0x9E3779B97F4A7C15ull + (seed << 6) + (seed >> 2);
        }

        [[nodiscard]] std::uint64_t BuildProjectTreeSignature(const std::filesystem::path& rootDirectory)
        {
            if (rootDirectory.empty())
            {
                return 0;
            }

            std::error_code errorCode;
            if (false == std::filesystem::is_directory(rootDirectory, errorCode) || errorCode)
            {
                return 0;
            }

            std::uint64_t signature = 1469598103934665603ull;
            std::size_t visitedCount = 0;
            constexpr std::size_t MaxVisitedEntries = 4096;
            const std::hash<std::wstring> pathHasher{};
            for (std::filesystem::recursive_directory_iterator iterator(
                    rootDirectory,
                    std::filesystem::directory_options::skip_permission_denied,
                    errorCode);
                false == errorCode && iterator != std::filesystem::recursive_directory_iterator{};
                iterator.increment(errorCode))
            {
                if (MaxVisitedEntries <= visitedCount)
                {
                    break;
                }

                const std::filesystem::directory_entry& entry = *iterator;
                CombineHash(signature, static_cast<std::uint64_t>(pathHasher(entry.path().wstring())));
                const auto writeTime = entry.last_write_time(errorCode);
                if (false == static_cast<bool>(errorCode))
                {
                    CombineHash(signature, static_cast<std::uint64_t>(writeTime.time_since_epoch().count()));
                }
                errorCode.clear();

                if (false == entry.is_directory(errorCode) && false == static_cast<bool>(errorCode))
                {
                    CombineHash(signature, static_cast<std::uint64_t>(entry.file_size(errorCode)));
                }
                errorCode.clear();
                ++visitedCount;
            }

            CombineHash(signature, static_cast<std::uint64_t>(visitedCount));
            return signature;
        }

        /// <summary>
        /// Assets の右クリックメニューから実行されたコマンド ID を表す。
        /// </summary>
        constexpr UINT_PTR CreateSpriteMenuCommandId = 1;

        /// <summary>
        /// Assets の右クリックメニューから Script Asset を作成するコマンド ID を表す。
        /// </summary>
        constexpr UINT_PTR CreateScriptMenuCommandId = 3;

        /// <summary>
        /// Assets の右クリックメニューから Material Asset を作成するコマンド ID を表す。
        /// </summary>
        constexpr UINT_PTR CreateMaterialMenuCommandId = 5;

        /// <summary>
        /// Sprite Asset へ Script Asset を割り当てるコマンド ID を表す。
        /// </summary>
        constexpr UINT_PTR AssignScriptMenuCommandId = 4;

        /// <summary>
        /// Assets 項目の削除メニューから実行されたコマンド ID を表す。
        /// </summary>
        constexpr UINT_PTR DeleteEntryMenuCommandId = 2;
    }

    AssetsPanelController::~AssetsPanelController()
    {
        if (m_ownsAssetsImageList && nullptr != m_assetsImageList)
        {
            ImageList_Destroy(m_assetsImageList);
            m_assetsImageList = nullptr;
            m_ownsAssetsImageList = false;
        }
    }

    void AssetsPanelController::Bind(const EditorShell& shell, Platform::ICursor& cursor)
    {
        m_assetsListView = shell.GetAssetsListView();
        m_assetsSummaryLabel = shell.GetAssetsSummaryLabel();
        m_cursor = &cursor;
        InitializeListView();
    }

    void AssetsPanelController::Refresh(const std::optional<EditorProjectInfo>& projectInfo)
    {
        if (false == projectInfo.has_value())
        {
            m_assetsRootDirectory.clear();
            m_currentDirectory.clear();
            ReloadEditorIconImages({});
            m_visibleEntries.clear();
            m_selectedFilePath.clear();
            m_selectedSpriteAssetId = {};
            m_draggingSpriteAssetId = {};
            m_draggingTextureAssetId = {};
            m_draggingScriptAssetId = {};
            m_draggingMaterialAssetId = {};
            m_draggingImagePath.clear();
            m_draggingScriptAssetPath.clear();
            m_canPlaceDraggingAssetInScene = false;
            m_createSpriteRequested = false;
            m_createSpriteTargetDirectory.clear();
            m_createScriptRequested = false;
            m_createScriptTargetDirectory.clear();
            m_createMaterialRequested = false;
            m_createMaterialTargetDirectory.clear();
            m_assignScriptRequested = false;
            m_assignScriptSpriteAssetPath.clear();
            m_lastWatchTick = 0;
            m_projectTreeSignature = 0;
            EndDragImage();
            RefreshListView();
            RefreshSummaryLabel();
            return;
        }

        const std::filesystem::path rootDirectory =
            EditorPathSecurity::NormalizeForContainment(projectInfo->projectFilePath.parent_path());
        if (rootDirectory != m_assetsRootDirectory)
        {
            m_assetsRootDirectory = rootDirectory;
            m_currentDirectory = m_assetsRootDirectory;
            m_selectedFilePath.clear();
            m_lastClickTick = 0;
            m_lastClickedIndex = -1;
            m_openMaterialRequested = false;
            m_openMaterialAssetId = {};
            m_lastWatchTick = 0;
        }

        ReloadEditorIconImages(m_assetsRootDirectory);
        m_projectTreeSignature = BuildProjectTreeSignature(m_assetsRootDirectory);
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
            if (nullptr == m_cursor)
            {
                return false;
            }

            const NMITEMACTIVATE* itemActivate = reinterpret_cast<NMITEMACTIVATE*>(notifyParameter);
            const POINT menuPoint = ToWin32Point(m_cursor->GetScreenPosition());

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

            return ShowCreateAssetContextMenu(createSpriteTargetDirectory, menuPoint);
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
            MoveDragImage(ToWin32Point(inputSnapshot.GetCursorScreenPoint()));
        }

        if (false == inputSnapshot.WasMouseButtonPressed(Core::MouseButton::Left))
        {
            return;
        }

        const int hitIndex = HitTestListView(ToWin32Point(inputSnapshot.GetCursorScreenPoint()));
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
        if (false == hitEntry.isDirectory
            && EditorPathSecurity::IsPathInsideOrEqual(hitEntry.path, m_assetsRootDirectory)
            && EditorAssetPathUtils::IsTextureImageFile(hitEntry.path))
        {
            m_draggingImagePath = hitEntry.path;
            m_draggingScriptAssetPath.clear();
            m_draggingTextureAssetId = EditorAssetPathUtils::BuildTextureAssetId(hitEntry.path, m_assetsRootDirectory);
            m_draggingSpriteAssetId = EditorAssetPathUtils::BuildSpriteAssetId(hitEntry.path, m_assetsRootDirectory);
            m_draggingScriptAssetId = {};
            m_draggingMaterialAssetId = {};
            m_isAssetDragActive = false == m_draggingSpriteAssetId.IsEmpty();
            m_canPlaceDraggingAssetInScene = false;
            if (m_isAssetDragActive)
            {
                BeginDragImage(hitEntry.path, hitEntry.iconIndex, ToWin32Point(inputSnapshot.GetCursorScreenPoint()));
            }
        }
        else if (false == hitEntry.isDirectory
            && EditorPathSecurity::IsPathInsideOrEqual(hitEntry.path, m_assetsRootDirectory)
            && ScriptAssetService::IsScriptAssetFile(hitEntry.path))
        {
            m_draggingImagePath.clear();
            m_draggingTextureAssetId = {};
            m_draggingSpriteAssetId = {};
            m_draggingScriptAssetPath = hitEntry.path;
            m_draggingScriptAssetId = ScriptAssetService::BuildScriptAssetId(m_assetsRootDirectory, hitEntry.path);
            m_draggingMaterialAssetId = {};
            m_isAssetDragActive = false == m_draggingScriptAssetId.IsEmpty();
            m_canPlaceDraggingAssetInScene = false;
            if (m_isAssetDragActive)
            {
                BeginDragPreview(hitEntry.path, ToWin32Point(inputSnapshot.GetCursorScreenPoint()));
            }
        }
        else if (false == hitEntry.isDirectory
            && EditorPathSecurity::IsPathInsideOrEqual(hitEntry.path, m_assetsRootDirectory)
            && EditorAssetPathUtils::IsMaterialAssetFile(hitEntry.path))
        {
            m_draggingImagePath.clear();
            m_draggingTextureAssetId = {};
            m_draggingSpriteAssetId = {};
            m_draggingScriptAssetPath.clear();
            m_draggingScriptAssetId = {};
            m_draggingMaterialAssetId = EditorAssetPathUtils::BuildMaterialAssetId(hitEntry.path, m_assetsRootDirectory);
            m_isAssetDragActive = false == m_draggingMaterialAssetId.IsEmpty();
            m_canPlaceDraggingAssetInScene = false;
            if (m_isAssetDragActive)
            {
                BeginDragPreview(hitEntry.path, ToWin32Point(inputSnapshot.GetCursorScreenPoint()));
            }
        }
        else
        {
            m_draggingImagePath.clear();
            m_draggingTextureAssetId = {};
            m_draggingSpriteAssetId = {};
            m_draggingScriptAssetPath.clear();
            m_draggingScriptAssetId = {};
            m_draggingMaterialAssetId = {};
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

    void AssetsPanelController::UpdateFileSystemWatch()
    {
        if (m_assetsRootDirectory.empty())
        {
            return;
        }

        const ULONGLONG currentTick = GetTickCount64();
        constexpr ULONGLONG WatchIntervalMilliseconds = 800;
        if (0 != m_lastWatchTick && currentTick - m_lastWatchTick < WatchIntervalMilliseconds)
        {
            return;
        }

        m_lastWatchTick = currentTick;
        const std::uint64_t nextSignature = BuildProjectTreeSignature(m_assetsRootDirectory);
        if (nextSignature == m_projectTreeSignature)
        {
            return;
        }

        m_projectTreeSignature = nextSignature;
        if (false == m_selectedFilePath.empty() && false == std::filesystem::exists(m_selectedFilePath))
        {
            m_selectedFilePath.clear();
            m_selectedSpriteAssetId = {};
        }

        if (false == std::filesystem::is_directory(m_currentDirectory))
        {
            m_currentDirectory = m_assetsRootDirectory;
        }

        RebuildVisibleEntries();
        RefreshListView();
        RefreshSummaryLabel();
    }

    void AssetsPanelController::CompleteReleasedDrag()
    {
        m_draggingSpriteAssetId = {};
        m_draggingTextureAssetId = {};
        m_draggingScriptAssetId = {};
        m_draggingMaterialAssetId = {};
        m_draggingImagePath.clear();
        m_draggingScriptAssetPath.clear();
        m_canPlaceDraggingAssetInScene = false;
        EndDragImage();
        RefreshSummaryLabel();
    }

    const Core::AssetId& AssetsPanelController::GetSelectedAssetId() const
    {
        return m_selectedSpriteAssetId;
    }

    const std::filesystem::path& AssetsPanelController::GetSelectedFilePath() const
    {
        return m_selectedFilePath;
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

    const Core::AssetId& AssetsPanelController::GetDraggingScriptAssetId() const
    {
        return m_draggingScriptAssetId;
    }

    const Core::AssetId& AssetsPanelController::GetDraggingMaterialAssetId() const
    {
        return m_draggingMaterialAssetId;
    }

    const std::filesystem::path& AssetsPanelController::GetDraggingScriptAssetPath() const
    {
        return m_draggingScriptAssetPath;
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
            if (false == entry.isDirectory && EditorAssetPathUtils::IsTextureImageFile(entry.path))
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

    bool AssetsPanelController::HasCreateScriptRequest() const
    {
        return m_createScriptRequested;
    }

    void AssetsPanelController::ClearCreateScriptRequest()
    {
        m_createScriptRequested = false;
        m_createScriptTargetDirectory.clear();
    }

    bool AssetsPanelController::HasCreateMaterialRequest() const
    {
        return m_createMaterialRequested;
    }

    void AssetsPanelController::ClearCreateMaterialRequest()
    {
        m_createMaterialRequested = false;
        m_createMaterialTargetDirectory.clear();
    }

    bool AssetsPanelController::HasOpenMaterialRequest() const
    {
        return m_openMaterialRequested;
    }

    const Core::AssetId& AssetsPanelController::GetOpenMaterialAssetId() const
    {
        return m_openMaterialAssetId;
    }

    void AssetsPanelController::ClearOpenMaterialRequest()
    {
        m_openMaterialRequested = false;
        m_openMaterialAssetId = {};
    }

    bool AssetsPanelController::HasAssignScriptRequest() const
    {
        return m_assignScriptRequested;
    }

    void AssetsPanelController::ClearAssignScriptRequest()
    {
        m_assignScriptRequested = false;
        m_assignScriptSpriteAssetPath.clear();
    }

    const std::filesystem::path& AssetsPanelController::GetCreateSpriteTargetDirectory() const
    {
        return m_createSpriteTargetDirectory;
    }

    const std::filesystem::path& AssetsPanelController::GetCreateScriptTargetDirectory() const
    {
        return m_createScriptTargetDirectory;
    }

    const std::filesystem::path& AssetsPanelController::GetCreateMaterialTargetDirectory() const
    {
        return m_createMaterialTargetDirectory;
    }

    const std::filesystem::path& AssetsPanelController::GetAssignScriptSpriteAssetPath() const
    {
        return m_assignScriptSpriteAssetPath;
    }

    void AssetsPanelController::InitializeListView()
    {
        if (nullptr == m_assetsListView || m_listViewInitialized)
        {
            return;
        }

        InitializeAssetsImageList();

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

    void AssetsPanelController::InitializeAssetsImageList()
    {
        if (nullptr == m_assetsListView)
        {
            return;
        }

        if (m_ownsAssetsImageList && nullptr != m_assetsImageList)
        {
            ImageList_Destroy(m_assetsImageList);
        }

        m_assetsImageList = ImageList_Create(
            AssetsIconSize,
            AssetsIconSize,
            ILC_COLOR32 | ILC_MASK,
            16,
            16);
        m_ownsAssetsImageList = nullptr != m_assetsImageList;
        ListView_SetImageList(m_assetsListView, m_assetsImageList, LVSIL_SMALL);
    }

    void AssetsPanelController::ReloadEditorIconImages(const std::filesystem::path& assetsRootDirectory)
    {
        m_fileIconIndices.clear();
        m_systemIconIndices.clear();
        m_defaultFileIconIndex.reset();
        m_folderIconIndex.reset();
        m_loadedIconRootDirectory.clear();

        InitializeAssetsImageList();
        if (nullptr == m_assetsImageList)
        {
            return;
        }

        const std::filesystem::path iconsDirectory = FindEditorIconsDirectory(assetsRootDirectory);
        if (iconsDirectory.empty())
        {
            return;
        }

        const auto addIcon = [this, &iconsDirectory](const wchar_t* fileName) -> std::optional<int>
        {
            HBITMAP bitmap = LoadPngIconBitmap(iconsDirectory / fileName);
            if (nullptr == bitmap)
            {
                return std::nullopt;
            }

            const int iconIndex = ImageList_Add(m_assetsImageList, bitmap, nullptr);
            DeleteObject(bitmap);
            if (iconIndex < 0)
            {
                return std::nullopt;
            }

            return iconIndex;
        };

        if (const std::optional<int> iconIndex = addIcon(L"file_proj.png"); iconIndex.has_value())
        {
            m_fileIconIndices.emplace(L".proj", *iconIndex);
        }

        if (const std::optional<int> iconIndex = addIcon(L"file_script.png"); iconIndex.has_value())
        {
            m_fileIconIndices.emplace(L".script", *iconIndex);
        }

        if (const std::optional<int> iconIndex = addIcon(L"file_sprite.png"); iconIndex.has_value())
        {
            m_fileIconIndices.emplace(L".sprite", *iconIndex);
        }

        if (const std::optional<int> iconIndex = addIcon(L"file_material.png"); iconIndex.has_value())
        {
            m_fileIconIndices.emplace(L".material", *iconIndex);
        }

        m_defaultFileIconIndex = addIcon(L"file_default.png");
        m_folderIconIndex = addIcon(L"folder.png");
        m_loadedIconRootDirectory = iconsDirectory;
    }

    int AssetsPanelController::ResolveAssetIconIndex(
        const std::filesystem::path& path,
        bool isDirectory,
        int fallbackIconIndex)
    {
        if (isDirectory)
        {
            return m_folderIconIndex.value_or(ResolveFallbackSystemIconIndex(fallbackIconIndex));
        }

        const std::wstring extension = NormalizeExtension(path.extension().wstring());
        const auto iconIt = m_fileIconIndices.find(extension);
        if (iconIt != m_fileIconIndices.end())
        {
            return iconIt->second;
        }

        return m_defaultFileIconIndex.value_or(ResolveFallbackSystemIconIndex(fallbackIconIndex));
    }

    int AssetsPanelController::ResolveFallbackSystemIconIndex(int systemIconIndex)
    {
        const auto cacheIt = m_systemIconIndices.find(systemIconIndex);
        if (cacheIt != m_systemIconIndices.end())
        {
            return cacheIt->second;
        }

        if (nullptr == m_assetsImageList)
        {
            return 0;
        }

        SHFILEINFOW fileInfo{};
        const HIMAGELIST systemImageList = reinterpret_cast<HIMAGELIST>(SHGetFileInfoW(
            L"C:\\",
            FILE_ATTRIBUTE_DIRECTORY,
            &fileInfo,
            sizeof(fileInfo),
            SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES));
        if (nullptr == systemImageList)
        {
            return 0;
        }

        HICON icon = ImageList_GetIcon(systemImageList, systemIconIndex, ILD_NORMAL);
        if (nullptr == icon)
        {
            return 0;
        }

        const int iconIndex = ImageList_AddIcon(m_assetsImageList, icon);
        DestroyIcon(icon);
        if (iconIndex < 0)
        {
            return 0;
        }

        m_systemIconIndices.emplace(systemIconIndex, iconIndex);
        return iconIndex;
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
        parentEntry.iconIndex = ResolveAssetIconIndex(parentEntry.path, true, GetFolderIconIndex());
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

            if (EditorPathSecurity::IsPathInsideOrEqual(entry.path(), m_assetsRootDirectory))
            {
                entries.push_back(entry);
            }
        }

        std::sort(entries.begin(), entries.end(), CompareDirectoryEntry);
        for (const std::filesystem::directory_entry& entry : entries)
        {
            AssetListEntry visibleEntry = BuildEntry(entry);
            visibleEntry.iconIndex = ResolveAssetIconIndex(
                visibleEntry.path,
                visibleEntry.isDirectory,
                visibleEntry.iconIndex);
            m_visibleEntries.push_back(std::move(visibleEntry));
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
            m_selectedSpriteAssetId = EditorPathSecurity::IsPathInsideOrEqual(entry.path, m_assetsRootDirectory)
                && EditorAssetPathUtils::IsTextureImageFile(entry.path)
                ? EditorAssetPathUtils::BuildSpriteAssetId(entry.path, m_assetsRootDirectory)
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
            if (EditorPathSecurity::IsPathInsideOrEqual(entry.path, m_assetsRootDirectory)
                && EditorAssetPathUtils::IsMaterialAssetFile(entry.path))
            {
                m_openMaterialAssetId = EditorAssetPathUtils::BuildMaterialAssetId(entry.path, m_assetsRootDirectory);
                m_openMaterialRequested = false == m_openMaterialAssetId.IsEmpty();
                return m_openMaterialRequested;
            }

            return TryOpenScriptAssetSource(entry);
        }

        if (entry.isParentLink && m_currentDirectory == m_assetsRootDirectory)
        {
            return false;
        }

        if (false == EditorPathSecurity::IsPathInsideOrEqual(entry.path, m_assetsRootDirectory))
        {
            return false;
        }

        m_currentDirectory = EditorPathSecurity::NormalizeForContainment(entry.path);
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

    bool AssetsPanelController::TryOpenScriptAssetSource(const AssetListEntry& entry)
    {
        if (entry.isDirectory
            || entry.isParentLink
            || false == ScriptAssetService::IsScriptAssetFile(entry.path)
            || false == EditorPathSecurity::IsPathInsideOrEqual(entry.path, m_assetsRootDirectory))
        {
            return false;
        }

        const auto sourcePath = ScriptAssetService::ResolveSourcePath(m_assetsRootDirectory, entry.path);
        if (false == sourcePath.has_value() || false == std::filesystem::exists(*sourcePath))
        {
            MessageBoxW(
                m_assetsListView,
                L"Script Asset に対応する C++ コードファイルを開けませんでした。",
                L"Assets",
                MB_OK | MB_ICONERROR);
            return true;
        }

        const HINSTANCE executeResult = ShellExecuteW(
            m_assetsListView,
            L"open",
            sourcePath->c_str(),
            nullptr,
            sourcePath->parent_path().c_str(),
            SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(executeResult) <= 32)
        {
            MessageBoxW(
                m_assetsListView,
                L"Visual Studio または関連付けられたエディタを起動できませんでした。",
                L"Assets",
                MB_OK | MB_ICONERROR);
        }

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
        if (false == entry.isDirectory && EditorAssetPathUtils::IsSpriteAssetFile(entry.path))
        {
            AppendMenuW(popupMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(popupMenu, MF_STRING, AssignScriptMenuCommandId, L"Scriptを割り当て");
        }

        const UINT command = TrackPopupMenu(
            popupMenu,
            TPM_RETURNCMD | TPM_RIGHTBUTTON,
            screenPoint.x,
            screenPoint.y,
            0,
            m_assetsListView,
            nullptr);
        DestroyMenu(popupMenu);

        if (AssignScriptMenuCommandId == command)
        {
            m_assignScriptRequested = true;
            m_assignScriptSpriteAssetPath = entry.path;
            return true;
        }

        if (DeleteEntryMenuCommandId != command)
        {
            return false;
        }

        return DeleteEntry(entryIndex);
    }

    bool AssetsPanelController::ShowCreateAssetContextMenu(
        const std::filesystem::path& targetDirectory,
        POINT screenPoint)
    {
        if (nullptr == m_assetsListView || true == targetDirectory.empty())
        {
            return false;
        }

        if (false == EditorPathSecurity::IsPathInsideOrEqual(targetDirectory, m_assetsRootDirectory))
        {
            return false;
        }

        HMENU popupMenu = CreatePopupMenu();
        if (nullptr == popupMenu)
        {
            return false;
        }

        AppendMenuW(popupMenu, MF_STRING, CreateSpriteMenuCommandId, L"Spriteを作成");
        AppendMenuW(popupMenu, MF_STRING, CreateMaterialMenuCommandId, L"Materialを作成");
        AppendMenuW(popupMenu, MF_STRING, CreateScriptMenuCommandId, L"Scriptを作成");

        const UINT command = TrackPopupMenu(
            popupMenu,
            TPM_RETURNCMD | TPM_RIGHTBUTTON,
            screenPoint.x,
            screenPoint.y,
            0,
            m_assetsListView,
            nullptr);
        DestroyMenu(popupMenu);

        if (CreateSpriteMenuCommandId == command)
        {
            m_createSpriteRequested = true;
            m_createSpriteTargetDirectory = targetDirectory;
            return true;
        }

        if (CreateScriptMenuCommandId == command)
        {
            m_createScriptRequested = true;
            m_createScriptTargetDirectory = targetDirectory;
            return true;
        }

        if (CreateMaterialMenuCommandId == command)
        {
            m_createMaterialRequested = true;
            m_createMaterialTargetDirectory = targetDirectory;
            return true;
        }

        if (0 != command)
        {
            return false;
        }

        return false;
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

        if (false == EditorPathSecurity::IsPathInsideOrEqual(entry.path, m_assetsRootDirectory))
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

        if (m_draggingImagePath == entry.path || m_draggingScriptAssetPath == entry.path)
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
        ScopedComInitialization comInitialization(COINIT_APARTMENTTHREADED);

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

        return thumbnail;
    }

    HIMAGELIST AssetsPanelController::CreateDragImageList(
        const std::filesystem::path& imagePath,
        int fallbackIconIndex) const
    {
        constexpr int DragImageSize = 48;
        ScopedComInitialization comInitialization(COINIT_APARTMENTTHREADED);

        HIMAGELIST imageList = ImageList_Create(
            DragImageSize,
            DragImageSize,
            ILC_COLOR32 | ILC_MASK,
            1,
            1);
        if (nullptr == imageList)
        {
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
                    return imageList;
                }
            }
        }

        ImageList_Destroy(imageList);
        return nullptr;
    }

}
