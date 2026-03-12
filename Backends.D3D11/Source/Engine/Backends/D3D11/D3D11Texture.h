#pragma once

#include <cstdint>

#include <d3d11.h>
#include <wrl/client.h>

#include "Engine/RHI/ITexture.h"
#include <dxgiformat.h>

namespace Xelqoria::Backends::D3D11
{
    /// <summary>
    /// D3D11Texture の生成パラメータ。
    /// </summary>
    struct D3D11TextureCreateDesc
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
        const void* initialData = nullptr;
        std::uint32_t initialDataRowPitch = 0;
    };

    /// <summary>
    /// Direct3D 11 テクスチャ実装。
    /// </summary>
    class D3D11Texture final : public RHI::ITexture
    {
    public:
        D3D11Texture() = default;
        ~D3D11Texture() override = default;

        D3D11Texture(const D3D11Texture&) = delete;
        D3D11Texture& operator=(const D3D11Texture&) = delete;

        /// <summary>
        /// テクスチャリソースと SRV を生成する。
        /// </summary>
        /// <param name="device">テクスチャ生成に使用する D3D11 デバイス。</param>
        /// <param name="desc">生成パラメータ。</param>
        /// <returns>生成に成功した場合は true。</returns>
        bool Initialize(ID3D11Device* device, const D3D11TextureCreateDesc& desc);

        /// <summary>
        /// ピクセルシェーダスロットへ SRV をバインドする。
        /// </summary>
        /// <param name="deviceContext">描画コマンドを発行するデバイスコンテキスト。</param>
        /// <param name="slot">バインド先スロット番号。</param>
        void BindPS(ID3D11DeviceContext* deviceContext, std::uint32_t slot) const;

        std::uint32_t GetWidth() const override { return m_width; }
        std::uint32_t GetHeight() const override { return m_height; }

        /// <summary>
        /// ネイティブテクスチャリソースを取得する。
        /// </summary>
        /// <returns>ID3D11Texture2D ポインタ。</returns>
        ID3D11Texture2D* GetNativeTexture() const { return m_texture.Get(); }

        /// <summary>
        /// シェーダリソースビューを取得する。
        /// </summary>
        /// <returns>ID3D11ShaderResourceView ポインタ。</returns>
        ID3D11ShaderResourceView* GetShaderResourceView() const { return m_shaderResourceView.Get(); }

    private:
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
        DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
    };
}
