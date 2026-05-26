#pragma once

#include <filesystem>
#include <vector>

#include "Project/EditorProject.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// 最近使ったプロジェクト一覧の永続化を行う。
    /// </summary>
    class RecentProjectsStore
    {
    public:
        /// <summary>
        /// 最近使ったプロジェクト一覧を読み込む。
        /// </summary>
        /// <returns>存在する `.proj` のみを含むプロジェクト情報一覧。</returns>
        [[nodiscard]] std::vector<EditorProjectInfo> Load() const;

        /// <summary>
        /// 指定プロジェクトを先頭へ記録する。
        /// </summary>
        /// <param name="projectInfo">記録するプロジェクト情報。</param>
        /// <returns>保存に成功した場合は true。</returns>
        [[nodiscard]] bool Record(const EditorProjectInfo& projectInfo) const;

    private:
        /// <summary>
        /// 最近使ったプロジェクト一覧の保存先を取得する。
        /// </summary>
        /// <returns>保存先ファイルパス。</returns>
        [[nodiscard]] std::filesystem::path GetStorePath() const;
    };
}
