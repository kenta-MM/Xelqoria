#include "Panels/EditorPanelViewBase.h"

#include <utility>

namespace Xelqoria::Editor
{
    EditorPanelViewBase::EditorPanelViewBase(
        EditorPanelHostContext& hostContext,
        EditorPanelId panelId,
        const wchar_t* title)
        : m_hostContext(hostContext)
        , m_panelId(panelId)
        , m_title(title)
    {
    }

    EditorPanelId EditorPanelViewBase::GetPanelId() const
    {
        return m_panelId;
    }

    const wchar_t* EditorPanelViewBase::GetTitle() const
    {
        return m_title;
    }

    HWND EditorPanelViewBase::GetRootWindow() const
    {
        return m_rootWindow;
    }

    bool EditorPanelViewBase::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        (void)parentWindow;
        (void)hInstance;
        return true;
    }

    void EditorPanelViewBase::Layout(const RECT& bounds)
    {
        (void)bounds;
    }

    void EditorPanelViewBase::Show(bool visible)
    {
        const int showCommand = visible ? SW_SHOW : SW_HIDE;
        for (HWND control : m_visibleControls)
        {
            ShowWindow(control, showCommand);
        }

        for (HWND control : m_alwaysHiddenControls)
        {
            ShowWindow(control, SW_HIDE);
        }

        if (false == visible)
        {
            for (HWND control : m_hideWhenInvisibleControls)
            {
                ShowWindow(control, SW_HIDE);
            }
        }
    }

    void EditorPanelViewBase::SetParent(HWND parentWindow)
    {
        if (nullptr == parentWindow)
        {
            return;
        }

        for (HWND control : m_controls)
        {
            if (nullptr != control && GetParent(control) != parentWindow)
            {
                ::SetParent(control, parentWindow);
            }
        }
    }

    void EditorPanelViewBase::CollectControls(std::vector<HWND>& controls) const
    {
        for (HWND control : m_controls)
        {
            controls.push_back(control);
        }
    }

    HWND EditorPanelViewBase::CreateChildWindow(
        HWND parentWindow,
        HINSTANCE hInstance,
        const wchar_t* className,
        const wchar_t* text,
        DWORD style,
        DWORD exStyle) const
    {
        return m_hostContext.CreateChildWindow(parentWindow, hInstance, className, text, style, exStyle);
    }

    void EditorPanelViewBase::MoveChildWindowNoRedraw(HWND window, int x, int y, int width, int height) const
    {
        m_hostContext.MoveChildWindowNoRedraw(window, x, y, width, height);
    }

    int EditorPanelViewBase::ScaleMetric(int value) const
    {
        return m_hostContext.ScaleMetric(value);
    }

    void EditorPanelViewBase::SetRootWindow(HWND rootWindow)
    {
        m_rootWindow = rootWindow;
    }

    void EditorPanelViewBase::SetControls(std::vector<HWND> controls)
    {
        m_controls = std::move(controls);
    }

    void EditorPanelViewBase::SetVisibleControls(std::vector<HWND> visibleControls)
    {
        m_visibleControls = std::move(visibleControls);
    }

    void EditorPanelViewBase::SetAlwaysHiddenControls(std::vector<HWND> alwaysHiddenControls)
    {
        m_alwaysHiddenControls = std::move(alwaysHiddenControls);
    }

    void EditorPanelViewBase::SetHideWhenInvisibleControls(std::vector<HWND> hideWhenInvisibleControls)
    {
        m_hideWhenInvisibleControls = std::move(hideWhenInvisibleControls);
    }
}
