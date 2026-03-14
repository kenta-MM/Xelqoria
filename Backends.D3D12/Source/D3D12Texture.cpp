#include "D3D12Texture.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace Xelqoria::Backends::D3D12
{
    bool D3D12Texture::Initialize(ID3D12Device* device, const D3D12TextureCreateDesc& desc)
    {
        if (device == nullptr || desc.width == 0 || desc.height == 0)
        {
            return false;
        }

        D3D12_HEAP_PROPERTIES textureHeapProperties{};
        textureHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        textureHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        textureHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        textureHeapProperties.CreationNodeMask = 1;
        textureHeapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC textureDesc{};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Alignment = 0;
        textureDesc.Width = static_cast<UINT64>(desc.width);
        textureDesc.Height = desc.height;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = desc.format;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        Microsoft::WRL::ComPtr<ID3D12Resource> texture;
        const HRESULT hr = device->CreateCommittedResource(
            &textureHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(texture.GetAddressOf()));

        if (FAILED(hr))
        {
            return false;
        }

        m_texture = texture;
        m_width = desc.width;
        m_height = desc.height;
        m_format = desc.format;
        return true;
    }

    bool D3D12Texture::UploadInitialData(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const void* pixelData,
        std::uint32_t sourceRowPitch,
        Microsoft::WRL::ComPtr<ID3D12Resource>& outUploadBuffer)
    {
        outUploadBuffer.Reset();

        if (device == nullptr || commandList == nullptr || m_texture == nullptr || pixelData == nullptr)
        {
            return false;
        }

        const D3D12_RESOURCE_DESC textureDesc = m_texture->GetDesc();

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        UINT numRows = 0;
        UINT64 rowSizeInBytes = 0;
        UINT64 uploadBufferSize = 0;

        device->GetCopyableFootprints(
            &textureDesc,
            0,
            1,
            0,
            &footprint,
            &numRows,
            &rowSizeInBytes,
            &uploadBufferSize);

        if (numRows == 0 || rowSizeInBytes == 0 || sourceRowPitch < rowSizeInBytes)
        {
            return false;
        }

        D3D12_HEAP_PROPERTIES uploadHeapProperties{};
        uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadHeapProperties.CreationNodeMask = 1;
        uploadHeapProperties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC uploadDesc{};
        uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadDesc.Alignment = 0;
        uploadDesc.Width = uploadBufferSize;
        uploadDesc.Height = 1;
        uploadDesc.DepthOrArraySize = 1;
        uploadDesc.MipLevels = 1;
        uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadDesc.SampleDesc.Count = 1;
        uploadDesc.SampleDesc.Quality = 0;
        uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
        const HRESULT uploadHr = device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &uploadDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(uploadBuffer.GetAddressOf()));

        if (FAILED(uploadHr))
        {
            return false;
        }

        std::byte* mappedData = nullptr;
        const D3D12_RANGE readRange{ 0, 0 };
        const HRESULT mapHr = uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));
        if (FAILED(mapHr) || mappedData == nullptr)
        {
            return false;
        }

        const auto* sourceBytes = static_cast<const std::byte*>(pixelData);
        for (UINT row = 0; row < numRows; ++row)
        {
            const auto sourceOffset = static_cast<std::size_t>(row) * static_cast<std::size_t>(sourceRowPitch);
            const auto destinationOffset = static_cast<std::size_t>(footprint.Offset) +
                static_cast<std::size_t>(row) * static_cast<std::size_t>(footprint.Footprint.RowPitch);

            std::memcpy(
                mappedData + destinationOffset,
                sourceBytes + sourceOffset,
                static_cast<std::size_t>(rowSizeInBytes));
        }

        uploadBuffer->Unmap(0, nullptr);

        D3D12_TEXTURE_COPY_LOCATION sourceLocation{};
        sourceLocation.pResource = uploadBuffer.Get();
        sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        sourceLocation.PlacedFootprint = footprint;

        D3D12_TEXTURE_COPY_LOCATION destinationLocation{};
        destinationLocation.pResource = m_texture.Get();
        destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        destinationLocation.SubresourceIndex = 0;

        commandList->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);

        D3D12_RESOURCE_BARRIER resourceBarrier{};
        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        resourceBarrier.Transition.pResource = m_texture.Get();
        resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        commandList->ResourceBarrier(1, &resourceBarrier);

        outUploadBuffer = uploadBuffer;
        return true;
    }
}

