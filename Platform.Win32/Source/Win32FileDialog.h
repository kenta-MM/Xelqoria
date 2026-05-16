#pragma once

#include "IFileDialog.h"

namespace Xelqoria::Platform::Win32
{
    /// <summary>
    /// Win32 API を使用するファイルダイアログ実装。
    /// </summary>
    class Win32FileDialog final : public IFileDialog
    {
    public:
        /// <summary>
        /// ファイルを開くダイアログを表示する。
        /// </summary>
        /// <param name="options">ダイアログの表示オプション。</param>
        /// <returns>選択されたパス。キャンセル時は std::nullopt。</returns>
        [[nodiscard]] std::optional<std::filesystem::path> OpenFile(const FileDialogOptions& options) override;

        /// <summary>
        /// ファイルを保存するダイアログを表示する。
        /// </summary>
        /// <param name="options">ダイアログの表示オプション。</param>
        /// <returns>選択された保存先パス。キャンセル時は std::nullopt。</returns>
        [[nodiscard]] std::optional<std::filesystem::path> SaveFile(const FileDialogOptions& options) override;

        /// <summary>
        /// フォルダを選択するダイアログを表示する。
        /// </summary>
        /// <param name="options">ダイアログの表示オプション。</param>
        /// <returns>選択されたフォルダ。キャンセル時は std::nullopt。</returns>
        [[nodiscard]] std::optional<std::filesystem::path> OpenFolder(const FolderDialogOptions& options) override;
    };
}
