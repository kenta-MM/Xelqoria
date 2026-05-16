#pragma once

#include "IEventLoop.h"

namespace Xelqoria::Platform::Win32
{
    /// <summary>
    /// Win32 メッセージキューを使用してイベントループを処理する。
    /// </summary>
    class Win32EventLoop final : public IEventLoop
    {
    public:
        bool PumpEvents() override;
        void RequestQuit() override;
    };
}
