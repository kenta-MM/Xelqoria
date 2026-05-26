#pragma once

#include <Windows.h>
#include <optional>
#include <cstdint>

#include "SceneView/EditorCamera2D.h"
#include "Shell/EditorShell.h"
#include "InputSystem.h"
#include "DragDrop/SceneDropPlacementService.h"
#include "SceneView/SceneViewInputTracker.h"
#include "SceneView/SceneViewInteractionTypes.h"
#include "SceneView/SceneViewStatusPresenter.h"
#include "Panels/AssetsPanelController.h"
#include "Project/EditorSceneDocument.h"
#include <Assets/IMaterialAssetResolver.h>
#include <Assets/SpriteAssetRegistry.h>
#include <Entity.h>
#include <Scene.h>
#include <TextureAssetRegistry.h>

namespace Xelqoria::Editor
{
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
        /// <param name="surface">SceneView の描画先境界。</param>
        void OnViewportChanged(const SceneViewSurface& surface);

        /// <summary>
        /// SceneView 入力状態を更新する。
        /// </summary>
        /// <param name="scene">編集中 Scene。</param>
        /// <param name="spriteAssetRegistry">SpriteAsset レジストリ。</param>
        /// <param name="materialAssetResolver">MaterialAsset Resolver。</param>
        /// <param name="textureAssetRegistry">Texture レジストリ。</param>
        /// <param name="assetsPanelController">Assets パネル状態。</param>
        /// <param name="currentSelectedEntityId">現在選択中の EntityId。</param>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        /// <returns>入力更新結果。</returns>
        SceneViewInteractionResult UpdateInteraction(
            Game::Scene* scene,
            const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
            const Game::Assets::IMaterialAssetResolver& materialAssetResolver,
            const Graphics::TextureAssetRegistry& textureAssetRegistry,
            const AssetsPanelController& assetsPanelController,
            std::optional<Game::EntityId> currentSelectedEntityId,
            const Core::InputSnapshot& inputSnapshot);

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

        /// <summary>
        /// 現在の SceneView 編集モードを取得する。
        /// </summary>
        /// <returns>現在の編集モード。</returns>
        [[nodiscard]] SceneViewEditMode GetEditMode() const;

    private:
        /// <summary>
        /// SceneView に関連付く HWND 群と責務分離済みサービス群を保持する。
        /// </summary>
        HWND m_sceneViewHost = nullptr;
        HWND m_sceneViewPlanLabel = nullptr;
        HWND m_sceneViewSizeLabel = nullptr;
        EditorCamera2D m_sceneViewCamera{};
        std::uint32_t m_sceneViewWidth = 0;
        std::uint32_t m_sceneViewHeight = 0;
        SceneViewInputTracker m_inputTracker{};
        SceneDropPlacementService m_dropPlacementService{};
        SceneViewStatusPresenter m_statusPresenter{};
    };
}
