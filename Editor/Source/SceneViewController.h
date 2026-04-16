#pragma once

#include <Windows.h>
#include <memory>
#include <optional>
#include <vector>

#include "AssetId.h"
#include "Assets/SpriteAssetRegistry.h"
#include "AssetsPanelController.h"
#include "EditorCamera2D.h"
#include "EditorSceneDocument.h"
#include "EditorShell.h"
#include "Entity.h"
#include "SceneViewOverlay.h"
#include "Texture2D.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// SceneView ドラッグプレビューの現在状態を表す。
    /// </summary>
    struct SceneDragPreviewState
    {
        /// <summary>
        /// プレビュー対象の SpriteAssetId を表す。
        /// </summary>
        Core::AssetId spriteAssetId{};

        /// <summary>
        /// プレビュー描画に使用するテクスチャを表す。
        /// </summary>
        std::shared_ptr<Graphics::Texture2D> texture{};

        /// <summary>
        /// プレビュー中心のワールド座標 X を表す。
        /// </summary>
        float worldX = 0.0f;

        /// <summary>
        /// プレビュー中心のワールド座標 Y を表す。
        /// </summary>
        float worldY = 0.0f;

        /// <summary>
        /// プレビュー描画位置の View X 座標を表す。
        /// </summary>
        float viewX = 0.0f;

        /// <summary>
        /// プレビュー描画位置の View Y 座標を表す。
        /// </summary>
        float viewY = 0.0f;

        /// <summary>
        /// プレビュー表示対象があるかを表す。
        /// </summary>
        bool hasPreview = false;

        /// <summary>
        /// カーソルが SceneView 内にあるかを表す。
        /// </summary>
        bool isCursorInside = false;
    };

    /// <summary>
    /// SceneView 入力更新による選択変更結果を表す。
    /// </summary>
    struct SceneViewInteractionResult
    {
        /// <summary>
        /// 選択状態が変化したかを表す。
        /// </summary>
        bool selectionChanged = false;

        /// <summary>
        /// 更新後の選択 EntityId を表す。
        /// </summary>
        std::optional<Game::EntityId> selectedEntityId{};
    };

    /// <summary>
    /// SceneView ドロップ処理結果を表す。
    /// </summary>
    struct SceneViewDropResult
    {
        /// <summary>
        /// Scene が変更されたかを表す。
        /// </summary>
        bool sceneChanged = false;

        /// <summary>
        /// コマンド履歴へスナップショット追加すべきかを表す。
        /// </summary>
        bool shouldPushHistory = false;

        /// <summary>
        /// 選択状態が変化したかを表す。
        /// </summary>
        bool selectionChanged = false;

        /// <summary>
        /// 更新後の選択 EntityId を表す。
        /// </summary>
        std::optional<Game::EntityId> selectedEntityId{};
    };

    /// <summary>
    /// SceneView の Editor 入力状態とドロップ処理を管理する。
    /// </summary>
    class SceneViewController
    {
    public:
        /// <summary>
        /// EditorShell の HWND 群へ接続する。
        /// </summary>
        /// <param name="shell">接続先の EditorShell。</param>
        void Bind(const EditorShell& shell);

        /// <summary>
        /// SceneView ビューポートサイズ変更を反映する。
        /// </summary>
        /// <param name="width">SceneView 幅。</param>
        /// <param name="height">SceneView 高さ。</param>
        void OnViewportChanged(std::uint32_t width, std::uint32_t height);

        /// <summary>
        /// SceneView 入力状態を更新する。
        /// </summary>
        /// <param name="scene">編集中 Scene。</param>
        /// <param name="spriteAssetRegistry">SpriteAsset レジストリ。</param>
        /// <param name="textureAssetRegistry">Texture レジストリ。</param>
        /// <param name="assetsPanelController">Assets パネル状態。</param>
        /// <param name="currentSelectedEntityId">現在選択中の EntityId。</param>
        /// <returns>入力更新結果。</returns>
        SceneViewInteractionResult UpdateInteraction(
            const Game::Scene* scene,
            const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
            const Graphics::TextureAssetRegistry& textureAssetRegistry,
            const AssetsPanelController& assetsPanelController,
            std::optional<Game::EntityId> currentSelectedEntityId);

        /// <summary>
        /// 保留中の SceneView ドロップを Scene へ反映する。
        /// </summary>
        /// <param name="document">更新対象の Scene ドキュメント。</param>
        /// <returns>ドロップ処理結果。</returns>
        SceneViewDropResult ProcessPendingSceneDrop(EditorSceneDocument& document);

        /// <summary>
        /// 現在選択状態に応じて SceneView ラベルを更新する。
        /// </summary>
        /// <param name="scene">参照対象の Scene。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        void RefreshSelectionStatus(const Game::Scene* scene, std::optional<Game::EntityId> selectedEntityId);

        /// <summary>
        /// SceneView に使用する EditorCamera2D を取得する。
        /// </summary>
        /// <returns>SceneView 用カメラ。</returns>
        [[nodiscard]] const EditorCamera2D& GetCamera() const;

        /// <summary>
        /// 現在のドラッグプレビュー状態を取得する。
        /// </summary>
        /// <returns>ドラッグプレビュー状態。</returns>
        [[nodiscard]] const SceneDragPreviewState& GetDragPreviewState() const;

    private:
        /// <summary>
        /// SceneView 上のヒットターゲット一覧を構築する。
        /// </summary>
        /// <param name="scene">対象 Scene。</param>
        /// <param name="spriteAssetRegistry">SpriteAsset レジストリ。</param>
        /// <param name="textureAssetRegistry">Texture レジストリ。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        /// <returns>ヒットターゲット一覧。</returns>
        std::vector<SceneViewHitTarget> BuildSceneHitTargets(
            const Game::Scene& scene,
            const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
            const Graphics::TextureAssetRegistry& textureAssetRegistry,
            std::optional<Game::EntityId> selectedEntityId) const;

        /// <summary>
        /// ドラッグプレビュー状態を更新する。
        /// </summary>
        /// <param name="spriteAssetId">プレビュー対象の SpriteAssetId。</param>
        /// <param name="worldPoint">プレビュー中心のワールド座標。</param>
        /// <param name="screenPoint">プレビュー描画に使うスクリーン座標。</param>
        /// <param name="isCursorInside">カーソルが SceneView 内にある場合は true。</param>
        /// <param name="spriteAssetRegistry">SpriteAsset レジストリ。</param>
        /// <param name="textureAssetRegistry">Texture レジストリ。</param>
        void UpdateSceneDragPreview(
            const Core::AssetId& spriteAssetId,
            const EditorWorldPoint& worldPoint,
            const EditorScreenPoint& screenPoint,
            bool isCursorInside,
            const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
            const Graphics::TextureAssetRegistry& textureAssetRegistry);

        /// <summary>
        /// ドラッグプレビュー状態を破棄する。
        /// </summary>
        void ClearSceneDragPreview();

    private:
        HWND m_sceneViewHost = nullptr;
        HWND m_sceneViewPlanLabel = nullptr;
        HWND m_sceneViewSizeLabel = nullptr;
        EditorCamera2D m_sceneViewCamera{};
        std::uint32_t m_sceneViewWidth = 0;
        std::uint32_t m_sceneViewHeight = 0;
        float m_lastSceneClickX = 0.0f;
        float m_lastSceneClickY = 0.0f;
        bool m_hasSceneClick = false;
        bool m_sceneViewLeftButtonDown = false;
        Core::AssetId m_pendingDroppedSpriteAssetId{};
        float m_pendingDropWorldX = 0.0f;
        float m_pendingDropWorldY = 0.0f;
        bool m_hasPendingSceneDrop = false;
        SceneDragPreviewState m_dragPreviewState{};
    };
}
