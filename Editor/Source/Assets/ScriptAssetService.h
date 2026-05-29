#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

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
    /// Script 管理 C++ ソースのビルド設定を表す。
    /// </summary>
    struct ScriptBuildOptions
    {
        /// <summary>
        /// C++ コンパイラ実行ファイルまたはコマンドを表す。
        /// </summary>
        std::filesystem::path compilerExecutable = L"cl.exe";

        /// <summary>
        /// コンパイラ実行前に呼び出す開発環境セットアップバッチを表す。
        /// </summary>
        std::filesystem::path environmentSetupBatch{};
    };

    /// <summary>
    /// Script ビルドで生成された実行用モジュールを表す。
    /// </summary>
    struct ScriptBuildArtifact
    {
        /// <summary>
        /// 対応する Script Asset 識別子を表す。
        /// </summary>
        Core::AssetId scriptAssetId{};

        /// <summary>
        /// ビルド対象になった C++ ソースファイルを表す。
        /// </summary>
        std::filesystem::path sourcePath{};

        /// <summary>
        /// 実行時にロードするモジュールファイルを表す。
        /// </summary>
        std::filesystem::path modulePath{};
    };

    /// <summary>
    /// Script 管理 C++ ソースのビルド結果を表す。
    /// </summary>
    struct ScriptBuildResult
    {
        /// <summary>
        /// ビルドに成功したかを表す。
        /// </summary>
        bool succeeded = false;

        /// <summary>
        /// ビルド対象になった C++ ソースファイル一覧を表す。
        /// </summary>
        std::vector<std::filesystem::path> sourcePaths{};

        /// <summary>
        /// ビルドにより生成された Script 実行モジュール一覧を表す。
        /// </summary>
        std::vector<ScriptBuildArtifact> artifacts{};

        /// <summary>
        /// Editor に表示するビルド診断メッセージを表す。
        /// </summary>
        std::wstring diagnostics{};
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
        /// プロジェクト配下の Script Asset ファイルパスから Script AssetId を生成する。
        /// </summary>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <param name="assetPath">Script Asset ファイルパス。</param>
        /// <returns>Script AssetId。生成できない場合は空。</returns>
        [[nodiscard]] static Core::AssetId BuildScriptAssetId(
            const std::filesystem::path& projectRootDirectory,
            const std::filesystem::path& assetPath);

        /// <summary>
        /// Script Asset マニフェストから Editor 管理下の C++ ソースパスを解決する。
        /// </summary>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <param name="assetPath">Script Asset ファイルパス。</param>
        /// <returns>解決できた C++ ソースパス。失敗時は空。</returns>
        [[nodiscard]] static std::optional<std::filesystem::path> ResolveSourcePath(
            const std::filesystem::path& projectRootDirectory,
            const std::filesystem::path& assetPath);

        /// <summary>
        /// Script Asset と初期 C++ コードを作成する。
        /// </summary>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <param name="targetDirectory">Script Asset 作成先ディレクトリ。</param>
        /// <returns>作成結果。</returns>
        [[nodiscard]] static ScriptAssetCreationResult CreateScriptAsset(
            const std::filesystem::path& projectRootDirectory,
            const std::filesystem::path& targetDirectory);

        /// <summary>
        /// プロジェクト配下の Script Asset が参照する管理 C++ ソースをビルドする。
        /// </summary>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <param name="options">ビルド設定。</param>
        /// <returns>ビルド結果。診断にはコンパイル出力を含む。</returns>
        [[nodiscard]] static ScriptBuildResult BuildProjectScripts(
            const std::filesystem::path& projectRootDirectory,
            const ScriptBuildOptions& options = {});

    };
}
