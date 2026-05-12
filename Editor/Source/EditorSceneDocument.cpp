#include "EditorSceneDocument.h"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#include "Assets/SpriteAsset.h"
#include "Assets/SpriteAssetLoader.h"
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

#include "EditorAssetPathUtils.h"
#include "EditorPathSecurity.h"
#include "EditorStringUtils.h"
#include "ScriptAssetService.h"

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr std::uintmax_t MaxSceneFileBytes = 4u * 1024u * 1024u;

        /// <summary>
        /// Vector3 を `x,y,z` 形式で出力する。
        /// </summary>
        /// <param name="stream">出力先ストリーム。</param>
        /// <param name="value">出力する Vector3。</param>
        void AppendVector3(std::ostringstream& stream, const Math::Vector3& value)
        {
            stream << value.x << "," << value.y << "," << value.z;
        }

        /// <summary>
        /// `key=value` 形式の行かを判定する。
        /// </summary>
        /// <param name="line">判定対象行。</param>
        /// <param name="key">確認するキー。</param>
        /// <returns>対象キーの行の場合は true。</returns>
        [[nodiscard]] bool IsFieldLine(std::string_view line, std::string_view key)
        {
            return line.size() > key.size()
                && line.substr(0, key.size()) == key
                && line[key.size()] == '=';
        }

        /// <summary>
        /// Sprite Asset テキストの scriptAssetId フィールドを書き換える。
        /// </summary>
        /// <param name="source">元の Sprite Asset テキスト。</param>
        /// <param name="scriptAssetId">保存する Script AssetId。</param>
        /// <returns>書き換え後の Sprite Asset テキスト。</returns>
        [[nodiscard]] std::string ReplaceSpriteAssetScriptAssetId(
            std::string_view source,
            const Core::AssetId& scriptAssetId)
        {
            std::ostringstream output;
            bool wroteScriptAssetId = false;
            std::size_t cursor = 0;
            while (cursor <= source.size())
            {
                const std::size_t lineEnd = source.find('\n', cursor);
                const std::size_t lineLength = lineEnd == std::string_view::npos
                    ? source.size() - cursor
                    : lineEnd - cursor;
                const std::string_view line = source.substr(cursor, lineLength);

                if (IsFieldLine(line, "scriptAssetId"))
                {
                    output << "scriptAssetId=" << scriptAssetId.GetValue();
                    wroteScriptAssetId = true;
                }
                else
                {
                    output << line;
                }

                output << "\n";
                if (lineEnd == std::string_view::npos)
                {
                    break;
                }

                cursor = lineEnd + 1;
            }

            if (false == wroteScriptAssetId)
            {
                output << "scriptAssetId=" << scriptAssetId.GetValue() << "\n";
            }

            return output.str();
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

        if (false == m_project.GetInfo().has_value())
        {
            return false;
        }

        return LoadSceneFromPath(m_project.GetInfo()->activeScenePath);
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

            if (entry.is_regular_file(errorCode)
                && false == static_cast<bool>(errorCode)
                && EditorPathSecurity::IsPathInsideOrEqual(entry.path(), rootDirectory))
            {
                if (EditorAssetPathUtils::IsTextureImageFile(entry.path()))
                {
                    (void)RegisterImageAsset(entry.path());
                }
                else if (EditorAssetPathUtils::IsSpriteAssetFile(entry.path()))
                {
                    (void)RegisterSpriteAssetFile(entry.path());
                }
            }
        }
    }

    bool EditorSceneDocument::RegisterImageAsset(const std::filesystem::path& imagePath)
    {
        if (nullptr == m_graphicsContext || false == EditorAssetPathUtils::IsTextureImageFile(imagePath))
        {
            return false;
        }

        const std::filesystem::path rootDirectory = m_project.GetInfo().has_value()
            ? m_project.GetInfo()->projectFilePath.parent_path()
            : std::filesystem::path{};
        if (false == rootDirectory.empty()
            && false == EditorPathSecurity::IsPathInsideOrEqual(imagePath, rootDirectory))
        {
            return false;
        }

        const Core::AssetId textureAssetId = EditorAssetPathUtils::BuildTextureAssetId(imagePath, rootDirectory);
        const Core::AssetId spriteAssetId = EditorAssetPathUtils::BuildSpriteAssetId(imagePath, rootDirectory);
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

    bool EditorSceneDocument::RegisterSpriteAssetFile(const std::filesystem::path& spriteAssetPath)
    {
        if (false == m_project.GetInfo().has_value()
            || false == EditorAssetPathUtils::IsSpriteAssetFile(spriteAssetPath))
        {
            return false;
        }

        const std::filesystem::path rootDirectory = m_project.GetInfo()->projectFilePath.parent_path();
        if (false == EditorPathSecurity::IsPathInsideOrEqual(spriteAssetPath, rootDirectory))
        {
            return false;
        }

        std::ifstream input(spriteAssetPath, std::ios::binary);
        if (false == input.is_open())
        {
            return false;
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();
        const auto loadResult = Game::Assets::SpriteAssetLoader::LoadFromText(buffer.str());
        if (false == loadResult.IsSuccess() || false == loadResult.asset.has_value())
        {
            return false;
        }

        const Core::AssetId spriteAssetId =
            EditorAssetPathUtils::BuildSpriteAssetId(spriteAssetPath, rootDirectory);
        if (spriteAssetId.IsEmpty())
        {
            return false;
        }

        m_spriteAssetRegistry.RegisterSpriteAsset(spriteAssetId, *loadResult.asset);
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

    std::optional<std::filesystem::path> EditorSceneDocument::ResolveSpriteAssetPath(
        const Core::AssetId& spriteAssetId) const
    {
        if (false == m_project.GetInfo().has_value() || spriteAssetId.IsEmpty())
        {
            return std::nullopt;
        }

        const auto registered = std::find(
            m_registeredSpriteAssetIds.begin(),
            m_registeredSpriteAssetIds.end(),
            spriteAssetId);
        if (registered == m_registeredSpriteAssetIds.end())
        {
            return std::nullopt;
        }

        constexpr std::string_view spriteAssetPrefix = "sprites/";
        const std::string& assetValue = spriteAssetId.GetValue();
        if (false == std::string_view(assetValue).starts_with(spriteAssetPrefix))
        {
            return std::nullopt;
        }

        const std::filesystem::path relativePath =
            ToWideString(std::string_view(assetValue).substr(spriteAssetPrefix.size()));
        if (relativePath.empty()
            || false == EditorPathSecurity::IsSafeRelativePath(relativePath)
            || false == EditorAssetPathUtils::IsSpriteAssetFile(relativePath))
        {
            return std::nullopt;
        }

        const std::filesystem::path rootDirectory = m_project.GetInfo()->projectFilePath.parent_path();
        const std::filesystem::path spriteAssetPath = rootDirectory / relativePath;
        if (false == EditorPathSecurity::IsPathInsideOrEqual(spriteAssetPath, rootDirectory)
            || false == std::filesystem::exists(spriteAssetPath))
        {
            return std::nullopt;
        }

        return spriteAssetPath;
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
            && false == static_cast<bool>(errorCode)
            && EditorPathSecurity::IsPathInsideOrEqual(targetDirectory, rootDirectory))
        {
            const std::filesystem::path relativeTargetDirectory =
                std::filesystem::relative(targetDirectory, rootDirectory, errorCode);
            if (false == static_cast<bool>(errorCode)
                && false == relativeTargetDirectory.empty()
                && EditorPathSecurity::IsSafeRelativePath(relativeTargetDirectory))
            {
                outputDirectory = targetDirectory;
            }
        }

        std::filesystem::create_directories(outputDirectory, errorCode);
        if (errorCode)
        {
            return false;
        }

        if (false == EditorPathSecurity::IsValidProjectName(ToWideString(entity.GetName())))
        {
            return false;
        }

        const std::filesystem::path spriteAssetPath = outputDirectory / (ToWideString(entity.GetName()) + L".sprite");
        const Game::Transform& transform = entity.GetTransform();
        const auto spriteComponent = entity.GetSpriteComponent();
        Core::AssetId spriteAssetRef{};
        Core::AssetId textureAssetId{};
        Core::AssetId scriptAssetId{};
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
                scriptAssetId = spriteAsset->scriptAssetId;
                const auto texture = m_textureAssetRegistry.ResolveTexture(textureAssetId);
                if (static_cast<bool>(texture))
                {
                    textureWidth = texture->GetWidth();
                    textureHeight = texture->GetHeight();
                }
            }
        }

        if (scriptAssetId.IsEmpty() && std::filesystem::exists(spriteAssetPath))
        {
            std::ifstream existingInput(spriteAssetPath, std::ios::binary);
            if (existingInput.is_open())
            {
                std::ostringstream buffer;
                buffer << existingInput.rdbuf();
                const auto loadResult = Game::Assets::SpriteAssetLoader::LoadFromText(buffer.str());
                if (loadResult.IsSuccess() && loadResult.asset.has_value())
                {
                    scriptAssetId = loadResult.asset->scriptAssetId;
                }
            }
        }

        std::ofstream output(spriteAssetPath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return false;
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
        text << "scriptAssetId=" << scriptAssetId.GetValue() << "\n";
        text << "texture.size=" << textureWidth << "," << textureHeight << "\n";
        text << "render.visible=" << (renderSettings.visible ? "true" : "false") << "\n";
        text << "render.sortOrder=" << renderSettings.sortOrder << "\n";
        text << "render.opacity=" << renderSettings.opacity << "\n";
        text << "render.color="
            << renderSettings.color[0] << ","
            << renderSettings.color[1] << ","
            << renderSettings.color[2] << ","
            << renderSettings.color[3] << "\n";

        output << text.str();
        return static_cast<bool>(output);
    }

    bool EditorSceneDocument::AssignScriptAssetToSpriteAssetFile(
        const std::filesystem::path& spriteAssetPath,
        const std::filesystem::path& scriptAssetPath)
    {
        if (false == m_project.GetInfo().has_value()
            || false == EditorAssetPathUtils::IsSpriteAssetFile(spriteAssetPath)
            || false == ScriptAssetService::IsScriptAssetFile(scriptAssetPath))
        {
            return false;
        }

        const std::filesystem::path rootDirectory = m_project.GetInfo()->projectFilePath.parent_path();
        if (false == EditorPathSecurity::IsPathInsideOrEqual(spriteAssetPath, rootDirectory)
            || false == EditorPathSecurity::IsPathInsideOrEqual(scriptAssetPath, rootDirectory))
        {
            return false;
        }

        const Core::AssetId scriptAssetId =
            ScriptAssetService::BuildScriptAssetId(rootDirectory, scriptAssetPath);
        if (scriptAssetId.IsEmpty())
        {
            return false;
        }

        std::ifstream input(spriteAssetPath, std::ios::binary);
        if (false == input.is_open())
        {
            return false;
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();
        const std::string source = buffer.str();
        input.close();
        const auto loadResult = Game::Assets::SpriteAssetLoader::LoadFromText(source);
        if (false == loadResult.IsSuccess() || false == loadResult.asset.has_value())
        {
            return false;
        }

        std::ofstream output(spriteAssetPath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return false;
        }

        output << ReplaceSpriteAssetScriptAssetId(source, scriptAssetId);
        if (false == static_cast<bool>(output))
        {
            return false;
        }
        output.close();

        return RegisterSpriteAssetFile(spriteAssetPath);
    }

    bool EditorSceneDocument::AssignScriptAssetToSpriteAsset(
        const Core::AssetId& spriteAssetId,
        const std::filesystem::path& scriptAssetPath)
    {
        const auto spriteAssetPath = ResolveSpriteAssetPath(spriteAssetId);
        if (false == spriteAssetPath.has_value())
        {
            return false;
        }

        return AssignScriptAssetToSpriteAssetFile(*spriteAssetPath, scriptAssetPath);
    }

    bool EditorSceneDocument::ClearScriptAssetFromSpriteAsset(const Core::AssetId& spriteAssetId)
    {
        const auto spriteAssetPath = ResolveSpriteAssetPath(spriteAssetId);
        if (false == spriteAssetPath.has_value())
        {
            return false;
        }

        std::ifstream input(*spriteAssetPath, std::ios::binary);
        if (false == input.is_open())
        {
            return false;
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();
        const std::string source = buffer.str();
        input.close();
        const auto loadResult = Game::Assets::SpriteAssetLoader::LoadFromText(source);
        if (false == loadResult.IsSuccess() || false == loadResult.asset.has_value())
        {
            return false;
        }

        std::ofstream output(*spriteAssetPath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return false;
        }

        output << ReplaceSpriteAssetScriptAssetId(source, {});
        if (false == static_cast<bool>(output))
        {
            return false;
        }
        output.close();

        return RegisterSpriteAssetFile(*spriteAssetPath);
    }

    ScriptAssetCreationResult EditorSceneDocument::CreateAndAssignScriptAssetToSpriteAsset(
        const Core::AssetId& spriteAssetId)
    {
        ScriptAssetCreationResult result{};
        const auto spriteAssetPath = ResolveSpriteAssetPath(spriteAssetId);
        if (false == spriteAssetPath.has_value())
        {
            return result;
        }

        result = CreateScriptAssetFile(spriteAssetPath->parent_path());
        if (false == result.succeeded)
        {
            return result;
        }

        if (false == AssignScriptAssetToSpriteAssetFile(*spriteAssetPath, result.assetPath))
        {
            result.succeeded = false;
        }

        return result;
    }

    ScriptAssetCreationResult EditorSceneDocument::CreateScriptAssetFile(
        const std::filesystem::path& targetDirectory)
    {
        if (false == m_project.GetInfo().has_value())
        {
            return {};
        }

        return ScriptAssetService::CreateScriptAsset(
            m_project.GetInfo()->projectFilePath.parent_path(),
            targetDirectory);
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

        std::error_code errorCode;
        const std::uintmax_t fileSize = std::filesystem::file_size(scenePath, errorCode);
        if (errorCode || fileSize > MaxSceneFileBytes)
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
