#pragma once

#include <filesystem>

namespace Xelqoria::Editor
{
    class EditorShell;

    /// <summary>
    /// Dock/Floating レイアウトの保存復元を担当する。
    /// </summary>
    class EditorDockingLayoutSerializer
    {
    public:
        /// <summary>
        /// DockingLayoutSerializer を生成する。
        /// </summary>
        /// <param name="shell">保存復元対象の EditorShell。</param>
        explicit EditorDockingLayoutSerializer(EditorShell& shell);

        /// <summary>
        /// 現在の Dock/Floating レイアウトを保存する。
        /// </summary>
        [[nodiscard]] bool Save(const std::filesystem::path& layoutPath) const;

        /// <summary>
        /// 保存済み Dock/Floating レイアウトを復元する。
        /// </summary>
        [[nodiscard]] bool Load(const std::filesystem::path& layoutPath);

    private:
        EditorShell& m_shell;
    };
}
