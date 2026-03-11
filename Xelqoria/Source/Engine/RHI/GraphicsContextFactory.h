#pragma once
#include <memory>

#include "Engine/RHI/GraphicsAPI.h"
#include "Engine/RHI/IGraphicsContext.h"

namespace Xelqoria::RHI
{
    /// <summary>
    /// 指定された GraphicsAPI に対応するグラフィックスコンテキストを生成する。
    /// </summary>
    /// <param name="api">使用するグラフィックス API。</param>
    /// <returns>
    /// 生成されたグラフィックスコンテキスト。
    /// 対応していない API の場合は nullptr。
    /// </returns>
    std::unique_ptr<IGraphicsContext> CreateGraphicsContext(GraphicsAPI api);
}
