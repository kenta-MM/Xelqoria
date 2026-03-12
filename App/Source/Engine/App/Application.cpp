#include <Windows.h>
#include <chrono>
#include <memory>

#include "Engine/App/Application.h"
#include "Engine/App/RenderBackendBootstrap.h"
#include "Engine/Graphics/Sprite.h"
#include "Engine/Graphics/SpriteRenderer.h"
#include "Engine/Graphics/Texture2D.h"
#include "Engine/RHI/GraphicsAPI.h"

namespace Xelqoria::App
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

            auto currentTime = clock::now();
            std::chrono::duration<float> delta = currentTime - lastTime;
            lastTime = currentTime;

            m_deltaTime = delta.count();

            Update(m_deltaTime);
            Render();

            ++m_count;
        }

        return 0;
    }

    bool Application::Initialize()
    {
        RHI::GraphicsAPI api = RHI::GraphicsAPI::D3D11;

        m_graphics = BootstrapRenderBackend(api);
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

        m_spriteRenderer = std::make_unique<Graphics::SpriteRenderer>(*m_graphics);
        m_spriteTexture = std::make_shared<Graphics::Texture2D>();
        m_sprite = std::make_unique<Graphics::Sprite>();

        if (!m_spriteTexture->LoadFromFile(L"../Resource\\mapchip.png", *m_graphics))
        {
            return false;
        }

        m_sprite->SetTexture(m_spriteTexture);

        return true;
    }

    void Application::Shutdown()
    {
        m_sprite.reset();
        m_spriteTexture.reset();
        m_spriteRenderer.reset();

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

        if (m_spriteRenderer && m_sprite)
        {
            m_spriteRenderer->Begin();
            m_spriteRenderer->Draw(*m_sprite);
            m_spriteRenderer->End();
        }

        m_graphics->EndFrame();
    }
}
