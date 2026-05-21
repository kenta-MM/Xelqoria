#pragma once

#include <Windows.h>
#include <cstdint>
#include <filesystem>
#include <future>
#include <optional>
#include <string>

#include "AssetsPanelController.h"
#include "InputSystem.h"
#include "Window.h"
#include "EditorCommandController.h"
#include "EditorSceneDocument.h"
#include "EditorShell.h"
#include "HierarchyPanelController.h"
#include "InspectorPanelController.h"
#include "LogOutputPanelController.h"
#include "MaterialPanelController.h"
#include "ProjectPanelController.h"
#include "SceneViewController.h"
#include "SceneViewRenderer.h"
#include "ScriptRuntimeSession.h"
#include "StartupScreenController.h"
#include "RecentProjectsStore.h"
#include "Win32Clipboard.h"
#include "Win32Cursor.h"
#include "Win32FileDialog.h"

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
        /// メインウィンドウのクライアント領域サイズ変更を Editor UI へ反映する。
        /// </summary>
        /// <param name="width">新しいクライアント領域の幅。</param>
        /// <param name="height">新しいクライアント領域の高さ。</param>
        void HandleWindowResized(std::uint32_t width, std::uint32_t height);

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
        /// 履歴状態から Scene Dirty 表示を同期する。
        /// </summary>
        void RefreshProjectDirtyState();

        /// <summary>
        /// Editor 再生開始前に Script をビルドする。
        /// </summary>
        /// <returns>再生開始前ビルドに成功した場合は true。</returns>
        [[nodiscard]] bool StartEditorPlay();

        /// <summary>
        /// Editor 再生開始前 Script ビルドをバックグラウンドで開始する。
        /// </summary>
        void StartEditorPlayBuild();

        /// <summary>
        /// バックグラウンド Script ビルドの完了を確認する。
        /// </summary>
        void PollEditorPlayBuild();

        /// <summary>
        /// ビルド完了後に Script Runtime を開始する。
        /// </summary>
        /// <param name="buildResult">Script ビルド結果。</param>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <returns>Runtime 開始に成功した場合は true。</returns>
        [[nodiscard]] bool BeginEditorPlayRuntime(
            const ScriptBuildResult& buildResult,
            const std::filesystem::path& projectRootDirectory);

        /// <summary>
        /// Editor 再生を停止または再開する。
        /// </summary>
        void ToggleEditorPlayPause();

        /// <summary>
        /// Editor 再生を終了する。
        /// </summary>
        void EndEditorPlay();

        /// <summary>
        /// Editor 再生操作ボタンの表示状態を更新する。
        /// </summary>
        void RefreshEditorPlayControls();

        /// <summary>
        /// Editor 再生操作ボタンの入力を処理する。
        /// </summary>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        void UpdateEditorPlayControls(const Core::InputSnapshot& inputSnapshot);

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
        /// メインウィンドウの Win32 ハンドルを取得する。
        /// </summary>
        /// <returns>メインウィンドウの HWND。</returns>
        [[nodiscard]] HWND GetMainWindowHandle() const;

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
        /// Material タブで指定 Material Asset を開く。
        /// </summary>
        /// <param name="materialAssetId">開く MaterialAssetId。</param>
        void OpenMaterialAsset(const Core::AssetId& materialAssetId);

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
        /// Scene 変更内容を必要に応じて履歴へ積み、Dirty 表示へ反映する。
        /// </summary>
        /// <param name="message">変更反映時の表示文面。</param>
        /// <param name="operationName">履歴へ記録する操作名。</param>
        /// <param name="pushHistory">履歴へ積む場合は true。</param>
        /// <returns>履歴追加または Dirty 反映を行った場合は true。</returns>
        bool PersistSceneChanges(
            const wchar_t* message,
            const std::string& operationName,
            bool pushHistory);

        /// <summary>
        /// 選択中 Sprite が Script 保存可能な `.sprite` を参照するようにする。
        /// </summary>
        /// <param name="requestedSpriteAssetId">Inspector 操作時点の SpriteAssetId。</param>
        /// <param name="sceneChanged">SpriteComponent 参照を変更した場合は true。</param>
        /// <returns>Script 操作に使う SpriteAssetId。準備できない場合は空。</returns>
        [[nodiscard]] std::optional<Core::AssetId> EnsureSelectedSpriteAssetFileForScript(
            const Core::AssetId& requestedSpriteAssetId,
            bool& sceneChanged);

        /// <summary>
        /// 現在のプロジェクトを最近使った一覧へ記録する。
        /// </summary>
        void RecordCurrentProject();

        /// <summary>
        /// エディタ操作ログを追加する。
        /// </summary>
        /// <param name="message">追加するログ文面。</param>
        void AppendEditorLog(const wchar_t* message);

        /// <summary>
        /// ゲーム実行ログを追加する。
        /// </summary>
        /// <param name="message">追加するログ文面。</param>
        void AppendGameLog(const std::wstring& message);

        /// <summary>
        /// ビルドログを追加する。
        /// </summary>
        /// <param name="message">追加するログ文面。</param>
        /// <param name="isError">エラーログとして表示する場合は true。</param>
        void AppendBuildLog(const std::wstring& message, bool isError = false);

    private:
        HINSTANCE m_hInstance = nullptr;
        Core::Window m_window{};
        Core::InputSystem m_inputSystem{};
        bool m_running = true;
        bool m_editorInitialized = false;
        bool m_projectDirty = false;
        HMENU m_projectMenu = nullptr;
        HMENU m_viewMenu = nullptr;
        Platform::Win32::Win32FileDialog m_fileDialog{};
        Platform::Win32::Win32Clipboard m_clipboard{};
        Platform::Win32::Win32Cursor m_cursor{};
        EditorShell m_editorShell{};
        StartupScreenController m_startupScreenController{};
        RecentProjectsStore m_recentProjectsStore{};
        EditorSceneDocument m_sceneDocument{};
        AssetsPanelController m_assetsPanelController{};
        HierarchyPanelController m_hierarchyPanelController{};
        InspectorPanelController m_inspectorPanelController{};
        MaterialPanelController m_materialPanelController{};
        LogOutputPanelController m_logOutputPanelController{};
        ProjectPanelController m_projectPanelController{};
        SceneViewController m_sceneViewController{};
        SceneViewRenderer m_sceneViewRenderer{};
        ScriptRuntimeSession m_scriptRuntimeSession{};
        EditorCommandController m_editorCommandController{};
        std::future<ScriptBuildResult> m_scriptBuildFuture{};
        std::optional<std::filesystem::path> m_scriptBuildProjectRootDirectory{};
        ButtonClickInputState m_editorPlayButtonInputState{};
        HWND m_buildAndPlayButton = nullptr;
        HWND m_pauseResumePlayButton = nullptr;
        HWND m_endPlayButton = nullptr;
        bool m_scriptBuildInProgress = false;
    };
}
