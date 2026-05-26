#pragma once

#include <Windows.h>
#include <filesystem>
#include <vector>

#include "Project/EditorSceneDocument.h"

namespace Xelqoria::Editor
{
    class SceneViewPanelView;

    /// <summary>
    /// 現在のプロジェクト内容と Scene 一覧の表示を管理する。
    /// </summary>
    class ProjectPanelController
    {
    public:
        /// <summary>
        /// SceneView Panel View 内の Project HWND 群へ接続する。
        /// </summary>
        /// <param name="view">接続先の Panel View。</param>
        void Bind(const SceneViewPanelView& view);

        /// <summary>
        /// プロジェクト情報と Scene 一覧を再表示する。
        /// </summary>
        /// <param name="document">表示対象のドキュメント。</param>
        /// <param name="sceneDirty">現在 Scene に未保存変更がある場合は true。</param>
        void Refresh(const EditorSceneDocument& document, bool sceneDirty = false);

        /// <summary>
        /// Scene 一覧の選択を監視し、選択 Scene を読み込む。
        /// </summary>
        /// <param name="document">更新対象のドキュメント。</param>
        /// <returns>Scene が読み替えられた場合は true。</returns>
        [[nodiscard]] bool Update(EditorSceneDocument& document);

        /// <summary>
        /// Scene 一覧で現在 Scene とは異なる項目が選択されているかを取得する。
        /// </summary>
        /// <returns>Scene 読み替え要求がある場合は true。</returns>
        [[nodiscard]] bool HasSceneSelectionChangeRequest() const;

    private:
        /// <summary>
        /// 選択中 Scene の概要を表示する。
        /// </summary>
        /// <param name="document">表示対象のドキュメント。</param>
        /// <param name="sceneDirty">現在 Scene に未保存変更がある場合は true。</param>
        void RefreshSelectedSceneDetail(const EditorSceneDocument& document, bool sceneDirty);

        /// <summary>
        /// Scene ファイル名に Dirty 表示を反映する。
        /// </summary>
        /// <param name="scenePath">表示対象 Scene パス。</param>
        /// <param name="sceneDirty">Dirty 表示を付ける場合は true。</param>
        /// <returns>表示用 Scene 名。</returns>
        [[nodiscard]] static std::wstring BuildSceneLabel(const std::filesystem::path& scenePath, bool sceneDirty);

    private:
        HWND m_projectSummaryLabel = nullptr;
        HWND m_projectSceneListBox = nullptr;
        HWND m_projectSceneDetailLabel = nullptr;
        std::vector<std::filesystem::path> m_sceneFiles{};
        std::filesystem::path m_selectedScenePath{};
    };
}
