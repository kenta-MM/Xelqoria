#include <Windows.h>
#include <chrono>
#include "Engine/Core/Application.h"
#include "Engine/RHI/GraphicsAPI.h"
#include "Engine/RHI/GraphicsContextFactory.h"

namespace Xelqoria::Core
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

        MSG msg{};

        using clock = std::chrono::steady_clock;
        auto lastTime = clock::now();

        while (m_running)
        {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    m_running = false;
                    break;
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (!m_running)
            {
                break;
            }

            // 時間計測
            auto currentTime = clock::now();
            std::chrono::duration<float> delta = currentTime - lastTime;
            lastTime = currentTime;

            m_deltaTime = delta.count();

            Update(m_deltaTime);
            Render();

            ++m_count;
        }

        return static_cast<int>(msg.wParam);
    }

    bool Application::Initialize()
    {
        RHI::GraphicsAPI api = RHI::GraphicsAPI::D3D12;

        m_graphics = RHI::CreateGraphicsContext(api);
        if (!m_graphics)
        {
            return false;
        }

        std::wstring title = L"Xelqoria - ";
        title += RHI::GraphicsAPIToString(api);

        if (!m_window.Create(m_hInstance, title.c_str(), 1280, 720))
        {
            return false;
        }

        m_window.Show();

   

        if (!m_graphics->Initialize(
            m_window.GetHwnd(),
            m_hInstance,
            m_window.GetWidth(),
            m_window.GetHeight()))
        {
            return false;
        }

        return true;
    }

    void Application::Shutdown()
    {
        if (m_graphics)
        {
            m_graphics->Shutdown();
            m_graphics.reset();
        }
    }

    void Application::Update(float dt)
    {
        (void)dt;
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