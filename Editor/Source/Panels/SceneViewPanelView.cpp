#include "Panels/SceneViewPanelView.h"

#include "Shell/EditorShell.h"

namespace Xelqoria::Editor
{
    SceneViewPanelView::SceneViewPanelView(EditorShell& shell)
        : EditorPanelViewBase(
            EditorPanelId::SceneView,
            L"Scene",
            [&shell]() { return std::vector<HWND>{ shell.m_sceneViewPanel, shell.m_sceneViewPlanLabel, shell.m_projectSummaryLabel, shell.m_projectSceneListBox, shell.m_projectSceneDetailLabel, shell.m_sceneViewHost, shell.m_sceneViewSizeLabel, shell.m_buildAndPlayButton, shell.m_pauseResumePlayButton, shell.m_endPlayButton }; },
            [&shell]() { return std::vector<HWND>{ shell.m_sceneViewPanel, shell.m_sceneViewHost, shell.m_buildAndPlayButton, shell.m_pauseResumePlayButton, shell.m_endPlayButton }; },
            [&shell]() { return std::vector<HWND>{ shell.m_sceneViewPlanLabel, shell.m_projectSummaryLabel, shell.m_projectSceneListBox, shell.m_projectSceneDetailLabel, shell.m_sceneViewSizeLabel }; },
            []() { return std::vector<HWND>{}; }),
          m_shell(shell)
    {
    }

    SceneViewSurface SceneViewPanelView::GetSceneViewSurface() const
    {
        return m_shell.GetSceneViewSurface();
    }

    HWND SceneViewPanelView::GetSceneViewPlanLabel() const
    {
        return m_shell.m_sceneViewPlanLabel;
    }

    HWND SceneViewPanelView::GetSceneViewSizeLabel() const
    {
        return m_shell.m_sceneViewSizeLabel;
    }

    HWND SceneViewPanelView::GetProjectSummaryLabel() const
    {
        return m_shell.m_projectSummaryLabel;
    }

    HWND SceneViewPanelView::GetProjectSceneListBox() const
    {
        return m_shell.m_projectSceneListBox;
    }

    HWND SceneViewPanelView::GetProjectSceneDetailLabel() const
    {
        return m_shell.m_projectSceneDetailLabel;
    }
}
