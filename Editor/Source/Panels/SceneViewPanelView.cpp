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
            []() { return std::vector<HWND>{}; })
    {
    }
}
