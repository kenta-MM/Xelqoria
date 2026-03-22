#include "Application.h"

#include <chrono>
#include <memory>
#include <string>

#include "GraphicsAPI.h"
#include "RenderBackendBootstrap.h"

namespace Xelqoria::Editor
{
    Application::Application(HINSTANCE hInstance)
        : m_hInstance(hInstance)
    {
    }

    Application::~Application()
    {
        Shutdown();
    }

    int Application::Run()
    {
        if (!Initialize())
        {
            return -1;
        }

        using clock = std::chrono::steady_clock;
        auto lastTime = clock::now();

        while (m_running)
        {
            if (!m_window.PumpMessages())
            {
                m_running = false;
                continue;
            }

            const auto currentTime = clock::now();
            const std::chrono::duration<float> delta = currentTime - lastTime;
            lastTime = currentTime;

            Update(delta.count());
            Render();
        }

        return 0;
    }

    bool Application::Initialize()
    {
        constexpr auto clientWidth = 1600u;
        constexpr auto clientHeight = 900u;
        constexpr RHI::GraphicsAPI api = RHI::GraphicsAPI::D3D11;

        m_graphics = BootstrapRenderBackend(api);
        if (!m_graphics)
        {
            return false;
        }

        std::wstring title = L"Xelqoria Editor - ";
        title += RHI::GraphicsAPIToString(api);

        if (!m_window.Create(m_hInstance, title.c_str(), clientWidth, clientHeight))
        {
            return false;
        }

        m_window.Show();

        return m_graphics->Initialize(
            m_window.GetHwnd(),
            m_hInstance,
            m_window.GetWidth(),
            m_window.GetHeight());
    }

    void Application::Shutdown()
    {
        if (m_graphics)
        {
            m_graphics->Shutdown();
            m_graphics.reset();
        }
    }

    void Application::Update(float deltaTime)
    {
        (void)deltaTime;
    }

    void Application::Render()
    {
        if (!m_graphics)
        {
            return;
        }

        m_graphics->BeginFrame();
        m_graphics->EndFrame();
    }
}
