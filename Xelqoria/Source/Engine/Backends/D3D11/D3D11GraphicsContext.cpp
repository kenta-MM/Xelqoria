#include "D3D11GraphicsContext.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <vector>

#include <d3dcompiler.h>
#include <wincodec.h>

#include "Engine/Backends/D3D11/D3D11Texture.h"
#include "Engine/Backends/D3D11/D3D11VertexBuffer.h"
#include <wrl/client.h>
#include <Windows.h>
#include <cstdint>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace Xelqoria::Backends::D3D11
{
    namespace
    {
        struct SpriteVertex
        {
            float position[3];
            float uv[2];
        };

        bool LoadRgbaPixelsFromFileWIC(const std::wstring& filePath, std::vector<std::uint8_t>& outPixels, std::uint32_t& outWidth, std::uint32_t& outHeight)
        {
            outPixels.clear();
            outWidth = 0;
            outHeight = 0;

            const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
            {
                return false;
            }

            // RAII
            struct CoUninitializeGuard
            {
                bool enabled = false;
                ~CoUninitializeGuard()
                {
                    if (enabled)
                    {
                        CoUninitialize();
                    }
                }
            } guard{ SUCCEEDED(hr) };

            Microsoft::WRL::ComPtr<IWICImagingFactory> imagingFactory;
            HRESULT localHr = CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(imagingFactory.GetAddressOf()));

            if (FAILED(localHr))
            {
                return false;
            }

            Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
            localHr = imagingFactory->CreateDecoderFromFilename(
                filePath.c_str(),
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,
                decoder.GetAddressOf());

            if (FAILED(localHr))
            {
                return false;
            }

            Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
            localHr = decoder->GetFrame(0, frame.GetAddressOf());
            if (FAILED(localHr))
            {
                return false;
            }

            UINT width = 0;
            UINT height = 0;
            localHr = frame->GetSize(&width, &height);
            if (FAILED(localHr) || width == 0 || height == 0)
            {
                return false;
            }

            Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
            localHr = imagingFactory->CreateFormatConverter(converter.GetAddressOf());
            if (FAILED(localHr))
            {
                return false;
            }

            localHr = converter->Initialize(
                frame.Get(),
                GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom);

            if (FAILED(localHr))
            {
                return false;
            }

            const std::uint32_t rowPitch = static_cast<std::uint32_t>(width) * 4u;
            outPixels.resize(static_cast<std::size_t>(rowPitch) * static_cast<std::size_t>(height));

            localHr = converter->CopyPixels(
                nullptr,
                rowPitch,
                static_cast<UINT>(outPixels.size()),
                outPixels.data());

            if (FAILED(localHr))
            {
                outPixels.clear();
                return false;
            }

            outWidth = static_cast<std::uint32_t>(width);
            outHeight = static_cast<std::uint32_t>(height);
            return true;
        }
        bool CompileShader(
            const char* source,
            const char* entryPoint,
            const char* target,
            Microsoft::WRL::ComPtr<ID3DBlob>& outBlob)
        {
            outBlob.Reset();

            UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
            compileFlags |= D3DCOMPILE_DEBUG;
            compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

            Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
            Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

            const HRESULT hr = D3DCompile(
                source,
                std::strlen(source),
                nullptr,
                nullptr,
                nullptr,
                entryPoint,
                target,
                compileFlags,
                0,
                shaderBlob.GetAddressOf(),
                errorBlob.GetAddressOf());

            if (FAILED(hr))
            {
                return false;
            }

            outBlob = shaderBlob;
            return true;
        }
    }

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

        if (!CreateDeviceAndSwapChain(hwnd, width, height))
        {
            return false;
        }

        if (!CreateRenderTarget())
        {
            return false;
        }

        if (!CreateSpritePipeline())
        {
            return false;
        }

        if (!CreateSpriteGeometry())
        {
            return false;
        }

        return true;
    }

    void D3D11GraphicsContext::Shutdown()
    {
        ReleaseSpriteResources();
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

        if (!LoadRgbaPixelsFromFileWIC(filePath, pixels, width, height))
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
        if (!texture->Initialize(m_device.Get(), desc))
        {
            return nullptr;
        }

        return texture;
    }

    void D3D11GraphicsContext::BindTexture(std::uint32_t slot, RHI::ITexture* texture)
    {
        if (!m_deviceContext)
        {
            return;
        }

        ID3D11ShaderResourceView* shaderResourceView = nullptr;

        if (texture)
        {
            auto* d3d11Texture = dynamic_cast<D3D11Texture*>(texture);
            if (d3d11Texture)
            {
                shaderResourceView = d3d11Texture->GetShaderResourceView();
            }
        }

        m_deviceContext->PSSetShaderResources(slot, 1, &shaderResourceView);
    }

    void D3D11GraphicsContext::Draw(std::uint32_t vertexCount, std::uint32_t startVertexLocation)
    {
        if (!m_deviceContext || !m_spriteVertexBuffer || !m_spriteInputLayout || !m_spriteVertexShader || !m_spritePixelShader)
        {
            return;
        }

        auto* d3d11VertexBuffer = dynamic_cast<D3D11VertexBuffer*>(m_spriteVertexBuffer.get());
        if (!d3d11VertexBuffer)
        {
            return;
        }

        ID3D11Buffer* nativeBuffer = d3d11VertexBuffer->GetBuffer();
        if (!nativeBuffer)
        {
            return;
        }

        const UINT stride = d3d11VertexBuffer->GetStrideSize();
        const UINT offset = 0;

        m_deviceContext->IASetInputLayout(m_spriteInputLayout.Get());
        m_deviceContext->IASetVertexBuffers(0, 1, &nativeBuffer, &stride, &offset);
        m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_deviceContext->VSSetShader(m_spriteVertexShader.Get(), nullptr, 0);
        m_deviceContext->PSSetShader(m_spritePixelShader.Get(), nullptr, 0);

        if (m_spriteSamplerState)
        {
            ID3D11SamplerState* sampler = m_spriteSamplerState.Get();
            m_deviceContext->PSSetSamplers(0, 1, &sampler);
        }

        const std::uint32_t drawVertexCount = (std::min)(vertexCount, static_cast<std::uint32_t>(6));
        if (drawVertexCount == 0)
        {
            return;
        }

        m_deviceContext->Draw(drawVertexCount, startVertexLocation);
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
            static_cast<UINT>(_countof(featureLevels)),
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

    bool D3D11GraphicsContext::CreateSpritePipeline()
    {
        if (!m_device)
        {
            return false;
        }

        static const char* kVertexShaderSource = R"(
struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput MainVS(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0f);
    output.uv = input.uv;
    return output;
}
)";

        static const char* kPixelShaderSource = R"(
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 MainPS(PSInput input) : SV_TARGET
{
    return gTexture.Sample(gSampler, input.uv);
}
)";

        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
        if (!CompileShader(kVertexShaderSource, "MainVS", "vs_5_0", vertexShaderBlob))
        {
            return false;
        }

        Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
        if (!CompileShader(kPixelShaderSource, "MainPS", "ps_5_0", pixelShaderBlob))
        {
            return false;
        }

        HRESULT hr = m_device->CreateVertexShader(
            vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            nullptr,
            m_spriteVertexShader.GetAddressOf());

        if (FAILED(hr))
        {
            return false;
        }

        hr = m_device->CreatePixelShader(
            pixelShaderBlob->GetBufferPointer(),
            pixelShaderBlob->GetBufferSize(),
            nullptr,
            m_spritePixelShader.GetAddressOf());

        if (FAILED(hr))
        {
            return false;
        }

        D3D11_INPUT_ELEMENT_DESC inputElements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        hr = m_device->CreateInputLayout(
            inputElements,
            static_cast<UINT>(_countof(inputElements)),
            vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            m_spriteInputLayout.GetAddressOf());

        if (FAILED(hr))
        {
            return false;
        }

        D3D11_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        hr = m_device->CreateSamplerState(&samplerDesc, m_spriteSamplerState.GetAddressOf());
        return SUCCEEDED(hr);
    }

    bool D3D11GraphicsContext::CreateSpriteGeometry()
    {
        if (!m_device)
        {
            return false;
        }

        constexpr SpriteVertex vertices[6] =
        {
            { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f } },
            { {  0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f } },
            { {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f } },
            { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f } },
            { {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f } },
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f } }
        };

        auto vertexBuffer = std::make_shared<D3D11VertexBuffer>();
        const bool initialized = vertexBuffer->Initialize(
            m_device.Get(),
            vertices,
            static_cast<std::uint32_t>(_countof(vertices)),
            static_cast<std::uint32_t>(sizeof(SpriteVertex)));

        if (!initialized)
        {
            return false;
        }

        m_spriteVertexBuffer = vertexBuffer;
        return true;
    }

    void D3D11GraphicsContext::ReleaseSpriteResources()
    {
        m_spriteSamplerState.Reset();
        m_spriteVertexBuffer.reset();
        m_spriteInputLayout.Reset();
        m_spritePixelShader.Reset();
        m_spriteVertexShader.Reset();
    }
}










