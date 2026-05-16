#pragma once

#include "IEventLoop.h"
#include "IInput.h"
#include "IWindow.h"
#include "PlatformTypes.h"

#include <memory>

namespace Xelqoria::Platform::Win32
{
    /// <summary>
    /// Win32 実装のウィンドウを生成する。
    /// </summary>
    /// <param name="applicationHandle">Win32 アプリケーションインスタンスハンドル。</param>
    /// <returns>生成されたウィンドウ実装。</returns>
    [[nodiscard]] std::unique_ptr<IWindow> CreateWin32Window(NativeApplicationHandle applicationHandle);

    /// <summary>
    /// Win32 実装のイベントループを生成する。
    /// </summary>
    /// <returns>生成されたイベントループ実装。</returns>
    [[nodiscard]] std::unique_ptr<IEventLoop> CreateWin32EventLoop();

    /// <summary>
    /// Win32 実装の入力読み取りを生成する。
    /// </summary>
    /// <returns>生成された入力実装。</returns>
    [[nodiscard]] std::unique_ptr<IInput> CreateWin32Input();
}
