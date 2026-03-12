#pragma once

#include <memory>

#include "Engine/RHI/GraphicsAPI.h"
#include "Engine/RHI/IGraphicsContext.h"

namespace Xelqoria::App
{
    std::unique_ptr<RHI::IGraphicsContext> BootstrapRenderBackend(RHI::GraphicsAPI api);
}
