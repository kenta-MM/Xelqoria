#include "D3D12GraphicsContext.h"

#include <array>
#include <memory>
#include <vector>

#include "D3D12Texture.h"
#include "D3D12TextureLoader.h"
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgiformat.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>

namespace Xelqoria::Backends::D3D12
{
    namespace
    {
        constexpr std::array<float, 4> kClearColor = { 0.1f, 0.1f, 0.2f, 1.0f };
    }

    D3D12GraphicsContext::~D3D12GraphicsContext()
    {
        Shutdown();
    }

    bool D3D12GraphicsContext::Initialize(HWND hwnd, HINSTANCE hInstance, std::uint32_t width, std::uint32_t height)
    {
        m_hwnd = hwnd;
        m_hInstance = hInstance;
        m_width = width;
        m_height = height;

        if (false == CreateFactory()
            || false == CreateDevice()
            || false == CreateCommandObjects()
            || false == CreateSwapChain()
            || false == CreateDescriptorHeaps()
            || false == CreateRenderTargets()
            || false == CreateFence()
            || false == CreateViewportAndScissor())
        {
            return false;
        }

        if (false == m_spritePipeline.Initialize(m_device.Get()))
        {
            return false;
        }

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        return true;
    }

    void D3D12GraphicsContext::Shutdown()
    {
        WaitForGpu();
        m_spritePipeline.Shutdown();
        ReleaseRenderTargets();

        if (m_fenceEvent)
        {
            CloseHandle(m_fenceEvent);
            m_fenceEvent = nullptr;
        }

        m_commandList.Reset();
        for (auto& allocator : m_commandAllocators)
        {
            allocator.Reset();
        }

        m_rtvHeap.Reset();
        m_swapChain.Reset();
        m_commandQueue.Reset();
        m_fence.Reset();
        m_device.Reset();
        m_factory.Reset();

        m_hwnd = nullptr;
        m_hInstance = nullptr;
        m_width = 0;
        m_height = 0;
        m_frameIndex = 0;
        m_rtvDescriptorSize = 0;
        m_fenceValues = {};
    }

    void D3D12GraphicsContext::BeginFrame()
    {
        if (!m_device || !m_commandQueue || !m_swapChain)
        {
            return;
        }

        if (!ResetCommandList())
        {
            return;
        }

        auto* commandList = m_commandList.Get();
        auto* backBuffer = m_renderTargets[m_frameIndex].Get();

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = backBuffer;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        commandList->ResourceBarrier(1, &barrier);

        const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{
            m_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr +
            static_cast<SIZE_T>(m_frameIndex) * static_cast<SIZE_T>(m_rtvDescriptorSize)
        };

        commandList->RSSetViewports(1, &m_viewport);
        commandList->RSSetScissorRects(1, &m_scissorRect);
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
        commandList->ClearRenderTargetView(rtvHandle, kClearColor.data(), 0, nullptr);
    }

    void D3D12GraphicsContext::EndFrame()
    {
        if (!m_commandList || !m_swapChain || !m_commandQueue)
        {
            return;
        }

        auto* commandList = m_commandList.Get();
        auto* backBuffer = m_renderTargets[m_frameIndex].Get();

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = backBuffer;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        commandList->ResourceBarrier(1, &barrier);

        HRESULT hr = commandList->Close();
        if (FAILED(hr))
        {
            return;
        }

        ID3D12CommandList* commandLists[] = { commandList };
        m_commandQueue->ExecuteCommandLists(1, commandLists);

        hr = m_swapChain->Present(1, 0);
        if (FAILED(hr))
        {
            return;
        }

        MoveToNextFrame();
    }

    std::shared_ptr<RHI::ITexture> D3D12GraphicsContext::CreateTextureFromFile(const std::wstring& filePath)
    {
        if (!m_device || !m_commandQueue || !m_fence || filePath.empty())
        {
            return nullptr;
        }

        std::vector<std::uint8_t> pixels;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        if (false == D3D12TextureLoader::LoadRgbaPixelsFromFile(filePath, pixels, width, height))
        {
            return nullptr;
        }

        D3D12TextureCreateDesc desc{};
        desc.width = width;
        desc.height = height;
        desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;

        auto texture = std::make_shared<D3D12Texture>();
        if (false == texture->Initialize(m_device.Get(), desc))
        {
            return nullptr;
        }

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> uploadCommandAllocator;
        HRESULT hr = m_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(uploadCommandAllocator.GetAddressOf()));
        if (FAILED(hr))
        {
            return nullptr;
        }

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> uploadCommandList;
        hr = m_device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            uploadCommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(uploadCommandList.GetAddressOf()));
        if (FAILED(hr))
        {
            return nullptr;
        }

        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
        if (false == texture->UploadInitialData(
            m_device.Get(),
            uploadCommandList.Get(),
            pixels.data(),
            width * 4u,
            uploadBuffer))
        {
            return nullptr;
        }

        hr = uploadCommandList->Close();
        if (FAILED(hr))
        {
            return nullptr;
        }

        ID3D12CommandList* const commandLists[] = { uploadCommandList.Get() };
        m_commandQueue->ExecuteCommandLists(1, commandLists);

        if (false == WaitForGpu())
        {
            return nullptr;
        }

        return texture;
    }

    void D3D12GraphicsContext::BindTexture(std::uint32_t slot, RHI::ITexture* texture)
    {
        m_spritePipeline.BindTexture(m_device.Get(), slot, texture);
    }

    void D3D12GraphicsContext::SetQuadTransform(const RHI::QuadTransform2D& transform)
    {
        m_spritePipeline.SetQuadTransform(transform);
    }

    void D3D12GraphicsContext::Draw(std::uint32_t vertexCount, std::uint32_t startVertexLocation)
    {
        m_spritePipeline.Draw(m_commandList.Get(), vertexCount, startVertexLocation);
    }

    void D3D12GraphicsContext::DrawIndexed(std::uint32_t indexCount, std::uint32_t startIndexLocation, std::int32_t baseVertexLocation)
    {
        if (!m_commandList || indexCount == 0)
        {
            return;
        }

        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
    }

    void D3D12GraphicsContext::Resize(std::uint32_t width, std::uint32_t height)
    {
        if (!m_swapChain || width == 0 || height == 0)
        {
            return;
        }

        WaitForGpu();
        ReleaseRenderTargets();

        m_width = width;
        m_height = height;

        const HRESULT hr = m_swapChain->ResizeBuffers(
            FrameCount,
            m_width,
            m_height,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            0);
        if (FAILED(hr))
        {
            return;
        }

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        if (false == CreateRenderTargets())
        {
            return;
        }

        CreateViewportAndScissor();
    }

    bool D3D12GraphicsContext::CreateFactory()
    {
        UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
        {
            Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();
                dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }
#endif

        const HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory));
        return SUCCEEDED(hr);
    }

    bool D3D12GraphicsContext::CreateDevice()
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

        for (UINT adapterIndex = 0; m_factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc{};
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
            {
                return true;
            }
        }

        const HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
        return SUCCEEDED(hr);
    }

    bool D3D12GraphicsContext::CreateCommandObjects()
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

        HRESULT hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
        if (FAILED(hr))
        {
            return false;
        }

        for (std::uint32_t i = 0; i < FrameCount; ++i)
        {
            hr = m_device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&m_commandAllocators[i]));
            if (FAILED(hr))
            {
                return false;
            }
        }

        hr = m_device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocators[0].Get(),
            nullptr,
            IID_PPV_ARGS(&m_commandList));
        if (FAILED(hr))
        {
            return false;
        }

        hr = m_commandList->Close();
        return SUCCEEDED(hr);
    }

    bool D3D12GraphicsContext::CreateSwapChain()
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.Width = m_width;
        swapChainDesc.Height = m_height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = FrameCount;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
        HRESULT hr = m_factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            m_hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1);
        if (FAILED(hr))
        {
            return false;
        }

        hr = m_factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);
        if (FAILED(hr))
        {
            return false;
        }

        hr = swapChain1.As(&m_swapChain);
        return SUCCEEDED(hr);
    }

    bool D3D12GraphicsContext::CreateDescriptorHeaps()
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

        const HRESULT hr = m_device->CreateDescriptorHeap(
            &rtvHeapDesc,
            IID_PPV_ARGS(&m_rtvHeap));
        if (FAILED(hr))
        {
            return false;
        }

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        return true;
    }

    bool D3D12GraphicsContext::CreateRenderTargets()
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

        for (std::uint32_t i = 0; i < FrameCount; ++i)
        {
            HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
            if (FAILED(hr))
            {
                return false;
            }

            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += static_cast<SIZE_T>(m_rtvDescriptorSize);
        }

        return true;
    }

    bool D3D12GraphicsContext::CreateFence()
    {
        const HRESULT hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
        if (FAILED(hr))
        {
            return false;
        }

        m_fenceValues.fill(0);
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        return m_fenceEvent != nullptr;
    }

    bool D3D12GraphicsContext::CreateViewportAndScissor()
    {
        m_viewport.TopLeftX = 0.0f;
        m_viewport.TopLeftY = 0.0f;
        m_viewport.Width = static_cast<float>(m_width);
        m_viewport.Height = static_cast<float>(m_height);
        m_viewport.MinDepth = D3D12_MIN_DEPTH;
        m_viewport.MaxDepth = D3D12_MAX_DEPTH;

        m_scissorRect.left = 0;
        m_scissorRect.top = 0;
        m_scissorRect.right = static_cast<LONG>(m_width);
        m_scissorRect.bottom = static_cast<LONG>(m_height);

        return true;
    }

    void D3D12GraphicsContext::ReleaseRenderTargets()
    {
        for (auto& renderTarget : m_renderTargets)
        {
            renderTarget.Reset();
        }
    }

    bool D3D12GraphicsContext::WaitForGpu()
    {
        if (!m_commandQueue || !m_fence || !m_fenceEvent)
        {
            return false;
        }

        const std::uint64_t fenceValue = ++m_fenceValues[m_frameIndex];
        HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fenceValue);
        if (FAILED(hr))
        {
            return false;
        }

        hr = m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
        if (FAILED(hr))
        {
            return false;
        }

        WaitForSingleObject(m_fenceEvent, INFINITE);
        return true;
    }

    bool D3D12GraphicsContext::MoveToNextFrame()
    {
        if (!m_commandQueue || !m_fence || !m_swapChain)
        {
            return false;
        }

        const std::uint64_t currentFenceValue = ++m_fenceValues[m_frameIndex];
        HRESULT hr = m_commandQueue->Signal(m_fence.Get(), currentFenceValue);
        if (FAILED(hr))
        {
            return false;
        }

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
        {
            hr = m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
            if (FAILED(hr))
            {
                return false;
            }

            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        m_fenceValues[m_frameIndex] = currentFenceValue;
        return true;
    }

    bool D3D12GraphicsContext::ResetCommandList()
    {
        HRESULT hr = m_commandAllocators[m_frameIndex]->Reset();
        if (FAILED(hr))
        {
            return false;
        }

        hr = m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr);
        return SUCCEEDED(hr);
    }
}
