#include "Panels/MaterialPanelView.h"

#include "Shell/EditorShell.h"

namespace Xelqoria::Editor
{
    std::vector<HWND> MaterialPanelView::GetMaterialControls(EditorShell& shell)
    {
        std::vector<HWND> controls{
            shell.m_materialPanel,
            shell.m_materialSummaryLabel,
            shell.m_materialSharedNoticeLabel,
            shell.m_materialDetailsSectionLabel
        };
        controls.insert(controls.end(), shell.m_materialDetailLabels.begin(), shell.m_materialDetailLabels.end());
        controls.insert(controls.end(), shell.m_materialDetailEditControls.begin(), shell.m_materialDetailEditControls.end());
        controls.push_back(shell.m_materialTextureDropHighlight);
        return controls;
    }

    std::vector<HWND> MaterialPanelView::GetMaterialVisibleControls(EditorShell& shell)
    {
        std::vector<HWND> controls{
            shell.m_materialPanel,
            shell.m_materialSharedNoticeLabel,
            shell.m_materialDetailsSectionLabel
        };
        controls.insert(controls.end(), shell.m_materialDetailLabels.begin(), shell.m_materialDetailLabels.end());
        controls.insert(controls.end(), shell.m_materialDetailEditControls.begin(), shell.m_materialDetailEditControls.end());
        return controls;
    }

    MaterialPanelView::MaterialPanelView(EditorShell& shell)
        : EditorPanelViewBase(
            EditorPanelId::Material,
            L"Material",
            [&shell]() { return GetMaterialControls(shell); },
            [&shell]() { return GetMaterialVisibleControls(shell); },
            [&shell]() { return std::vector<HWND>{ shell.m_materialSummaryLabel }; },
            [&shell]() { return std::vector<HWND>{ shell.m_materialTextureDropHighlight }; }),
          m_shell(shell)
    {
    }

    HWND MaterialPanelView::GetSummaryLabel() const
    {
        return m_shell.m_materialSummaryLabel;
    }

    HWND MaterialPanelView::GetSharedNoticeLabel() const
    {
        return m_shell.m_materialSharedNoticeLabel;
    }

    HWND MaterialPanelView::GetDetailsSectionLabel() const
    {
        return m_shell.m_materialDetailsSectionLabel;
    }

    const std::array<HWND, 5>& MaterialPanelView::GetDetailLabels() const
    {
        return m_shell.m_materialDetailLabels;
    }

    const std::array<HWND, 5>& MaterialPanelView::GetDetailEditControls() const
    {
        return m_shell.m_materialDetailEditControls;
    }

    HWND MaterialPanelView::GetTextureDropHighlight() const
    {
        return m_shell.m_materialTextureDropHighlight;
    }
}
