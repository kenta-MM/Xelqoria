#include <Windows.h>
#include <chrono>
#include <memory>
#include <string>

#include "Application.h"
#include "RenderBackendBootstrap.h"
#include "Scene.h"
#include "Assets/SpriteAsset.h"
#include "SpriteRenderer.h"
#include "Texture2D.h"
#include "GraphicsAPI.h"

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
		m_scene = std::make_unique<Game::Scene>();
		m_sceneRuntimeReady = true;

        auto spriteTexture = std::make_shared<Graphics::Texture2D>();
		if (!spriteTexture->LoadFromFile(L"../Resource\\mapchip.png", *m_graphics))
		{
			return false;
		}

		m_textureAssetRegistry.RegisterTexture("textures/mapchip", spriteTexture);
		m_spriteAssetRegistry.RegisterSpriteAsset(
			"sprites/mapchip-left",
			Game::Assets::SpriteAsset{ "textures/mapchip" });
		m_spriteAssetRegistry.RegisterSpriteAsset(
			"sprites/mapchip-right",
			Game::Assets::SpriteAsset{ "textures/mapchip" });

		{
			auto& entity = m_scene->CreateEntity();
			entity.GetTransform().SetPosition(-160.0f, 0.0f, 0.0f);
			entity.SetSpriteComponent(Game::SpriteComponent{
				"sprites/mapchip-left",
				{
					true,
					0,
					1.0f
				}
			});
		}

		{
			auto& entity = m_scene->CreateEntity();
			entity.GetTransform().SetPosition(160.0f, 90.0f, 0.0f);
			entity.GetTransform().scale = { 0.75f, 0.75f, 1.0f };
			entity.SetSpriteComponent(Game::SpriteComponent{
				"sprites/mapchip-right",
				{
					true,
					1,
					1.0f
				}
			});
		}

		return true;
	}

    void Application::Shutdown()
    {
        m_sceneRuntimeReady = false;
        m_hasLoggedSceneResolution = false;
        m_scene.reset();
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

		if (m_spriteRenderer && m_scene)
		{
			const auto resolvedSprites = m_scene->ResolveSprites(
				m_spriteAssetRegistry,
				m_textureAssetRegistry,
				m_hasLoggedSceneResolution
					? std::function<void(const std::string&)>{}
					: std::function<void(const std::string&)>(
						[](const std::string& message)
						{
							const std::string line = message + "\n";
							::OutputDebugStringA(line.c_str());
						}));
			m_hasLoggedSceneResolution = true;

			m_spriteRenderer->Begin();
			for (const auto& sprite : resolvedSprites)
			{
				m_spriteRenderer->Draw(sprite);
			}
			m_spriteRenderer->End();
		}

        m_graphics->EndFrame();
    }
}
