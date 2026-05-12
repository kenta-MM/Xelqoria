#pragma once

#include <filesystem>

#include "AssetId.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Script Asset 生成結果を表す。
    /// </summary>
    struct ScriptAssetCreationResult
    {
        /// <summary>
        /// 生成に成功したかを表す。
        /// </summary>
        bool succeeded = false;

        /// <summary>
        /// 作成した Script Asset ファイルを表す。
        /// </summary>
        std::filesystem::path assetPath{};

        /// <summary>
        /// Editor が管理する C++ ソースファイルを表す。
        /// </summary>
        std::filesystem::path sourcePath{};

        /// <summary>
        /// 作成した Script Asset の識別子を表す。
        /// </summary>
        Core::AssetId scriptAssetId{};
    };

    /// <summary>
    /// Editor プロジェクト内の Script Asset と管理コードファイルを生成する。
    /// </summary>
    class ScriptAssetService
    {
    public:
        /// <summary>
        /// 指定パスが Script Asset ファイルかを判定する。
        /// </summary>
        /// <param name="path">判定対象パス。</param>
        /// <returns>Script Asset ファイルの場合は true。</returns>
        [[nodiscard]] static bool IsScriptAssetFile(const std::filesystem::path& path);

        /// <summary>
        /// Script Asset と初期 C++ コードを作成する。
        /// </summary>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <param name="targetDirectory">Script Asset 作成先ディレクトリ。</param>
        /// <returns>作成結果。</returns>
        [[nodiscard]] static ScriptAssetCreationResult CreateScriptAsset(
            const std::filesystem::path& projectRootDirectory,
            const std::filesystem::path& targetDirectory);
    };
}
