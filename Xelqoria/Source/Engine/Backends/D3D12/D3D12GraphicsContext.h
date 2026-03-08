#pragma once
#include <Windows.h>
#include <wrl/client.h>
#include <cstdint>
#include <array>

#include <d3d12.h>
#include <dxgi1_6.h>

#include "Engine/RHI/IGraphicsContext.h"

namespace Xelqoria::Backends::D3D12
{
    class D3D12GraphicsContext final : public RHI::IGraphicsContext
    {
    public:
        D3D12GraphicsContext() = default;
        ~D3D12GraphicsContext() override;

        D3D12GraphicsContext(const D3D12GraphicsContext&) = delete;
        D3D12GraphicsContext& operator=(const D3D12GraphicsContext&) = delete;

        bool Initialize(HWND hwnd, HINSTANCE hInstance, std::uint32_t width, std::uint32_t height) override;
        void Shutdown() override;

        void BeginFrame() override;
        void EndFrame() override;

        void Resize(std::uint32_t width, std::uint32_t height);

    private:
        static constexpr std::uint32_t FrameCount = 2;

    private:
        bool CreateFactory();
        bool CreateDevice();
        bool CreateCommandObjects();
        bool CreateSwapChain();
        bool CreateDescriptorHeaps();
        bool CreateRenderTargets();
        bool CreateFence();
        bool CreateViewportAndScissor();

        void ReleaseRenderTargets();

        bool WaitForGpu();
        bool MoveToNextFrame();
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

        std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FrameCount> m_renderTargets;
        std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, FrameCount> m_commandAllocators;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

        Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
        std::array<std::uint64_t, FrameCount> m_fenceValues = {};
        HANDLE m_fenceEvent = nullptr;

        std::uint32_t m_frameIndex = 0;
    };
}