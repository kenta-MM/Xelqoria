#pragma once

#include <cstdint>

#include <d3d12.h>
#include <wrl/client.h>

#include "IVertexBuffer.h"

namespace Xelqoria::Backends::D3D12
{
    /// <summary>
    /// Direct3D 12 用頂点バッファ実装。
    /// </summary>
    class D3D12VertexBuffer final : public RHI::IVertexBuffer
    {
    public:
        D3D12VertexBuffer() = default;
        ~D3D12VertexBuffer() override = default;

        D3D12VertexBuffer(const D3D12VertexBuffer&) = delete;
        D3D12VertexBuffer& operator=(const D3D12VertexBuffer&) = delete;

        /// <summary>
        /// 頂点データから D3D12 バッファを生成する。
        /// </summary>
        /// <param name="device">バッファ生成に使用する D3D12 デバイス。</param>
        /// <param name="vertexData">先頭頂点データへのポインタ。</param>
        /// <param name="vertexCount">頂点数。</param>
        /// <param name="vertexSize">1 頂点あたりのサイズ（バイト）。</param>
        /// <returns>生成に成功した場合は true。</returns>
        bool Initialize(ID3D12Device* device, const void* vertexData, std::uint32_t vertexCount, std::uint32_t vertexSize);

        /// <summary>
        /// ネイティブ D3D12 バッファを取得する。
        /// </summary>
        /// <returns>ID3D12Resource ポインタ。</returns>
        ID3D12Resource* GetBuffer() const { return m_buffer.Get(); }

        /// <summary>
        /// 頂点バッファビューを取得する。
        /// </summary>
        /// <returns>D3D12 頂点バッファビュー。</returns>
        const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return m_view; }

        /// <inheritdoc/>
        std::uint32_t GetBufferSize() const override { return m_bufferSize; }

        /// <inheritdoc/>
        std::uint32_t GetStrideSize() const override { return m_strideSize; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;
        D3D12_VERTEX_BUFFER_VIEW m_view{};
        std::uint32_t m_bufferSize = 0;
        std::uint32_t m_strideSize = 0;
    };
}
