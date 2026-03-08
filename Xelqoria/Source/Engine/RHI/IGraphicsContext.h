#pragma once
#include <Windows.h>
#include <cstdint>

namespace Xelqoria::RHI
{
    /**
     * @brief グラフィックスコンテキストのインターフェース
     *
     * 各グラフィックスAPI（Direct3D11 / Direct3D12 / Vulkan など）の
     * 実装が共通で利用する描画コンテキストの抽象インターフェース。
     *
     * レンダリングシステムはこのインターフェースを介して
     * 初期化、フレーム開始・終了、リサイズ処理などを実行する。
     */
    class IGraphicsContext
    {
    public:

        /**
         * @brief デストラクタ
         *
         * 派生クラスの安全な破棄のために仮想デストラクタとして定義する。
         */
        virtual ~IGraphicsContext() = default;
        

        /**
         * @brief グラフィックスコンテキストを初期化する
         *
         * 指定されたウィンドウハンドルに対して描画を行うための
         * デバイスやスワップチェーンなどのグラフィックスリソースを初期化する。
         *
         * @param hWnd 描画対象となるウィンドウハンドル
         * @param hInstance アプリケーションインスタンスハンドル
         * @param width 描画領域の幅
         * @param height 描画領域の高さ
         *
         * @return 初期化に成功した場合 true
         */
        virtual bool Initialize(HWND hWnd, HINSTANCE hInstance, std::uint32_t width, std::uint32_t height) = 0;

        /**
         * @brief グラフィックスコンテキストを終了する
         *
         * 作成したデバイス、スワップチェーン、レンダーターゲットなど
         * すべてのグラフィックスリソースを解放する。
         */
        virtual void Shutdown() = 0;

        /**
         * @brief フレーム描画を開始する
         *
         * レンダーターゲットの設定やクリア処理など、
         * フレーム描画開始時に必要な処理を行う。
         */
        virtual void BeginFrame() = 0;

        /**
         * @brief フレーム描画を終了する
         *
         * 描画結果をスワップチェーンに提示し、
         * 画面へ表示する処理を行う。
         */
        virtual void EndFrame() = 0;

        /**
         * @brief 描画サイズを変更する
         *
         * ウィンドウサイズ変更時などに呼び出され、
         * スワップチェーンやレンダーターゲットを再作成する。
         *
         * @param width 新しい描画幅
         * @param height 新しい描画高さ
         */
        virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
    };
}
