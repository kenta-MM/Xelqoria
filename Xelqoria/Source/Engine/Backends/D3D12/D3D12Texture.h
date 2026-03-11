#pragma once

#include <cstdint>

#include <d3d12.h>
#include <dxgiformat.h>
#include <wrl/client.h>

#include "Engine/RHI/ITexture.h"

namespace Xelqoria::Backends::D3D12
{
    /// <summary>
    /// D3D12Texture の生成パラメータ。
    /// </summary>
    struct D3D12TextureCreateDesc
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    };

    /// <summary>
    /// Direct3D 12 テクスチャ実装。
    /// </summary>
    class D3D12Texture final : public RHI::ITexture
    {
    public:
        D3D12Texture() = default;
        ~D3D12Texture() override = default;

        D3D12Texture(const D3D12Texture&) = delete;
        D3D12Texture& operator=(const D3D12Texture&) = delete;

        /// <summary>
        /// テクスチャリソースを生成する。
        /// </summary>
        /// <param name="device">テクスチャ生成に使用する D3D12 デバイス。</param>
        /// <param name="desc">生成パラメータ。</param>
        /// <returns>生成に成功した場合は true。</returns>
        bool Initialize(ID3D12Device* device, const D3D12TextureCreateDesc& desc);

        /// <summary>
        /// 初期ピクセルデータをアップロードする。
        /// </summary>
        /// <param name="device">アップロードバッファ作成に使用する D3D12 デバイス。</param>
        /// <param name="commandList">コピーコマンドを記録するコマンドリスト。</param>
        /// <param name="pixelData">先頭ピクセルデータへのポインタ。</param>
        /// <param name="sourceRowPitch">入力データの行ピッチ（バイト）。</param>
        /// <param name="outUploadBuffer">作成したアップロードバッファの出力先。</param>
        /// <returns>アップロード準備に成功した場合は true。</returns>
        bool UploadInitialData(
            ID3D12Device* device,
            ID3D12GraphicsCommandList* commandList,
            const void* pixelData,
            std::uint32_t sourceRowPitch,
            Microsoft::WRL::ComPtr<ID3D12Resource>& outUploadBuffer);

        std::uint32_t GetWidth() const override { return m_width; }
        std::uint32_t GetHeight() const override { return m_height; }

        /// <summary>
        /// ネイティブテクスチャリソースを取得する。
        /// </summary>
        /// <returns>ID3D12Resource ポインタ。</returns>
        ID3D12Resource* GetNativeTexture() const { return m_texture.Get(); }

    private:
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
        DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
    };
}
