#pragma once

#include <Windows.h>
#include <filesystem>

#include "InputSystem.h"
#include "Shell/EditorPanelId.h"

namespace Xelqoria::Editor
{
    class EditorShell;

    /// <summary>
    /// EditorShell の Dock/Floating 操作を集約して呼び出す。
    /// </summary>
    class EditorDockingController
    {
    public:
        /// <summary>
        /// DockingController を生成する。
        /// </summary>
        /// <param name="shell">Docking 操作を適用する EditorShell。</param>
        explicit EditorDockingController(EditorShell& shell);

        /// <summary>
        /// Dock UI のフレーム入力を処理する。
        /// </summary>
        [[nodiscard]] bool Update(HWND parentWindow, const Core::InputSnapshot& inputSnapshot);

        /// <summary>
        /// Dock 用 TabControl 通知を処理する。
        /// </summary>
        [[nodiscard]] bool HandleNotify(LPARAM notifyParameter);

        /// <summary>
        /// Dock レイアウトを初期状態へ戻す。
        /// </summary>
        void ResetLayout();

        /// <summary>
        /// 指定ビューを既定の Dock 位置へ表示する。
        /// </summary>
        void ShowPanelAtDefaultDock(EditorPanelId panelId);

        /// <summary>
        /// 指定ビューがある Dock または Floating のタブをアクティブにする。
        /// </summary>
        void ActivatePanel(EditorPanelId panelId);

        /// <summary>
        /// 現在の Dock / Floating レイアウトを保存する。
        /// </summary>
        [[nodiscard]] bool SaveLayout(const std::filesystem::path& layoutPath) const;

        /// <summary>
        /// 保存済み Dock / Floating レイアウトを復元する。
        /// </summary>
        [[nodiscard]] bool LoadLayout(const std::filesystem::path& layoutPath);

    private:
        EditorShell& m_shell;
    };
}
