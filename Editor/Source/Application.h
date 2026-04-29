#pragma once

#include <Windows.h>

#include "AssetsPanelController.h"
#include "InputSystem.h"
#include "Window.h"
#include "EditorCommandController.h"
#include "EditorSceneDocument.h"
#include "EditorShell.h"
#include "HierarchyPanelController.h"
#include "InspectorPanelController.h"
#include "ProjectPanelController.h"
#include "SceneViewController.h"
#include "SceneViewRenderer.h"
#include "StartupScreenController.h"
#include "RecentProjectsStore.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Editor ターゲットの実行フローを組み立てる。
    /// </summary>
    class Application
    {
    public:
        /// <summary>
        /// Editor 用 Application を生成する。
        /// </summary>
        /// <param name="hInstance">Windows アプリケーションのインスタンスハンドル。</param>
        explicit Application(HINSTANCE hInstance);

        /// <summary>
        /// 内部リソースを解放する。
        /// </summary>
        ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        /// <summary>
        /// Editor のメインループを実行する。
        /// </summary>
        /// <returns>プロセス終了コード。</returns>
        int Run();

    private:
        /// <summary>
        /// Editor ターゲットを初期化する。
        /// </summary>
        /// <returns>初期化に成功した場合は true。</returns>
        bool Initialize();

        /// <summary>
        /// Editor 本体画面を初期化する。
        /// </summary>
        /// <returns>初期化に成功した場合は true。</returns>
        bool InitializeEditorWorkspace();

        /// <summary>
        /// ヘッダーのプロジェクトメニューを初期化する。
        /// </summary>
        void InitializeProjectMenu();

        /// <summary>
        /// プロジェクトメニューのコマンドを処理する。
        /// </summary>
        /// <param name="commandId">選択されたコマンド ID。</param>
        void HandleProjectMenuCommand(unsigned commandId);

        /// <summary>
        /// 終了要求を処理する。
        /// </summary>
        /// <returns>終了を継続してよい場合は true。</returns>
        bool HandleCloseRequest();

        /// <summary>
        /// 起動画面から新規プロジェクトを作成して Editor へ遷移する。
        /// </summary>
        /// <returns>遷移に成功した場合は true。</returns>
        bool EnterEditorWithNewProject();

        /// <summary>
        /// 起動画面から既存プロジェクトを開いて Editor へ遷移する。
        /// </summary>
        /// <returns>遷移に成功した場合は true。</returns>
        bool EnterEditorWithExistingProject();

        /// <summary>
        /// ヘッダー操作から新規プロジェクトを作成する。
        /// </summary>
        void CreateProjectFromMenu();

        /// <summary>
        /// ヘッダー操作から既存プロジェクトを開く。
        /// </summary>
        void OpenProjectFromMenu();

        /// <summary>
        /// 現在のプロジェクトを保存する。
        /// </summary>
        /// <returns>保存に成功した場合は true。</returns>
        bool SaveProjectFromMenu();

        /// <summary>
        /// 現在のプロジェクトを別名で保存する。
        /// </summary>
        void SaveProjectAsFromMenu();

        /// <summary>
        /// 未保存変更があれば保存確認を表示する。
        /// </summary>
        /// <returns>後続操作を継続してよい場合は true。</returns>
        bool ConfirmSaveIfDirty();

        /// <summary>
        /// Scene に未保存変更があることを記録する。
        /// </summary>
        void MarkProjectDirty();

        /// <summary>
        /// Scene の未保存変更が保存されたことを記録する。
        /// </summary>
        void ClearProjectDirty();

        /// <summary>
        /// 終了時のクリーンアップを行う。
        /// </summary>
        void Shutdown();

        /// <summary>
        /// Editor 用のフレーム更新処理を行う。
        /// </summary>
        /// <param name="deltaTime">前フレームからの経過時間（秒）。</param>
        void Update(float deltaTime);

        /// <summary>
        /// Editor 用のフレーム描画を行う。
        /// </summary>
        void Render();

        /// <summary>
        /// Assets パネル、Hierarchy、Inspector の表示を現在状態へ同期する。
        /// </summary>
        /// <param name="canAddSpriteComponent">SpriteComponent を追加可能な場合は true。</param>
        /// <param name="resetTrackedEntity">Inspector の追跡状態をリセットする場合は true。</param>
        void RefreshEditorPanels(bool canAddSpriteComponent, bool resetTrackedEntity);

        /// <summary>
        /// 現在選択中 Entity に合わせて SceneView 状態ラベルを更新する。
        /// </summary>
        void RefreshSceneViewSelectionStatus();

        /// <summary>
        /// 選択 Entity を更新し、関連パネルを再同期する。
        /// </summary>
        /// <param name="selectedEntityId">更新後の選択 EntityId。</param>
        /// <param name="canAddSpriteComponent">SpriteComponent を追加可能な場合は true。</param>
        /// <param name="resetTrackedEntity">Inspector の追跡状態をリセットする場合は true。</param>
        /// <param name="refreshSceneViewSelectionStatus">SceneView 状態ラベルも更新する場合は true。</param>
        void ApplySelectionChange(
            std::optional<Game::EntityId> selectedEntityId,
            bool canAddSpriteComponent,
            bool resetTrackedEntity,
            bool refreshSceneViewSelectionStatus);

        /// <summary>
        /// Scene 変更内容を保存し、必要に応じて履歴へ積む。
        /// </summary>
        /// <param name="successMessage">保存成功時の表示文面。</param>
        /// <param name="failureMessage">保存失敗時の表示文面。</param>
        /// <param name="pushHistory">保存成功時に履歴へ積む場合は true。</param>
        /// <returns>保存に成功した場合は true。</returns>
        bool PersistSceneChanges(
            const wchar_t* successMessage,
            const wchar_t* failureMessage,
            bool pushHistory);

        /// <summary>
        /// 現在のプロジェクトを最近使った一覧へ記録する。
        /// </summary>
        void RecordCurrentProject();

    private:
        HINSTANCE m_hInstance = nullptr;
        Core::Window m_window{};
        Core::InputSystem m_inputSystem{};
        bool m_running = true;
        bool m_editorInitialized = false;
        bool m_projectDirty = false;
        HMENU m_projectMenu = nullptr;
        EditorShell m_editorShell{};
        StartupScreenController m_startupScreenController{};
        RecentProjectsStore m_recentProjectsStore{};
        EditorSceneDocument m_sceneDocument{};
        AssetsPanelController m_assetsPanelController{};
        HierarchyPanelController m_hierarchyPanelController{};
        InspectorPanelController m_inspectorPanelController{};
        ProjectPanelController m_projectPanelController{};
        SceneViewController m_sceneViewController{};
        SceneViewRenderer m_sceneViewRenderer{};
        EditorCommandController m_editorCommandController{};
    };
}
