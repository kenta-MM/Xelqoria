#include "EditorSceneDocument.h"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <iomanip>
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
#include <Vector3.h>

#include "EditorStringUtils.h"

namespace Xelqoria::Editor
{
    namespace
    {
        /// <summary>
        /// Vector3 を `x,y,z` 形式で出力する。
        /// </summary>
        /// <param name="stream">出力先ストリーム。</param>
        /// <param name="value">出力する Vector3。</param>
        void AppendVector3(std::ostringstream& stream, const Math::Vector3& value)
        {
            stream << value.x << "," << value.y << "," << value.z;
        }
    }

    bool EditorSceneDocument::Initialize(RHI::IGraphicsContext& graphicsContext)
    {
        m_graphicsContext = &graphicsContext;

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

    void EditorSceneDocument::RefreshProjectAssetRegistries()
    {
        if (false == m_project.GetInfo().has_value())
        {
            return;
        }

        const std::filesystem::path rootDirectory = m_project.GetInfo()->projectFilePath.parent_path();
        std::error_code errorCode;
        for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(rootDirectory, errorCode))
        {
            if (errorCode)
            {
                return;
            }

            if (entry.is_regular_file(errorCode) && false == static_cast<bool>(errorCode) && IsTextureImageFile(entry.path()))
            {
                (void)RegisterImageAsset(entry.path());
            }
        }
    }

    bool EditorSceneDocument::RegisterImageAsset(const std::filesystem::path& imagePath)
    {
        if (nullptr == m_graphicsContext || false == IsTextureImageFile(imagePath))
        {
            return false;
        }

        const Core::AssetId textureAssetId = BuildTextureAssetId(imagePath);
        const Core::AssetId spriteAssetId = BuildSpriteAssetId(imagePath);
        if (textureAssetId.IsEmpty() || spriteAssetId.IsEmpty())
        {
            return false;
        }

        if (false == static_cast<bool>(m_textureAssetRegistry.ResolveTexture(textureAssetId)))
        {
            auto texture = std::make_shared<Graphics::Texture2D>();
            if (false == texture->LoadFromFile(imagePath.wstring(), *m_graphicsContext))
            {
                return false;
            }

            m_textureAssetRegistry.RegisterTexture(textureAssetId, texture);
        }

        m_spriteAssetRegistry.RegisterSpriteAsset(
            spriteAssetId,
            Game::Assets::SpriteAsset{ textureAssetId });

        const auto alreadyRegistered = std::find(
            m_registeredSpriteAssetIds.begin(),
            m_registeredSpriteAssetIds.end(),
            spriteAssetId);
        if (alreadyRegistered == m_registeredSpriteAssetIds.end())
        {
            m_registeredSpriteAssetIds.push_back(spriteAssetId);
        }

        return true;
    }

    bool EditorSceneDocument::CreateSpriteAssetFile(
        const Game::Entity& entity,
        const std::filesystem::path& targetDirectory)
    {
        if (false == m_project.GetInfo().has_value() || true == entity.GetName().empty())
        {
            return false;
        }

        const std::filesystem::path rootDirectory = m_project.GetInfo()->projectFilePath.parent_path();
        std::filesystem::path outputDirectory = rootDirectory;
        std::error_code errorCode;
        if (false == targetDirectory.empty()
            && std::filesystem::is_directory(targetDirectory, errorCode)
            && false == static_cast<bool>(errorCode))
        {
            const std::filesystem::path relativeTargetDirectory =
                std::filesystem::relative(targetDirectory, rootDirectory, errorCode);
            if (false == static_cast<bool>(errorCode)
                && false == relativeTargetDirectory.empty()
                && relativeTargetDirectory.native().find(L"..") != 0)
            {
                outputDirectory = targetDirectory;
            }
        }

        std::filesystem::create_directories(outputDirectory, errorCode);
        if (errorCode)
        {
            return false;
        }

        const std::filesystem::path spriteAssetPath = outputDirectory / (ToWideString(entity.GetName()) + L".sprite");
        std::ofstream output(spriteAssetPath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return false;
        }

        const Game::Transform& transform = entity.GetTransform();
        const auto spriteComponent = entity.GetSpriteComponent();
        Core::AssetId spriteAssetRef{};
        Core::AssetId textureAssetId{};
        std::uint32_t textureWidth = 0;
        std::uint32_t textureHeight = 0;
        Game::SpriteRenderSettings renderSettings{};

        if (spriteComponent.has_value())
        {
            spriteAssetRef = spriteComponent->get().spriteAssetRef;
            renderSettings = spriteComponent->get().renderSettings;

            const auto spriteAsset = m_spriteAssetRegistry.ResolveSpriteAsset(spriteAssetRef);
            if (spriteAsset.has_value())
            {
                textureAssetId = spriteAsset->textureAssetId;
                const auto texture = m_textureAssetRegistry.ResolveTexture(textureAssetId);
                if (static_cast<bool>(texture))
                {
                    textureWidth = texture->GetWidth();
                    textureHeight = texture->GetHeight();
                }
            }
        }

        std::ostringstream text;
        text << std::fixed << std::setprecision(6);
        text << "magic=XelqoriaSpriteAsset\n";
        text << "version=1\n";
        text << "name=" << std::quoted(entity.GetName()) << "\n";
        text << "transform.position=";
        AppendVector3(text, transform.position);
        text << "\n";
        text << "transform.rotation=";
        AppendVector3(text, transform.rotation);
        text << "\n";
        text << "transform.scale=";
        AppendVector3(text, transform.scale);
        text << "\n";
        text << "hasSpriteComponent=" << (spriteComponent.has_value() ? "true" : "false") << "\n";
        text << "spriteAssetRef=" << spriteAssetRef.GetValue() << "\n";
        text << "textureAssetId=" << textureAssetId.GetValue() << "\n";
        text << "texture.size=" << textureWidth << "," << textureHeight << "\n";
        text << "render.visible=" << (renderSettings.visible ? "true" : "false") << "\n";
        text << "render.sortOrder=" << renderSettings.sortOrder << "\n";
        text << "render.opacity=" << renderSettings.opacity << "\n";

        output << text.str();
        return static_cast<bool>(output);
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

    bool EditorSceneDocument::IsTextureImageFile(const std::filesystem::path& path)
    {
        const std::wstring extension = path.extension().wstring();
        return extension == L".png"
            || extension == L".jpg"
            || extension == L".jpeg"
            || extension == L".bmp";
    }

    Core::AssetId EditorSceneDocument::BuildTextureAssetId(const std::filesystem::path& path) const
    {
        if (false == m_project.GetInfo().has_value())
        {
            return {};
        }

        std::error_code errorCode;
        const std::filesystem::path relativePath = std::filesystem::relative(
            path,
            m_project.GetInfo()->projectFilePath.parent_path(),
            errorCode);
        if (errorCode)
        {
            return {};
        }

        return Core::AssetId("textures/" + ToNarrowString(relativePath.generic_wstring()));
    }

    Core::AssetId EditorSceneDocument::BuildSpriteAssetId(const std::filesystem::path& path) const
    {
        if (false == m_project.GetInfo().has_value())
        {
            return {};
        }

        std::error_code errorCode;
        const std::filesystem::path relativePath = std::filesystem::relative(
            path,
            m_project.GetInfo()->projectFilePath.parent_path(),
            errorCode);
        if (errorCode)
        {
            return {};
        }

        return Core::AssetId("sprites/" + ToNarrowString(relativePath.generic_wstring()));
    }
}
