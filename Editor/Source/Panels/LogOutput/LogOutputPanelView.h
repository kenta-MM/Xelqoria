#pragma once

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// LogOutput Panel の HWND 群を管理する View。
    /// </summary>
    class LogOutputPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した LogOutput control 群へ接続する。
        /// </summary>
        explicit LogOutputPanelView(EditorPanelHostContext& hostContext);

        bool Initialize(HWND parentWindow, HINSTANCE hInstance) override;
        void Layout(const RECT& bounds) override;

        [[nodiscard]] HWND GetTabControl() const;
        [[nodiscard]] HWND GetClearButton() const;
        [[nodiscard]] HWND GetCopyButton() const;
        [[nodiscard]] HWND GetFilterEdit() const;
        [[nodiscard]] HWND GetListBox() const;

    private:
        HWND m_panel = nullptr;
        HWND m_tabControl = nullptr;
        HWND m_clearButton = nullptr;
        HWND m_copyButton = nullptr;
        HWND m_filterEdit = nullptr;
        HWND m_listBox = nullptr;
    };
}
