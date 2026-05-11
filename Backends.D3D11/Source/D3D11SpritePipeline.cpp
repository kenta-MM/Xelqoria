#include "D3D11SpritePipeline.h"

#include <algorithm>
#include <cstring>

#include <d3dcompiler.h>
#include <d3dcommon.h>

#include "D3D11Texture.h"
#include "D3D11VertexBuffer.h"
#include <dxgiformat.h>
#include <d3d11.h>
#include <Windows.h>
#include <wrl/client.h>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <IGraphicsContext.h>
#include <ITexture.h>

#pragma comment(lib, "d3dcompiler.lib")

namespace Xelqoria::Backends::D3D11
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

    bool D3D11SpritePipeline::Initialize(ID3D11Device* device)
    {
        if (nullptr == device)
        {
            return false;
        }

        return CreateShaderState(device) && CreateGeometry(device);
    }

    void D3D11SpritePipeline::Shutdown()
    {
        m_samplerState.Reset();
        m_vertexBuffer.reset();
        m_transformBuffer.Reset();
        m_inputLayout.Reset();
        m_pixelShader.Reset();
        m_vertexShader.Reset();
        m_hasBoundTexture = false;
        m_shaderConstants = {};
    }

    void D3D11SpritePipeline::BindTexture(ID3D11DeviceContext* deviceContext, std::uint32_t slot, RHI::ITexture* texture)
    {
        if (nullptr == deviceContext)
        {
            return;
        }

        ID3D11ShaderResourceView* shaderResourceView = nullptr;
        if (texture)
        {
            auto* d3d11Texture = dynamic_cast<D3D11Texture*>(texture);
            if (nullptr != d3d11Texture)
            {
                shaderResourceView = d3d11Texture->GetShaderResourceView();
            }
        }

        m_hasBoundTexture = nullptr != shaderResourceView;
        deviceContext->PSSetShaderResources(slot, 1, &shaderResourceView);
    }

    void D3D11SpritePipeline::SetShaderConstants(std::span<const float> constants)
    {
        m_shaderConstants = {};
        const std::size_t copyCount = (std::min)(constants.size(), m_shaderConstants.size());
        std::copy_n(constants.begin(), copyCount, m_shaderConstants.begin());
    }

    void D3D11SpritePipeline::Draw(
        ID3D11DeviceContext* deviceContext,
        std::uint32_t vertexCount,
        std::uint32_t startVertexLocation)
    {
        if (nullptr == deviceContext
            || !m_vertexBuffer
            || !m_inputLayout
            || !m_vertexShader
            || !m_pixelShader
            || !m_transformBuffer)
        {
            return;
        }

        auto* d3d11VertexBuffer = dynamic_cast<D3D11VertexBuffer*>(m_vertexBuffer.get());
        if (nullptr == d3d11VertexBuffer)
        {
            return;
        }

        ID3D11Buffer* nativeBuffer = d3d11VertexBuffer->GetBuffer();
        if (nullptr == nativeBuffer)
        {
            return;
        }

        const UINT stride = d3d11VertexBuffer->GetStrideSize();
        const UINT offset = 0;

        deviceContext->IASetInputLayout(m_inputLayout.Get());
        deviceContext->IASetVertexBuffers(0, 1, &nativeBuffer, &stride, &offset);
        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

        std::array<float, 20> shaderConstants = m_shaderConstants;
        shaderConstants[16] = m_hasBoundTexture ? 1.0f : 0.0f;
        deviceContext->UpdateSubresource(m_transformBuffer.Get(), 0, nullptr, shaderConstants.data(), 0, 0);
        ID3D11Buffer* transformBuffer = m_transformBuffer.Get();
        deviceContext->VSSetConstantBuffers(0, 1, &transformBuffer);
        deviceContext->PSSetConstantBuffers(0, 1, &transformBuffer);

        if (m_samplerState)
        {
            ID3D11SamplerState* sampler = m_samplerState.Get();
            deviceContext->PSSetSamplers(0, 1, &sampler);
        }

        const std::uint32_t drawVertexCount = (std::min)(vertexCount, static_cast<std::uint32_t>(6));
        if (drawVertexCount == 0)
        {
            return;
        }

        deviceContext->Draw(drawVertexCount, startVertexLocation);
    }

    bool D3D11SpritePipeline::CreateShaderState(ID3D11Device* device)
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

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

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

        HRESULT hr = device->CreateVertexShader(
            vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            nullptr,
            m_vertexShader.GetAddressOf());
        if (FAILED(hr))
        {
            return false;
        }

        hr = device->CreatePixelShader(
            pixelShaderBlob->GetBufferPointer(),
            pixelShaderBlob->GetBufferSize(),
            nullptr,
            m_pixelShader.GetAddressOf());
        if (FAILED(hr))
        {
            return false;
        }

        D3D11_INPUT_ELEMENT_DESC inputElements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        hr = device->CreateInputLayout(
            inputElements,
            static_cast<UINT>(_countof(inputElements)),
            vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            m_inputLayout.GetAddressOf());
        if (FAILED(hr))
        {
            return false;
        }

        D3D11_BUFFER_DESC constantBufferDesc{};
        constantBufferDesc.ByteWidth = sizeof(float) * 20u;
        constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        hr = device->CreateBuffer(&constantBufferDesc, nullptr, m_transformBuffer.GetAddressOf());
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
        hr = device->CreateSamplerState(&samplerDesc, m_samplerState.GetAddressOf());
        return SUCCEEDED(hr);
    }

    bool D3D11SpritePipeline::CreateGeometry(ID3D11Device* device)
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

        auto vertexBuffer = std::make_shared<D3D11VertexBuffer>();
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
}
