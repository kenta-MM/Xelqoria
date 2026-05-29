#include "Panels/SceneView/SceneViewController.h"

#include "Panels/Assets/AssetsPanelController.h"
#include "Project/EditorSceneDocument.h"
#include <cwchar>
#include <cstdint>
#include <iterator>
#include <optional>
#include "Panels/SceneView/SceneViewPanelView.h"
#include "SceneView/EditorCamera2D.h"
#include "SceneView/SceneViewInteractionTypes.h"
#include <Assets/IMaterialAssetResolver.h>
#include <Assets/SpriteAssetRegistry.h>
#include <Entity.h>
#include <Scene.h>
#include <TextureAssetRegistry.h>

namespace Xelqoria::Editor
{
    void SceneViewController::Bind(const SceneViewPanelView& view)
    {
        const SceneViewSurface sceneViewSurface = view.GetSceneViewSurface();
        m_sceneViewHost = static_cast<HWND>(sceneViewSurface.nativeWindow);
        m_sceneViewPlanLabel = view.GetSceneViewPlanLabel();
        m_sceneViewSizeLabel = view.GetSceneViewSizeLabel();
        m_inputTracker.Bind(m_sceneViewHost);
    }

    void SceneViewController::OnViewportChanged(const SceneViewSurface& surface)
    {
        m_sceneViewWidth = surface.width;
        m_sceneViewHeight = surface.height;
        m_sceneViewCamera.SetViewport(m_sceneViewWidth, m_sceneViewHeight);
    }

    SceneViewInteractionResult SceneViewController::UpdateInteraction(
        Game::Scene* scene,
        const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
        const Game::Assets::IMaterialAssetResolver& materialAssetResolver,
        const Graphics::TextureAssetRegistry& textureAssetRegistry,
        const AssetsPanelController& assetsPanelController,
        std::optional<Game::EntityId> currentSelectedEntityId,
        const Core::InputSnapshot& inputSnapshot)
    {
        const SceneViewInteractionResult result = m_inputTracker.UpdateInteraction(
            scene,
            spriteAssetRegistry,
            materialAssetResolver,
            textureAssetRegistry,
            assetsPanelController,
            m_sceneViewCamera,
            m_sceneViewWidth,
            m_sceneViewHeight,
            currentSelectedEntityId,
            inputSnapshot);
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
        if (nullptr != m_sceneViewPlanLabel)
        {
            wchar_t cameraText[160]{};
            std::swprintf(
                cameraText,
                std::size(cameraText),
                L"Scene Camera: zoom %.0f%% / pan Space + Left Drag / wheel zoom",
                m_sceneViewCamera.GetZoom() * 100.0f);
            SetWindowTextW(m_sceneViewPlanLabel, cameraText);
        }
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

    SceneViewEditMode SceneViewController::GetEditMode() const
    {
        return m_inputTracker.GetEditMode();
    }
}
