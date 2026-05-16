#include "Win32PlatformFactory.h"

#include "Win32EventLoop.h"
#include "Win32Input.h"
#include "Win32Window.h"

namespace Xelqoria::Platform::Win32
{
    std::unique_ptr<IWindow> CreateWin32Window(NativeApplicationHandle applicationHandle)
    {
        return std::make_unique<Win32Window>(applicationHandle);
    }

    std::unique_ptr<IEventLoop> CreateWin32EventLoop()
    {
        return std::make_unique<Win32EventLoop>();
    }

    std::unique_ptr<IInput> CreateWin32Input()
    {
        return std::make_unique<Win32Input>();
    }
}
