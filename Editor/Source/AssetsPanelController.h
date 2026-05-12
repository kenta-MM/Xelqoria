#pragma once

#include <Windows.h>
#include <CommCtrl.h>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "AssetId.h"
#include "EditorProject.h"
#include "EditorShell.h"
#include "InputSystem.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Assets 詳細リストに表示するファイルシステム項目を表す。
    /// </summary>
    struct AssetListEntry
    {
        /// <summary>
        /// 項目の絶対パスを表す。
        /// </summary>
        std::filesystem::path path{};

        /// <summary>
        /// 名前列の表示文字列を表す。
        /// </summary>
        std::wstring displayName{};

        /// <summary>
        /// 更新日時列の表示文字列を表す。
        /// </summary>
        std::wstring modifiedTimeText{};

        /// <summary>
        /// 種類列の表示文字列を表す。
        /// </summary>
        std::wstring typeName{};

        /// <summary>
        /// サイズ列の表示文字列を表す。
        /// </summary>
        std::wstring sizeText{};

        /// <summary>
        /// システムイメージリスト上のアイコン番号を表す。
        /// </summary>
        int iconIndex = 0;

        /// <summary>
        /// フォルダ項目かを表す。
        /// </summary>
        bool isDirectory = false;

        /// <summary>
        /// 親フォルダへ戻るための項目かを表す。
        /// </summary>
        bool isParentLink = false;
    };

    /// <summary>
    /// Assets パネルのファイル詳細リスト表示と選択状態を管理する。
    /// </summary>
    class AssetsPanelController
    {
    public:
        /// <summary>
        /// EditorShell の HWND 群へ接続する。
        /// </summary>
        /// <param name="shell">接続先の EditorShell。</param>
        void Bind(const EditorShell& shell);

        /// <summary>
        /// Assets パネルのファイル詳細リスト表示を更新する。
        /// </summary>
        /// <param name="projectInfo">開いている Editor プロジェクト情報。</param>
        void Refresh(const std::optional<EditorProjectInfo>& projectInfo);

        /// <summary>
        /// ListView 選択状態をファイル選択状態へ同期する。
        /// </summary>
        void SyncSelection();

        /// <summary>
        /// Assets ListView からの Win32 通知を処理する。
        /// </summary>
        /// <param name="notifyParameter">WM_NOTIFY の LPARAM。</param>
        /// <returns>通知を処理した場合は true。</returns>
        [[nodiscard]] bool HandleNotify(LPARAM notifyParameter);

        /// <summary>
        /// Assets パネル由来の入力状態を更新する。
        /// </summary>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        void UpdateDragState(const Core::InputSnapshot& inputSnapshot);

        /// <summary>
        /// ドラッグ解放を SceneView 側で処理した後に状態を確定する。
        /// </summary>
        void CompleteReleasedDrag();

        /// <summary>
        /// 現在選択中の SpriteAssetId を取得する。
        /// </summary>
        /// <returns>選択中 SpriteAssetId。</returns>
        [[nodiscard]] const Core::AssetId& GetSelectedAssetId() const;

        /// <summary>
        /// 現在ドラッグ中の SpriteAssetId を取得する。
        /// </summary>
        /// <returns>ドラッグ中 SpriteAssetId。</returns>
        [[nodiscard]] const Core::AssetId& GetDraggingSpriteAssetId() const;

        /// <summary>
        /// 現在ドラッグ中の TextureAssetId を取得する。
        /// </summary>
        /// <returns>ドラッグ中 TextureAssetId。</returns>
        [[nodiscard]] const Core::AssetId& GetDraggingTextureAssetId() const;

        /// <summary>
        /// 現在ドラッグ中の画像ファイルパスを取得する。
        /// </summary>
        /// <returns>ドラッグ中画像ファイルパス。</returns>
        [[nodiscard]] const std::filesystem::path& GetDraggingImagePath() const;

        /// <summary>
        /// ドラッグ中かを取得する。
        /// </summary>
        /// <returns>ドラッグ中の場合は true。</returns>
        [[nodiscard]] bool IsDragActive() const;

        /// <summary>
        /// 現在ドラッグ中のアセットを SceneView へ配置できるかを取得する。
        /// </summary>
        /// <returns>SceneView へ配置できる場合は true。</returns>
        [[nodiscard]] bool CanPlaceDraggingAssetInScene() const;

        /// <summary>
        /// 今フレームでドラッグ解放を検出したかを取得する。
        /// </summary>
        /// <returns>今フレームでドラッグ解放した場合は true。</returns>
        [[nodiscard]] bool WasDragReleasedThisFrame() const;

        /// <summary>
        /// Inspector から追加可能な SpriteAsset が存在するかを取得する。
        /// </summary>
        /// <returns>表示可能な SpriteAsset が 1 件以上ある場合は true。</returns>
        [[nodiscard]] bool HasVisibleSpriteAssets() const;

        /// <summary>
        /// Assets 空白右クリックから Sprite 作成が要求されたかを取得する。
        /// </summary>
        /// <returns>Sprite 作成要求がある場合は true。</returns>
        [[nodiscard]] bool HasCreateSpriteRequest() const;

        /// <summary>
        /// Sprite 作成要求を消費する。
        /// </summary>
        void ClearCreateSpriteRequest();

        /// <summary>
        /// Assets 空白右クリックから Script 作成が要求されたかを取得する。
        /// </summary>
        /// <returns>Script 作成要求がある場合は true。</returns>
        [[nodiscard]] bool HasCreateScriptRequest() const;

        /// <summary>
        /// Script 作成要求を消費する。
        /// </summary>
        void ClearCreateScriptRequest();

        /// <summary>
        /// Sprite アセットファイルの作成先フォルダを取得する。
        /// </summary>
        /// <returns>作成先フォルダ。未指定時は空。</returns>
        [[nodiscard]] const std::filesystem::path& GetCreateSpriteTargetDirectory() const;

        /// <summary>
        /// Script Asset ファイルの作成先フォルダを取得する。
        /// </summary>
        /// <returns>作成先フォルダ。未指定時は空。</returns>
        [[nodiscard]] const std::filesystem::path& GetCreateScriptTargetDirectory() const;

    private:
        /// <summary>
        /// ListView の列とシステムアイコンを初期化する。
        /// </summary>
        void InitializeListView();

        /// <summary>
        /// 現在フォルダから表示項目を再構築する。
        /// </summary>
        void RebuildVisibleEntries();

        /// <summary>
        /// 親フォルダへ戻る項目を追加する。
        /// </summary>
        void AppendParentEntry();

        /// <summary>
        /// 現在フォルダ配下の項目を追加する。
        /// </summary>
        void AppendCurrentDirectoryEntries();

        /// <summary>
        /// ListView の表示内容を現在の表示項目へ同期する。
        /// </summary>
        void RefreshListView();

        /// <summary>
        /// ListView の現在選択を内部選択状態へ同期する。
        /// </summary>
        void SyncSelectedPathFromListView();

        /// <summary>
        /// 指定項目を開ける場合は現在フォルダを移動する。
        /// </summary>
        /// <param name="entryIndex">開く表示項目のインデックス。</param>
        /// <returns>フォルダ移動した場合は true。</returns>
        [[nodiscard]] bool TryOpenEntry(std::size_t entryIndex);

        /// <summary>
        /// 現在選択しているフォルダ項目を開く。
        /// </summary>
        /// <returns>フォルダ移動した場合は true。</returns>
        [[nodiscard]] bool TryOpenSelectedEntry();

        /// <summary>
        /// ListView の選択インデックスを取得する。
        /// </summary>
        /// <returns>選択インデックス。未選択時は -1。</returns>
        [[nodiscard]] int GetSelectedListViewIndex() const;

        /// <summary>
        /// スクリーン座標から ListView 項目を取得する。
        /// </summary>
        /// <param name="screenPoint">スクリーン座標。</param>
        /// <returns>項目インデックス。項目外の場合は -1。</returns>
        [[nodiscard]] int HitTestListView(POINT screenPoint) const;

        /// <summary>
        /// スクリーン座標から名前列の表示名上にある ListView 項目を取得する。
        /// </summary>
        /// <param name="screenPoint">スクリーン座標。</param>
        /// <returns>名前列の表示名上にある項目インデックス。対象外の場合は -1。</returns>
        [[nodiscard]] int HitTestListViewNameLabel(POINT screenPoint) const;

        /// <summary>
        /// Assets 項目用の右クリックメニューを表示して操作を適用する。
        /// </summary>
        /// <param name="entryIndex">対象項目のインデックス。</param>
        /// <param name="screenPoint">メニュー表示位置のスクリーン座標。</param>
        /// <returns>操作を処理した場合は true。</returns>
        [[nodiscard]] bool ShowEntryContextMenu(std::size_t entryIndex, POINT screenPoint);

        /// <summary>
        /// Assets 空白またはフォルダ用の作成メニューを表示する。
        /// </summary>
        /// <param name="targetDirectory">作成先フォルダ。</param>
        /// <param name="screenPoint">メニュー表示位置のスクリーン座標。</param>
        /// <returns>操作を処理した場合は true。</returns>
        [[nodiscard]] bool ShowCreateAssetContextMenu(
            const std::filesystem::path& targetDirectory,
            POINT screenPoint);

        /// <summary>
        /// 指定した Assets 項目をファイルシステムから削除する。
        /// </summary>
        /// <param name="entryIndex">削除対象項目のインデックス。</param>
        /// <returns>削除に成功した場合は true。</returns>
        [[nodiscard]] bool DeleteEntry(std::size_t entryIndex);

        /// <summary>
        /// Assets パネルの要約ラベルを更新する。
        /// </summary>
        void RefreshSummaryLabel();

        /// <summary>
        /// ドラッグ中の画像表示を開始する。
        /// </summary>
        /// <param name="imagePath">ドラッグ対象画像パス。</param>
        /// <param name="fallbackIconIndex">サムネイル取得失敗時に使うシステムアイコン番号。</param>
        /// <param name="screenPoint">開始時のスクリーン座標。</param>
        void BeginDragImage(
            const std::filesystem::path& imagePath,
            int fallbackIconIndex,
            POINT screenPoint);

        /// <summary>
        /// ドラッグ中の画像表示を現在カーソル位置へ移動する。
        /// </summary>
        /// <param name="screenPoint">スクリーン座標。</param>
        void MoveDragImage(POINT screenPoint);

        /// <summary>
        /// ドラッグ中の画像表示を終了してリソースを解放する。
        /// </summary>
        void EndDragImage();

        /// <summary>
        /// ドラッグ中ファイルの追従プレビューを開始する。
        /// </summary>
        /// <param name="imagePath">ドラッグ対象画像パス。</param>
        /// <param name="screenPoint">開始時のスクリーン座標。</param>
        void BeginDragPreview(const std::filesystem::path& imagePath, POINT screenPoint);

        /// <summary>
        /// ドラッグ中ファイルの追従プレビューを移動する。
        /// </summary>
        /// <param name="screenPoint">スクリーン座標。</param>
        void MoveDragPreview(POINT screenPoint);

        /// <summary>
        /// ドラッグ中ファイルの追従プレビューを破棄する。
        /// </summary>
        void EndDragPreview();

        /// <summary>
        /// ドラッグプレビュー用サムネイル画像を取得する。
        /// </summary>
        /// <param name="imagePath">対象画像パス。</param>
        /// <returns>取得した HBITMAP。失敗時は nullptr。</returns>
        [[nodiscard]] HBITMAP CreateDragPreviewBitmap(const std::filesystem::path& imagePath) const;

        /// <summary>
        /// 画像ファイル用のドラッグ表示 ImageList を作成する。
        /// </summary>
        /// <param name="imagePath">対象画像パス。</param>
        /// <param name="fallbackIconIndex">サムネイル取得失敗時に使うシステムアイコン番号。</param>
        /// <returns>作成した ImageList。失敗時は nullptr。</returns>
        [[nodiscard]] HIMAGELIST CreateDragImageList(
            const std::filesystem::path& imagePath,
            int fallbackIconIndex) const;

    private:
        HWND m_assetsListView = nullptr;
        HWND m_assetsSummaryLabel = nullptr;
        std::filesystem::path m_assetsRootDirectory{};
        std::filesystem::path m_currentDirectory{};
        std::vector<AssetListEntry> m_visibleEntries{};
        std::filesystem::path m_selectedFilePath{};
        Core::AssetId m_selectedSpriteAssetId{};
        Core::AssetId m_draggingSpriteAssetId{};
        Core::AssetId m_draggingTextureAssetId{};
        std::filesystem::path m_draggingImagePath{};
        ULONGLONG m_lastClickTick = 0;
        int m_lastClickedIndex = -1;
        bool m_isAssetDragActive = false;
        bool m_canPlaceDraggingAssetInScene = false;
        bool m_assetDragReleasedThisFrame = false;
        bool m_createSpriteRequested = false;
        std::filesystem::path m_createSpriteTargetDirectory{};
        bool m_createScriptRequested = false;
        std::filesystem::path m_createScriptTargetDirectory{};
        bool m_listViewInitialized = false;
        HIMAGELIST m_dragImageList = nullptr;
        bool m_isDragImageVisible = false;
        HWND m_dragPreviewWindow = nullptr;
        HWND m_dragPreviewImage = nullptr;
        HWND m_dragPreviewText = nullptr;
        HBITMAP m_dragPreviewBitmap = nullptr;
        HICON m_dragPreviewIcon = nullptr;
    };
}
