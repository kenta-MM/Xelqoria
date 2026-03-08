#pragma once
#include <memory>

#include "Engine/RHI/GraphicsAPI.h"
#include "Engine/RHI/IGraphicsContext.h"

namespace Xelqoria::RHI
{
    /**
     * @brief グラフィックスコンテキストを生成する
     *
     * 指定された GraphicsAPI に応じて、対応する
     * グラフィックスコンテキストの実装を生成する
     * ファクトリ関数。
     *
     * 例：
     * - GraphicsAPI::D3D11 → D3D11GraphicsContext
     *
     * 将来的に Direct3D12 や Vulkan をサポートする場合は、
     * この関数内で適切なコンテキストを生成する処理を追加する。
     *
     * @param api 使用するグラフィックス API
     * @return 生成されたグラフィックスコンテキスト。
     *         対応していない API が指定された場合は nullptr を返す。
     */
    std::unique_ptr<IGraphicsContext> CreateGraphicsContext(GraphicsAPI api);
}