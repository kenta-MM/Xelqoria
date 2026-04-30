#include "Window.h"
#include <Windows.h>
#include <utility>

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
#include <cstdint>
#include <functional>

namespace Xelqoria::Core
{

    Window::~Window()
    {
        if (m_hWnd)
        {
            DestroyWindow(m_hWnd);
            m_hWnd = nullptr;
        }

        if (m_hInstance)
        {
            UnregisterClassW(m_className.c_str(), m_hInstance);
        }
    }

    bool Window::Create(HINSTANCE hInstance, const wchar_t* title, uint32_t clientWidth, uint32_t clientHeight)
    {
        m_hInstance = hInstance;
        m_title = title;
        m_width = clientWidth;
        m_height = clientHeight;

        WNDCLASSW wc{};
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &Window::StaticWndProc;
        wc.hInstance = m_hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        wc.lpszClassName = m_className.c_str();

        if (!RegisterClassW(&wc))
        {
            return false;
        }

        const DWORD style = WS_OVERLAPPEDWINDOW;

        RECT rc{ 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height )};
        if(!AdjustWindowRect(&rc, style, FALSE))
        {
            UnregisterClassW(wc.lpszClassName, m_hInstance);
            return false;
        }

        m_hWnd = CreateWindowW(
            m_className.c_str(),
            m_title.c_str(),
            style,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rc.right - rc.left,
            rc.bottom - rc.top,
            nullptr, 
            nullptr, 
            m_hInstance, 
            this
        );

        return m_hWnd != nullptr;
    }

    void Window::Show(int nCmdShow)
    {
        ShowWindow(m_hWnd, nCmdShow);
        SetForegroundWindow(m_hWnd);
        SetFocus(m_hWnd);
    }

    bool Window::PumpMessages()
    {
        MSG msg{};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                return false;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return true;
    }

    HWND Window::GetHwnd() const 
    { 
        return m_hWnd; 
    }

    void Window::SetCommandHandler(std::function<void(unsigned)> handler)
    {
        m_commandHandler = std::move(handler);
    }

    void Window::SetNotifyHandler(std::function<bool(LPARAM)> handler)
    {
        m_notifyHandler = std::move(handler);
    }

    void Window::SetCloseRequestHandler(std::function<bool()> handler)
    {
        m_closeRequestHandler = std::move(handler);
    }

    void Window::SetResizeHandler(std::function<void(uint32_t, uint32_t)> handler)
    {
        m_resizeHandler = std::move(handler);
    }

    uint32_t Window::GetWidth() const 
    { 
        return m_width;
    }
    uint32_t Window::GetHeight() const 
    { 
        return m_height; 
    }

    LRESULT CALLBACK Window::StaticWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) 
    {
        Window* self = nullptr;
    
        if (msg == WM_NCCREATE) 
        {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = reinterpret_cast<Window*>(cs->lpCreateParams);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        }

        if (self)
        {
            return self->WndProc(hWnd, msg, wp, lp);
        }

        return DefWindowProcW(hWnd, msg, wp, lp);
    }

    LRESULT Window::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        switch (msg) 
        {
        case WM_COMMAND:
            if (m_commandHandler)
            {
                m_commandHandler(LOWORD(wp));
                return 0;
            }
            break;

        case WM_NOTIFY:
            if (m_notifyHandler && m_notifyHandler(lp))
            {
                return 0;
            }
            break;

        case WM_CLOSE:
            if (m_closeRequestHandler && false == m_closeRequestHandler())
            {
                return 0;
            }

            DestroyWindow(hWnd);
            return 0;
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZE:
            if (wp != SIZE_MINIMIZED)
            {
                m_width = static_cast<uint32_t>(LOWORD(lp));
                m_height = static_cast<uint32_t>(HIWORD(lp));
                if (m_resizeHandler)
                {
                    m_resizeHandler(m_width, m_height);
                }

                InvalidateRect(hWnd, nullptr, TRUE);
            }
            return 0;

        case WM_DPICHANGED:
            if (0 != lp)
            {
                const RECT* suggestedRect = reinterpret_cast<RECT*>(lp);
                SetWindowPos(
                    hWnd,
                    nullptr,
                    suggestedRect->left,
                    suggestedRect->top,
                    suggestedRect->right - suggestedRect->left,
                    suggestedRect->bottom - suggestedRect->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }

            InvalidateRect(hWnd, nullptr, TRUE);
            return 0;

        case WM_KEYDOWN:
            if (wp == VK_ESCAPE) 
            {
                if (m_closeRequestHandler && false == m_closeRequestHandler())
                {
                    return 0;
                }

                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        return DefWindowProcW(hWnd, msg, wp, lp);
    }

} // namespace Xelqoria::Core
