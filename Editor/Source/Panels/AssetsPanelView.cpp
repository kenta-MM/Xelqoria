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
            []() { return std::vector<HWND>{}; })
    {
    }
}
