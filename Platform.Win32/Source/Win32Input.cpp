#include "Win32Input.h"

#include <Windows.h>

namespace Xelqoria::Platform::Win32
{
    void Win32Input::Update()
    {
    }

    bool Win32Input::IsKeyDown(std::uint32_t keyCode) const
    {
        return 0 != (::GetAsyncKeyState(static_cast<int>(keyCode)) & 0x8000);
    }

    bool Win32Input::IsMouseButtonDown(std::uint32_t buttonCode) const
    {
        return IsKeyDown(buttonCode);
    }

    Point Win32Input::GetCursorScreenPosition() const
    {
        POINT cursorScreenPoint{};
        ::GetCursorPos(&cursorScreenPoint);
        return Point{
            static_cast<std::int32_t>(cursorScreenPoint.x),
            static_cast<std::int32_t>(cursorScreenPoint.y)
        };
    }

    int Win32Input::GetMouseWheelDelta() const
    {
        return 0;
    }
}
