#include "Panels/AssetsPanelView.h"

#include "Shell/EditorShell.h"

namespace Xelqoria::Editor
{
    AssetsPanelView::AssetsPanelView(EditorShell& shell)
        : EditorPanelViewBase(
            EditorPanelId::Assets,
            L"Assets",
            [&shell]() { return std::vector<HWND>{ shell.m_assetsPanel, shell.m_assetsSummaryLabel, shell.m_assetsListView }; },
            [&shell]() { return std::vector<HWND>{ shell.m_assetsPanel, shell.m_assetsListView }; },
            [&shell]() { return std::vector<HWND>{ shell.m_assetsSummaryLabel }; },
            []() { return std::vector<HWND>{}; }),
          m_shell(shell)
    {
    }

    HWND AssetsPanelView::GetListView() const
    {
        return m_shell.m_assetsListView;
    }

    HWND AssetsPanelView::GetSummaryLabel() const
    {
        return m_shell.m_assetsSummaryLabel;
    }

    void AssetsPanelView::ConfigureListHeaderTheme() const
    {
        m_shell.ConfigureAssetsListHeaderTheme();
    }
}
