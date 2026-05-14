#pragma once

#include <Windows.h>
#include <array>
#include <string>
#include <vector>

#include "EditorShell.h"
#include "HierarchyPanelController.h"
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
    /// LogOutput パネルの表示と操作を管理する。
    /// </summary>
    class LogOutputPanelController
    {
    public:
        /// <summary>
        /// EditorShell の HWND 群へ接続する。
        /// </summary>
        /// <param name="shell">接続先の EditorShell。</param>
        void Bind(const EditorShell& shell);

        /// <summary>
        /// 指定カテゴリへログ行を追加する。
        /// </summary>
        /// <param name="category">追加先カテゴリ。</param>
        /// <param name="message">追加するログ文面。</param>
        void Append(LogOutputCategory category, std::wstring message);

        /// <summary>
        /// LogOutput パネルの入力操作と表示を更新する。
        /// </summary>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        void Update(const Core::InputSnapshot& inputSnapshot);

    private:
        /// <summary>
        /// 現在選択中カテゴリのログ一覧を ListBox へ反映する。
        /// </summary>
        void RefreshVisibleRows();

        /// <summary>
        /// 選択中のログ行をクリップボードへコピーする。
        /// </summary>
        void CopySelectedRow() const;

        /// <summary>
        /// 現在選択中のログカテゴリを取得する。
        /// </summary>
        /// <returns>選択中ログカテゴリ。</returns>
        [[nodiscard]] LogOutputCategory GetActiveCategory() const;

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
        std::array<std::vector<std::wstring>, 3> m_logs{};
        HierarchyButtonInputState m_buttonInputState{};
        int m_lastActiveTab = 0;
        std::wstring m_lastFilterText{};
    };
}
