#include "Panels/AssetsPanelView.h"

#include <algorithm>
#include <CommCtrl.h>

namespace Xelqoria::Editor
{
    AssetsPanelView::AssetsPanelView(EditorPanelHostContext& hostContext)
        : EditorPanelViewBase(hostContext, EditorPanelId::Assets, L"Assets")
    {
    }

    bool AssetsPanelView::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE;
        m_panel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"ASSETS", panelStyle);
        m_summaryLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Assets: pending", WS_CHILD);
        m_listView = CreateChildWindow(
            parentWindow,
            hInstance,
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS);
        if (nullptr == m_panel || nullptr == m_summaryLabel || nullptr == m_listView)
        {
            return false;
        }

        ConfigureEditorAssetsListView(m_listView);

        SetRootWindow(m_panel);
        SetControls({ m_panel, m_summaryLabel, m_listView });
        SetVisibleControls({ m_panel, m_listView });
        SetAlwaysHiddenControls({ m_summaryLabel });
        SetHideWhenInvisibleControls({});
        return true;
    }

    void AssetsPanelView::Layout(const RECT& bounds)
    {
        const int outerPadding = ScaleMetric(12);
        const int groupHeaderHeight = 0;
        const int width = bounds.right - bounds.left;
        const int height = bounds.bottom - bounds.top;
        const int innerWidth = (std::max)(0, width - outerPadding * 2);

        MoveChildWindowNoRedraw(m_panel, bounds.left, bounds.top, width, height);
        MoveChildWindowNoRedraw(m_summaryLabel, bounds.left + outerPadding, bounds.top + groupHeaderHeight, 0, 0);
        MoveChildWindowNoRedraw(
            m_listView,
            bounds.left + outerPadding,
            bounds.top + groupHeaderHeight + ScaleMetric(6),
            innerWidth,
            (std::max)(0, height - groupHeaderHeight - outerPadding - ScaleMetric(12)));
    }

    HWND AssetsPanelView::GetListView() const
    {
        return m_listView;
    }

    HWND AssetsPanelView::GetSummaryLabel() const
    {
        return m_summaryLabel;
    }

    void AssetsPanelView::ConfigureListHeaderTheme() const
    {
        ConfigureEditorAssetsListHeaderTheme(m_listView);
    }
}
