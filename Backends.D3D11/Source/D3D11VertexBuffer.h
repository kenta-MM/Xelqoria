#pragma once

#include <cstdint>

#include <d3d11.h>
#include <wrl/client.h>

#include "IVertexBuffer.h"

namespace Xelqoria::Backends::D3D11
{
    /// <summary>
    /// Direct3D 11 用頂点バッファ実装。
    /// </summary>
    class D3D11VertexBuffer final : public RHI::IVertexBuffer
    {
    public:
        D3D11VertexBuffer() = default;
        ~D3D11VertexBuffer() override = default;

        D3D11VertexBuffer(const D3D11VertexBuffer&) = delete;
        D3D11VertexBuffer& operator=(const D3D11VertexBuffer&) = delete;

        /// <summary>
        /// 頂点データから D3D11 バッファを生成する。
        /// </summary>
        /// <param name="device">バッファ生成に使用する D3D11 デバイス。</param>
        /// <param name="vertexData">先頭頂点データへのポインタ。</param>
        /// <param name="vertexCount">頂点数。</param>
        /// <param name="vertexSize">1 頂点あたりのサイズ（バイト）。</param>
        /// <returns>生成に成功した場合は true。</returns>
        bool Initialize(ID3D11Device* device, const void* vertexData, std::uint32_t vertexCount, std::uint32_t vertexSize);

        /// <summary>
        /// ネイティブ D3D11 バッファを取得する。
        /// </summary>
        /// <returns>ID3D11Buffer ポインタ。</returns>
        ID3D11Buffer* GetBuffer() const { return m_buffer.Get(); }

        /// <inheritdoc/>
        std::uint32_t GetBufferSize() const override { return m_bufferSize; }
        
        /// <inheritdoc/>
        std::uint32_t GetStrideSize() const override { return m_strideSize; }

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer;
        std::uint32_t m_bufferSize = 0;
        std::uint32_t m_strideSize = 0;
    };
}
