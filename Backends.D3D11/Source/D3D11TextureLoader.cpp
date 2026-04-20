#include "D3D11TextureLoader.h"

#include <wincodec.h>

#include <Windows.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <vector>

namespace Xelqoria::Backends::D3D11
{
    bool D3D11TextureLoader::LoadRgbaPixelsFromFile(
        const std::wstring& filePath,
        std::vector<std::uint8_t>& outPixels,
        std::uint32_t& outWidth,
        std::uint32_t& outHeight)
    {
        outPixels.clear();
        outWidth = 0;
        outHeight = 0;

        const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        {
            return false;
        }

        struct CoUninitializeGuard
        {
            bool enabled = false;

            ~CoUninitializeGuard()
            {
                if (enabled)
                {
                    CoUninitialize();
                }
            }
        } guard{ SUCCEEDED(hr) };

        Microsoft::WRL::ComPtr<IWICImagingFactory> imagingFactory;
        HRESULT localHr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(imagingFactory.GetAddressOf()));
        if (FAILED(localHr))
        {
            return false;
        }

        Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
        localHr = imagingFactory->CreateDecoderFromFilename(
            filePath.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            decoder.GetAddressOf());
        if (FAILED(localHr))
        {
            return false;
        }

        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        localHr = decoder->GetFrame(0, frame.GetAddressOf());
        if (FAILED(localHr))
        {
            return false;
        }

        UINT width = 0;
        UINT height = 0;
        localHr = frame->GetSize(&width, &height);
        if (FAILED(localHr) || width == 0 || height == 0)
        {
            return false;
        }

        Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
        localHr = imagingFactory->CreateFormatConverter(converter.GetAddressOf());
        if (FAILED(localHr))
        {
            return false;
        }

        localHr = converter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom);
        if (FAILED(localHr))
        {
            return false;
        }

        const std::uint32_t rowPitch = static_cast<std::uint32_t>(width) * 4u;
        outPixels.resize(static_cast<std::size_t>(rowPitch) * static_cast<std::size_t>(height));
        localHr = converter->CopyPixels(
            nullptr,
            rowPitch,
            static_cast<UINT>(outPixels.size()),
            outPixels.data());
        if (FAILED(localHr))
        {
            outPixels.clear();
            return false;
        }

        outWidth = static_cast<std::uint32_t>(width);
        outHeight = static_cast<std::uint32_t>(height);
        return true;
    }
}
