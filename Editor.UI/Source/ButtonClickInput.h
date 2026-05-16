#pragma once

#include <Windows.h>

namespace Xelqoria::Editor
{
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
        POINT cursorScreenPoint{};
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
        /// 押下開始を検出したボタン HWND を表す。
        /// </summary>
        HWND pressedButtonHandle = nullptr;
    };

    /// <summary>
    /// 共有入力スナップショットから対象ボタンのクリック成立を判定する。
    /// </summary>
    /// <param name="buttonHandle">判定対象のボタン HWND。</param>
    /// <param name="frameInput">現在フレームの入力状態。</param>
    /// <param name="inputState">前フレームから持ち越す継続状態。</param>
    /// <returns>今回のフレームでクリックが成立した場合は true。</returns>
    inline bool TryConsumeButtonClick(
        HWND buttonHandle,
        const ButtonClickFrameInput& frameInput,
        ButtonClickInputState& inputState)
    {
        if (nullptr == buttonHandle || false == IsWindowVisible(buttonHandle) || false == IsWindowEnabled(buttonHandle))
        {
            return false;
        }

        RECT buttonRect{};
        GetWindowRect(buttonHandle, &buttonRect);
        const bool isCursorInsideButton = PtInRect(&buttonRect, frameInput.cursorScreenPoint) != FALSE;

        if (frameInput.isLeftMouseButtonDown
            && false == inputState.wasLeftMouseButtonDown
            && true == isCursorInsideButton)
        {
            inputState.pressedButtonHandle = buttonHandle;
        }

        return false == frameInput.isLeftMouseButtonDown
            && true == inputState.wasLeftMouseButtonDown
            && inputState.pressedButtonHandle == buttonHandle
            && true == isCursorInsideButton;
    }
}
