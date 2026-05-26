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
            []() { return std::vector<HWND>{}; })
    {
    }
}
