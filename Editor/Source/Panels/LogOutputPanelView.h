#pragma once

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    class EditorShell;

    /// <summary>
    /// LogOutput Panel の HWND 群を管理する View。
    /// </summary>
    class LogOutputPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した LogOutput control 群へ接続する。
        /// </summary>
        explicit LogOutputPanelView(EditorShell& shell);

        [[nodiscard]] HWND GetTabControl() const;
        [[nodiscard]] HWND GetClearButton() const;
        [[nodiscard]] HWND GetCopyButton() const;
        [[nodiscard]] HWND GetFilterEdit() const;
        [[nodiscard]] HWND GetListBox() const;

    private:
        EditorShell& m_shell;
    };
}
