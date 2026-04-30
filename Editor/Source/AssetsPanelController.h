#pragma once

#include <Windows.h>
#include <vector>

#include "AssetId.h"
#include "Assets/SpriteAssetRegistry.h"
#include "EditorShell.h"
#include "InputSystem.h"
#include "TextureAssetRegistry.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Assets パネルの表示状態とドラッグ状態を管理する。
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
        /// Assets パネルの一覧表示と選択表示を更新する。
        /// </summary>
        /// <param name="registeredSpriteAssetIds">登録済み SpriteAssetId 一覧。</param>
        /// <param name="spriteAssetRegistry">SpriteAsset レジストリ。</param>
        /// <param name="textureAssetRegistry">Texture レジストリ。</param>
        void Refresh(
            const std::vector<Core::AssetId>& registeredSpriteAssetIds,
            const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
            const Graphics::TextureAssetRegistry& textureAssetRegistry);

        /// <summary>
        /// ListBox 選択状態を内部選択状態へ同期する。
        /// </summary>
        void SyncSelection();

        /// <summary>
        /// Assets パネル由来のドラッグ状態を更新する。
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
        /// ドラッグ中かを取得する。
        /// </summary>
        /// <returns>ドラッグ中の場合は true。</returns>
        [[nodiscard]] bool IsDragActive() const;

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

    private:
        /// <summary>
        /// Assets パネルの要約ラベルを更新する。
        /// </summary>
        void RefreshSummaryLabel();

    private:
        HWND m_assetsListBox = nullptr;
        HWND m_assetsSummaryLabel = nullptr;
        std::vector<Core::AssetId> m_visibleSpriteAssetIds{};
        Core::AssetId m_selectedSpriteAssetId{};
        Core::AssetId m_draggingSpriteAssetId{};
        bool m_isAssetDragActive = false;
        bool m_assetsListLeftButtonDown = false;
        bool m_assetDragReleasedThisFrame = false;
        POINT m_assetDragStartScreenPoint{};
        std::size_t m_registeredSpriteAssetCount = 0;
    };
}
