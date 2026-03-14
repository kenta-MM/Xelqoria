#pragma once

#include <Windows.h>
#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "IGraphicsContext.h"
#include "IVertexBuffer.h"

namespace Xelqoria::Backends::D3D12
{
    /// <summary>
    /// Direct3D 12 バックエンドのグラフィックスコンテキスト実装。
    /// </summary>
    class D3D12GraphicsContext final : public RHI::IGraphicsContext
    {
    public:
        D3D12GraphicsContext() = default;
        ~D3D12GraphicsContext() override;

        D3D12GraphicsContext(const D3D12GraphicsContext&) = delete;
        D3D12GraphicsContext& operator=(const D3D12GraphicsContext&) = delete;

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
        static constexpr std::uint32_t FrameCount = 2;

    private:
        /// <summary>DXGI ファクトリを生成する。</summary>
        bool CreateFactory();
        /// <summary>D3D12 デバイスを生成する。</summary>
        bool CreateDevice();
        /// <summary>コマンドキューとアロケータを生成する。</summary>
        bool CreateCommandObjects();
        /// <summary>スワップチェーンを生成する。</summary>
        bool CreateSwapChain();
        /// <summary>ディスクリプタヒープを生成する。</summary>
        bool CreateDescriptorHeaps();
        /// <summary>レンダーターゲットを生成する。</summary>
        bool CreateRenderTargets();
        /// <summary>フェンス同期オブジェクトを生成する。</summary>
        bool CreateFence();
        /// <summary>ビューポートとシザー矩形を更新する。</summary>
        bool CreateViewportAndScissor();

        /// <summary>スプライト描画用パイプラインを生成する。</summary>
        bool CreateSpritePipeline();
        /// <summary>スプライト描画用ジオメトリを生成する。</summary>
        bool CreateSpriteGeometry();

        /// <summary>レンダーターゲット関連リソースを解放する。</summary>
        void ReleaseRenderTargets();
        /// <summary>スプライト描画関連リソースを解放する。</summary>
        void ReleaseSpriteResources();

        /// <summary>GPU 完了を待機する。</summary>
        bool WaitForGpu();
        /// <summary>フレームインデックスを進めて同期する。</summary>
        bool MoveToNextFrame();
        /// <summary>コマンドリストをリセットして記録可能にする。</summary>
        bool ResetCommandList();

    private:
        HWND m_hwnd = nullptr;
        HINSTANCE m_hInstance = nullptr;
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;

        D3D12_VIEWPORT m_viewport{};
        D3D12_RECT m_scissorRect{};

        Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
        Microsoft::WRL::ComPtr<ID3D12Device> m_device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        std::uint32_t m_rtvDescriptorSize = 0;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
        std::uint32_t m_srvDescriptorSize = 0;

        std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FrameCount> m_renderTargets;
        std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, FrameCount> m_commandAllocators;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_spriteRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_spritePipelineState;
        std::shared_ptr<RHI::IVertexBuffer> m_spriteVertexBuffer;

        D3D12_GPU_DESCRIPTOR_HANDLE m_boundTextureSrvGpu{};
        bool m_hasBoundTexture = false;

        Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
        std::array<std::uint64_t, FrameCount> m_fenceValues = {};
        HANDLE m_fenceEvent = nullptr;

        std::uint32_t m_frameIndex = 0;
    };
}
