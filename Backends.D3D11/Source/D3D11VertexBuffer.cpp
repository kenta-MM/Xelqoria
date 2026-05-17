#include "D3D11VertexBuffer.h"
#include <d3d11.h>
#include <Windows.h>

namespace Xelqoria::Backends::D3D11
{
    bool D3D11VertexBuffer::Initialize(ID3D11Device* device, const void* vertexData, std::uint32_t vertexCount, std::uint32_t vertexSize)
    {
        m_buffer.Reset();
        m_bufferSize = 0;
        m_strideSize = 0;

        if (device == nullptr || vertexData == nullptr || vertexCount == 0 || vertexSize == 0)
        {
            return false;
        }

        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = vertexCount * vertexSize;
        bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = vertexSize;

        D3D11_SUBRESOURCE_DATA subresourceData{};
        subresourceData.pSysMem = vertexData;

        const HRESULT hr = device->CreateBuffer(&bufferDesc, &subresourceData, m_buffer.GetAddressOf());
        if (FAILED(hr))
        {
            return false;
        }

        m_bufferSize = bufferDesc.ByteWidth;
        m_strideSize = vertexSize;
        return true;
    }
}
