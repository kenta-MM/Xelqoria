#include "Window.h"

#include <utility>

namespace Xelqoria::Core
{
    Window::Window(std::unique_ptr<Platform::IWindow> window, std::unique_ptr<Platform::IEventLoop> eventLoop)
        : m_window(std::move(window))
        , m_eventLoop(std::move(eventLoop))
    {
    }

    Window::~Window() = default;

    void Window::SetPlatformWindow(
        std::unique_ptr<Platform::IWindow> window,
        std::unique_ptr<Platform::IEventLoop> eventLoop)
    {
        m_window = std::move(window);
        m_eventLoop = std::move(eventLoop);
    }

    bool Window::Create(const wchar_t* title, uint32_t clientWidth, uint32_t clientHeight)
    {
        if (nullptr == m_window)
        {
            return false;
        }

        return m_window->Create(std::wstring(title), clientWidth, clientHeight);
    }

    void Window::Show(int showCommand)
    {
        (void)showCommand;
        if (nullptr != m_window)
        {
            m_window->Show();
        }
    }

    bool Window::PumpMessages()
    {
        if (nullptr == m_eventLoop)
        {
            return false;
        }

        return m_eventLoop->PumpEvents();
    }

    Platform::NativeWindowHandle Window::GetHwnd() const
    {
        if (nullptr == m_window)
        {
            return nullptr;
        }

        return m_window->GetNativeHandle();
    }

    void Window::SetCommandHandler(std::function<void(unsigned)> handler)
    {
        if (nullptr != m_window)
        {
            m_window->SetCommandHandler(std::move(handler));
        }
    }

    void Window::SetNotifyHandler(std::function<bool(Platform::NativeMessageParameter)> handler)
    {
        if (nullptr != m_window)
        {
            m_window->SetNotifyHandler(std::move(handler));
        }
    }

    void Window::SetDrawItemHandler(std::function<bool(Platform::NativeMessageParameter)> handler)
    {
        if (nullptr != m_window)
        {
            m_window->SetDrawItemHandler(std::move(handler));
        }
    }

    void Window::SetCloseRequestHandler(std::function<bool()> handler)
    {
        if (nullptr != m_window)
        {
            m_window->SetCloseRequestHandler(std::move(handler));
        }
    }

    void Window::SetResizeHandler(std::function<void(uint32_t, uint32_t)> handler)
    {
        if (nullptr != m_window)
        {
            m_window->SetResizeHandler(std::move(handler));
        }
    }

    uint32_t Window::GetWidth() const
    {
        if (nullptr == m_window)
        {
            return 0;
        }

        return m_window->GetClientWidth();
    }

    uint32_t Window::GetHeight() const
    {
        if (nullptr == m_window)
        {
            return 0;
        }

        return m_window->GetClientHeight();
    }

    int Window::ConsumeMouseWheelDelta()
    {
        if (nullptr == m_window)
        {
            return 0;
        }

        return m_window->ConsumeMouseWheelDelta();
    }
}
