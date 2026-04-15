#pragma once

#include <Windows.h>

#include "AssetsPanelController.h"
#include "Window.h"
#include "EditorCommandController.h"
#include "EditorSceneDocument.h"
#include "EditorShell.h"
#include "HierarchyPanelController.h"
#include "InspectorPanelController.h"
#include "SceneViewController.h"
#include "SceneViewRenderer.h"

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

    private:
        HINSTANCE m_hInstance = nullptr;
        Core::Window m_window{};
        bool m_running = true;
        EditorShell m_editorShell{};
        EditorSceneDocument m_sceneDocument{};
        AssetsPanelController m_assetsPanelController{};
        HierarchyPanelController m_hierarchyPanelController{};
        InspectorPanelController m_inspectorPanelController{};
        SceneViewController m_sceneViewController{};
        SceneViewRenderer m_sceneViewRenderer{};
        EditorCommandController m_editorCommandController{};
    };
}
