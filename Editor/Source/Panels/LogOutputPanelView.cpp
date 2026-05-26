#include "Panels/LogOutputPanelView.h"

#include "Shell/EditorShell.h"

namespace Xelqoria::Editor
{
    LogOutputPanelView::LogOutputPanelView(EditorShell& shell)
        : EditorPanelViewBase(
            EditorPanelId::LogOutput,
            L"Console",
            [&shell]() { return std::vector<HWND>{ shell.m_logOutputPanel, shell.m_logOutputTabControl, shell.m_logClearButton, shell.m_logCopyButton, shell.m_logFilterEdit, shell.m_logListBox }; },
            [&shell]() { return std::vector<HWND>{ shell.m_logOutputPanel, shell.m_logOutputTabControl, shell.m_logClearButton, shell.m_logFilterEdit, shell.m_logListBox }; },
            [&shell]() { return std::vector<HWND>{ shell.m_logCopyButton }; },
            []() { return std::vector<HWND>{}; }),
          m_shell(shell)
    {
    }

    HWND LogOutputPanelView::GetTabControl() const
    {
        return m_shell.m_logOutputTabControl;
    }

    HWND LogOutputPanelView::GetClearButton() const
    {
        return m_shell.m_logClearButton;
    }

    HWND LogOutputPanelView::GetCopyButton() const
    {
        return m_shell.m_logCopyButton;
    }

    HWND LogOutputPanelView::GetFilterEdit() const
    {
        return m_shell.m_logFilterEdit;
    }

    HWND LogOutputPanelView::GetListBox() const
    {
        return m_shell.m_logListBox;
    }
}
