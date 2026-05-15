#include "LogOutputPanelController.h"

#include <CommCtrl.h>
#include <cstring>
#include <utility>

namespace Xelqoria::Editor
{
    namespace
    {
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

        void SetClipboardText(HWND ownerWindow, const std::wstring& text)
        {
            if (text.empty() || FALSE == OpenClipboard(ownerWindow))
            {
                return;
            }

            EmptyClipboard();
            const SIZE_T byteSize = (text.size() + 1u) * sizeof(wchar_t);
            HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, byteSize);
            if (nullptr == memory)
            {
                CloseClipboard();
                return;
            }

            void* data = GlobalLock(memory);
            if (nullptr == data)
            {
                GlobalFree(memory);
                CloseClipboard();
                return;
            }

            std::memcpy(data, text.c_str(), byteSize);
            GlobalUnlock(memory);
            SetClipboardData(CF_UNICODETEXT, memory);
            CloseClipboard();
        }
    }

    void LogOutputPanelController::Bind(const EditorShell& shell)
    {
        m_tabControl = shell.GetLogOutputTabControl();
        m_clearButton = shell.GetLogClearButton();
        m_copyButton = shell.GetLogCopyButton();
        m_filterEdit = shell.GetLogFilterEdit();
        m_listBox = shell.GetLogListBox();
        RefreshVisibleRows();
    }

    void LogOutputPanelController::Append(LogOutputCategory category, std::wstring message)
    {
        const std::size_t index = ToLogIndex(category);
        if (m_logs.size() <= index)
        {
            return;
        }

        m_logs[index].push_back(std::move(message));
        if (GetActiveCategory() == category)
        {
            RefreshVisibleRows();
        }
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
            inputSnapshot.GetCursorScreenPoint()
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

    void LogOutputPanelController::RefreshVisibleRows()
    {
        if (nullptr == m_listBox)
        {
            return;
        }

        SendMessageW(m_listBox, LB_RESETCONTENT, 0, 0);
        const std::vector<std::wstring>& logs = m_logs[ToLogIndex(GetActiveCategory())];
        const std::wstring filterText = GetFilterText();
        for (const std::wstring& log : logs)
        {
            if (ContainsFilter(log, filterText))
            {
                SendMessageW(m_listBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(log.c_str()));
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
        SetClipboardText(GetParent(m_listBox), text);
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
