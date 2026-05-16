#include "Win32EventLoop.h"

#include <Windows.h>

namespace Xelqoria::Platform::Win32
{
    bool Win32EventLoop::PumpEvents()
    {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (WM_QUIT == message.message)
            {
                return false;
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        return true;
    }

    void Win32EventLoop::RequestQuit()
    {
        PostQuitMessage(0);
    }
}
