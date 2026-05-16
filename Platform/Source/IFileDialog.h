#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "PlatformTypes.h"

namespace Xelqoria::Platform
{
    /// <summary>
    /// ファイルダイアログに表示する拡張子フィルタを表す。
    /// </summary>
    struct FileDialogFilter
    {
        /// <summary>
        /// 利用者へ表示するフィルタ名。
        /// </summary>
        std::wstring displayName{};

        /// <summary>
        /// OS に渡すフィルタパターン。
        /// </summary>
        std::wstring pattern{};
    };

    /// <summary>
    /// ファイル選択ダイアログの表示オプションを表す。
    /// </summary>
    struct FileDialogOptions
    {
        /// <summary>
        /// 親ウィンドウの OS 固有ハンドル。
        /// </summary>
        NativeWindowHandle ownerWindow = nullptr;

        /// <summary>
        /// ダイアログのタイトル。
        /// </summary>
        std::wstring title{};

        /// <summary>
        /// 選択候補のフィルタ一覧。
        /// </summary>
        std::vector<FileDialogFilter> filters{};

        /// <summary>
        /// 初期表示ディレクトリ。
        /// </summary>
        std::filesystem::path initialDirectory{};
    };

    /// <summary>
    /// フォルダ選択ダイアログの表示オプションを表す。
    /// </summary>
    struct FolderDialogOptions
    {
        /// <summary>
        /// 親ウィンドウの OS 固有ハンドル。
        /// </summary>
        NativeWindowHandle ownerWindow = nullptr;

        /// <summary>
        /// ダイアログに表示する説明文。
        /// </summary>
        std::wstring title{};
    };

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
        /// <param name="options">ダイアログの表示オプション。</param>
        /// <returns>選択されたパス。キャンセル時は std::nullopt。</returns>
        [[nodiscard]] virtual std::optional<std::filesystem::path> OpenFile(const FileDialogOptions& options) = 0;

        /// <summary>
        /// ファイルを保存するダイアログを表示する。
        /// </summary>
        /// <param name="options">ダイアログの表示オプション。</param>
        /// <returns>選択された保存先パス。キャンセル時は std::nullopt。</returns>
        [[nodiscard]] virtual std::optional<std::filesystem::path> SaveFile(const FileDialogOptions& options) = 0;

        /// <summary>
        /// フォルダを選択するダイアログを表示する。
        /// </summary>
        /// <param name="options">ダイアログの表示オプション。</param>
        /// <returns>選択されたフォルダ。キャンセル時は std::nullopt。</returns>
        [[nodiscard]] virtual std::optional<std::filesystem::path> OpenFolder(const FolderDialogOptions& options) = 0;
    };
}
