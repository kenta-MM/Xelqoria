#pragma once

#include <Windows.h>
#include <cstdint>
#include <memory>
#include <string>

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include "Engine/RHI/IGraphicsContext.h"
#include "Engine/RHI/IVertexBuffer.h"

namespace Xelqoria::Backends::D3D11
{
    /// <summary>
    /// Direct3D 11 バックエンドのグラフィックスコンテキスト実装。
    /// </summary>
    class D3D11GraphicsContext final : public RHI::IGraphicsContext
    {
    public:
        D3D11GraphicsContext() = default;
        ~D3D11GraphicsContext() override;

        D3D11GraphicsContext(const D3D11GraphicsContext&) = delete;
        D3D11GraphicsContext& operator=(const D3D11GraphicsContext&) = delete;

        /// <inheritdoc/>
        bool Initialize(HWND hwnd, HINSTANCE hInstance, std::uint32_t width, std::uint32_t height) override;
        /// <inheritdoc/>
        void Shutdown() override;

        /// <inheritdoc/>
        void BeginFrame() override;
        /// <inheritdoc/>
        void EndFrame() override;

        /// <inheritdoc/>
        std::shared_ptr<RHI::ITexture> CreateTextureFromFile(const std::wstring& filePath) override;
        /// <inheritdoc/>
        void BindTexture(std::uint32_t slot, RHI::ITexture* texture) override;

        /// <inheritdoc/>
        void Draw(std::uint32_t vertexCount, std::uint32_t startVertexLocation = 0) override;
        /// <inheritdoc/>
        void DrawIndexed(std::uint32_t indexCount, std::uint32_t startIndexLocation = 0, std::int32_t baseVertexLocation = 0) override;

        /// <inheritdoc/>
        void Resize(std::uint32_t width, std::uint32_t height) override;

    private:
        /// <summary>
        /// D3D11 デバイスとスワップチェーンを生成する。
        /// </summary>
        bool CreateDeviceAndSwapChain(HWND hWnd, std::uint32_t width, std::uint32_t height);
        /// <summary>
        /// レンダーターゲットビューを生成する。
        /// </summary>
        bool CreateRenderTarget();
        /// <summary>
        /// レンダーターゲット関連リソースを解放する。
        /// </summary>
        void ReleaseRenderTarget();

        /// <summary>
        /// スプライト描画用パイプラインを生成する。
        /// </summary>
        bool CreateSpritePipeline();
        /// <summary>
        /// スプライト描画用ジオメトリを生成する。
        /// </summary>
        bool CreateSpriteGeometry();
        /// <summary>
        /// スプライト描画関連リソースを解放する。
        /// </summary>
        void ReleaseSpriteResources();

    private:
        HWND m_hwnd = nullptr;
        HINSTANCE m_hInstance = nullptr;
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;

        Microsoft::WRL::ComPtr<ID3D11Device> m_device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;
        Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;

        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_spriteVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_spritePixelShader;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_spriteInputLayout;
        std::shared_ptr<RHI::IVertexBuffer> m_spriteVertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_spriteSamplerState;
    };
}
