#pragma once

#include "PlatformTypes.h"

namespace Xelqoria::Platform
{
    /// <summary>
    /// OS のカーソル表示と座標操作を抽象化する。
    /// </summary>
    class ICursor
    {
    public:
        virtual ~ICursor() = default;

        /// <summary>
        /// カーソルを表示する。
        /// </summary>
        virtual void Show() = 0;

        /// <summary>
        /// カーソルを非表示にする。
        /// </summary>
        virtual void Hide() = 0;

        /// <summary>
        /// カーソルのスクリーン座標を設定する。
        /// </summary>
        /// <param name="position">設定するスクリーン座標。</param>
        virtual void SetScreenPosition(Point position) = 0;

        /// <summary>
        /// カーソルのスクリーン座標を取得する。
        /// </summary>
        /// <returns>現在のスクリーン座標。</returns>
        [[nodiscard]] virtual Point GetScreenPosition() const = 0;
    };
}
