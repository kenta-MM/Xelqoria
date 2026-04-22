#include "EditorSceneDocument.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <system_error>

#include "Assets/SpriteAsset.h"
#include "SceneSerializer.h"
#include "SpriteComponent.h"
#include "Texture2D.h"
#include <utility>
#include <vector>
#include <AssetId.h>
#include <Assets/SpriteAssetRegistry.h>
#include <Scene.h>
#include <TextureAssetRegistry.h>
#include <IGraphicsContext.h>    

namespace Xelqoria::Editor
{
    bool EditorSceneDocument::Initialize(RHI::IGraphicsContext& graphicsContext)
    {
        auto spriteTexture = std::make_shared<Graphics::Texture2D>();
        if (false == spriteTexture->LoadFromFile(L"../Resource\\mapchip.png", graphicsContext))
        {
            return false;
        }

        m_textureAssetRegistry.RegisterTexture("textures/mapchip", spriteTexture);

        m_registeredSpriteAssetIds.clear();
        m_registeredSpriteAssetIds.emplace_back("sprites/mapchip-left");
        m_registeredSpriteAssetIds.emplace_back("sprites/mapchip-right");
        m_registeredSpriteAssetIds.emplace_back("sprites/invalid-missing-texture");

        m_spriteAssetRegistry.RegisterSpriteAsset(
            m_registeredSpriteAssetIds[0],
            Game::Assets::SpriteAsset{ "textures/mapchip" });
        m_spriteAssetRegistry.RegisterSpriteAsset(
            m_registeredSpriteAssetIds[1],
            Game::Assets::SpriteAsset{ "textures/mapchip" });
        m_spriteAssetRegistry.RegisterSpriteAsset(
            m_registeredSpriteAssetIds[2],
            Game::Assets::SpriteAsset{ "textures/missing" });

        if (LoadSceneDocument())
        {
            return true;
        }

        m_scene = std::make_unique<Game::Scene>();

        auto& firstEntity = m_scene->CreateEntity();
        firstEntity.SetPosition(-160.0f, 0.0f, 0.0f);
        firstEntity.SetSpriteComponent(Game::SpriteComponent{
            m_registeredSpriteAssetIds[0],
            {
                true,
                0,
                1.0f
            }
        });

        auto& secondEntity = m_scene->CreateEntity();
        secondEntity.SetPosition(160.0f, 90.0f, 0.0f);
        secondEntity.SetScale(0.75f, 0.75f, 1.0f);
        secondEntity.SetSpriteComponent(Game::SpriteComponent{
            m_registeredSpriteAssetIds[1],
            {
                true,
                1,
                1.0f
            }
        });

        if (false == Save())
        {
            return false;
        }

        return true;
    }

    Game::Scene* EditorSceneDocument::GetScene()
    {
        return m_scene.get();
    }

    const Game::Scene* EditorSceneDocument::GetScene() const
    {
        return m_scene.get();
    }

    void EditorSceneDocument::ReplaceScene(std::unique_ptr<Game::Scene> scene)
    {
        m_scene = std::move(scene);
    }

    bool EditorSceneDocument::Save() const
    {
        if (nullptr == m_scene)
        {
            return false;
        }

        if (m_project.HasProject())
        {
            return m_project.Save(*m_scene);
        }

        const std::filesystem::path sceneDocumentPath = GetSceneDocumentPath();
        std::error_code errorCode;
        std::filesystem::create_directories(sceneDocumentPath.parent_path(), errorCode);
        if (errorCode)
        {
            return false;
        }

        std::ofstream output(sceneDocumentPath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return false;
        }

        output << Game::SceneSerializer::SaveToText(*m_scene);
        return output.good();
    }

    bool EditorSceneDocument::CreateProject(
        const std::wstring& projectName,
        const std::filesystem::path& parentDirectory)
    {
        if (nullptr == m_scene)
        {
            return false;
        }

        return m_project.Create(projectName, parentDirectory, *m_scene);
    }

    bool EditorSceneDocument::OpenProject(const std::filesystem::path& projectFilePath)
    {
        if (false == m_project.Open(projectFilePath) || false == m_project.GetInfo().has_value())
        {
            return false;
        }

        return LoadSceneFromPath(m_project.GetInfo()->activeScenePath);
    }

    bool EditorSceneDocument::SaveProjectAs(
        const std::wstring& projectName,
        const std::filesystem::path& parentDirectory)
    {
        if (nullptr == m_scene)
        {
            return false;
        }

        return m_project.SaveAs(projectName, parentDirectory, *m_scene);
    }

    bool EditorSceneDocument::OpenProjectScene(const std::filesystem::path& scenePath)
    {
        if (false == m_project.SelectSceneFile(scenePath))
        {
            return false;
        }

        return LoadSceneFromPath(scenePath);
    }

    const std::optional<EditorProjectInfo>& EditorSceneDocument::GetProjectInfo() const
    {
        return m_project.GetInfo();
    }

    std::vector<std::filesystem::path> EditorSceneDocument::EnumerateProjectSceneFiles() const
    {
        return m_project.EnumerateSceneFiles();
    }

    Game::Assets::SpriteAssetRegistry& EditorSceneDocument::GetSpriteAssetRegistry()
    {
        return m_spriteAssetRegistry;
    }

    const Game::Assets::SpriteAssetRegistry& EditorSceneDocument::GetSpriteAssetRegistry() const
    {
        return m_spriteAssetRegistry;
    }

    Graphics::TextureAssetRegistry& EditorSceneDocument::GetTextureAssetRegistry()
    {
        return m_textureAssetRegistry;
    }

    const Graphics::TextureAssetRegistry& EditorSceneDocument::GetTextureAssetRegistry() const
    {
        return m_textureAssetRegistry;
    }

    const std::vector<Core::AssetId>& EditorSceneDocument::GetRegisteredSpriteAssetIds() const
    {
        return m_registeredSpriteAssetIds;
    }

    bool EditorSceneDocument::LoadSceneDocument()
    {
        return LoadSceneFromPath(GetSceneDocumentPath());
    }

    bool EditorSceneDocument::LoadSceneFromPath(const std::filesystem::path& scenePath)
    {
        if (false == std::filesystem::exists(scenePath))
        {
            return false;
        }

        std::ifstream input(scenePath, std::ios::binary);
        if (false == input.is_open())
        {
            return false;
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();

        const auto loadResult = Game::SceneSerializer::LoadFromText(buffer.str());
        if (false == loadResult.IsSuccess() || false == loadResult.scene.has_value())
        {
            return false;
        }

        m_scene = std::make_unique<Game::Scene>(*loadResult.scene);
        return true;
    }

    std::filesystem::path EditorSceneDocument::GetSceneDocumentPath() const
    {
        return std::filesystem::path("Saved") / "EditorScene.xelqoria.scene";
    }
}
