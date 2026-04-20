#include "D3D12SpritePipeline.h"

#include <algorithm>
#include <cstring>

#include <d3dcompiler.h>
#include <d3dcommon.h>

#include "D3D12Texture.h"
#include "D3D12VertexBuffer.h"
#include <dxgiformat.h>
#include <d3d12.h>
#include <Windows.h>
#include <wrl/client.h>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <IGraphicsContext.h>
#include <ITexture.h>

#pragma comment(lib, "d3dcompiler.lib")

namespace Xelqoria::Backends::D3D12
{
    namespace
    {
        struct SpriteVertex
        {
            float position[3];
            float uv[2];
        };

        /// <summary>
        /// インライン HLSL ソースコードを指定ターゲット向けにコンパイルする。
        /// </summary>
        bool CompileShaderSource(
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

    bool D3D12SpritePipeline::Initialize(ID3D12Device* device)
    {
        if (nullptr == device)
        {
            return false;
        }

        return CreateDescriptorHeap(device)
            && CreateShaderState(device)
            && CreateGeometry(device);
    }

    void D3D12SpritePipeline::Shutdown()
    {
        m_quadTransform = {};
        m_hasBoundTexture = false;
        m_boundTextureSrvGpu = {};
        m_vertexBuffer.reset();
        m_pipelineState.Reset();
        m_rootSignature.Reset();
        m_srvHeap.Reset();
    }

    void D3D12SpritePipeline::BindTexture(ID3D12Device* device, std::uint32_t slot, RHI::ITexture* texture)
    {
        if (nullptr == device || nullptr == m_srvHeap.Get() || slot != 0)
        {
            m_hasBoundTexture = false;
            return;
        }

        if (nullptr == texture)
        {
            WriteNullShaderResourceView(device);
            m_hasBoundTexture = false;
            return;
        }

        auto* d3d12Texture = dynamic_cast<D3D12Texture*>(texture);
        if (nullptr == d3d12Texture)
        {
            WriteNullShaderResourceView(device);
            m_hasBoundTexture = false;
            return;
        }

        ID3D12Resource* nativeTexture = d3d12Texture->GetNativeTexture();
        if (nullptr == nativeTexture)
        {
            WriteNullShaderResourceView(device);
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

        device->CreateShaderResourceView(
            nativeTexture,
            &srvDesc,
            m_srvHeap->GetCPUDescriptorHandleForHeapStart());
        m_boundTextureSrvGpu = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
        m_hasBoundTexture = true;
    }

    void D3D12SpritePipeline::SetQuadTransform(const RHI::QuadTransform2D& transform)
    {
        m_quadTransform = transform;
    }

    void D3D12SpritePipeline::Draw(
        ID3D12GraphicsCommandList* commandList,
        std::uint32_t vertexCount,
        std::uint32_t startVertexLocation)
    {
        if (nullptr == commandList
            || !m_rootSignature
            || !m_pipelineState
            || !m_vertexBuffer
            || !m_srvHeap)
        {
            return;
        }

        auto* d3d12VertexBuffer = dynamic_cast<D3D12VertexBuffer*>(m_vertexBuffer.get());
        if (nullptr == d3d12VertexBuffer)
        {
            return;
        }

        const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView = d3d12VertexBuffer->GetView();
        if (vertexBufferView.BufferLocation == 0
            || vertexBufferView.SizeInBytes == 0
            || vertexBufferView.StrideInBytes == 0)
        {
            return;
        }

        ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvHeap.Get() };
        commandList->SetDescriptorHeaps(1, descriptorHeaps);
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_pipelineState.Get());

        const float quadTransformData[20] =
        {
            m_quadTransform.scaleX,
            m_quadTransform.scaleY,
            m_quadTransform.rotationCos,
            m_quadTransform.rotationSin,
            m_quadTransform.translateX,
            m_quadTransform.translateY,
            m_quadTransform.outlineEnabled,
            m_quadTransform.outlineThickness,
            m_quadTransform.outlineColorR,
            m_quadTransform.outlineColorG,
            m_quadTransform.outlineColorB,
            m_quadTransform.outlineColorA,
            m_quadTransform.fillColorR,
            m_quadTransform.fillColorG,
            m_quadTransform.fillColorB,
            m_quadTransform.fillColorA,
            m_hasBoundTexture ? 1.0f : 0.0f,
            m_quadTransform.reserved2,
            m_quadTransform.reserved3,
            m_quadTransform.reserved4
        };

        commandList->SetGraphicsRoot32BitConstants(0, 20, quadTransformData, 0);
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

    bool D3D12SpritePipeline::CreateDescriptorHeap(ID3D12Device* device)
    {
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
        srvHeapDesc.NumDescriptors = 1;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        const HRESULT hr = device->CreateDescriptorHeap(
            &srvHeapDesc,
            IID_PPV_ARGS(m_srvHeap.GetAddressOf()));
        if (FAILED(hr))
        {
            return false;
        }

        WriteNullShaderResourceView(device);
        return true;
    }

    bool D3D12SpritePipeline::CreateShaderState(ID3D12Device* device)
    {
        static const char* kVertexShaderSource = R"(
cbuffer SpriteTransformBuffer : register(b0)
{
    float2 gScale;
    float2 gRotation;
    float2 gTranslate;
    float2 gOutlineState;
    float4 gOutlineColor;
    float4 gFillColor;
    float4 gTextureState;
};
struct VSInput { float3 position : POSITION; float2 uv : TEXCOORD0; };
struct VSOutput { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
VSOutput MainVS(VSInput input)
{
    VSOutput output;
    float2 scaledPosition = float2(
        input.position.x * gScale.x,
        input.position.y * gScale.y);
    float2 rotatedPosition = float2(
        scaledPosition.x * gRotation.x - scaledPosition.y * gRotation.y,
        scaledPosition.x * gRotation.y + scaledPosition.y * gRotation.x);
    output.position = float4(
        rotatedPosition.x + gTranslate.x,
        rotatedPosition.y + gTranslate.y,
        input.position.z,
        1.0f);
    output.uv = input.uv;
    return output;
}
)";

        static const char* kPixelShaderSource = R"(
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);
cbuffer SpriteTransformBuffer : register(b0)
{
    float2 gScale;
    float2 gRotation;
    float2 gTranslate;
    float2 gOutlineState;
    float4 gOutlineColor;
    float4 gFillColor;
    float4 gTextureState;
};
struct PSInput { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
float4 MainPS(PSInput input) : SV_TARGET
{
    if (gTextureState.x <= 0.5f)
    {
        return gFillColor;
    }

    float4 baseColor = gTexture.Sample(gSampler, input.uv);
    if (gOutlineState.x > 0.5f)
    {
        uint textureWidth = 0;
        uint textureHeight = 0;
        gTexture.GetDimensions(textureWidth, textureHeight);

        float outlineWidthU = gOutlineState.y / max((float)textureWidth, 1.0f);
        float outlineWidthV = gOutlineState.y / max((float)textureHeight, 1.0f);
        bool isOutlinePixel =
            input.uv.x <= outlineWidthU
            || input.uv.x >= 1.0f - outlineWidthU
            || input.uv.y <= outlineWidthV
            || input.uv.y >= 1.0f - outlineWidthV;

        if (isOutlinePixel)
        {
            return gOutlineColor;
        }
    }

    return baseColor;
}
)";

        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
        if (false == CompileShaderSource(kVertexShaderSource, "MainVS", "vs_5_0", vertexShaderBlob))
        {
            return false;
        }

        Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
        if (false == CompileShaderSource(kPixelShaderSource, "MainPS", "ps_5_0", pixelShaderBlob))
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
        rootParameters[0].Constants.Num32BitValues = 20;
        rootParameters[0].Constants.ShaderRegister = 0;
        rootParameters[0].Constants.RegisterSpace = 0;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

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
        HRESULT hr = D3D12SerializeRootSignature(
            &rootSignatureDesc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            rootSignatureBlob.GetAddressOf(),
            rootSignatureErrorBlob.GetAddressOf());
        if (FAILED(hr))
        {
            return false;
        }

        hr = device->CreateRootSignature(
            0,
            rootSignatureBlob->GetBufferPointer(),
            rootSignatureBlob->GetBufferSize(),
            IID_PPV_ARGS(m_rootSignature.GetAddressOf()));
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
        auto& renderTargetBlend = blendDesc.RenderTarget[0];
        renderTargetBlend.BlendEnable = TRUE;
        renderTargetBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        renderTargetBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        renderTargetBlend.BlendOp = D3D12_BLEND_OP_ADD;
        renderTargetBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
        renderTargetBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
        renderTargetBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        renderTargetBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
        depthStencilDesc.DepthEnable = FALSE;
        depthStencilDesc.StencilEnable = FALSE;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};
        pipelineStateDesc.pRootSignature = m_rootSignature.Get();
        pipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
        pipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
        pipelineStateDesc.BlendState = blendDesc;
        pipelineStateDesc.SampleMask = UINT_MAX;
        pipelineStateDesc.RasterizerState = rasterizerDesc;
        pipelineStateDesc.DepthStencilState = depthStencilDesc;
        pipelineStateDesc.InputLayout = { inputElements, static_cast<UINT>(_countof(inputElements)) };
        pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateDesc.NumRenderTargets = 1;
        pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pipelineStateDesc.SampleDesc.Count = 1;

        hr = device->CreateGraphicsPipelineState(
            &pipelineStateDesc,
            IID_PPV_ARGS(m_pipelineState.GetAddressOf()));
        return SUCCEEDED(hr);
    }

    bool D3D12SpritePipeline::CreateGeometry(ID3D12Device* device)
    {
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
            device,
            vertices,
            static_cast<std::uint32_t>(_countof(vertices)),
            static_cast<std::uint32_t>(sizeof(SpriteVertex)));
        if (false == initialized)
        {
            return false;
        }

        m_vertexBuffer = vertexBuffer;
        return true;
    }

    void D3D12SpritePipeline::WriteNullShaderResourceView(ID3D12Device* device)
    {
        if (nullptr == device || nullptr == m_srvHeap.Get())
        {
            m_boundTextureSrvGpu = {};
            return;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc{};
        nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        nullSrvDesc.Texture2D.MostDetailedMip = 0;
        nullSrvDesc.Texture2D.MipLevels = 1;
        nullSrvDesc.Texture2D.PlaneSlice = 0;
        nullSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        device->CreateShaderResourceView(
            nullptr,
            &nullSrvDesc,
            m_srvHeap->GetCPUDescriptorHandleForHeapStart());
        m_boundTextureSrvGpu = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
    }
}
