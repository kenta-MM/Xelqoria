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
            [&shell]() { return std::vector<HWND>{ shell.m_materialTextureDropHighlight }; })
    {
    }
}
