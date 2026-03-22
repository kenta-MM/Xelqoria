#pragma once

#include <memory>

#include "GraphicsAPI.h"
#include "IGraphicsContext.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Editor ターゲットで使用する描画バックエンドを生成する。
    /// </summary>
    /// <param name="api">生成対象の GraphicsAPI。</param>
    /// <returns>生成した描画コンテキスト。未対応の場合は nullptr。</returns>
    std::unique_ptr<RHI::IGraphicsContext> BootstrapRenderBackend(RHI::GraphicsAPI api);
}
