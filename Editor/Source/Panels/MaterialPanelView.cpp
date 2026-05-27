#include "Panels/MaterialPanelView.h"

#include <algorithm>

namespace Xelqoria::Editor
{
    MaterialPanelView::MaterialPanelView(EditorPanelHostContext& hostContext)
        : EditorPanelViewBase(hostContext, EditorPanelId::Material, L"Material")
    {
    }

    std::vector<HWND> MaterialPanelView::BuildControls() const
    {
        std::vector<HWND> controls{
            m_panel,
            m_summaryLabel,
            m_sharedNoticeLabel,
            m_detailsSectionLabel
        };
        controls.insert(controls.end(), m_detailLabels.begin(), m_detailLabels.end());
        controls.insert(controls.end(), m_detailEditControls.begin(), m_detailEditControls.end());
        controls.insert(
            controls.end(),
            {
                m_textureBrowseButton,
                m_tintColorButton,
                m_outlineEnabledCheckBox,
                m_outlineColorButton,
                m_textureDropHighlight
            });
        return controls;
    }

    std::vector<HWND> MaterialPanelView::BuildVisibleControls() const
    {
        std::vector<HWND> controls{
            m_panel,
            m_sharedNoticeLabel,
            m_detailsSectionLabel
        };
        controls.insert(controls.end(), m_detailLabels.begin(), m_detailLabels.end());
        controls.insert(controls.end(), m_detailEditControls.begin(), m_detailEditControls.end());
        controls.insert(
            controls.end(),
            {
                m_textureBrowseButton,
                m_tintColorButton,
                m_outlineEnabledCheckBox,
                m_outlineColorButton
            });
        return controls;
    }

    bool MaterialPanelView::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD;
        m_panel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"MATERIAL", panelStyle);
        m_summaryLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Material: no material selected", WS_CHILD);
        m_sharedNoticeLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Shared Material editing. Changes affect sprites using this material.",
            WS_CHILD | WS_VISIBLE);
        m_detailsSectionLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Material Details (from SpriteComponent)",
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW);
        if (nullptr == m_panel || nullptr == m_summaryLabel || nullptr == m_sharedNoticeLabel || nullptr == m_detailsSectionLabel)
        {
            return false;
        }

        const std::array<const wchar_t*, 5> materialDetailLabels{
            L"Texture",
            L"Tint",
            L"OutlineEnabled",
            L"OutlineThickness",
            L"OutlineColor"
        };
        for (std::size_t index = 0; index < m_detailLabels.size(); ++index)
        {
            m_detailLabels[index] = CreateChildWindow(parentWindow, hInstance, L"Static", materialDetailLabels[index], WS_CHILD | WS_VISIBLE);
            m_detailEditControls[index] = CreateChildWindow(
                parentWindow,
                hInstance,
                L"Edit",
                L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
            if (nullptr == m_detailLabels[index] || nullptr == m_detailEditControls[index])
            {
                return false;
            }
        }

        m_textureBrowseButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_tintColorButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_outlineEnabledCheckBox = CreateChildWindow(parentWindow, hInstance, L"Button", L"", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
        m_outlineColorButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_textureDropHighlight = CreateChildWindow(parentWindow, hInstance, L"Static", L"", WS_CHILD | SS_BLACKFRAME);
        if (nullptr == m_textureBrowseButton
            || nullptr == m_tintColorButton
            || nullptr == m_outlineEnabledCheckBox
            || nullptr == m_outlineColorButton
            || nullptr == m_textureDropHighlight)
        {
            return false;
        }

        SetRootWindow(m_panel);
        SetControls(BuildControls());
        SetVisibleControls(BuildVisibleControls());
        SetAlwaysHiddenControls({ m_summaryLabel });
        SetHideWhenInvisibleControls({ m_textureDropHighlight });
        return true;
    }

    void MaterialPanelView::Layout(const RECT& bounds)
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
        const int noticeTop = bounds.top + groupHeaderHeight + ScaleMetric(6);
        const int sectionTop = noticeTop + labelHeight + ScaleMetric(10);
        const int firstRowTop = sectionTop + labelHeight + ScaleMetric(4);
        const int editWidth = (std::max)(0, innerWidth - labelWidth);

        MoveChildWindowNoRedraw(m_panel, bounds.left, bounds.top, width, height);
        MoveChildWindowNoRedraw(m_summaryLabel, innerX, bounds.top + groupHeaderHeight, 0, 0);
        MoveChildWindowNoRedraw(m_sharedNoticeLabel, innerX, noticeTop, innerWidth, labelHeight);
        MoveChildWindowNoRedraw(m_detailsSectionLabel, innerX, sectionTop, innerWidth, labelHeight);
        for (std::size_t index = 0; index < m_detailLabels.size(); ++index)
        {
            const int rowTop = firstRowTop + static_cast<int>(index) * (rowHeight + rowSpacing);
            MoveChildWindowNoRedraw(m_detailLabels[index], innerX, rowTop + ScaleMetric(4), labelWidth, rowHeight);
            MoveChildWindowNoRedraw(m_detailEditControls[index], innerX + labelWidth, rowTop, editWidth, rowHeight);
        }

        MoveChildWindowNoRedraw(
            m_textureDropHighlight,
            innerX + labelWidth - ScaleMetric(2),
            firstRowTop - ScaleMetric(2),
            editWidth + ScaleMetric(4),
            rowHeight + ScaleMetric(4));
    }

    HWND MaterialPanelView::GetSummaryLabel() const { return m_summaryLabel; }
    HWND MaterialPanelView::GetSharedNoticeLabel() const { return m_sharedNoticeLabel; }
    HWND MaterialPanelView::GetDetailsSectionLabel() const { return m_detailsSectionLabel; }
    const std::array<HWND, 5>& MaterialPanelView::GetDetailLabels() const { return m_detailLabels; }
    const std::array<HWND, 5>& MaterialPanelView::GetDetailEditControls() const { return m_detailEditControls; }
    HWND MaterialPanelView::GetTextureDropHighlight() const { return m_textureDropHighlight; }
}
