#pragma once

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    class EditorShell;

    /// <summary>
    /// Assets Panel の HWND 群を管理する View。
    /// </summary>
    class AssetsPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した Assets control 群へ接続する。
        /// </summary>
        explicit AssetsPanelView(EditorShell& shell);

        [[nodiscard]] HWND GetListView() const;
        [[nodiscard]] HWND GetSummaryLabel() const;
        void ConfigureListHeaderTheme() const;

    private:
        EditorShell& m_shell;
    };
}
