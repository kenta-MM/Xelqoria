#include "Application.h"

#include <chrono>
#include <string>

#include "GraphicsAPI.h"

namespace Xelqoria::Editor
{
    Application::Application(HINSTANCE hInstance)
        : m_hInstance(hInstance)
    {
    }

    Application::~Application()
    {
        Shutdown();
    }

    int Application::Run()
    {
        if (false == Initialize())
        {
            return -1;
        }

        using clock = std::chrono::steady_clock;
        auto lastTime = clock::now();

        while (m_running)
        {
            if (false == m_window.PumpMessages())
            {
                m_running = false;
                continue;
            }

            const auto currentTime = clock::now();
            const std::chrono::duration<float> delta = currentTime - lastTime;
            lastTime = currentTime;

            Update(delta.count());
            Render();
        }

        return 0;
    }

    bool Application::Initialize()
    {
        constexpr auto clientWidth = 1600u;
        constexpr auto clientHeight = 900u;
        constexpr RHI::GraphicsAPI api = RHI::GraphicsAPI::D3D11;

        std::wstring title = L"Xelqoria Editor - ";
        title += RHI::GraphicsAPIToString(api);
        if (false == m_window.Create(m_hInstance, title.c_str(), clientWidth, clientHeight))
        {
            return false;
        }

        if (false == m_editorShell.Initialize(m_window.GetHwnd(), m_hInstance))
        {
            return false;
        }

        m_assetsPanelController.Bind(m_editorShell);
        m_hierarchyPanelController.Bind(m_editorShell);
        m_inspectorPanelController.Bind(m_editorShell);
        m_sceneViewController.Bind(m_editorShell);

        const bool sceneViewSizeChanged = m_editorShell.UpdateLayout(m_window.GetHwnd());
        if (true == sceneViewSizeChanged)
        {
            m_sceneViewController.OnViewportChanged(
                m_editorShell.GetSceneViewWidth(),
                m_editorShell.GetSceneViewHeight());
        }

        m_window.Show();

        if (false == m_sceneViewRenderer.Initialize(
                m_hInstance,
                m_editorShell.GetSceneViewHost(),
                m_editorShell.GetSceneViewWidth(),
                m_editorShell.GetSceneViewHeight(),
                api))
        {
            return false;
        }

        if (false == m_sceneDocument.Initialize(m_sceneViewRenderer.GetGraphicsContext()))
        {
            return false;
        }

        m_assetsPanelController.Refresh(
            m_sceneDocument.GetRegisteredSpriteAssetIds(),
            m_sceneDocument.GetSpriteAssetRegistry(),
            m_sceneDocument.GetTextureAssetRegistry());
        const bool canAddSpriteComponent = m_assetsPanelController.HasVisibleSpriteAssets();
        m_hierarchyPanelController.Refresh(m_sceneDocument.GetScene());
        m_inspectorPanelController.Refresh(
            m_sceneDocument.GetScene(),
            m_hierarchyPanelController.GetSelectedEntityId(),
            canAddSpriteComponent);
        m_sceneViewController.RefreshSelectionStatus(
            m_sceneDocument.GetScene(),
            m_hierarchyPanelController.GetSelectedEntityId());
        m_editorCommandController.Reset(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
        return true;
    }

    void Application::Shutdown()
    {
        m_sceneViewRenderer.Shutdown();
    }

    void Application::Update(float deltaTime)
    {
        (void)deltaTime;

        const bool sceneViewSizeChanged = m_editorShell.UpdateLayout(m_window.GetHwnd());
        if (true == sceneViewSizeChanged)
        {
            m_sceneViewController.OnViewportChanged(
                m_editorShell.GetSceneViewWidth(),
                m_editorShell.GetSceneViewHeight());
            m_sceneViewRenderer.Resize(
                m_editorShell.GetSceneViewWidth(),
                m_editorShell.GetSceneViewHeight());
        }

        m_assetsPanelController.SyncSelection();
        m_assetsPanelController.UpdateDragState();
        const bool canAddSpriteComponent = m_assetsPanelController.HasVisibleSpriteAssets();

        if (true == m_hierarchyPanelController.SyncSelection())
        {
            m_inspectorPanelController.ResetTrackedEntity();
            m_hierarchyPanelController.Refresh(m_sceneDocument.GetScene());
            m_inspectorPanelController.Refresh(
                m_sceneDocument.GetScene(),
                m_hierarchyPanelController.GetSelectedEntityId(),
                canAddSpriteComponent);
        }

        const InspectorApplyResult inspectorResult = m_inspectorPanelController.ApplyEdits(
            m_sceneDocument.GetScene(),
            m_hierarchyPanelController.GetSelectedEntityId(),
            canAddSpriteComponent);
        if (true == inspectorResult.changed)
        {
            if (m_sceneDocument.Save())
            {
                m_editorCommandController.PushSnapshot(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
                SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Inspector の編集内容を Scene へ保存しました。");
            }
            else
            {
                SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Inspector の編集内容は反映されましたが、Scene の保存に失敗しました。");
            }

            m_hierarchyPanelController.Refresh(m_sceneDocument.GetScene());
            m_inspectorPanelController.Refresh(
                m_sceneDocument.GetScene(),
                m_hierarchyPanelController.GetSelectedEntityId(),
                canAddSpriteComponent);
            m_sceneViewController.RefreshSelectionStatus(
                m_sceneDocument.GetScene(),
                m_hierarchyPanelController.GetSelectedEntityId());
        }

        const SceneViewInteractionResult interactionResult = m_sceneViewController.UpdateInteraction(
            m_sceneDocument.GetScene(),
            m_sceneDocument.GetSpriteAssetRegistry(),
            m_sceneDocument.GetTextureAssetRegistry(),
            m_assetsPanelController,
            m_hierarchyPanelController.GetSelectedEntityId());
        if (true == interactionResult.selectionChanged)
        {
            m_hierarchyPanelController.SetSelectedEntityId(interactionResult.selectedEntityId);
            m_inspectorPanelController.ResetTrackedEntity();
            m_hierarchyPanelController.Refresh(m_sceneDocument.GetScene());
            m_inspectorPanelController.Refresh(
                m_sceneDocument.GetScene(),
                m_hierarchyPanelController.GetSelectedEntityId(),
                canAddSpriteComponent);
        }

        if (true == m_assetsPanelController.WasDragReleasedThisFrame())
        {
            m_assetsPanelController.CompleteReleasedDrag();
        }

        const SceneViewDropResult dropResult = m_sceneViewController.ProcessPendingSceneDrop(m_sceneDocument);
        if (true == dropResult.selectionChanged)
        {
            m_hierarchyPanelController.SetSelectedEntityId(dropResult.selectedEntityId);
        }

        if (true == dropResult.sceneChanged || true == dropResult.selectionChanged)
        {
            m_inspectorPanelController.ResetTrackedEntity();
            m_hierarchyPanelController.Refresh(m_sceneDocument.GetScene());
            m_inspectorPanelController.Refresh(
                m_sceneDocument.GetScene(),
                m_hierarchyPanelController.GetSelectedEntityId(),
                canAddSpriteComponent);
        }

        if (true == dropResult.shouldPushHistory)
        {
            m_editorCommandController.PushSnapshot(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
        }

        const EditorCommandUpdateResult commandResult = m_editorCommandController.Update(
            m_sceneDocument,
            m_hierarchyPanelController.GetSelectedEntityId(),
            m_editorShell.GetSceneViewPlanLabel());
        if (true == commandResult.changed)
        {
            m_hierarchyPanelController.SetSelectedEntityId(commandResult.selectedEntityId);
            m_inspectorPanelController.ResetTrackedEntity();
            m_hierarchyPanelController.Refresh(m_sceneDocument.GetScene());
            m_inspectorPanelController.Refresh(
                m_sceneDocument.GetScene(),
                m_hierarchyPanelController.GetSelectedEntityId(),
                canAddSpriteComponent);
            m_sceneViewController.RefreshSelectionStatus(
                m_sceneDocument.GetScene(),
                m_hierarchyPanelController.GetSelectedEntityId());
        }
    }

    void Application::Render()
    {
        m_sceneViewRenderer.RenderFrame(
            m_sceneDocument.GetScene(),
            m_sceneDocument.GetSpriteAssetRegistry(),
            m_sceneDocument.GetTextureAssetRegistry(),
            m_sceneViewController.GetCamera(),
            m_hierarchyPanelController.GetSelectedEntityId(),
            m_sceneViewController.GetDragPreviewState());
    }
}
