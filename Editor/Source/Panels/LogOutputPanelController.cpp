#include "Panels/LogOutputPanelController.h"

#include <algorithm>
#include <CommCtrl.h>
#include <chrono>
#include <ctime>
#include <cwchar>
#include <utility>

#include "PlatformAdapters/ButtonClickWin32Adapter.h"
#include "EditorTheme.h"

namespace Xelqoria::Editor
{
    namespace
    {
        [[nodiscard]] bool ContainsFilter(const std::wstring& text, const std::wstring& filter)
        {
            if (filter.empty())
            {
                return true;
            }

            return text.find(filter) != std::wstring::npos;
        }

        [[nodiscard]] BYTE ToColorByte(float value)
        {
            const float clampedValue = (std::max)(0.0f, (std::min)(1.0f, value));
            return static_cast<BYTE>((clampedValue * 255.0f) + 0.5f);
        }

        [[nodiscard]] COLORREF ToColorRef(EditorColor color)
        {
            return RGB(ToColorByte(color.red), ToColorByte(color.green), ToColorByte(color.blue));
        }

        void FillRectWithThemeColor(HDC deviceContext, const RECT& rect, EditorColor color)
        {
            HBRUSH brush = CreateSolidBrush(ToColorRef(color));
            FillRect(deviceContext, &rect, brush);
            DeleteObject(brush);
        }

        [[nodiscard]] LogOutputSeverity InferSeverity(
            const std::wstring& message,
            bool isError)
        {
            if (isError)
            {
                return LogOutputSeverity::Error;
            }

            if (message.find(L"warning") != std::wstring::npos
                || message.find(L"Warning") != std::wstring::npos
                || message.find(L"警告") != std::wstring::npos)
            {
                return LogOutputSeverity::Warning;
            }

            return LogOutputSeverity::Normal;
        }

        [[nodiscard]] const wchar_t* ToCategoryText(LogOutputCategory category)
        {
            switch (category)
            {
            case LogOutputCategory::Game:
                return L"Game";
            case LogOutputCategory::Build:
                return L"Build";
            case LogOutputCategory::Editor:
                return L"Editor";
            default:
                return L"Log";
            }
        }

        [[nodiscard]] std::wstring BuildTimestampText()
        {
            using clock = std::chrono::system_clock;
            const std::time_t currentTime = clock::to_time_t(clock::now());
            std::tm localTime{};
            localtime_s(&localTime, &currentTime);

            wchar_t text[32]{};
            std::wcsftime(text, std::size(text), L"%H:%M:%S", &localTime);
            return text;
        }

        [[nodiscard]] std::wstring FormatLogLine(LogOutputCategory category, LogOutputSeverity severity, const std::wstring& message)
        {
            std::wstring line = L"[";
            line += BuildTimestampText();
            line += L"] [";
            if (LogOutputSeverity::Warning == severity)
            {
                line += L"Warn";
            }
            else if (LogOutputSeverity::Error == severity)
            {
                line += L"Error";
            }
            else if (LogOutputCategory::Editor == category)
            {
                line += L"Editor";
            }
            else
            {
                line += L"Info";
            }
            line += L"] [";
            line += ToCategoryText(category);
            line += L"] ";
            line += message;
            return line;
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
        EnsureDummyLogs();
        RefreshVisibleRows();
    }

    void LogOutputPanelController::Append(LogOutputCategory category, std::wstring message, bool isError)
    {
        const LogOutputSeverity severity = InferSeverity(message, isError);
        m_logs.push_back(LogOutputEntry{ FormatLogLine(category, severity, message), category, severity });
        RefreshVisibleRows();
    }

    void LogOutputPanelController::SelectCategory(LogOutputCategory category)
    {
        (void)category;
        if (nullptr == m_tabControl)
        {
            return;
        }

        const int tabIndex = 0;
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

        const ButtonClickFrameInput frameInput{
            inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left),
            inputSnapshot.GetCursorScreenPoint()
        };
        if (TryConsumeButtonClick(BuildButtonClickTarget(m_clearButton), frameInput, m_buttonInputState))
        {
            m_visibleLogStartIndex = m_logs.size();
            RefreshVisibleRows();
            m_buttonInputState.pressedButtonId = 0;
        }
        else if (TryConsumeButtonClick(BuildButtonClickTarget(m_copyButton), frameInput, m_buttonInputState))
        {
            CopySelectedRow();
            m_buttonInputState.pressedButtonId = 0;
        }

        if (false == frameInput.isLeftMouseButtonDown && true == m_buttonInputState.wasLeftMouseButtonDown)
        {
            m_buttonInputState.pressedButtonId = 0;
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
        const auto severity = static_cast<LogOutputSeverity>(drawItem->itemData % 10);
        const auto category = static_cast<LogOutputCategory>(drawItem->itemData / 10);
        const EditorColor backgroundColor =
            isSelected ? EditorThemes::XelqoriaDark.selection : EditorThemes::XelqoriaDark.panelBackground;
        EditorColor textColor = EditorColor::FromRgb8(0x78, 0xB7, 0xFF);
        if (LogOutputSeverity::Warning == severity)
        {
            textColor = EditorThemes::XelqoriaDark.warning;
        }
        else if (LogOutputSeverity::Error == severity)
        {
            textColor = EditorThemes::XelqoriaDark.error;
        }
        else if (LogOutputCategory::Editor == category)
        {
            textColor = EditorThemes::XelqoriaDark.accent;
        }

        FillRectWithThemeColor(drawItem->hDC, drawItem->rcItem, backgroundColor);

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
            textRect.left += 8;
            textRect.right -= 8;
            const COLORREF previousTextColor = SetTextColor(drawItem->hDC, ToColorRef(textColor));
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
            RECT focusRect = drawItem->rcItem;
            InflateRect(&focusRect, -1, -1);
            HPEN focusPen = CreatePen(PS_SOLID, 1, ToColorRef(EditorThemes::XelqoriaDark.accent));
            HGDIOBJ previousPen = SelectObject(drawItem->hDC, focusPen);
            HGDIOBJ previousBrush = SelectObject(drawItem->hDC, GetStockObject(NULL_BRUSH));
            Rectangle(drawItem->hDC, focusRect.left, focusRect.top, focusRect.right, focusRect.bottom);
            SelectObject(drawItem->hDC, previousBrush);
            SelectObject(drawItem->hDC, previousPen);
            DeleteObject(focusPen);
        }

        return true;
    }

    void LogOutputPanelController::EnsureDummyLogs()
    {
        if (false == m_logs.empty())
        {
            return;
        }

        m_logs.push_back(LogOutputEntry{ FormatLogLine(LogOutputCategory::Editor, LogOutputSeverity::Normal, L"Xelqoria Editor started"), LogOutputCategory::Editor, LogOutputSeverity::Normal });
        m_logs.push_back(LogOutputEntry{ FormatLogLine(LogOutputCategory::Editor, LogOutputSeverity::Normal, L"Project loaded"), LogOutputCategory::Editor, LogOutputSeverity::Normal });
        m_logs.push_back(LogOutputEntry{ FormatLogLine(LogOutputCategory::Editor, LogOutputSeverity::Normal, L"Assets scan completed"), LogOutputCategory::Editor, LogOutputSeverity::Normal });
        m_logs.push_back(LogOutputEntry{ FormatLogLine(LogOutputCategory::Game, LogOutputSeverity::Warning, L"Scene initialized"), LogOutputCategory::Game, LogOutputSeverity::Warning });
        m_logs.push_back(LogOutputEntry{ FormatLogLine(LogOutputCategory::Build, LogOutputSeverity::Error, L"Renderer ready"), LogOutputCategory::Build, LogOutputSeverity::Error });
    }

    void LogOutputPanelController::RefreshVisibleRows()
    {
        if (nullptr == m_listBox)
        {
            return;
        }

        SendMessageW(m_listBox, WM_SETREDRAW, FALSE, 0);
        SendMessageW(m_listBox, LB_RESETCONTENT, 0, 0);
        const std::wstring filterText = GetFilterText();
        const std::optional<LogOutputSeverity> severityFilter = GetActiveSeverityFilter();
        const int activeTab = nullptr != m_tabControl ? TabCtrl_GetCurSel(m_tabControl) : 0;
        for (std::size_t index = m_visibleLogStartIndex; index < m_logs.size(); ++index)
        {
            const LogOutputEntry& log = m_logs[index];
            const bool severityMatched = false == severityFilter.has_value() || log.severity == *severityFilter;
            const bool categoryMatched =
                0 == activeTab
                || (1 == activeTab && LogOutputSeverity::Normal == log.severity && LogOutputCategory::Editor != log.category)
                || (2 == activeTab && LogOutputCategory::Editor == log.category)
                || (3 == activeTab && LogOutputSeverity::Warning == log.severity)
                || (4 == activeTab && LogOutputSeverity::Error == log.severity);
            if (severityMatched && categoryMatched && ContainsFilter(log.message, filterText))
            {
                const LRESULT itemIndex =
                    SendMessageW(m_listBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(log.message.c_str()));
                if (LB_ERR != itemIndex && LB_ERRSPACE != itemIndex)
                {
                    SendMessageW(
                        m_listBox,
                        LB_SETITEMDATA,
                        static_cast<WPARAM>(itemIndex),
                        static_cast<LPARAM>(static_cast<int>(log.category) * 10 + static_cast<int>(log.severity)));
                }
            }
        }

        const LRESULT rowCount = SendMessageW(m_listBox, LB_GETCOUNT, 0, 0);
        if (0 < rowCount)
        {
            SendMessageW(m_listBox, LB_SETTOPINDEX, static_cast<WPARAM>(rowCount - 1), 0);
        }
        SendMessageW(m_listBox, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(m_listBox, nullptr, FALSE);
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

    std::optional<LogOutputSeverity> LogOutputPanelController::GetActiveSeverityFilter() const
    {
        const int activeTab = nullptr != m_tabControl ? TabCtrl_GetCurSel(m_tabControl) : 0;
        if (1 == activeTab)
        {
            return LogOutputSeverity::Normal;
        }
        if (2 == activeTab)
        {
            return std::nullopt;
        }
        if (3 == activeTab)
        {
            return LogOutputSeverity::Warning;
        }
        if (4 == activeTab)
        {
            return LogOutputSeverity::Error;
        }

        return std::nullopt;
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
