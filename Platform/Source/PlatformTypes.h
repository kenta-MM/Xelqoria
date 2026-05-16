#pragma once

#include <cstdint>

namespace Xelqoria::Platform
{
    /// <summary>
    /// OS 固有のウィンドウハンドルを保持する不透明な値を表す。
    /// </summary>
    using NativeWindowHandle = void*;

    /// <summary>
    /// OS 固有のアプリケーションインスタンスハンドルを保持する不透明な値を表す。
    /// </summary>
    using NativeApplicationHandle = void*;

    /// <summary>
    /// 2 次元の整数座標を表す。
    /// </summary>
    struct Point
    {
        std::int32_t x = 0;
        std::int32_t y = 0;
    };
}
