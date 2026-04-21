#pragma once

#include <Windows.h>
#include <filesystem>
#include <vector>

#include "EditorSceneDocument.h"
#include "EditorShell.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// 現在のプロジェクト内容と Scene 一覧の表示を管理する。
    /// </summary>
    class ProjectPanelController
    {
    public:
        /// <summary>
        /// EditorShell の HWND 群へ接続する。
        /// </summary>
        /// <param name="shell">接続先の EditorShell。</param>
        void Bind(const EditorShell& shell);

        /// <summary>
        /// プロジェクト情報と Scene 一覧を再表示する。
        /// </summary>
        /// <param name="document">表示対象のドキュメント。</param>
        void Refresh(const EditorSceneDocument& document);

        /// <summary>
        /// Scene 一覧の選択を監視し、選択 Scene を読み込む。
        /// </summary>
        /// <param name="document">更新対象のドキュメント。</param>
        /// <returns>Scene が読み替えられた場合は true。</returns>
        [[nodiscard]] bool Update(EditorSceneDocument& document);

    private:
        /// <summary>
        /// 選択中 Scene の概要を表示する。
        /// </summary>
        /// <param name="document">表示対象のドキュメント。</param>
        void RefreshSelectedSceneDetail(const EditorSceneDocument& document);

    private:
        HWND m_projectSummaryLabel = nullptr;
        HWND m_projectSceneListBox = nullptr;
        HWND m_projectSceneDetailLabel = nullptr;
        std::vector<std::filesystem::path> m_sceneFiles{};
        std::filesystem::path m_selectedScenePath{};
    };
}
