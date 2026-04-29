#pragma once

#include <Windows.h>
#include <optional>
#include <cstdint>
#include <vector>

#include "Assets/SpriteAssetRegistry.h"
#include "AssetsPanelController.h"
#include "EditorCamera2D.h"
#include "InputSystem.h"
#include "Scene.h"
#include "SceneViewInteractionTypes.h"
#include "SceneViewOverlay.h"
#include "TextureAssetRegistry.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// SceneView の入力状態、ドラッグプレビュー、保留ドロップを追跡する。
    /// </summary>
    class SceneViewInputTracker
    {
    public:
        /// <summary>
        /// SceneView 描画先 child window を設定する。
        /// </summary>
        /// <param name="sceneViewHost">SceneView host の HWND。</param>
        void Bind(HWND sceneViewHost);

        /// <summary>
        /// SceneView 入力状態を更新する。
        /// </summary>
        /// <param name="scene">編集中 Scene。</param>
        /// <param name="spriteAssetRegistry">SpriteAsset レジストリ。</param>
        /// <param name="textureAssetRegistry">Texture レジストリ。</param>
        /// <param name="assetsPanelController">Assets パネル状態。</param>
        /// <param name="camera">SceneView 用カメラ。</param>
        /// <param name="sceneViewWidth">SceneView 幅。</param>
        /// <param name="sceneViewHeight">SceneView 高さ。</param>
        /// <param name="currentSelectedEntityId">現在選択中の EntityId。</param>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        /// <returns>入力更新結果。</returns>
        SceneViewInteractionResult UpdateInteraction(
            Game::Scene* scene,
            const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
            const Graphics::TextureAssetRegistry& textureAssetRegistry,
            const AssetsPanelController& assetsPanelController,
            const EditorCamera2D& camera,
            std::uint32_t sceneViewWidth,
            std::uint32_t sceneViewHeight,
            std::optional<Game::EntityId> currentSelectedEntityId,
            const Core::InputSnapshot& inputSnapshot);

        /// <summary>
        /// 現在のドラッグプレビュー状態を取得する。
        /// </summary>
        /// <returns>ドラッグプレビュー状態。</returns>
        [[nodiscard]] const SceneDragPreviewState& GetDragPreviewState() const;

        /// <summary>
        /// 現在の保留中ドロップ状態を取得する。
        /// </summary>
        /// <returns>保留中ドロップ状態。</returns>
        [[nodiscard]] const ScenePendingDropState& GetPendingDropState() const;

        /// <summary>
        /// 保留中ドロップ状態を破棄する。
        /// </summary>
        void ClearPendingSceneDrop();

        /// <summary>
        /// 直近の SceneView クリック座標を保持しているかを取得する。
        /// </summary>
        /// <returns>保持している場合は true。</returns>
        [[nodiscard]] bool HasSceneClick() const;

        /// <summary>
        /// 直近の SceneView クリック X 座標を取得する。
        /// </summary>
        /// <returns>ワールド X 座標。</returns>
        [[nodiscard]] float GetLastSceneClickX() const;

        /// <summary>
        /// 直近の SceneView クリック Y 座標を取得する。
        /// </summary>
        /// <returns>ワールド Y 座標。</returns>
        [[nodiscard]] float GetLastSceneClickY() const;

    private:
        /// <summary>
        /// 解決済み Sprite 一覧からヒットターゲットを構築する。
        /// </summary>
        /// <param name="resolvedSprites">解決済み Sprite 一覧。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        /// <returns>ヒットターゲット一覧。</returns>
        std::vector<SceneViewHitTarget> BuildSceneHitTargets(
            const std::vector<Game::ResolvedSceneSprite>& resolvedSprites,
            std::optional<Game::EntityId> selectedEntityId) const;

        /// <summary>
        /// ドラッグプレビュー状態を更新する。
        /// </summary>
        /// <param name="spriteAssetId">プレビュー対象の SpriteAssetId。</param>
        /// <param name="worldPoint">プレビュー中心のワールド座標。</param>
        /// <param name="screenPoint">プレビュー描画に使うスクリーン座標。</param>
        /// <param name="isCursorInside">カーソルが SceneView 内にある場合は true。</param>
        /// <param name="sceneViewWidth">SceneView 幅。</param>
        /// <param name="sceneViewHeight">SceneView 高さ。</param>
        /// <param name="spriteAssetRegistry">SpriteAsset レジストリ。</param>
        /// <param name="textureAssetRegistry">Texture レジストリ。</param>
        void UpdateSceneDragPreview(
            const Core::AssetId& spriteAssetId,
            const EditorWorldPoint& worldPoint,
            const EditorScreenPoint& screenPoint,
            bool isCursorInside,
            std::uint32_t sceneViewWidth,
            std::uint32_t sceneViewHeight,
            const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
            const Graphics::TextureAssetRegistry& textureAssetRegistry);

        /// <summary>
        /// ドラッグプレビュー状態を破棄する。
        /// </summary>
        void ClearSceneDragPreview();

        /// <summary>
        /// Entity ドラッグ状態を破棄する。
        /// </summary>
        void ClearEntityDrag();

    private:
        /// <summary>
        /// SceneView 上での Entity ドラッグ中状態を表す。
        /// </summary>
        struct SceneEntityDragState
        {
            /// <summary>
            /// ドラッグ対象の EntityId を表す。
            /// </summary>
            std::optional<Game::EntityId> entityId{};

            /// <summary>
            /// ドラッグ開始時のカーソルと Entity 位置の X 差分を表す。
            /// </summary>
            float grabOffsetX = 0.0f;

            /// <summary>
            /// ドラッグ開始時のカーソルと Entity 位置の Y 差分を表す。
            /// </summary>
            float grabOffsetY = 0.0f;

            /// <summary>
            /// ドラッグ開始時の Entity X 座標を表す。
            /// </summary>
            float initialWorldX = 0.0f;

            /// <summary>
            /// ドラッグ開始時の Entity Y 座標を表す。
            /// </summary>
            float initialWorldY = 0.0f;

            /// <summary>
            /// 現在フレームまでに位置が変化したかを表す。
            /// </summary>
            bool hasMoved = false;
        };

        HWND m_sceneViewHost = nullptr;
        float m_lastSceneClickX = 0.0f;
        float m_lastSceneClickY = 0.0f;
        bool m_hasSceneClick = false;
        bool m_sceneViewLeftButtonDown = false;
        ScenePendingDropState m_pendingDropState{};
        SceneDragPreviewState m_dragPreviewState{};
        SceneEntityDragState m_entityDragState{};
    };
}
