#include "Panels/HierarchyPanelView.h"

#include "Shell/EditorShell.h"

namespace Xelqoria::Editor
{
    HierarchyPanelView::HierarchyPanelView(EditorShell& shell)
        : EditorPanelViewBase(
            EditorPanelId::Hierarchy,
            L"Hierarchy",
            [&shell]() { return std::vector<HWND>{ shell.m_hierarchyPanel, shell.m_hierarchySummaryLabel, shell.m_hierarchyListBox, shell.m_hierarchySearchEdit, shell.m_hierarchyNameEdit, shell.m_hierarchyCreateButton, shell.m_hierarchyDuplicateButton, shell.m_hierarchyDeleteButton }; },
            [&shell]() { return std::vector<HWND>{ shell.m_hierarchyPanel, shell.m_hierarchyListBox, shell.m_hierarchySearchEdit, shell.m_hierarchyNameEdit, shell.m_hierarchyCreateButton, shell.m_hierarchyDuplicateButton, shell.m_hierarchyDeleteButton }; },
            [&shell]() { return std::vector<HWND>{ shell.m_hierarchySummaryLabel }; },
            []() { return std::vector<HWND>{}; }),
          m_shell(shell)
    {
    }

    HWND HierarchyPanelView::GetListBox() const
    {
        return m_shell.m_hierarchyListBox;
    }

    HWND HierarchyPanelView::GetSummaryLabel() const
    {
        return m_shell.m_hierarchySummaryLabel;
    }

    HWND HierarchyPanelView::GetSearchEdit() const
    {
        return m_shell.m_hierarchySearchEdit;
    }

    HWND HierarchyPanelView::GetNameEdit() const
    {
        return m_shell.m_hierarchyNameEdit;
    }

    HWND HierarchyPanelView::GetCreateButton() const
    {
        return m_shell.m_hierarchyCreateButton;
    }

    HWND HierarchyPanelView::GetDuplicateButton() const
    {
        return m_shell.m_hierarchyDuplicateButton;
    }

    HWND HierarchyPanelView::GetDeleteButton() const
    {
        return m_shell.m_hierarchyDeleteButton;
    }
}
