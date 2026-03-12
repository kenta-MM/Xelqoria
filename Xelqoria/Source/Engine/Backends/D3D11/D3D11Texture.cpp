#include "D3D11Texture.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <Windows.h>
#include <d3dcommon.h>
#include <cstdint>

namespace Xelqoria::Backends::D3D11
{
    bool D3D11Texture::Initialize(ID3D11Device* device, const D3D11TextureCreateDesc& desc)
    {
        if (device == nullptr || desc.width == 0 || desc.height == 0)
        {
            return false;
        }

        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = desc.width;
        textureDesc.Height = desc.height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = desc.format;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA initialData{};
        const D3D11_SUBRESOURCE_DATA* initialDataPtr = nullptr;
        if (desc.initialData != nullptr)
        {
            initialData.pSysMem = desc.initialData;
            initialData.SysMemPitch = (desc.initialDataRowPitch != 0)
                ? desc.initialDataRowPitch
                : (desc.width * 4);
            initialData.SysMemSlicePitch = initialData.SysMemPitch * desc.height;
            initialDataPtr = &initialData;
        }

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        const HRESULT textureHr = device->CreateTexture2D(&textureDesc, initialDataPtr, texture.GetAddressOf());
        if (FAILED(textureHr))
        {
            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
        const HRESULT srvHr = device->CreateShaderResourceView(texture.Get(), &srvDesc, shaderResourceView.GetAddressOf());
        if (FAILED(srvHr))
        {
            return false;
        }

        m_texture = texture;
        m_shaderResourceView = shaderResourceView;
        m_width = desc.width;
        m_height = desc.height;
        m_format = desc.format;

        return true;
    }

    void D3D11Texture::BindPS(ID3D11DeviceContext* deviceContext, std::uint32_t slot) const
    {
        if (deviceContext == nullptr)
        {
            return;
        }

        ID3D11ShaderResourceView* const shaderResourceView = m_shaderResourceView.Get();
        deviceContext->PSSetShaderResources(slot, 1, &shaderResourceView);
    }
}
