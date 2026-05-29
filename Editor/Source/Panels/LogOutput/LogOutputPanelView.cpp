#include "Panels/LogOutput/LogOutputPanelView.h"

#include <algorithm>
#include <array>
#include <CommCtrl.h>

namespace Xelqoria::Editor
{
    LogOutputPanelView::LogOutputPanelView(EditorPanelHostContext& hostContext)
        : EditorPanelViewBase(hostContext, EditorPanelId::LogOutput, L"Console")
    {
    }

    bool LogOutputPanelView::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE;
        m_panel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"CONSOLE", panelStyle);
        m_tabControl = CreateChildWindow(
            parentWindow,
            hInstance,
            WC_TABCONTROLW,
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_OWNERDRAWFIXED);
        m_clearButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"Clear", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_copyButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"Copy", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_filterEdit = CreateChildWindow(parentWindow, hInstance, L"Edit", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_CENTER);
        m_listBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | WS_BORDER);
        if (nullptr == m_panel
            || nullptr == m_tabControl
            || nullptr == m_clearButton
            || nullptr == m_copyButton
            || nullptr == m_filterEdit
            || nullptr == m_listBox)
        {
            return false;
        }

        ConfigureEditorTabControl(m_tabControl, ScaleMetric(96), ScaleMetric(28));
        const std::array<const wchar_t*, 5> tabNames{ L"All", L"Info", L"Editor", L"Warn", L"Error" };
        for (std::size_t index = 0; index < tabNames.size(); ++index)
        {
            TCITEMW item{};
            item.mask = TCIF_TEXT;
            item.pszText = const_cast<LPWSTR>(tabNames[index]);
            TabCtrl_InsertItem(m_tabControl, static_cast<int>(index), &item);
        }
        TabCtrl_SetCurSel(m_tabControl, 0);
        SendMessageW(m_filterEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"検索フィルタ"));

        SetRootWindow(m_panel);
        SetControls({ m_panel, m_tabControl, m_clearButton, m_copyButton, m_filterEdit, m_listBox });
        SetVisibleControls({ m_panel, m_tabControl, m_clearButton, m_filterEdit, m_listBox });
        SetAlwaysHiddenControls({ m_copyButton });
        SetHideWhenInvisibleControls({});
        return true;
    }

    void LogOutputPanelView::Layout(const RECT& bounds)
    {
        const int outerPadding = ScaleMetric(8);
        const int rowHeight = ScaleMetric(24);
        const int gap = ScaleMetric(6);
        const int width = static_cast<int>(bounds.right - bounds.left);
        const int height = static_cast<int>(bounds.bottom - bounds.top);
        const int innerWidth = (std::max)(0, width - outerPadding * 2);
        const int buttonWidth = ScaleMetric(64);
        const int tabWidth = (std::min)(innerWidth - buttonWidth - gap, ScaleMetric(300));
        const int toolbarTop = bounds.top + outerPadding;
        const int listTop = toolbarTop + rowHeight + gap;

        MoveChildWindowNoRedraw(m_panel, bounds.left, bounds.top, width, height);
        MoveChildWindowNoRedraw(m_tabControl, bounds.left + outerPadding, toolbarTop, tabWidth, rowHeight);
        MoveChildWindowNoRedraw(m_clearButton, bounds.right - outerPadding - buttonWidth, toolbarTop, buttonWidth, rowHeight);
        MoveChildWindowNoRedraw(m_copyButton, bounds.left, bounds.top, 0, 0);
        MoveChildWindowNoRedraw(m_filterEdit, static_cast<int>(bounds.left), toolbarTop, 0, rowHeight);
        MoveChildWindowNoRedraw(
            m_listBox,
            bounds.left + outerPadding,
            listTop,
            innerWidth,
            (std::max)(0, height - outerPadding - (listTop - static_cast<int>(bounds.top))));
    }

    HWND LogOutputPanelView::GetTabControl() const { return m_tabControl; }
    HWND LogOutputPanelView::GetClearButton() const { return m_clearButton; }
    HWND LogOutputPanelView::GetCopyButton() const { return m_copyButton; }
    HWND LogOutputPanelView::GetFilterEdit() const { return m_filterEdit; }
    HWND LogOutputPanelView::GetListBox() const { return m_listBox; }
}
