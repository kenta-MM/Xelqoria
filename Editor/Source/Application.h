#pragma once

#include <Windows.h>
#include <memory>

#include "IGraphicsContext.h"
#include "Window.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Editor ターゲットの最小実行ループを管理する。
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
        /// <summary>
        /// Windows アプリケーションインスタンスを保持する。
        /// </summary>
        HINSTANCE m_hInstance = nullptr;

        /// <summary>
        /// Editor のメインウィンドウを保持する。
        /// </summary>
        Core::Window m_window{};

        /// <summary>
        /// 将来の SceneView 実装に備える描画コンテキストを保持する。
        /// </summary>
        std::unique_ptr<RHI::IGraphicsContext> m_graphics;

        /// <summary>
        /// メインループの継続状態を表す。
        /// </summary>
        bool m_running = true;
    };
}
