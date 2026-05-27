#include "Panels/HierarchyPanelView.h"

#include <algorithm>
#include <CommCtrl.h>

namespace Xelqoria::Editor
{
    HierarchyPanelView::HierarchyPanelView(EditorPanelHostContext& hostContext)
        : EditorPanelViewBase(hostContext, EditorPanelId::Hierarchy, L"Hierarchy")
    {
    }

    bool HierarchyPanelView::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE;
        m_panel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"HIERARCHY", panelStyle);
        m_summaryLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Entities: pending", WS_CHILD);
        m_searchEdit = CreateChildWindow(parentWindow, hInstance, L"Edit", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
        m_listBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_BORDER);
        m_nameEdit = CreateChildWindow(parentWindow, hInstance, L"Edit", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
        m_createButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"New", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_duplicateButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"Duplicate", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_deleteButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"Delete", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_panel
            || nullptr == m_summaryLabel
            || nullptr == m_searchEdit
            || nullptr == m_listBox
            || nullptr == m_nameEdit
            || nullptr == m_createButton
            || nullptr == m_duplicateButton
            || nullptr == m_deleteButton)
        {
            return false;
        }

        SendMessageW(m_searchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Search Entity"));
        ConfigureEditorHierarchyListBox(m_listBox, ScaleMetric(EditorPanelRowHeight));

        SetRootWindow(m_panel);
        SetControls({ m_panel, m_summaryLabel, m_listBox, m_searchEdit, m_nameEdit, m_createButton, m_duplicateButton, m_deleteButton });
        SetVisibleControls({ m_panel, m_listBox, m_searchEdit, m_nameEdit, m_createButton, m_duplicateButton, m_deleteButton });
        SetAlwaysHiddenControls({ m_summaryLabel });
        SetHideWhenInvisibleControls({});
        return true;
    }

    void HierarchyPanelView::Layout(const RECT& bounds)
    {
        const int outerPadding = ScaleMetric(12);
        const int groupHeaderHeight = 0;
        const int labelHeight = ScaleMetric(24);
        const int buttonHeight = ScaleMetric(28);
        int hierarchyButtonGap = ScaleMetric(8);
        const int width = bounds.right - bounds.left;
        const int height = bounds.bottom - bounds.top;
        const int innerWidth = (std::max)(0, width - outerPadding * 2);
        if (innerWidth < hierarchyButtonGap * 2)
        {
            hierarchyButtonGap = 0;
        }

        const int searchTop = bounds.top + groupHeaderHeight + ScaleMetric(6);
        const int buttonTop = searchTop + labelHeight + ScaleMetric(6);
        const int buttonWidth = (std::max)(0, (innerWidth - hierarchyButtonGap * 2) / 3);
        MoveChildWindowNoRedraw(m_panel, bounds.left, bounds.top, width, height);
        MoveChildWindowNoRedraw(m_summaryLabel, bounds.left + outerPadding, bounds.top + groupHeaderHeight, 0, 0);
        MoveChildWindowNoRedraw(m_searchEdit, bounds.left + outerPadding, searchTop, innerWidth, labelHeight);
        MoveChildWindowNoRedraw(m_createButton, bounds.left + outerPadding, buttonTop, buttonWidth, buttonHeight);
        MoveChildWindowNoRedraw(
            m_duplicateButton,
            bounds.left + outerPadding + buttonWidth + hierarchyButtonGap,
            buttonTop,
            buttonWidth,
            buttonHeight);
        MoveChildWindowNoRedraw(
            m_deleteButton,
            bounds.left + outerPadding + (buttonWidth + hierarchyButtonGap) * 2,
            buttonTop,
            (std::max)(0, innerWidth - (buttonWidth + hierarchyButtonGap) * 2),
            buttonHeight);

        const int hierarchyListTop = buttonTop + buttonHeight + ScaleMetric(8);
        const int hierarchyNameTop = bounds.top + height - outerPadding - labelHeight;
        MoveChildWindowNoRedraw(
            m_listBox,
            bounds.left + outerPadding,
            hierarchyListTop,
            innerWidth,
            (std::max)(0, hierarchyNameTop - hierarchyListTop - ScaleMetric(8)));
        MoveChildWindowNoRedraw(m_nameEdit, bounds.left + outerPadding, hierarchyNameTop, innerWidth, labelHeight);
    }

    HWND HierarchyPanelView::GetListBox() const { return m_listBox; }
    HWND HierarchyPanelView::GetSummaryLabel() const { return m_summaryLabel; }
    HWND HierarchyPanelView::GetSearchEdit() const { return m_searchEdit; }
    HWND HierarchyPanelView::GetNameEdit() const { return m_nameEdit; }
    HWND HierarchyPanelView::GetCreateButton() const { return m_createButton; }
    HWND HierarchyPanelView::GetDuplicateButton() const { return m_duplicateButton; }
    HWND HierarchyPanelView::GetDeleteButton() const { return m_deleteButton; }
}
