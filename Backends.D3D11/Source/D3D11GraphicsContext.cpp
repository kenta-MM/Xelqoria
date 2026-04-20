#include "D3D11GraphicsContext.h"

#include <array>
#include <memory>
#include <vector>

#include "D3D11Texture.h"
#include "D3D11TextureLoader.h"
#include <Windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgiformat.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <wrl/client.h>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <IGraphicsContext.h>
#include <ITexture.h>

namespace Xelqoria::Backends::D3D11
{
    D3D11GraphicsContext::~D3D11GraphicsContext()
    {
        Shutdown();
    }

    bool D3D11GraphicsContext::Initialize(HWND hwnd, HINSTANCE hInstance, std::uint32_t width, std::uint32_t height)
    {
        m_hwnd = hwnd;
        m_hInstance = hInstance;
        m_width = width;
        m_height = height;

        if (false == CreateDeviceAndSwapChain(hwnd, width, height))
        {
            return false;
        }

        if (false == CreateRenderTarget())
        {
            return false;
        }

        return m_spritePipeline.Initialize(m_device.Get());
    }

    void D3D11GraphicsContext::Shutdown()
    {
        m_spritePipeline.Shutdown();
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

        constexpr std::array<float, 4> clearColor = { 0.1f, 0.1f, 0.2f, 1.0f };

        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(m_width);
        viewport.Height = static_cast<float>(m_height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
        m_deviceContext->RSSetViewports(1, &viewport);
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

    std::shared_ptr<RHI::ITexture> D3D11GraphicsContext::CreateTextureFromFile(const std::wstring& filePath)
    {
        if (!m_device || filePath.empty())
        {
            return nullptr;
        }

        std::vector<std::uint8_t> pixels;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        if (false == D3D11TextureLoader::LoadRgbaPixelsFromFile(filePath, pixels, width, height))
        {
            return nullptr;
        }

        D3D11TextureCreateDesc desc{};
        desc.width = width;
        desc.height = height;
        desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.initialData = pixels.data();
        desc.initialDataRowPitch = width * 4u;

        auto texture = std::make_shared<D3D11Texture>();
        if (false == texture->Initialize(m_device.Get(), desc))
        {
            return nullptr;
        }

        return texture;
    }

    void D3D11GraphicsContext::BindTexture(std::uint32_t slot, RHI::ITexture* texture)
    {
        m_spritePipeline.BindTexture(m_deviceContext.Get(), slot, texture);
    }

    void D3D11GraphicsContext::SetQuadTransform(const RHI::QuadTransform2D& transform)
    {
        m_spritePipeline.SetQuadTransform(transform);
    }

    void D3D11GraphicsContext::Draw(std::uint32_t vertexCount, std::uint32_t startVertexLocation)
    {
        m_spritePipeline.Draw(m_deviceContext.Get(), vertexCount, startVertexLocation);
    }

    void D3D11GraphicsContext::DrawIndexed(std::uint32_t indexCount, std::uint32_t startIndexLocation, std::int32_t baseVertexLocation)
    {
        if (!m_deviceContext || indexCount == 0)
        {
            return;
        }

        m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_deviceContext->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
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
        const HRESULT deviceHr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createDeviceFlags,
            featureLevels,
            static_cast<UINT>(_countof(featureLevels)),
            D3D11_SDK_VERSION,
            m_device.GetAddressOf(),
            &createdFeatureLevel,
            m_deviceContext.GetAddressOf());
        if (FAILED(deviceHr))
        {
            return false;
        }

        Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
        const HRESULT factoryHr = CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()));
        if (FAILED(factoryHr))
        {
            return false;
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
        const HRESULT swapChainHr = factory->CreateSwapChainForHwnd(
            m_device.Get(),
            hWnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            swapChain.GetAddressOf());
        if (FAILED(swapChainHr))
        {
            return false;
        }

        factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
        return SUCCEEDED(swapChain.As(&m_swapChain));
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
