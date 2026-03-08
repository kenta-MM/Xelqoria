#pragma once
#include <Windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi.h>

#include "Engine/RHI/IGraphicsContext.h"

namespace Xelqoria::Backends::D3D11
{
    /// <summary>
    /// Direct3D11 を使用したグラフィックスコンテキストの実装クラス。
    /// </summary>
    /// <remarks>
    /// RHI::IGraphicsContext の D3D11 バックエンド実装。
    /// デバイス・デバイスコンテキスト・スワップチェーンなどの
    /// Direct3D11 の主要リソースを管理し、フレーム描画の開始と終了を提供する。
    /// </remarks>
    class D3D11GraphicsContext final : public RHI::IGraphicsContext
    {
    public:

        /// <summary>
        /// D3D11GraphicsContext を生成する。
        /// </summary>
        D3D11GraphicsContext() = default;

        /// <summary>
        /// デストラクタ。
        /// Direct3D リソースの解放を行う。
        /// </summary>
        ~D3D11GraphicsContext() override;

        /// <summary>
        /// コピーコンストラクタは禁止。
        /// </summary>
        D3D11GraphicsContext(const D3D11GraphicsContext&) = delete;

        /// <summary>
        /// コピー代入は禁止。
        /// </summary>
        D3D11GraphicsContext& operator=(const D3D11GraphicsContext&) = delete;

        /// <summary>
        /// グラフィックスコンテキストを初期化する。
        /// </summary>
        /// <param name="hwnd">描画対象となるウィンドウハンドル</param>
        /// <param name="hInstance">アプリケーションインスタンス</param>
        /// <param name="width">描画領域の幅</param>
        /// <param name="height">描画領域の高さ</param>
        /// <returns>初期化に成功した場合 true</returns>
        bool Initialize(HWND hwnd, HINSTANCE hInstance, std::uint32_t width, std::uint32_t height) override;

        /// <summary>
        /// グラフィックスコンテキストを終了し、リソースを解放する。
        /// </summary>
        void Shutdown() override;

        /// <summary>
        /// フレーム描画を開始する。
        /// </summary>
        /// <remarks>
        /// レンダーターゲットの設定やクリア処理などを行う。
        /// </remarks>
        void BeginFrame() override;

        /// <summary>
        /// フレーム描画を終了する。
        /// </summary>
        /// <remarks>
        /// スワップチェーンの Present を呼び出し、
        /// バックバッファを画面へ表示する。
        /// </remarks>
        void EndFrame() override;

        /// <summary>
        /// ウィンドウサイズ変更時にレンダリングターゲットを再生成する。
        /// </summary>
        /// <param name="width">新しい幅</param>
        /// <param name="height">新しい高さ</param>
        void Resize(std::uint32_t width, std::uint32_t height);

    private:

        /// <summary>
        /// Direct3D デバイスとスワップチェーンを作成する。
        /// </summary>
        /// <param name="hWnd">描画対象ウィンドウ</param>
        /// <param name="width">バックバッファの幅</param>
        /// <param name="height">バックバッファの高さ</param>
        /// <returns>作成に成功した場合 true</returns>
        bool CreateDeviceAndSwapChain(HWND hWnd, std::uint32_t width, std::uint32_t height);

        /// <summary>
        /// レンダーターゲットビューを作成する。
        /// </summary>
        /// <returns>作成に成功した場合 true</returns>
        bool CreateRenderTarget();

        /// <summary>
        /// レンダーターゲットビューを解放する。
        /// </summary>
        void ReleaseRenderTarget();

    private:

        /// <summary>
        /// 描画対象ウィンドウハンドル。
        /// </summary>
        HWND m_hwnd = nullptr;

        /// <summary>
        /// アプリケーションインスタンス。
        /// </summary>
        HINSTANCE m_hInstance = nullptr;

        /// <summary>
        /// 現在の描画幅。
        /// </summary>
        std::uint32_t m_width = 0;

        /// <summary>
        /// 現在の描画高さ。
        /// </summary>
        std::uint32_t m_height = 0;

        /// <summary>
        /// Direct3D11 デバイス。
        /// GPU リソースの生成などを担当する。
        /// </summary>
        Microsoft::WRL::ComPtr<ID3D11Device> m_device;

        /// <summary>
        /// Direct3D11 デバイスコンテキスト。
        /// 描画コマンドの実行を担当する。
        /// </summary>
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;

        /// <summary>
        /// スワップチェーン。
        /// バックバッファとフロントバッファの交換を管理する。
        /// </summary>
        Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;

        /// <summary>
        /// レンダーターゲットビュー。
        /// バックバッファを描画ターゲットとして使用するためのビュー。
        /// </summary>
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    };
}