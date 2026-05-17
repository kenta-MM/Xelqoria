#pragma once

#include "PlatformTypes.h"

#include <cstdint>

namespace Xelqoria::Platform
{
    /// <summary>
    /// OS から取得する入力状態を抽象化する。
    /// </summary>
    class IInput
    {
    public:
        virtual ~IInput() = default;

        /// <summary>
        /// 現在フレームの入力状態を更新する。
        /// </summary>
        virtual void Update() = 0;

        /// <summary>
        /// 指定キーが現在押下中かを取得する。
        /// </summary>
        /// <param name="keyCode">プラットフォーム実装が解釈するキーコード。</param>
        /// <returns>押下中の場合は true。</returns>
        [[nodiscard]] virtual bool IsKeyDown(std::uint32_t keyCode) const = 0;

        /// <summary>
        /// 指定マウスボタンが現在押下中かを取得する。
        /// </summary>
        /// <param name="buttonCode">プラットフォーム実装が解釈するマウスボタンコード。</param>
        /// <returns>押下中の場合は true。</returns>
        [[nodiscard]] virtual bool IsMouseButtonDown(std::uint32_t buttonCode) const = 0;

        /// <summary>
        /// カーソルのスクリーン座標を取得する。
        /// </summary>
        /// <returns>現在のスクリーン座標。</returns>
        [[nodiscard]] virtual Point GetCursorScreenPosition() const = 0;
    };
}
