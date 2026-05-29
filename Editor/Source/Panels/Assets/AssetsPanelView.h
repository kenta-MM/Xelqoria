#pragma once

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Assets Panel の HWND 群を管理する View。
    /// </summary>
    class AssetsPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// Assets control 群を生成・管理する View を作成する。
        /// </summary>
        explicit AssetsPanelView(EditorPanelHostContext& hostContext);

        bool Initialize(HWND parentWindow, HINSTANCE hInstance) override;
        void Layout(const RECT& bounds) override;

        [[nodiscard]] HWND GetListView() const;
        [[nodiscard]] HWND GetSummaryLabel() const;
        void ConfigureListHeaderTheme() const;

    private:
        HWND m_panel = nullptr;
        HWND m_summaryLabel = nullptr;
        HWND m_listView = nullptr;
    };
}
