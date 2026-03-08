#include "Engine/RHI/GraphicsContextFactory.h"
#include "Engine/Backends/D3D11/D3D11GraphicsContext.h"
#include "Engine/Backends/D3D12/D3D12GraphicsContext.h"
#include <memory>
#include "GraphicsAPI.h"
#include "IGraphicsContext.h"

namespace Xelqoria::RHI
{
    std::unique_ptr<IGraphicsContext> CreateGraphicsContext(GraphicsAPI api)
    {
        switch (api)
        {
        case GraphicsAPI::D3D11:
            return std::make_unique<Xelqoria::Backends::D3D11::D3D11GraphicsContext>();
        case GraphicsAPI::D3D12:
            return std::make_unique<Xelqoria::Backends::D3D12::D3D12GraphicsContext>();
        case GraphicsAPI::None:
        default:
            return nullptr;
        }
    }
}