#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Scene.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Editor が扱うプロジェクトの保存先情報を保持する。
    /// </summary>
    struct EditorProjectInfo
    {
        /// <summary>
        /// プロジェクト名を表す。
        /// </summary>
        std::wstring name{};

        /// <summary>
        /// プロジェクトルートフォルダを表す。
        /// </summary>
        std::filesystem::path rootDirectory{};

        /// <summary>
        /// `.proj` ファイルのパスを表す。
        /// </summary>
        std::filesystem::path projectFilePath{};

        /// <summary>
        /// Scene 保存フォルダを表す。
        /// </summary>
        std::filesystem::path scenesDirectory{};

        /// <summary>
        /// 現在編集中の Scene ファイルパスを表す。
        /// </summary>
        std::filesystem::path activeScenePath{};
    };

    /// <summary>
    /// Editor プロジェクトのファイル構成と Scene 保存を管理する。
    /// </summary>
    class EditorProject
    {
    public:
        /// <summary>
        /// 新規プロジェクトを作成し、初期 Scene を保存する。
        /// </summary>
        /// <param name="projectName">作成するプロジェクト名。</param>
        /// <param name="parentDirectory">プロジェクトフォルダを作成する親フォルダ。</param>
        /// <param name="initialScene">初期 Scene として保存する内容。</param>
        /// <returns>作成に成功した場合は true。</returns>
        [[nodiscard]] bool Create(
            const std::wstring& projectName,
            const std::filesystem::path& parentDirectory,
            const Game::Scene& initialScene);

        /// <summary>
        /// 既存の `.proj` ファイルを開く。
        /// </summary>
        /// <param name="projectFilePath">開く `.proj` ファイル。</param>
        /// <returns>読込に成功した場合は true。</returns>
        [[nodiscard]] bool Open(const std::filesystem::path& projectFilePath);

        /// <summary>
        /// 現在のプロジェクトへ Scene を保存する。
        /// </summary>
        /// <param name="scene">保存対象 Scene。</param>
        /// <returns>保存に成功した場合は true。</returns>
        [[nodiscard]] bool Save(const Game::Scene& scene) const;

        /// <summary>
        /// 現在の Scene を別名プロジェクトとして保存する。
        /// </summary>
        /// <param name="projectName">新しいプロジェクト名。</param>
        /// <param name="parentDirectory">保存先親フォルダ。</param>
        /// <param name="scene">保存対象 Scene。</param>
        /// <returns>保存に成功した場合は true。</returns>
        [[nodiscard]] bool SaveAs(
            const std::wstring& projectName,
            const std::filesystem::path& parentDirectory,
            const Game::Scene& scene);

        /// <summary>
        /// 現在のプロジェクトで管理している Scene ファイルを列挙する。
        /// </summary>
        /// <returns>Scene ファイルパス一覧。</returns>
        [[nodiscard]] std::vector<std::filesystem::path> EnumerateSceneFiles() const;

        /// <summary>
        /// プロジェクトが開かれているかを取得する。
        /// </summary>
        /// <returns>プロジェクト情報を保持している場合は true。</returns>
        [[nodiscard]] bool HasProject() const;

        /// <summary>
        /// 現在のプロジェクト情報を取得する。
        /// </summary>
        /// <returns>プロジェクト情報。未オープン時は空。</returns>
        [[nodiscard]] const std::optional<EditorProjectInfo>& GetInfo() const;

    private:
        /// <summary>
        /// `.proj` ファイルを書き出す。
        /// </summary>
        /// <param name="info">書き出すプロジェクト情報。</param>
        /// <returns>書き出しに成功した場合は true。</returns>
        [[nodiscard]] bool WriteProjectFile(const EditorProjectInfo& info) const;

        /// <summary>
        /// Scene ファイルを書き出す。
        /// </summary>
        /// <param name="scenePath">保存先 Scene ファイルパス。</param>
        /// <param name="scene">保存対象 Scene。</param>
        /// <returns>書き出しに成功した場合は true。</returns>
        [[nodiscard]] bool WriteSceneFile(
            const std::filesystem::path& scenePath,
            const Game::Scene& scene) const;

    private:
        std::optional<EditorProjectInfo> m_info{};
    };
}
