#pragma once

#include <filesystem>
#include <optional>

namespace Xelqoria::Platform
{
    /// <summary>
    /// OS 標準のファイル選択ダイアログを抽象化する。
    /// </summary>
    class IFileDialog
    {
    public:
        virtual ~IFileDialog() = default;

        /// <summary>
        /// ファイルを開くダイアログを表示する。
        /// </summary>
        /// <returns>選択されたパス。キャンセル時は std::nullopt。</returns>
        [[nodiscard]] virtual std::optional<std::filesystem::path> OpenFile() = 0;

        /// <summary>
        /// ファイルを保存するダイアログを表示する。
        /// </summary>
        /// <returns>選択された保存先パス。キャンセル時は std::nullopt。</returns>
        [[nodiscard]] virtual std::optional<std::filesystem::path> SaveFile() = 0;
    };
}
