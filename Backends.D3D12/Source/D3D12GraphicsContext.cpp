#include "D3D12GraphicsContext.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <d3dcompiler.h>
#include <wincodec.h>

#include "D3D12Texture.h"
#include "D3D12VertexBuffer.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace Xelqoria::Backends::D3D12
{
    namespace
    {
        struct SpriteVertex
        {
            float position[3];
            float uv[2];
        };

        constexpr std::array<float, 4> kClearColor = { 0.1f, 0.1f, 0.2f, 1.0f };

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

        bool CompileShaderSource(const char* source, const char* entryPoint, const char* target, Microsoft::WRL::ComPtr<ID3DBlob>& outBlob)
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

        if (!CreateFactory() || !CreateDevice() || !CreateCommandObjects() || !CreateSwapChain() || !CreateDescriptorHeaps() || !CreateRenderTargets() || !CreateFence() || !CreateViewportAndScissor() || !CreateSpritePipeline() || !CreateSpriteGeometry())
        {
            return false;
        }

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        return true;
    }

    void D3D12GraphicsContext::Shutdown()
    {
        WaitForGpu();
        ReleaseSpriteResources();
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

        m_srvHeap.Reset();
        m_rtvHeap.Reset();
        m_swapChain.Reset();
        m_commandQueue.Reset();
        m_fence.Reset();
        m_device.Reset();
        m_factory.Reset();

        m_frameIndex = 0;
        m_rtvDescriptorSize = 0;
        m_srvDescriptorSize = 0;
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
        if (!LoadRgbaPixelsFromFileWIC(filePath, pixels, width, height))
        {
            return nullptr;
        }

        D3D12TextureCreateDesc desc{};
        desc.width = width;
        desc.height = height;
        desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;

        auto texture = std::make_shared<D3D12Texture>();
        if (!texture->Initialize(m_device.Get(), desc))
        {
            return nullptr;
        }

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> uploadCommandAllocator;
        HRESULT hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(uploadCommandAllocator.GetAddressOf()));
        if (FAILED(hr))
        {
            return nullptr;
        }

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> uploadCommandList;
        hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, uploadCommandAllocator.Get(), nullptr, IID_PPV_ARGS(uploadCommandList.GetAddressOf()));
        if (FAILED(hr))
        {
            return nullptr;
        }

        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
        if (!texture->UploadInitialData(m_device.Get(), uploadCommandList.Get(), pixels.data(), width * 4u, uploadBuffer))
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

        if (!WaitForGpu())
        {
            return nullptr;
        }

        return texture;
    }

    void D3D12GraphicsContext::BindTexture(std::uint32_t slot, RHI::ITexture* texture)
    {
        if (slot != 0 || !m_device || !m_srvHeap || texture == nullptr)
        {
            m_hasBoundTexture = false;
            return;
        }

        auto* d3d12Texture = dynamic_cast<D3D12Texture*>(texture);
        if (!d3d12Texture)
        {
            m_hasBoundTexture = false;
            return;
        }

        ID3D12Resource* nativeTexture = d3d12Texture->GetNativeTexture();
        if (!nativeTexture)
        {
            m_hasBoundTexture = false;
            return;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = nativeTexture->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        m_device->CreateShaderResourceView(nativeTexture, &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
        m_boundTextureSrvGpu = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
        m_hasBoundTexture = true;
    }

    void D3D12GraphicsContext::SetQuadTransform(const RHI::QuadTransform2D& transform)
    {
        m_quadTransform = transform;
    }

    void D3D12GraphicsContext::Draw(std::uint32_t vertexCount, std::uint32_t startVertexLocation)
    {
        if (!m_commandList || !m_spriteRootSignature || !m_spritePipelineState || !m_spriteVertexBuffer || !m_hasBoundTexture)
        {
            return;
        }

        auto* d3d12VertexBuffer = dynamic_cast<D3D12VertexBuffer*>(m_spriteVertexBuffer.get());
        if (!d3d12VertexBuffer)
        {
            return;
        }

        const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView = d3d12VertexBuffer->GetView();
        if (vertexBufferView.BufferLocation == 0 || vertexBufferView.SizeInBytes == 0 || vertexBufferView.StrideInBytes == 0)
        {
            return;
        }

        auto* commandList = m_commandList.Get();
        ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvHeap.Get() };
        commandList->SetDescriptorHeaps(1, descriptorHeaps);
        commandList->SetGraphicsRootSignature(m_spriteRootSignature.Get());
        commandList->SetPipelineState(m_spritePipelineState.Get());
        const float quadTransformData[4] =
        {
            m_quadTransform.scaleX,
            m_quadTransform.scaleY,
            m_quadTransform.translateX,
            m_quadTransform.translateY
        };
        commandList->SetGraphicsRoot32BitConstants(0, 4, quadTransformData, 0);
        commandList->SetGraphicsRootDescriptorTable(1, m_boundTextureSrvGpu);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

        const std::uint32_t drawVertexCount = (std::min)(vertexCount, static_cast<std::uint32_t>(6));
        if (drawVertexCount == 0)
        {
            return;
        }

        commandList->DrawInstanced(drawVertexCount, 1, startVertexLocation, 0);
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

        HRESULT hr = m_swapChain->ResizeBuffers(FrameCount, m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        if (FAILED(hr))
        {
            return;
        }

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        if (!CreateRenderTargets())
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
            hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i]));
            if (FAILED(hr))
            {
                return false;
            }
        }

        hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_commandList));
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
        HRESULT hr = m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
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

        HRESULT hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
        if (FAILED(hr))
        {
            return false;
        }

        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
        srvHeapDesc.NumDescriptors = 1;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        hr = m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));
        if (FAILED(hr))
        {
            return false;
        }

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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

    bool D3D12GraphicsContext::CreateSpritePipeline()
    {
        if (!m_device)
        {
            return false;
        }

        static const char* kVertexShaderSource = R"(
cbuffer SpriteTransformBuffer : register(b0)
{
    float2 gScale;
    float2 gTranslate;
};
struct VSInput { float3 position : POSITION; float2 uv : TEXCOORD0; };
struct VSOutput { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
VSOutput MainVS(VSInput input)
{
    VSOutput output;
    output.position = float4(
        input.position.x * gScale.x + gTranslate.x,
        input.position.y * gScale.y + gTranslate.y,
        input.position.z,
        1.0f);
    output.uv = input.uv;
    return output;
}
)";

        static const char* kPixelShaderSource = R"(
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);
struct PSInput { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
float4 MainPS(PSInput input) : SV_TARGET
{
    return gTexture.Sample(gSampler, input.uv);
}
)";

        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
        if (!CompileShaderSource(kVertexShaderSource, "MainVS", "vs_5_0", vertexShaderBlob))
        {
            return false;
        }

        Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
        if (!CompileShaderSource(kPixelShaderSource, "MainPS", "ps_5_0", pixelShaderBlob))
        {
            return false;
        }
        D3D12_DESCRIPTOR_RANGE descriptorRange{};
        descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descriptorRange.NumDescriptors = 1;
        descriptorRange.BaseShaderRegister = 0;
        descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER rootParameters[2]{};
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[0].Constants.Num32BitValues = 4;
        rootParameters[0].Constants.ShaderRegister = 0;
        rootParameters[0].Constants.RegisterSpace = 0;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[1].DescriptorTable.pDescriptorRanges = &descriptorRange;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        samplerDesc.ShaderRegister = 0;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.NumParameters = static_cast<UINT>(_countof(rootParameters));
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumStaticSamplers = 1;
        rootSignatureDesc.pStaticSamplers = &samplerDesc;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureErrorBlob;
        HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, rootSignatureBlob.GetAddressOf(), rootSignatureErrorBlob.GetAddressOf());
        if (FAILED(hr))
        {
            return false;
        }

        hr = m_device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_spriteRootSignature));
        if (FAILED(hr))
        {
            return false;
        }

        D3D12_INPUT_ELEMENT_DESC inputElements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.DepthClipEnable = TRUE;

        D3D12_BLEND_DESC blendDesc{};
        auto& rtBlend = blendDesc.RenderTarget[0];
        rtBlend.BlendEnable = TRUE;
        rtBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rtBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
        rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
        rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
        rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
        depthStencilDesc.DepthEnable = FALSE;
        depthStencilDesc.StencilEnable = FALSE;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = m_spriteRootSignature.Get();
        psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
        psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
        psoDesc.BlendState = blendDesc;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState = rasterizerDesc;
        psoDesc.DepthStencilState = depthStencilDesc;
        psoDesc.InputLayout = { inputElements, static_cast<UINT>(_countof(inputElements)) };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_spritePipelineState));
        return SUCCEEDED(hr);
    }

    bool D3D12GraphicsContext::CreateSpriteGeometry()
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

        auto vertexBuffer = std::make_shared<D3D12VertexBuffer>();
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

    void D3D12GraphicsContext::ReleaseRenderTargets()
    {
        for (auto& renderTarget : m_renderTargets)
        {
            renderTarget.Reset();
        }
    }

    void D3D12GraphicsContext::ReleaseSpriteResources()
    {
        m_hasBoundTexture = false;
        m_boundTextureSrvGpu = {};
        m_quadTransform = {};
        m_spriteVertexBuffer.reset();
        m_spritePipelineState.Reset();
        m_spriteRootSignature.Reset();
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
