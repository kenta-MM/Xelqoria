#include <stdexcept>
#include <dxgi.h>
#include <wrl/client.h>
#include <dxgiformat.h>
#include <d3d11.h>
#include "D3D11GraphicsContext.h"
#include <array>
#include <Windows.h>

namespace Xelqoria::Backends::D3D11
{
    D3D11GraphicsContext::~D3D11GraphicsContext()
    {
        Shutdown();
    }

    bool D3D11GraphicsContext::Initialize(HWND hwnd, HINSTANCE /*hInstance*/, std::uint32_t width, ::uint32_t height)
    {
        m_hwnd = hwnd;
        m_width = width;
        m_height = height;

        if (!CreateDeviceAndSwapChain(hwnd, width, height))
        {
            return false;
        }

        if (!CreateRenderTarget())
        {
            return false;
        }

        return true;
    }

    void D3D11GraphicsContext::Shutdown()
    {
        ReleaseRenderTarget();

        if (m_swapChain) 
        {
            m_swapChain->SetFullscreenState(FALSE, nullptr);
        }

        m_swapChain.Reset();
        m_deviceContext.Reset();
        m_device.Reset();

        m_hwnd = nullptr;
        m_hInstance = nullptr;
        m_width = 0;
        m_height = 0;
    }


    void D3D11GraphicsContext::BeginFrame()
    {
        if (!m_deviceContext || !m_renderTargetView)
        {
            return;
        }

        constexpr std::array<float, 4> clearColor = {0.1f, 0.1f, 0.2f, 1.0f};

        m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
        m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor.data());
    }
    void D3D11GraphicsContext::EndFrame()
    {
        if (!m_swapChain)
        {
            return;
        }
     
        m_swapChain->Present(1, 0);
    }

    void D3D11GraphicsContext::Resize(std::uint32_t width, std::uint32_t height)
    {
        if (!m_swapChain || width == 0 || height == 0)
        {
            return;
        }

        m_width = width;
        m_height = height;

        ReleaseRenderTarget();

        m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        CreateRenderTarget();
    }

    bool D3D11GraphicsContext::CreateDeviceAndSwapChain(HWND hWnd, std::uint32_t width, std::uint32_t height)
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc{};
        swapChainDesc.BufferCount = 1;
        swapChainDesc.BufferDesc.Width = width;
        swapChainDesc.BufferDesc.Height = height;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hWnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT createDeviceFlags = 0;

#if defined(_DEBUG)
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        constexpr D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        D3D_FEATURE_LEVEL createdFeatureLevel{};

        const HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createDeviceFlags,
            featureLevels,
            static_cast<UINT>(std::size(featureLevels)),
            D3D11_SDK_VERSION,
            &swapChainDesc,
            m_swapChain.GetAddressOf(),
            m_device.GetAddressOf(),
            &createdFeatureLevel,
            m_deviceContext.GetAddressOf());

        return SUCCEEDED(hr);
    }

    bool D3D11GraphicsContext::CreateRenderTarget()
    {
        if (!m_swapChain || !m_device)
        {
            return false;
        }

        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        const HRESULT hr = m_swapChain->GetBuffer(
            0,
            __uuidof(ID3D11Texture2D),
            reinterpret_cast<void**>(backBuffer.GetAddressOf()));

        if (FAILED(hr))
        {
            return false;
        }

        const HRESULT rtvHr = m_device->CreateRenderTargetView(
            backBuffer.Get(),
            nullptr,
            m_renderTargetView.GetAddressOf());

        return SUCCEEDED(rtvHr);
    }

    void D3D11GraphicsContext::ReleaseRenderTarget()
    {
        if (m_deviceContext)
        {
            m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        }

        m_renderTargetView.Reset();
    }
}
