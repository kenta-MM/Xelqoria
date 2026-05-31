#pragma once

#include <filesystem>

namespace Xelqoria::Editor
{
    class EditorDockingController;

    /// <summary>
    /// Dock/Floating レイアウトの保存復元を担当する。
    /// </summary>
    class EditorDockingLayoutSerializer
    {
    public:
        /// <summary>
        /// DockingLayoutSerializer を生成する。
        /// </summary>
        /// <param name="controller">保存復元対象の DockingController。</param>
        explicit EditorDockingLayoutSerializer(EditorDockingController& controller);

        /// <summary>
        /// 現在の Dock/Floating レイアウトを保存する。
        /// </summary>
        [[nodiscard]] bool Save(const std::filesystem::path& layoutPath) const;

        /// <summary>
        /// 保存済み Dock/Floating レイアウトを復元する。
        /// </summary>
        [[nodiscard]] bool Load(const std::filesystem::path& layoutPath);

    private:
        EditorDockingController& m_controller;
    };
}
