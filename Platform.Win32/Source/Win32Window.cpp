#include "Win32Window.h"

#include <utility>

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

namespace Xelqoria::Platform::Win32
{
    Win32Window::Win32Window(NativeApplicationHandle applicationHandle)
        : m_applicationHandle(reinterpret_cast<HINSTANCE>(applicationHandle))
    {
    }

    Win32Window::~Win32Window()
    {
        if (nullptr != m_windowHandle)
        {
            DestroyWindow(m_windowHandle);
            m_windowHandle = nullptr;
        }

        if (nullptr != m_applicationHandle)
        {
            UnregisterClassW(m_className.c_str(), m_applicationHandle);
        }
    }

    bool Win32Window::Create(const std::wstring& title, std::uint32_t clientWidth, std::uint32_t clientHeight)
    {
        m_title = title;
        m_width = clientWidth;
        m_height = clientHeight;

        WNDCLASSW windowClass{};
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = &Win32Window::StaticWndProc;
        windowClass.hInstance = m_applicationHandle;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        windowClass.lpszClassName = m_className.c_str();

        if (FALSE == RegisterClassW(&windowClass))
        {
            return false;
        }

        constexpr DWORD style = WS_OVERLAPPEDWINDOW;
        RECT rect{ 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
        if (FALSE == AdjustWindowRect(&rect, style, FALSE))
        {
            UnregisterClassW(windowClass.lpszClassName, m_applicationHandle);
            return false;
        }

        m_windowHandle = CreateWindowW(
            m_className.c_str(),
            m_title.c_str(),
            style,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            m_applicationHandle,
            this);

        return nullptr != m_windowHandle;
    }

    void Win32Window::Show()
    {
        ShowWindow(m_windowHandle, SW_SHOW);
        SetForegroundWindow(m_windowHandle);
        SetFocus(m_windowHandle);
    }

    void Win32Window::Close()
    {
        if (nullptr != m_windowHandle)
        {
            DestroyWindow(m_windowHandle);
            m_windowHandle = nullptr;
        }
    }

    bool Win32Window::IsOpen() const
    {
        return nullptr != m_windowHandle;
    }

    NativeWindowHandle Win32Window::GetNativeHandle() const
    {
        return m_windowHandle;
    }

    std::uint32_t Win32Window::GetClientWidth() const
    {
        return m_width;
    }

    std::uint32_t Win32Window::GetClientHeight() const
    {
        return m_height;
    }

    void Win32Window::SetCommandHandler(CommandHandler handler)
    {
        m_commandHandler = std::move(handler);
    }

    void Win32Window::SetNotifyHandler(NativeMessageHandler handler)
    {
        m_notifyHandler = std::move(handler);
    }

    void Win32Window::SetDrawItemHandler(NativeMessageHandler handler)
    {
        m_drawItemHandler = std::move(handler);
    }

    void Win32Window::SetCloseRequestHandler(CloseRequestHandler handler)
    {
        m_closeRequestHandler = std::move(handler);
    }

    void Win32Window::SetResizeHandler(ResizeHandler handler)
    {
        m_resizeHandler = std::move(handler);
    }

    int Win32Window::ConsumeMouseWheelDelta()
    {
        const int mouseWheelDelta = m_pendingMouseWheelDelta;
        m_pendingMouseWheelDelta = 0;
        return mouseWheelDelta;
    }

    LRESULT CALLBACK Win32Window::StaticWndProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Win32Window* self = nullptr;

        if (WM_NCCREATE == message)
        {
            auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<Win32Window*>(createStruct->lpCreateParams);
            SetWindowLongPtrW(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(windowHandle, GWLP_USERDATA));
        }

        if (nullptr != self)
        {
            return self->WndProc(windowHandle, message, wParam, lParam);
        }

        return DefWindowProcW(windowHandle, message, wParam, lParam);
    }

    LRESULT Win32Window::WndProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_COMMAND:
            if (m_commandHandler)
            {
                m_commandHandler(LOWORD(wParam));
                return 0;
            }
            break;

        case WM_NOTIFY:
            if (m_notifyHandler && m_notifyHandler(static_cast<NativeMessageParameter>(lParam)))
            {
                return 0;
            }
            break;

        case WM_DRAWITEM:
            if (m_drawItemHandler && m_drawItemHandler(static_cast<NativeMessageParameter>(lParam)))
            {
                return TRUE;
            }
            break;

        case WM_CLOSE:
            if (m_closeRequestHandler && false == m_closeRequestHandler())
            {
                return 0;
            }

            DestroyWindow(windowHandle);
            m_windowHandle = nullptr;
            return 0;

        case WM_DESTROY:
            m_windowHandle = nullptr;
            PostQuitMessage(0);
            return 0;

        case WM_SIZE:
            if (SIZE_MINIMIZED != wParam)
            {
                m_width = static_cast<std::uint32_t>(LOWORD(lParam));
                m_height = static_cast<std::uint32_t>(HIWORD(lParam));
                if (m_resizeHandler)
                {
                    m_resizeHandler(m_width, m_height);
                }

                InvalidateRect(windowHandle, nullptr, TRUE);
            }
            return 0;

        case WM_DPICHANGED:
            if (0 != lParam)
            {
                const RECT* suggestedRect = reinterpret_cast<RECT*>(lParam);
                SetWindowPos(
                    windowHandle,
                    nullptr,
                    suggestedRect->left,
                    suggestedRect->top,
                    suggestedRect->right - suggestedRect->left,
                    suggestedRect->bottom - suggestedRect->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }

            InvalidateRect(windowHandle, nullptr, TRUE);
            return 0;

        case WM_MOUSEWHEEL:
            m_pendingMouseWheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
            return 0;

        case WM_KEYDOWN:
            if (VK_ESCAPE == wParam)
            {
                if (m_closeRequestHandler && false == m_closeRequestHandler())
                {
                    return 0;
                }

                DestroyWindow(windowHandle);
                m_windowHandle = nullptr;
                return 0;
            }
            break;
        }

        return DefWindowProcW(windowHandle, message, wParam, lParam);
    }
}
