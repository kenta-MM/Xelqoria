#include "LogOutputPanelController.h"

#include <CommCtrl.h>
#include <utility>

namespace Xelqoria::Editor
{
    namespace
    {
        [[nodiscard]] POINT ToWin32Point(Platform::Point point)
        {
            return POINT{ static_cast<LONG>(point.x), static_cast<LONG>(point.y) };
        }

        [[nodiscard]] std::size_t ToLogIndex(LogOutputCategory category)
        {
            return static_cast<std::size_t>(category);
        }

        [[nodiscard]] bool ContainsFilter(const std::wstring& text, const std::wstring& filter)
        {
            if (filter.empty())
            {
                return true;
            }

            return text.find(filter) != std::wstring::npos;
        }
    }

    void LogOutputPanelController::Bind(const EditorShell& shell, Platform::IClipboard& clipboard)
    {
        m_tabControl = shell.GetLogOutputTabControl();
        m_clearButton = shell.GetLogClearButton();
        m_copyButton = shell.GetLogCopyButton();
        m_filterEdit = shell.GetLogFilterEdit();
        m_listBox = shell.GetLogListBox();
        m_clipboard = &clipboard;
        RefreshVisibleRows();
    }

    void LogOutputPanelController::Append(LogOutputCategory category, std::wstring message, bool isError)
    {
        const std::size_t index = ToLogIndex(category);
        if (m_logs.size() <= index)
        {
            return;
        }

        m_logs[index].push_back(LogOutputEntry{ std::move(message), isError });
        if (GetActiveCategory() == category)
        {
            RefreshVisibleRows();
        }
    }

    void LogOutputPanelController::SelectCategory(LogOutputCategory category)
    {
        if (nullptr == m_tabControl)
        {
            return;
        }

        const int tabIndex = static_cast<int>(ToLogIndex(category));
        TabCtrl_SetCurSel(m_tabControl, tabIndex);
        m_lastActiveTab = tabIndex;
        RefreshVisibleRows();
    }

    void LogOutputPanelController::Update(const Core::InputSnapshot& inputSnapshot)
    {
        const int activeTab = nullptr != m_tabControl ? TabCtrl_GetCurSel(m_tabControl) : 0;
        const std::wstring filterText = GetFilterText();
        if (activeTab != m_lastActiveTab || filterText != m_lastFilterText)
        {
            m_lastActiveTab = activeTab;
            m_lastFilterText = filterText;
            RefreshVisibleRows();
        }

        const HierarchyButtonFrameInput frameInput{
            inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left),
            ToWin32Point(inputSnapshot.GetCursorScreenPoint())
        };
        if (TryConsumeHierarchyButtonClick(m_clearButton, frameInput, m_buttonInputState))
        {
            m_logs[ToLogIndex(GetActiveCategory())].clear();
            RefreshVisibleRows();
            m_buttonInputState.pressedButtonHandle = nullptr;
        }
        else if (TryConsumeHierarchyButtonClick(m_copyButton, frameInput, m_buttonInputState))
        {
            CopySelectedRow();
            m_buttonInputState.pressedButtonHandle = nullptr;
        }

        if (false == frameInput.isLeftMouseButtonDown && true == m_buttonInputState.wasLeftMouseButtonDown)
        {
            m_buttonInputState.pressedButtonHandle = nullptr;
        }

        m_buttonInputState.wasLeftMouseButtonDown = frameInput.isLeftMouseButtonDown;
    }

    bool LogOutputPanelController::HandleDrawItem(LPARAM drawItemParameter) const
    {
        const DRAWITEMSTRUCT* drawItem = reinterpret_cast<const DRAWITEMSTRUCT*>(drawItemParameter);
        if (nullptr == drawItem || ODT_LISTBOX != drawItem->CtlType || drawItem->hwndItem != m_listBox)
        {
            return false;
        }

        if (static_cast<UINT>(-1) == drawItem->itemID)
        {
            return true;
        }

        const bool isSelected = 0 != (drawItem->itemState & ODS_SELECTED);
        const COLORREF backgroundColor = GetSysColor(isSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW);
        const COLORREF textColor =
            0 != drawItem->itemData
                ? RGB(190, 0, 0)
                : GetSysColor(isSelected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);

        HBRUSH backgroundBrush = CreateSolidBrush(backgroundColor);
        FillRect(drawItem->hDC, &drawItem->rcItem, backgroundBrush);
        DeleteObject(backgroundBrush);

        const LRESULT textLength = SendMessageW(
            drawItem->hwndItem,
            LB_GETTEXTLEN,
            static_cast<WPARAM>(drawItem->itemID),
            0);
        if (0 <= textLength)
        {
            std::wstring text(static_cast<std::size_t>(textLength) + 1u, L'\0');
            SendMessageW(
                drawItem->hwndItem,
                LB_GETTEXT,
                static_cast<WPARAM>(drawItem->itemID),
                reinterpret_cast<LPARAM>(text.data()));
            text.resize(static_cast<std::size_t>(textLength));

            RECT textRect = drawItem->rcItem;
            textRect.left += 2;
            const COLORREF previousTextColor = SetTextColor(drawItem->hDC, textColor);
            const int previousBackgroundMode = SetBkMode(drawItem->hDC, TRANSPARENT);
            DrawTextW(
                drawItem->hDC,
                text.c_str(),
                static_cast<int>(text.size()),
                &textRect,
                DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
            SetBkMode(drawItem->hDC, previousBackgroundMode);
            SetTextColor(drawItem->hDC, previousTextColor);
        }

        if (0 != (drawItem->itemState & ODS_FOCUS))
        {
            DrawFocusRect(drawItem->hDC, &drawItem->rcItem);
        }

        return true;
    }

    void LogOutputPanelController::RefreshVisibleRows()
    {
        if (nullptr == m_listBox)
        {
            return;
        }

        SendMessageW(m_listBox, LB_RESETCONTENT, 0, 0);
        const std::vector<LogOutputEntry>& logs = m_logs[ToLogIndex(GetActiveCategory())];
        const std::wstring filterText = GetFilterText();
        for (const LogOutputEntry& log : logs)
        {
            if (ContainsFilter(log.message, filterText))
            {
                const LRESULT itemIndex =
                    SendMessageW(m_listBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(log.message.c_str()));
                if (LB_ERR != itemIndex && LB_ERRSPACE != itemIndex)
                {
                    SendMessageW(
                        m_listBox,
                        LB_SETITEMDATA,
                        static_cast<WPARAM>(itemIndex),
                        static_cast<LPARAM>(log.isError ? 1 : 0));
                }
            }
        }

        const LRESULT rowCount = SendMessageW(m_listBox, LB_GETCOUNT, 0, 0);
        if (0 < rowCount)
        {
            SendMessageW(m_listBox, LB_SETTOPINDEX, static_cast<WPARAM>(rowCount - 1), 0);
        }
    }

    void LogOutputPanelController::CopySelectedRow() const
    {
        if (nullptr == m_listBox)
        {
            return;
        }

        const LRESULT selectedIndex = SendMessageW(m_listBox, LB_GETCURSEL, 0, 0);
        if (LB_ERR == selectedIndex)
        {
            return;
        }

        const LRESULT textLength = SendMessageW(m_listBox, LB_GETTEXTLEN, static_cast<WPARAM>(selectedIndex), 0);
        if (textLength <= 0)
        {
            return;
        }

        std::wstring text(static_cast<std::size_t>(textLength) + 1u, L'\0');
        SendMessageW(m_listBox, LB_GETTEXT, static_cast<WPARAM>(selectedIndex), reinterpret_cast<LPARAM>(text.data()));
        text.resize(static_cast<std::size_t>(textLength));
        if (nullptr != m_clipboard)
        {
            (void)m_clipboard->SetText(text);
        }
    }

    LogOutputCategory LogOutputPanelController::GetActiveCategory() const
    {
        const int activeTab = nullptr != m_tabControl ? TabCtrl_GetCurSel(m_tabControl) : 0;
        if (1 == activeTab)
        {
            return LogOutputCategory::Build;
        }
        if (2 == activeTab)
        {
            return LogOutputCategory::Editor;
        }

        return LogOutputCategory::Game;
    }

    std::wstring LogOutputPanelController::GetFilterText() const
    {
        if (nullptr == m_filterEdit)
        {
            return {};
        }

        const int textLength = GetWindowTextLengthW(m_filterEdit);
        if (textLength <= 0)
        {
            return {};
        }

        std::wstring text(static_cast<std::size_t>(textLength) + 1u, L'\0');
        GetWindowTextW(m_filterEdit, text.data(), textLength + 1);
        text.resize(static_cast<std::size_t>(textLength));
        return text;
    }
}
