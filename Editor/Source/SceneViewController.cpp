#include "SceneViewController.h"

#include "AssetsPanelController.h"
#include "EditorSceneDocument.h"
#include <cstdint>
#include <optional>
#include "EditorCamera2D.h"
#include "EditorShell.h"
#include "SceneViewInteractionTypes.h"
#include <Assets/SpriteAssetRegistry.h>
#include <Entity.h>
#include <Scene.h>
#include <TextureAssetRegistry.h>

namespace Xelqoria::Editor
{
    void SceneViewController::Bind(const EditorShell& shell)
    {
        m_sceneViewHost = shell.GetSceneViewHost();
        m_sceneViewPlanLabel = shell.GetSceneViewPlanLabel();
        m_sceneViewSizeLabel = shell.GetSceneViewSizeLabel();
        m_inputTracker.Bind(m_sceneViewHost);
    }

    void SceneViewController::OnViewportChanged(std::uint32_t width, std::uint32_t height)
    {
        m_sceneViewWidth = width;
        m_sceneViewHeight = height;
        m_sceneViewCamera.SetViewport(m_sceneViewWidth, m_sceneViewHeight);
    }

    SceneViewInteractionResult SceneViewController::UpdateInteraction(
        const Game::Scene* scene,
        const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
        const Graphics::TextureAssetRegistry& textureAssetRegistry,
        const AssetsPanelController& assetsPanelController,
        std::optional<Game::EntityId> currentSelectedEntityId)
    {
        const SceneViewInteractionResult result = m_inputTracker.UpdateInteraction(
            scene,
            spriteAssetRegistry,
            textureAssetRegistry,
            assetsPanelController,
            m_sceneViewCamera,
            m_sceneViewWidth,
            m_sceneViewHeight,
            currentSelectedEntityId);
        const std::optional<Game::EntityId> nextSelectedEntityId =
            true == result.selectionChanged ? result.selectedEntityId : currentSelectedEntityId;
        m_statusPresenter.PresentInteractionStatus(
            m_sceneViewSizeLabel,
            m_sceneViewPlanLabel,
            m_sceneViewWidth,
            m_sceneViewHeight,
            scene,
            nextSelectedEntityId,
            m_inputTracker.GetPendingDropState(),
            m_inputTracker.GetDragPreviewState(),
            m_inputTracker.HasSceneClick(),
            m_inputTracker.GetLastSceneClickX(),
            m_inputTracker.GetLastSceneClickY());
        return result;
    }

    SceneViewDropResult SceneViewController::ProcessPendingSceneDrop(EditorSceneDocument& document)
    {
        const SceneViewDropResult result = m_dropPlacementService.ProcessPendingSceneDrop(
            document,
            m_inputTracker.GetPendingDropState());
        m_inputTracker.ClearPendingSceneDrop();
        m_statusPresenter.PresentDropStatus(
            m_sceneViewSizeLabel,
            m_sceneViewPlanLabel,
            m_sceneViewWidth,
            m_sceneViewHeight,
            result);
        return result;
    }

    void SceneViewController::RefreshSelectionStatus(const Game::Scene* scene, std::optional<Game::EntityId> selectedEntityId)
    {
        m_statusPresenter.PresentSelectionStatus(
            m_sceneViewSizeLabel,
            m_sceneViewWidth,
            m_sceneViewHeight,
            scene,
            selectedEntityId);
    }

    const EditorCamera2D& SceneViewController::GetCamera() const
    {
        return m_sceneViewCamera;
    }

    const SceneDragPreviewState& SceneViewController::GetDragPreviewState() const
    {
        return m_inputTracker.GetDragPreviewState();
    }
}
