#include "Panels/SpritePanelView.h"

#include <algorithm>

namespace Xelqoria::Editor
{
    SpritePanelView::SpritePanelView(EditorPanelHostContext& hostContext)
        : EditorPanelViewBase(hostContext, EditorPanelId::Sprite, L"Sprite")
    {
    }

    std::vector<HWND> SpritePanelView::BuildControls() const
    {
        std::vector<HWND> controls{
            m_panel,
            m_summaryLabel,
            m_detailsSectionLabel
        };
        controls.insert(controls.end(), m_detailLabels.begin(), m_detailLabels.end());
        controls.insert(controls.end(), m_detailEditControls.begin(), m_detailEditControls.end());
        return controls;
    }

    std::vector<HWND> SpritePanelView::BuildVisibleControls() const
    {
        std::vector<HWND> controls{
            m_panel,
            m_detailsSectionLabel
        };
        controls.insert(controls.end(), m_detailLabels.begin(), m_detailLabels.end());
        controls.insert(controls.end(), m_detailEditControls.begin(), m_detailEditControls.end());
        return controls;
    }

    bool SpritePanelView::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE;
        m_panel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"SPRITE", panelStyle);
        m_summaryLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Sprite: no sprite selected", WS_CHILD);
        m_detailsSectionLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Sprite", WS_CHILD | WS_VISIBLE);
        if (nullptr == m_panel || nullptr == m_summaryLabel || nullptr == m_detailsSectionLabel)
        {
            return false;
        }

        const std::array<const wchar_t*, 4> spriteDetailLabels{
            L"Texture",
            L"Material",
            L"Script",
            L"Collider2D"
        };
        for (std::size_t index = 0; index < m_detailLabels.size(); ++index)
        {
            m_detailLabels[index] = CreateChildWindow(parentWindow, hInstance, L"Static", spriteDetailLabels[index], WS_CHILD | WS_VISIBLE);
            m_detailEditControls[index] = CreateChildWindow(
                parentWindow,
                hInstance,
                L"Edit",
                L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY);
            if (nullptr == m_detailLabels[index] || nullptr == m_detailEditControls[index])
            {
                return false;
            }
        }

        SetRootWindow(m_panel);
        SetControls(BuildControls());
        SetVisibleControls(BuildVisibleControls());
        SetAlwaysHiddenControls({ m_summaryLabel });
        SetHideWhenInvisibleControls({});
        return true;
    }

    void SpritePanelView::Layout(const RECT& bounds)
    {
        const int outerPadding = ScaleMetric(12);
        const int groupHeaderHeight = 0;
        const int labelHeight = ScaleMetric(24);
        const int rowHeight = ScaleMetric(24);
        const int rowSpacing = ScaleMetric(8);
        const int labelWidth = ScaleMetric(116);
        const int width = bounds.right - bounds.left;
        const int height = bounds.bottom - bounds.top;
        const int innerX = bounds.left + outerPadding;
        const int innerWidth = (std::max)(0, width - outerPadding * 2);
        const int sectionTop = bounds.top + groupHeaderHeight + ScaleMetric(6);
        const int firstRowTop = sectionTop + labelHeight + ScaleMetric(8);
        const int editWidth = (std::max)(0, innerWidth - labelWidth);

        MoveChildWindowNoRedraw(m_panel, bounds.left, bounds.top, width, height);
        MoveChildWindowNoRedraw(m_summaryLabel, innerX, bounds.top + groupHeaderHeight, 0, 0);
        MoveChildWindowNoRedraw(m_detailsSectionLabel, innerX, sectionTop, innerWidth, labelHeight);
        for (std::size_t index = 0; index < m_detailLabels.size(); ++index)
        {
            const int rowTop = firstRowTop + static_cast<int>(index) * (rowHeight + rowSpacing);
            MoveChildWindowNoRedraw(m_detailLabels[index], innerX, rowTop + ScaleMetric(4), labelWidth, rowHeight);
            MoveChildWindowNoRedraw(m_detailEditControls[index], innerX + labelWidth, rowTop, editWidth, rowHeight);
        }
    }

    HWND SpritePanelView::GetSummaryLabel() const { return m_summaryLabel; }
    HWND SpritePanelView::GetDetailsSectionLabel() const { return m_detailsSectionLabel; }
    const std::array<HWND, 4>& SpritePanelView::GetDetailLabels() const { return m_detailLabels; }
    const std::array<HWND, 4>& SpritePanelView::GetDetailEditControls() const { return m_detailEditControls; }
}
