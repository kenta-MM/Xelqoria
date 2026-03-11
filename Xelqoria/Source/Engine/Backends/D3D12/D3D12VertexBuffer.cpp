#include "Engine/Backends/D3D12/D3D12VertexBuffer.h"

#include <cstddef>
#include <cstring>

namespace Xelqoria::Backends::D3D12
{
    bool D3D12VertexBuffer::Initialize(ID3D12Device* device, const void* vertexData, std::uint32_t vertexCount, std::uint32_t vertexSize)
    {
        m_buffer.Reset();
        m_view = {};
        m_bufferSize = 0;
        m_strideSize = 0;

        if (device == nullptr || vertexData == nullptr || vertexCount == 0 || vertexSize == 0)
        {
            return false;
        }

        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = static_cast<UINT64>(vertexCount) * static_cast<UINT64>(vertexSize);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_buffer.GetAddressOf()));

        if (FAILED(hr))
        {
            return false;
        }

        void* mappedData = nullptr;
        const D3D12_RANGE readRange{ 0, 0 };
        hr = m_buffer->Map(0, &readRange, &mappedData);
        if (FAILED(hr) || mappedData == nullptr)
        {
            m_buffer.Reset();
            return false;
        }

        std::memcpy(mappedData, vertexData, static_cast<std::size_t>(resourceDesc.Width));
        m_buffer->Unmap(0, nullptr);

        m_bufferSize = static_cast<std::uint32_t>(resourceDesc.Width);
        m_strideSize = vertexSize;

        m_view.BufferLocation = m_buffer->GetGPUVirtualAddress();
        m_view.StrideInBytes = m_strideSize;
        m_view.SizeInBytes = m_bufferSize;
        return true;
    }
}
