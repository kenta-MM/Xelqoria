#pragma once

#include "PlatformTypes.h"

#include <cstdint>

namespace Xelqoria::Editor
{
    /// <summary>
    /// SceneView / Game Preview の描画先境界を表す。
    /// </summary>
    struct SceneViewSurface
    {
        /// <summary>
        /// Platform が提供する描画先ネイティブウィンドウ。
        /// </summary>
        Platform::NativeWindowHandle nativeWindow = nullptr;

        /// <summary>
        /// 描画先の幅。
        /// </summary>
        std::uint32_t width = 0;

        /// <summary>
        /// 描画先の高さ。
        /// </summary>
        std::uint32_t height = 0;
    };
}
