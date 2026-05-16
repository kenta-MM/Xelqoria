#include "Win32Cursor.h"

#include <Windows.h>

namespace Xelqoria::Platform::Win32
{
    namespace
    {
        [[nodiscard]] const wchar_t* ToCursorResource(CursorShape shape)
        {
            if (CursorShape::HorizontalResize == shape)
            {
                return IDC_SIZEWE;
            }
            if (CursorShape::VerticalResize == shape)
            {
                return IDC_SIZENS;
            }

            return IDC_ARROW;
        }
    }

    void Win32Cursor::Show()
    {
        ShowCursor(TRUE);
    }

    void Win32Cursor::Hide()
    {
        ShowCursor(FALSE);
    }

    void Win32Cursor::SetScreenPosition(Point position)
    {
        SetCursorPos(position.x, position.y);
    }

    void Win32Cursor::SetShape(CursorShape shape)
    {
        SetCursor(LoadCursorW(nullptr, ToCursorResource(shape)));
    }

    Point Win32Cursor::GetScreenPosition() const
    {
        POINT cursorPoint{};
        if (FALSE == GetCursorPos(&cursorPoint))
        {
            return {};
        }

        return Point{
            static_cast<std::int32_t>(cursorPoint.x),
            static_cast<std::int32_t>(cursorPoint.y)
        };
    }
}
