#include "Panels/EditorPanelViewBase.h"

#include <utility>

namespace Xelqoria::Editor
{
    EditorPanelViewBase::EditorPanelViewBase(
        EditorPanelId panelId,
        const wchar_t* title,
        ControlProvider controls,
        ControlProvider visibleControls,
        ControlProvider alwaysHiddenControls,
        ControlProvider hideWhenInvisibleControls)
        : m_panelId(panelId)
        , m_title(title)
        , m_controls(std::move(controls))
        , m_visibleControls(std::move(visibleControls))
        , m_alwaysHiddenControls(std::move(alwaysHiddenControls))
        , m_hideWhenInvisibleControls(std::move(hideWhenInvisibleControls))
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
        for (HWND control : GetControls(m_visibleControls))
        {
            ShowWindow(control, showCommand);
        }

        for (HWND control : GetControls(m_alwaysHiddenControls))
        {
            ShowWindow(control, SW_HIDE);
        }

        if (false == visible)
        {
            for (HWND control : GetControls(m_hideWhenInvisibleControls))
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

        for (HWND control : GetControls(m_controls))
        {
            if (nullptr != control && GetParent(control) != parentWindow)
            {
                ::SetParent(control, parentWindow);
            }
        }
    }

    void EditorPanelViewBase::CollectControls(std::vector<HWND>& controls) const
    {
        for (HWND control : GetControls(m_controls))
        {
            controls.push_back(control);
        }
    }

    std::vector<HWND> EditorPanelViewBase::GetControls(const ControlProvider& provider) const
    {
        if (false == static_cast<bool>(provider))
        {
            return {};
        }

        return provider();
    }
}
