#include "Engine/App/RenderBackendBootstrap.h"

#include <memory>

#include "Engine/Backends/D3D11/D3D11GraphicsContext.h"
#include "Engine/Backends/D3D12/D3D12GraphicsContext.h"

namespace Xelqoria::App
{
    std::unique_ptr<RHI::IGraphicsContext> BootstrapRenderBackend(RHI::GraphicsAPI api)
    {
        switch (api)
        {
        case RHI::GraphicsAPI::D3D11:
            return std::make_unique<Xelqoria::Backends::D3D11::D3D11GraphicsContext>();
        case RHI::GraphicsAPI::D3D12:
            return std::make_unique<Xelqoria::Backends::D3D12::D3D12GraphicsContext>();
        case RHI::GraphicsAPI::None:
        default:
            return nullptr;
        }
    }
}
