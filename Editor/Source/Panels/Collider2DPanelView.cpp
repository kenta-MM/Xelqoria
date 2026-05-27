#include "Panels/Collider2DPanelView.h"

#include <algorithm>

namespace Xelqoria::Editor
{
    Collider2DPanelView::Collider2DPanelView(EditorPanelHostContext& hostContext)
        : EditorPanelViewBase(hostContext, EditorPanelId::Collider2D, L"Collider2D")
    {
    }

    std::vector<HWND> Collider2DPanelView::BuildControls() const
    {
        std::vector<HWND> controls{
            m_panel,
            m_summaryLabel,
            m_componentSectionLabel,
            m_enabledCheckBox,
            m_triggerCheckBox,
            m_shapeTypeLabel,
            m_shapeTypeEdit,
            m_offsetLabel,
            m_sizeLabel
        };
        controls.insert(controls.end(), m_editControls.begin(), m_editControls.end());
        return controls;
    }

    bool Collider2DPanelView::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD;
        m_panel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"COLLIDER2D", panelStyle);
        m_summaryLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Collider2D: no entity selected", WS_CHILD | WS_VISIBLE);
        m_componentSectionLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Collider2DComponent", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW);
        m_enabledCheckBox = CreateChildWindow(parentWindow, hInstance, L"Button", L"Enabled", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
        m_triggerCheckBox = CreateChildWindow(parentWindow, hInstance, L"Button", L"Is Trigger", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
        m_shapeTypeLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Shape", WS_CHILD | WS_VISIBLE);
        m_shapeTypeEdit = CreateChildWindow(parentWindow, hInstance, L"Edit", L"Box", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY);
        m_offsetLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Offset", WS_CHILD | WS_VISIBLE);
        m_sizeLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Size", WS_CHILD | WS_VISIBLE);
        if (nullptr == m_panel
            || nullptr == m_summaryLabel
            || nullptr == m_componentSectionLabel
            || nullptr == m_enabledCheckBox
            || nullptr == m_triggerCheckBox
            || nullptr == m_shapeTypeLabel
            || nullptr == m_shapeTypeEdit
            || nullptr == m_offsetLabel
            || nullptr == m_sizeLabel)
        {
            return false;
        }

        for (HWND& handle : m_editControls)
        {
            handle = CreateChildWindow(parentWindow, hInstance, L"Edit", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
            if (nullptr == handle)
            {
                return false;
            }
        }

        SetRootWindow(m_panel);
        SetControls(BuildControls());
        SetVisibleControls(BuildControls());
        SetAlwaysHiddenControls({});
        SetHideWhenInvisibleControls({});
        return true;
    }

    void Collider2DPanelView::Layout(const RECT& bounds)
    {
        const int outerPadding = ScaleMetric(12);
        const int groupHeaderHeight = 0;
        const int labelHeight = ScaleMetric(24);
        const int rowHeight = ScaleMetric(26);
        const int rowSpacing = ScaleMetric(8);
        const int labelWidth = ScaleMetric(78);
        const int width = bounds.right - bounds.left;
        const int height = bounds.bottom - bounds.top;
        const int innerX = bounds.left + outerPadding;
        const int innerWidth = (std::max)(0, width - outerPadding * 2);
        const int editWidth = (std::max)(0, (innerWidth - labelWidth - ScaleMetric(16)) / 3);
        const int sectionTop = bounds.top + groupHeaderHeight + labelHeight + ScaleMetric(8);
        const int checkTop = sectionTop + labelHeight + ScaleMetric(6);
        const int shapeTop = checkTop + rowHeight + rowSpacing;
        const int offsetTop = shapeTop + rowHeight + rowSpacing;
        const int sizeTop = offsetTop + rowHeight + rowSpacing;

        MoveChildWindowNoRedraw(m_panel, bounds.left, bounds.top, width, height);
        MoveChildWindowNoRedraw(m_summaryLabel, innerX, bounds.top + groupHeaderHeight, innerWidth, labelHeight);
        MoveChildWindowNoRedraw(m_componentSectionLabel, innerX, sectionTop, innerWidth, labelHeight);
        MoveChildWindowNoRedraw(m_enabledCheckBox, innerX, checkTop, (std::max)(0, innerWidth / 2), rowHeight);
        MoveChildWindowNoRedraw(m_triggerCheckBox, innerX + (std::max)(0, innerWidth / 2), checkTop, (std::max)(0, innerWidth / 2), rowHeight);
        MoveChildWindowNoRedraw(m_shapeTypeLabel, innerX, shapeTop + ScaleMetric(4), labelWidth, rowHeight);
        MoveChildWindowNoRedraw(m_shapeTypeEdit, innerX + labelWidth, shapeTop, (std::max)(0, innerWidth - labelWidth), rowHeight);
        MoveChildWindowNoRedraw(m_offsetLabel, innerX, offsetTop + ScaleMetric(4), labelWidth, rowHeight);
        MoveChildWindowNoRedraw(m_editControls[0], innerX + labelWidth, offsetTop, editWidth, rowHeight);
        MoveChildWindowNoRedraw(m_editControls[1], innerX + labelWidth + editWidth + ScaleMetric(8), offsetTop, editWidth, rowHeight);
        MoveChildWindowNoRedraw(m_sizeLabel, innerX, sizeTop + ScaleMetric(4), labelWidth, rowHeight);
        MoveChildWindowNoRedraw(m_editControls[2], innerX + labelWidth, sizeTop, editWidth, rowHeight);
        MoveChildWindowNoRedraw(m_editControls[3], innerX + labelWidth + editWidth + ScaleMetric(8), sizeTop, editWidth, rowHeight);
    }
}
