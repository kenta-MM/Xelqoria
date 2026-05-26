#pragma once

#include <Windows.h>
#include <optional>
#include <string>
#include <vector>

#include "ButtonClickInput.h"
#include "EditorShell.h"
#include "IClipboard.h"
#include "InputSystem.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// LogOutput パネルに表示するログ種別を表す。
    /// </summary>
    enum class LogOutputCategory
    {
        Game = 0,
        Build = 1,
        Editor = 2
    };

    /// <summary>
    /// LogOutput の表示 severity を表す。
    /// </summary>
    enum class LogOutputSeverity
    {
        Normal = 0,
        Warning = 1,
        Error = 2
    };

    /// <summary>
    /// LogOutput パネルの表示と操作を管理する。
    /// </summary>
    class LogOutputPanelController
    {
    public:
        /// <summary>
        /// EditorShell の HWND 群へ接続する。
        /// </summary>
        /// <param name="shell">接続先の EditorShell。</param>
        void Bind(const EditorShell& shell, Platform::IClipboard& clipboard);

        /// <summary>
        /// 指定カテゴリへログ行を追加する。
        /// </summary>
        /// <param name="category">追加先カテゴリ。</param>
        /// <param name="message">追加するログ文面。</param>
        /// <param name="isError">エラーログとして表示する場合は true。</param>
        void Append(LogOutputCategory category, std::wstring message, bool isError = false);

        /// <summary>
        /// 指定ログカテゴリを選択して表示する。
        /// </summary>
        /// <param name="category">選択するログカテゴリ。</param>
        void SelectCategory(LogOutputCategory category);

        /// <summary>
        /// LogOutput パネルの入力操作と表示を更新する。
        /// </summary>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        void Update(const Core::InputSnapshot& inputSnapshot);

        /// <summary>
        /// LogOutput ListBox の owner-draw 描画を処理する。
        /// </summary>
        /// <param name="drawItemParameter">WM_DRAWITEM の LPARAM。</param>
        /// <returns>描画を処理した場合は true。</returns>
        bool HandleDrawItem(LPARAM drawItemParameter) const;

    private:
        /// <summary>
        /// LogOutput の 1 行を表す。
        /// </summary>
        struct LogOutputEntry
        {
            std::wstring message{};
            LogOutputCategory category = LogOutputCategory::Game;
            LogOutputSeverity severity = LogOutputSeverity::Normal;
        };

        /// <summary>
        /// ダミーログを初期投入する。
        /// </summary>
        void EnsureDummyLogs();

        /// <summary>
        /// 現在選択中フィルターのログ一覧を ListBox へ反映する。
        /// </summary>
        void RefreshVisibleRows();

        /// <summary>
        /// 選択中のログ行をクリップボードへコピーする。
        /// </summary>
        void CopySelectedRow() const;

        /// <summary>
        /// 現在選択中の severity フィルターを取得する。
        /// </summary>
        /// <returns>severity フィルター。すべての場合は空。</returns>
        [[nodiscard]] std::optional<LogOutputSeverity> GetActiveSeverityFilter() const;

        /// <summary>
        /// 現在のフィルタ文字列を取得する。
        /// </summary>
        /// <returns>フィルタ文字列。</returns>
        [[nodiscard]] std::wstring GetFilterText() const;

    private:
        HWND m_tabControl = nullptr;
        HWND m_clearButton = nullptr;
        HWND m_copyButton = nullptr;
        HWND m_filterEdit = nullptr;
        HWND m_listBox = nullptr;
        Platform::IClipboard* m_clipboard = nullptr;
        std::vector<LogOutputEntry> m_logs{};
        std::size_t m_visibleLogStartIndex = 0;
        ButtonClickInputState m_buttonInputState{};
        int m_lastActiveTab = 0;
        std::wstring m_lastFilterText{};
    };
}
