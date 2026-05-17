#pragma once

#include "ICursor.h"

namespace Xelqoria::Platform::Win32
{
    /// <summary>
    /// Win32 API を使用するカーソル実装。
    /// </summary>
    class Win32Cursor final : public ICursor
    {
    public:
        /// <summary>
        /// カーソルを表示する。
        /// </summary>
        void Show() override;

        /// <summary>
        /// カーソルを非表示にする。
        /// </summary>
        void Hide() override;

        /// <summary>
        /// カーソルのスクリーン座標を設定する。
        /// </summary>
        /// <param name="position">設定するスクリーン座標。</param>
        void SetScreenPosition(Point position) override;

        /// <summary>
        /// カーソル形状を設定する。
        /// </summary>
        /// <param name="shape">設定するカーソル形状。</param>
        void SetShape(CursorShape shape) override;

        /// <summary>
        /// カーソルのスクリーン座標を取得する。
        /// </summary>
        /// <returns>現在のスクリーン座標。</returns>
        [[nodiscard]] Point GetScreenPosition() const override;
    };
}
