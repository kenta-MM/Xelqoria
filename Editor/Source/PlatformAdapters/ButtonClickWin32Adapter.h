#pragma once

#include "ButtonClickInput.h"

#include <Windows.h>
#include <cstdint>

namespace Xelqoria::Editor
{
    /// <summary>
    /// Win32 ボタン HWND から平台クリック判定対象を構築する。
    /// </summary>
    /// <param name="buttonHandle">判定対象のボタン HWND。</param>
    /// <returns>平台クリック判定対象。</returns>
    [[nodiscard]] inline ButtonClickTarget BuildButtonClickTarget(HWND buttonHandle)
    {
        if (nullptr == buttonHandle)
        {
            return ButtonClickTarget{};
        }

        RECT buttonRect{};
        GetWindowRect(buttonHandle, &buttonRect);

        return ButtonClickTarget{
            reinterpret_cast<std::uintptr_t>(buttonHandle),
            IsWindowVisible(buttonHandle) != FALSE,
            IsWindowEnabled(buttonHandle) != FALSE,
            ButtonClickRect{ buttonRect.left, buttonRect.top, buttonRect.right, buttonRect.bottom }
        };
    }
}
