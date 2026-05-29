#include "Panels/SceneViewPanelView.h"

#include <algorithm>

namespace Xelqoria::Editor
{
    SceneViewPanelView::SceneViewPanelView(EditorPanelHostContext& hostContext)
        : EditorPanelViewBase(hostContext, EditorPanelId::SceneView, L"Scene")
    {
    }

    bool SceneViewPanelView::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE;
        m_panel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"SCENE", panelStyle);
        m_sceneViewPlanLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Runtime 描画は SceneView 専用 child HWND に埋め込みます。",
            WS_CHILD | WS_VISIBLE);
        m_projectSummaryLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Project: pending", WS_CHILD | WS_VISIBLE);
        m_projectSceneListBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER);
        m_projectSceneDetailLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Scene: pending", WS_CHILD | WS_VISIBLE);
        m_sceneViewHost = CreateChildWindow(parentWindow, hInstance, L"Static", L"", WS_CHILD | WS_VISIBLE | SS_NOTIFY, WS_EX_CLIENTEDGE);
        m_sceneViewSizeLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"SceneView size: pending", WS_CHILD | WS_VISIBLE);
        m_buildAndPlayButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"▶", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_pauseResumePlayButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"Ⅱ", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_endPlayButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"■", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_panel
            || nullptr == m_sceneViewPlanLabel
            || nullptr == m_projectSummaryLabel
            || nullptr == m_projectSceneListBox
            || nullptr == m_projectSceneDetailLabel
            || nullptr == m_sceneViewHost
            || nullptr == m_sceneViewSizeLabel
            || nullptr == m_buildAndPlayButton
            || nullptr == m_pauseResumePlayButton
            || nullptr == m_endPlayButton)
        {
            return false;
        }

        SetRootWindow(m_panel);
        SetControls({
            m_panel,
            m_sceneViewPlanLabel,
            m_projectSummaryLabel,
            m_projectSceneListBox,
            m_projectSceneDetailLabel,
            m_sceneViewHost,
            m_sceneViewSizeLabel,
            m_buildAndPlayButton,
            m_pauseResumePlayButton,
            m_endPlayButton
        });
        SetVisibleControls({ m_panel, m_sceneViewHost, m_buildAndPlayButton, m_pauseResumePlayButton, m_endPlayButton });
        SetAlwaysHiddenControls({ m_sceneViewPlanLabel, m_projectSummaryLabel, m_projectSceneListBox, m_projectSceneDetailLabel, m_sceneViewSizeLabel });
        SetHideWhenInvisibleControls({});
        return true;
    }

    void SceneViewPanelView::Layout(const RECT& bounds)
    {
        const int borderInset = ScaleMetric(8);
        const int toolbarHeight = ScaleMetric(26);
        const int toolbarGap = ScaleMetric(8);
        const int buttonGap = ScaleMetric(6);
        const int buttonWidth = ScaleMetric(42);
        const int width = static_cast<int>(bounds.right - bounds.left);
        const int height = static_cast<int>(bounds.bottom - bounds.top);
        const int sceneHostWidth = (std::max)(0, width - borderInset * 2);
        const int sceneHostTop = bounds.top + borderInset + toolbarHeight + toolbarGap;
        const int sceneHostHeight = (std::max)(0, static_cast<int>(bounds.bottom) - borderInset - sceneHostTop);

        MoveChildWindowNoRedraw(m_panel, bounds.left, bounds.top, width, height);
        MoveChildWindowNoRedraw(m_projectSummaryLabel, bounds.left, bounds.top, 0, 0);
        MoveChildWindowNoRedraw(m_projectSceneListBox, bounds.left, bounds.top, 0, 0);
        MoveChildWindowNoRedraw(m_projectSceneDetailLabel, bounds.left, bounds.top, 0, 0);
        MoveChildWindowNoRedraw(m_sceneViewPlanLabel, bounds.left, bounds.top, 0, 0);
        MoveChildWindowNoRedraw(m_sceneViewSizeLabel, bounds.left, bounds.top, 0, 0);
        MoveChildWindowNoRedraw(m_buildAndPlayButton, bounds.left + borderInset, bounds.top + borderInset, buttonWidth, toolbarHeight);
        MoveChildWindowNoRedraw(
            m_pauseResumePlayButton,
            bounds.left + borderInset + buttonWidth + buttonGap,
            bounds.top + borderInset,
            buttonWidth,
            toolbarHeight);
        MoveChildWindowNoRedraw(
            m_endPlayButton,
            bounds.left + borderInset + (buttonWidth + buttonGap) * 2,
            bounds.top + borderInset,
            buttonWidth,
            toolbarHeight);
        MoveChildWindowNoRedraw(m_sceneViewHost, bounds.left + borderInset, sceneHostTop, sceneHostWidth, sceneHostHeight);

        m_sceneViewWidth = static_cast<std::uint32_t>(sceneHostWidth);
        m_sceneViewHeight = static_cast<std::uint32_t>(sceneHostHeight);
    }

    SceneViewSurface SceneViewPanelView::GetSceneViewSurface() const
    {
        return SceneViewSurface{
            m_sceneViewHost,
            m_sceneViewWidth,
            m_sceneViewHeight
        };
    }

    HWND SceneViewPanelView::GetSceneViewHost() const { return m_sceneViewHost; }
    HWND SceneViewPanelView::GetSceneViewPlanLabel() const { return m_sceneViewPlanLabel; }
    HWND SceneViewPanelView::GetSceneViewSizeLabel() const { return m_sceneViewSizeLabel; }
    HWND SceneViewPanelView::GetProjectSummaryLabel() const { return m_projectSummaryLabel; }
    HWND SceneViewPanelView::GetProjectSceneListBox() const { return m_projectSceneListBox; }
    HWND SceneViewPanelView::GetProjectSceneDetailLabel() const { return m_projectSceneDetailLabel; }
    HWND SceneViewPanelView::GetBuildAndPlayButton() const { return m_buildAndPlayButton; }
    HWND SceneViewPanelView::GetPauseResumePlayButton() const { return m_pauseResumePlayButton; }
    HWND SceneViewPanelView::GetEndPlayButton() const { return m_endPlayButton; }
}
