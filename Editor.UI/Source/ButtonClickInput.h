#pragma once

#include "PlatformTypes.h"

#include <cstdint>

namespace Xelqoria::Editor
{
    /// <summary>
    /// UI ボタンのスクリーン座標矩形を表す。
    /// </summary>
    struct ButtonClickRect
    {
        std::int32_t left = 0;
        std::int32_t top = 0;
        std::int32_t right = 0;
        std::int32_t bottom = 0;
    };

    /// <summary>
    /// クリック判定対象となる UI ボタンの状態を表す。
    /// </summary>
    struct ButtonClickTarget
    {
        /// <summary>
        /// ボタンを識別する不透明な値を表す。
        /// </summary>
        std::uintptr_t id = 0;

        /// <summary>
        /// ボタンが表示中かを表す。
        /// </summary>
        bool isVisible = false;

        /// <summary>
        /// ボタンが入力可能かを表す。
        /// </summary>
        bool isEnabled = false;

        /// <summary>
        /// ボタンのスクリーン座標矩形を表す。
        /// </summary>
        ButtonClickRect screenRect{};
    };

    /// <summary>
    /// UI ボタン入力を 1 フレーム分だけ表す。
    /// </summary>
    struct ButtonClickFrameInput
    {
        /// <summary>
        /// 現在フレームで左マウスボタンが押下中かを表す。
        /// </summary>
        bool isLeftMouseButtonDown = false;

        /// <summary>
        /// 現在フレームのカーソル位置をスクリーン座標で表す。
        /// </summary>
        Platform::Point cursorScreenPoint{};
    };

    /// <summary>
    /// UI ボタン入力の継続状態を表す。
    /// </summary>
    struct ButtonClickInputState
    {
        /// <summary>
        /// 前フレームで左マウスボタンが押下中だったかを表す。
        /// </summary>
        bool wasLeftMouseButtonDown = false;

        /// <summary>
        /// 押下開始を検出したボタン識別子を表す。
        /// </summary>
        std::uintptr_t pressedButtonId = 0;
    };

    /// <summary>
    /// スクリーン座標点がボタン矩形に含まれるかを判定する。
    /// </summary>
    /// <param name="rect">判定対象の矩形。</param>
    /// <param name="point">判定するスクリーン座標点。</param>
    /// <returns>矩形に含まれる場合は true。</returns>
    [[nodiscard]] inline bool ContainsButtonClickPoint(const ButtonClickRect& rect, Platform::Point point)
    {
        return rect.left <= point.x
            && point.x < rect.right
            && rect.top <= point.y
            && point.y < rect.bottom;
    }

    /// <summary>
    /// 共有入力スナップショットから対象ボタンのクリック成立を平台に判定する。
    /// </summary>
    /// <param name="target">判定対象のボタン状態。</param>
    /// <param name="frameInput">現在フレームの入力状態。</param>
    /// <param name="inputState">前フレームから持ち越す継続状態。</param>
    /// <returns>今回のフレームでクリックが成立した場合は true。</returns>
    inline bool TryConsumeButtonClick(
        const ButtonClickTarget& target,
        const ButtonClickFrameInput& frameInput,
        ButtonClickInputState& inputState)
    {
        if (0 == target.id || false == target.isVisible || false == target.isEnabled)
        {
            return false;
        }

        const bool isCursorInsideButton = ContainsButtonClickPoint(target.screenRect, frameInput.cursorScreenPoint);

        if (frameInput.isLeftMouseButtonDown
            && false == inputState.wasLeftMouseButtonDown
            && true == isCursorInsideButton)
        {
            inputState.pressedButtonId = target.id;
        }

        return false == frameInput.isLeftMouseButtonDown
            && true == inputState.wasLeftMouseButtonDown
            && inputState.pressedButtonId == target.id
            && true == isCursorInsideButton;
    }
}
