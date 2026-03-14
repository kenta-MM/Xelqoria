#pragma once

#include <memory>

#include "GraphicsAPI.h"
#include "IGraphicsContext.h"

namespace Xelqoria::App
{
    std::unique_ptr<RHI::IGraphicsContext> BootstrapRenderBackend(RHI::GraphicsAPI api);
}
