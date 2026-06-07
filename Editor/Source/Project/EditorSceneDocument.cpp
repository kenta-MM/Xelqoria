#include "Project/EditorSceneDocument.h"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#include "Assets/SpriteAsset.h"
#include "Assets/Collider2DAsset.h"
#include "Assets/Collider2DAssetLoader.h"
#include "Assets/SpriteAssetLoader.h"
#include "Assets/SpriteMaterialAsset.h"
#include "Assets/SpriteMaterialAssetLoader.h"
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

#include "Assets/EditorAssetPathUtils.h"
#include "Utils/EditorPathSecurity.h"
#include "Utils/EditorStringUtils.h"
#include "Assets/ScriptAssetService.h"

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

        /// <summary>
        /// Sprite アセットファイルの出力先パスを Entity 名と作成先から解決する。
        /// </summary>
        /// <param name="rootDirectory">プロジェクトルートディレクトリ。</param>
        /// <param name="entityName">Sprite Entity 名。</param>
        /// <param name="targetDirectory">作成先フォルダ。</param>
        /// <returns>出力先パス。解決できない場合は空。</returns>
        [[nodiscard]] std::optional<std::filesystem::path> BuildSpriteAssetFilePath(
            const std::filesystem::path& rootDirectory,
            const std::string& entityName,
            const std::filesystem::path& targetDirectory)
        {
            if (rootDirectory.empty()
                || entityName.empty()
                || false == EditorPathSecurity::IsValidProjectName(ToWideString(entityName)))
            {
                return std::nullopt;
            }

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

            return outputDirectory / (ToWideString(entityName) + L".sprite");
        }

        [[nodiscard]] std::filesystem::path ResolveAssetOutputDirectory(
            const std::filesystem::path& rootDirectory,
            const std::filesystem::path& targetDirectory)
        {
            std::filesystem::path outputDirectory = rootDirectory;
            std::error_code errorCode;
            if (false == targetDirectory.empty()
                && std::filesystem::is_directory(targetDirectory, errorCode)
                && false == static_cast<bool>(errorCode)
                && EditorPathSecurity::IsPathInsideOrEqual(targetDirectory, rootDirectory))
            {
                outputDirectory = targetDirectory;
            }

            return outputDirectory;
        }

        /// <summary>
        /// AssetId 生成の基準ディレクトリを asset の場所から取得する。
        /// </summary>
        /// <param name="projectInfo">現在のプロジェクト情報。</param>
        /// <param name="assetPath">対象 asset ファイル。</param>
        /// <returns>Assets ルート配下なら Assets ルート、そうでなければプロジェクトルート。</returns>
        [[nodiscard]] std::filesystem::path GetAssetIdBaseDirectory(
            const EditorProjectInfo& projectInfo,
            const std::filesystem::path& assetPath)
        {
            if (false == projectInfo.assetRootDirectory.empty()
                && EditorPathSecurity::IsPathInsideOrEqual(assetPath, projectInfo.assetRootDirectory))
            {
                return projectInfo.assetRootDirectory;
            }

            return projectInfo.projectFilePath.parent_path();
        }

        /// <summary>
        /// AssetId の相対パスから実ファイルパスを解決する。
        /// </summary>
        /// <param name="projectInfo">現在のプロジェクト情報。</param>
        /// <param name="relativePath">AssetId prefix を除いた相対パス。</param>
        /// <returns>存在する asset パス。見つからない場合は空。</returns>
        [[nodiscard]] std::optional<std::filesystem::path> ResolveAssetPathFromId(
            const EditorProjectInfo& projectInfo,
            const std::filesystem::path& relativePath)
        {
            const std::array<std::filesystem::path, 2> baseDirectories{
                projectInfo.assetRootDirectory,
                projectInfo.projectFilePath.parent_path()
            };

            for (const std::filesystem::path& baseDirectory : baseDirectories)
            {
                if (baseDirectory.empty())
                {
                    continue;
                }

                const std::filesystem::path assetPath = baseDirectory / relativePath;
                if (EditorPathSecurity::IsPathInsideOrEqual(assetPath, baseDirectory)
                    && std::filesystem::exists(assetPath))
                {
                    return assetPath;
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::filesystem::path BuildUniqueMaterialAssetFilePath(
            const std::filesystem::path& rootDirectory,
            const std::filesystem::path& targetDirectory)
        {
            const std::filesystem::path outputDirectory = ResolveAssetOutputDirectory(rootDirectory, targetDirectory);
            std::filesystem::path candidate = outputDirectory / L"NewMaterial.material";
            for (int index = 1; std::filesystem::exists(candidate); ++index)
            {
                candidate = outputDirectory / (L"NewMaterial" + std::to_wstring(index) + L".material");
            }

            return candidate;
        }

        [[nodiscard]] std::filesystem::path BuildUniqueSpriteAssetFilePath(
            const std::filesystem::path& rootDirectory,
            const std::filesystem::path& targetDirectory)
        {
            const std::filesystem::path outputDirectory = ResolveAssetOutputDirectory(rootDirectory, targetDirectory);
            std::filesystem::path candidate = outputDirectory / L"NewSprite.sprite";
            for (int index = 1; std::filesystem::exists(candidate); ++index)
            {
                candidate = outputDirectory / (L"NewSprite" + std::to_wstring(index) + L".sprite");
            }

            return candidate;
        }

        [[nodiscard]] std::filesystem::path BuildUniqueCollider2DAssetFilePath(
            const std::filesystem::path& rootDirectory,
            const std::filesystem::path& targetDirectory)
        {
            const std::filesystem::path outputDirectory = ResolveAssetOutputDirectory(rootDirectory, targetDirectory);
            std::filesystem::path candidate = outputDirectory / L"NewCollider2D.collider2d";
            for (int index = 1; std::filesystem::exists(candidate); ++index)
            {
                candidate = outputDirectory / (L"NewCollider2D" + std::to_wstring(index) + L".collider2d");
            }

            return candidate;
        }

        void AppendVector2(std::ostringstream& stream, const Math::Vector2& value)
        {
            stream << value.x << "," << value.y;
        }

        void AppendColor(std::ostringstream& stream, const std::array<float, 4>& color)
        {
            stream << color[0] << "," << color[1] << "," << color[2] << "," << color[3];
        }

        [[nodiscard]] std::string WriteMaterialAssetText(const Game::Assets::SpriteMaterialAsset& materialAsset)
        {
            std::ostringstream text;
            text << std::fixed << std::setprecision(6);
            text << "magic=XelqoriaSpriteMaterialAsset\n";
            text << "version=1\n";
            text << "textureAssetId=" << materialAsset.textureAssetId.GetValue() << "\n";
            text << "color=";
            AppendColor(text, materialAsset.color);
            text << "\n";
            text << "outline.enabled=" << (materialAsset.outlineEnabled ? "true" : "false") << "\n";
            text << "outline.thickness=" << materialAsset.outlineThickness << "\n";
            text << "outline.color=";
            AppendColor(text, materialAsset.outlineColor);
            text << "\n";
            return text.str();
        }

        [[nodiscard]] std::string WriteCollider2DAssetText(const Game::Assets::Collider2DAsset& collider2DAsset)
        {
            std::ostringstream text;
            text << std::fixed << std::setprecision(6);
            const Game::Collider2DComponent& collider = collider2DAsset.collider;
            text << "magic=XelqoriaCollider2DAsset\n";
            text << "version=1\n";
            text << "enabled=" << (collider.enabled ? "true" : "false") << "\n";
            text << "isTrigger=" << (collider.isTrigger ? "true" : "false") << "\n";
            text << "shapeType=Box\n";
            text << "offset=";
            AppendVector2(text, collider.offset);
            text << "\n";
            text << "size=";
            AppendVector2(text, collider.size);
            text << "\n";
            return text.str();
        }

        [[nodiscard]] std::string ReplaceSpriteAssetField(
            std::string_view source,
            std::string_view fieldName,
            const Core::AssetId& assetId)
        {
            std::ostringstream output;
            bool wroteField = false;
            std::size_t cursor = 0;
            while (cursor <= source.size())
            {
                const std::size_t lineEnd = source.find('\n', cursor);
                const std::size_t lineLength = lineEnd == std::string_view::npos
                    ? source.size() - cursor
                    : lineEnd - cursor;
                const std::string_view line = source.substr(cursor, lineLength);

                if (IsFieldLine(line, fieldName))
                {
                    output << fieldName << "=" << assetId.GetValue();
                    wroteField = true;
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

            if (false == wroteField)
            {
                output << fieldName << "=" << assetId.GetValue() << "\n";
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
                else if (EditorAssetPathUtils::IsMaterialAssetFile(entry.path()))
                {
                    (void)RegisterMaterialAssetFile(entry.path());
                }
                else if (EditorAssetPathUtils::IsCollider2DAssetFile(entry.path()))
                {
                    (void)RegisterCollider2DAssetFile(entry.path());
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

        const std::filesystem::path assetIdBaseDirectory =
            m_project.GetInfo().has_value()
            ? GetAssetIdBaseDirectory(*m_project.GetInfo(), imagePath)
            : rootDirectory;
        const Core::AssetId textureAssetId = EditorAssetPathUtils::BuildTextureAssetId(imagePath, assetIdBaseDirectory);
        const Core::AssetId spriteAssetId = EditorAssetPathUtils::BuildSpriteAssetId(imagePath, assetIdBaseDirectory);
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

    bool EditorSceneDocument::RegisterCollider2DAssetFile(const std::filesystem::path& collider2DAssetPath)
    {
        if (false == m_project.GetInfo().has_value()
            || false == EditorAssetPathUtils::IsCollider2DAssetFile(collider2DAssetPath))
        {
            return false;
        }

        const std::filesystem::path rootDirectory = m_project.GetInfo()->projectFilePath.parent_path();
        if (false == EditorPathSecurity::IsPathInsideOrEqual(collider2DAssetPath, rootDirectory))
        {
            return false;
        }

        std::ifstream input(collider2DAssetPath, std::ios::binary);
        if (false == input.is_open())
        {
            return false;
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();
        const auto loadResult = Game::Assets::Collider2DAssetLoader::LoadFromText(buffer.str());
        if (false == loadResult.IsSuccess() || false == loadResult.asset.has_value())
        {
            return false;
        }

        const std::filesystem::path assetIdBaseDirectory =
            GetAssetIdBaseDirectory(*m_project.GetInfo(), collider2DAssetPath);
        const Core::AssetId collider2DAssetId =
            EditorAssetPathUtils::BuildCollider2DAssetId(collider2DAssetPath, assetIdBaseDirectory);
        if (collider2DAssetId.IsEmpty())
        {
            return false;
        }

        m_collider2DAssetRegistry.RegisterCollider2DAsset(collider2DAssetId, *loadResult.asset);
        const auto alreadyRegistered = std::find(
            m_registeredCollider2DAssetIds.begin(),
            m_registeredCollider2DAssetIds.end(),
            collider2DAssetId);
        if (alreadyRegistered == m_registeredCollider2DAssetIds.end())
        {
            m_registeredCollider2DAssetIds.push_back(collider2DAssetId);
        }

        return true;
    }

    bool EditorSceneDocument::RegisterMaterialAssetFile(const std::filesystem::path& materialAssetPath)
    {
        if (false == m_project.GetInfo().has_value()
            || false == EditorAssetPathUtils::IsMaterialAssetFile(materialAssetPath))
        {
            return false;
        }

        const std::filesystem::path rootDirectory = m_project.GetInfo()->projectFilePath.parent_path();
        if (false == EditorPathSecurity::IsPathInsideOrEqual(materialAssetPath, rootDirectory))
        {
            return false;
        }

        std::ifstream input(materialAssetPath, std::ios::binary);
        if (false == input.is_open())
        {
            return false;
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();
        const auto loadResult = Game::Assets::SpriteMaterialAssetLoader::LoadFromText(buffer.str());
        if (false == loadResult.IsSuccess() || false == loadResult.asset.has_value())
        {
            return false;
        }

        const std::filesystem::path assetIdBaseDirectory =
            GetAssetIdBaseDirectory(*m_project.GetInfo(), materialAssetPath);
        const Core::AssetId materialAssetId =
            EditorAssetPathUtils::BuildMaterialAssetId(materialAssetPath, assetIdBaseDirectory);
        if (materialAssetId.IsEmpty())
        {
            return false;
        }

        m_materialAssetRegistry.RegisterMaterialAsset(materialAssetId, *loadResult.asset);
        const auto alreadyRegistered = std::find(
            m_registeredMaterialAssetIds.begin(),
            m_registeredMaterialAssetIds.end(),
            materialAssetId);
        if (alreadyRegistered == m_registeredMaterialAssetIds.end())
        {
            m_registeredMaterialAssetIds.push_back(materialAssetId);
        }

        return true;
    }

    std::optional<Core::AssetId> EditorSceneDocument::CreateMaterialAssetFile(
        const std::filesystem::path& targetDirectory,
        const Core::AssetId& textureAssetId)
    {
        if (false == m_project.GetInfo().has_value())
        {
            return std::nullopt;
        }

        const std::filesystem::path rootDirectory = m_project.GetInfo()->projectFilePath.parent_path();
        const std::filesystem::path materialAssetPath =
            BuildUniqueMaterialAssetFilePath(rootDirectory, targetDirectory);
        std::error_code errorCode;
        std::filesystem::create_directories(materialAssetPath.parent_path(), errorCode);
        if (errorCode)
        {
            return std::nullopt;
        }

        Game::Assets::SpriteMaterialAsset materialAsset{};
        materialAsset.textureAssetId = textureAssetId;
        std::ofstream output(materialAssetPath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return std::nullopt;
        }

        output << WriteMaterialAssetText(materialAsset);
        if (false == static_cast<bool>(output))
        {
            return std::nullopt;
        }
        output.close();

        if (false == RegisterMaterialAssetFile(materialAssetPath))
        {
            return std::nullopt;
        }

        return EditorAssetPathUtils::BuildMaterialAssetId(
            materialAssetPath,
            GetAssetIdBaseDirectory(*m_project.GetInfo(), materialAssetPath));
    }

    std::optional<Core::AssetId> EditorSceneDocument::CreateCollider2DAssetFile(
        const std::filesystem::path& targetDirectory,
        const Game::Collider2DComponent& collider)
    {
        if (false == m_project.GetInfo().has_value())
        {
            return std::nullopt;
        }

        const std::filesystem::path rootDirectory = m_project.GetInfo()->projectFilePath.parent_path();
        const std::filesystem::path collider2DAssetPath =
            BuildUniqueCollider2DAssetFilePath(rootDirectory, targetDirectory);
        std::error_code errorCode;
        std::filesystem::create_directories(collider2DAssetPath.parent_path(), errorCode);
        if (errorCode)
        {
            return std::nullopt;
        }

        Game::Assets::Collider2DAsset collider2DAsset{};
        collider2DAsset.collider = collider;
        std::ofstream output(collider2DAssetPath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return std::nullopt;
        }

        output << WriteCollider2DAssetText(collider2DAsset);
        if (false == static_cast<bool>(output))
        {
            return std::nullopt;
        }
        output.close();

        if (false == RegisterCollider2DAssetFile(collider2DAssetPath))
        {
            return std::nullopt;
        }

        return EditorAssetPathUtils::BuildCollider2DAssetId(
            collider2DAssetPath,
            GetAssetIdBaseDirectory(*m_project.GetInfo(), collider2DAssetPath));
    }

    std::optional<std::filesystem::path> EditorSceneDocument::ResolveMaterialAssetPath(
        const Core::AssetId& materialAssetId) const
    {
        if (false == m_project.GetInfo().has_value() || materialAssetId.IsEmpty())
        {
            return std::nullopt;
        }

        constexpr std::string_view materialAssetPrefix = "materials/";
        const std::string& assetValue = materialAssetId.GetValue();
        if (false == std::string_view(assetValue).starts_with(materialAssetPrefix))
        {
            return std::nullopt;
        }

        const std::filesystem::path relativePath =
            ToWideString(std::string_view(assetValue).substr(materialAssetPrefix.size()));
        if (relativePath.empty()
            || false == EditorPathSecurity::IsSafeRelativePath(relativePath)
            || false == EditorAssetPathUtils::IsMaterialAssetFile(relativePath))
        {
            return std::nullopt;
        }

        const auto materialAssetPath = ResolveAssetPathFromId(*m_project.GetInfo(), relativePath);
        if (false == materialAssetPath.has_value())
        {
            return std::nullopt;
        }

        return *materialAssetPath;
    }

    std::optional<std::filesystem::path> EditorSceneDocument::ResolveCollider2DAssetPath(
        const Core::AssetId& collider2DAssetId) const
    {
        if (false == m_project.GetInfo().has_value() || collider2DAssetId.IsEmpty())
        {
            return std::nullopt;
        }

        constexpr std::string_view collider2DAssetPrefix = "colliders2d/";
        const std::string& assetValue = collider2DAssetId.GetValue();
        if (false == std::string_view(assetValue).starts_with(collider2DAssetPrefix))
        {
            return std::nullopt;
        }

        const std::filesystem::path relativePath =
            ToWideString(std::string_view(assetValue).substr(collider2DAssetPrefix.size()));
        if (relativePath.empty()
            || false == EditorPathSecurity::IsSafeRelativePath(relativePath)
            || false == EditorAssetPathUtils::IsCollider2DAssetFile(relativePath))
        {
            return std::nullopt;
        }

        const auto collider2DAssetPath = ResolveAssetPathFromId(*m_project.GetInfo(), relativePath);
        if (false == collider2DAssetPath.has_value())
        {
            return std::nullopt;
        }

        return *collider2DAssetPath;
    }

    bool EditorSceneDocument::SaveMaterialAsset(
        const Core::AssetId& materialAssetId,
        const Game::Assets::SpriteMaterialAsset& materialAsset)
    {
        const auto materialAssetPath = ResolveMaterialAssetPath(materialAssetId);
        if (false == materialAssetPath.has_value())
        {
            return false;
        }

        std::ofstream output(*materialAssetPath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return false;
        }

        output << WriteMaterialAssetText(materialAsset);
        if (false == static_cast<bool>(output))
        {
            return false;
        }
        output.close();

        return RegisterMaterialAssetFile(*materialAssetPath);
    }

    bool EditorSceneDocument::EnsureMaterialAssetTexture(
        const Core::AssetId& materialAssetId,
        const Core::AssetId& textureAssetId)
    {
        if (materialAssetId.IsEmpty() || textureAssetId.IsEmpty())
        {
            return false;
        }

        const auto materialAsset = m_materialAssetRegistry.ResolveMaterialAsset(materialAssetId);
        if (false == materialAsset.has_value())
        {
            return false;
        }

        if (false == materialAsset->textureAssetId.IsEmpty())
        {
            return true;
        }

        Game::Assets::SpriteMaterialAsset updatedMaterialAsset = *materialAsset;
        updatedMaterialAsset.textureAssetId = textureAssetId;
        return SaveMaterialAsset(materialAssetId, updatedMaterialAsset);
    }

    bool EditorSceneDocument::AssignCollider2DAssetToSpriteAsset(
        const Core::AssetId& spriteAssetId,
        const Core::AssetId& collider2DAssetId)
    {
        const auto spriteAssetPath = ResolveSpriteAssetPath(spriteAssetId);
        if (false == spriteAssetPath.has_value())
        {
            return false;
        }

        if (false == collider2DAssetId.IsEmpty()
            && false == ResolveCollider2DAssetPath(collider2DAssetId).has_value())
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

        output << ReplaceSpriteAssetField(source, "collider2DAssetId", collider2DAssetId);
        if (false == static_cast<bool>(output))
        {
            return false;
        }
        output.close();

        return RegisterSpriteAssetFile(*spriteAssetPath);
    }

    std::optional<Core::AssetId> EditorSceneDocument::CreateAndAssignCollider2DAssetToSpriteAsset(
        const Core::AssetId& spriteAssetId,
        const Game::Collider2DComponent& collider)
    {
        const auto spriteAssetPath = ResolveSpriteAssetPath(spriteAssetId);
        if (false == spriteAssetPath.has_value())
        {
            return std::nullopt;
        }

        const std::optional<Core::AssetId> collider2DAssetId =
            CreateCollider2DAssetFile(spriteAssetPath->parent_path(), collider);
        if (false == collider2DAssetId.has_value())
        {
            return std::nullopt;
        }

        if (false == AssignCollider2DAssetToSpriteAsset(spriteAssetId, *collider2DAssetId))
        {
            return std::nullopt;
        }

        return collider2DAssetId;
    }

    bool EditorSceneDocument::MigrateSpriteComponentsToMaterialAssets()
    {
        if (nullptr == m_scene || false == m_project.GetInfo().has_value())
        {
            return false;
        }

        bool changed = false;
        const std::filesystem::path rootDirectory = m_project.GetInfo()->projectFilePath.parent_path();
        for (const Game::Entity& constEntity : m_scene->GetEntities())
        {
            const auto entity = m_scene->FindEntity(constEntity.GetId());
            if (false == entity.has_value())
            {
                continue;
            }

            auto spriteComponent = entity->get().GetSpriteComponent();
            if (false == spriteComponent.has_value()
                || false == spriteComponent->get().materialAssetRef.IsEmpty()
                || true == spriteComponent->get().spriteAssetRef.IsEmpty())
            {
                continue;
            }

            const auto spriteAsset = m_spriteAssetRegistry.ResolveSpriteAsset(spriteComponent->get().spriteAssetRef);
            if (false == spriteAsset.has_value())
            {
                continue;
            }

            const std::filesystem::path targetPath =
                BuildUniqueMaterialAssetFilePath(rootDirectory, rootDirectory);
            const auto materialAssetId = CreateMaterialAssetFile(targetPath.parent_path(), spriteAsset->textureAssetId);
            if (false == materialAssetId.has_value())
            {
                continue;
            }

            spriteComponent->get().materialAssetRef = *materialAssetId;
            changed = true;
        }

        return changed;
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
            EditorAssetPathUtils::BuildSpriteAssetId(
                spriteAssetPath,
                GetAssetIdBaseDirectory(*m_project.GetInfo(), spriteAssetPath));
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

        const auto spriteAssetPath = ResolveAssetPathFromId(*m_project.GetInfo(), relativePath);
        if (false == spriteAssetPath.has_value())
        {
            return std::nullopt;
        }

        return *spriteAssetPath;
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
        const auto spriteAssetPath = BuildSpriteAssetFilePath(rootDirectory, entity.GetName(), targetDirectory);
        if (false == spriteAssetPath.has_value())
        {
            return false;
        }

        std::error_code errorCode;
        std::filesystem::create_directories(spriteAssetPath->parent_path(), errorCode);
        if (errorCode)
        {
            return false;
        }

        const Game::Transform& transform = entity.GetTransform();
        const auto spriteComponent = entity.GetSpriteComponent();
        Core::AssetId spriteAssetRef{};
        Core::AssetId textureAssetId{};
        Core::AssetId scriptAssetId{};
        Core::AssetId materialAssetId{};
        Core::AssetId collider2DAssetId{};
        std::uint32_t textureWidth = 0;
        std::uint32_t textureHeight = 0;
        Game::SpriteRenderSettings renderSettings{};

        if (spriteComponent.has_value())
        {
            spriteAssetRef = spriteComponent->get().spriteAssetRef;
            materialAssetId = spriteComponent->get().materialAssetRef;
            renderSettings = spriteComponent->get().renderSettings;

            const auto spriteAsset = m_spriteAssetRegistry.ResolveSpriteAsset(spriteAssetRef);
            if (spriteAsset.has_value())
            {
                textureAssetId = spriteAsset->textureAssetId;
                scriptAssetId = spriteAsset->scriptAssetId;
                if (materialAssetId.IsEmpty())
                {
                    materialAssetId = spriteAsset->materialAssetId;
                }
                collider2DAssetId = spriteAsset->collider2DAssetId;
                const auto texture = m_textureAssetRegistry.ResolveTexture(textureAssetId);
                if (static_cast<bool>(texture))
                {
                    textureWidth = texture->GetWidth();
                    textureHeight = texture->GetHeight();
                }
            }
        }

        if (scriptAssetId.IsEmpty() && std::filesystem::exists(*spriteAssetPath))
        {
            std::ifstream existingInput(*spriteAssetPath, std::ios::binary);
            if (existingInput.is_open())
            {
                std::ostringstream buffer;
                buffer << existingInput.rdbuf();
                const auto loadResult = Game::Assets::SpriteAssetLoader::LoadFromText(buffer.str());
                if (loadResult.IsSuccess() && loadResult.asset.has_value())
                {
                    scriptAssetId = loadResult.asset->scriptAssetId;
                    if (materialAssetId.IsEmpty())
                    {
                        materialAssetId = loadResult.asset->materialAssetId;
                    }
                    collider2DAssetId = loadResult.asset->collider2DAssetId;
                }
            }
        }

        std::ofstream output(*spriteAssetPath, std::ios::binary | std::ios::trunc);
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
        text << "materialAssetId=" << materialAssetId.GetValue() << "\n";
        text << "collider2DAssetId=" << collider2DAssetId.GetValue() << "\n";
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
        if (false == static_cast<bool>(output))
        {
            return false;
        }
        output.close();

        return RegisterSpriteAssetFile(*spriteAssetPath);
    }

    bool EditorSceneDocument::CreateSpriteAssetFile(const std::filesystem::path& targetDirectory)
    {
        if (false == m_project.GetInfo().has_value())
        {
            return false;
        }

        const std::filesystem::path rootDirectory = m_project.GetInfo()->projectFilePath.parent_path();
        const std::filesystem::path spriteAssetPath =
            BuildUniqueSpriteAssetFilePath(rootDirectory, targetDirectory);
        if (false == EditorPathSecurity::IsPathInsideOrEqual(spriteAssetPath, rootDirectory))
        {
            return false;
        }

        std::error_code errorCode;
        std::filesystem::create_directories(spriteAssetPath.parent_path(), errorCode);
        if (errorCode)
        {
            return false;
        }

        const Game::SpriteRenderSettings renderSettings{};
        std::ofstream output(spriteAssetPath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return false;
        }

        std::ostringstream text;
        text << std::fixed << std::setprecision(6);
        text << "magic=XelqoriaSpriteAsset\n";
        text << "version=1\n";
        text << "name=\"NewSprite\"\n";
        text << "transform.position=0.000000,0.000000,0.000000\n";
        text << "transform.rotation=0.000000,0.000000,0.000000\n";
        text << "transform.scale=1.000000,1.000000,1.000000\n";
        text << "hasSpriteComponent=false\n";
        text << "spriteAssetRef=\n";
        text << "textureAssetId=\n";
        text << "scriptAssetId=\n";
        text << "materialAssetId=\n";
        text << "collider2DAssetId=\n";
        text << "texture.size=0,0\n";
        text << "render.visible=" << (renderSettings.visible ? "true" : "false") << "\n";
        text << "render.sortOrder=" << renderSettings.sortOrder << "\n";
        text << "render.opacity=" << renderSettings.opacity << "\n";
        text << "render.color="
            << renderSettings.color[0] << ","
            << renderSettings.color[1] << ","
            << renderSettings.color[2] << ","
            << renderSettings.color[3] << "\n";

        output << text.str();
        if (false == static_cast<bool>(output))
        {
            return false;
        }
        output.close();

        return RegisterSpriteAssetFile(spriteAssetPath);
    }

    std::optional<Core::AssetId> EditorSceneDocument::EnsureSpriteAssetFileForEntity(
        Game::Entity& entity,
        const std::filesystem::path& targetDirectory)
    {
        const auto spriteComponent = entity.GetSpriteComponent();
        if (false == m_project.GetInfo().has_value()
            || false == spriteComponent.has_value()
            || true == entity.GetName().empty())
        {
            return std::nullopt;
        }

        if (ResolveSpriteAssetPath(spriteComponent->get().spriteAssetRef).has_value())
        {
            return spriteComponent->get().spriteAssetRef;
        }

        if (false == CreateSpriteAssetFile(entity, targetDirectory))
        {
            return std::nullopt;
        }

        const std::filesystem::path rootDirectory = m_project.GetInfo()->projectFilePath.parent_path();
        const auto spriteAssetPath = BuildSpriteAssetFilePath(rootDirectory, entity.GetName(), targetDirectory);
        if (false == spriteAssetPath.has_value())
        {
            return std::nullopt;
        }

        const Core::AssetId spriteAssetId =
            EditorAssetPathUtils::BuildSpriteAssetId(
                *spriteAssetPath,
                GetAssetIdBaseDirectory(*m_project.GetInfo(), *spriteAssetPath));
        if (spriteAssetId.IsEmpty())
        {
            return std::nullopt;
        }

        spriteComponent->get().spriteAssetRef = spriteAssetId;
        return spriteAssetId;
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

    const std::vector<std::wstring>& EditorSceneDocument::GetProjectMigrationMessages() const
    {
        return m_project.GetLastMigrationMessages();
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

    Game::Assets::SpriteMaterialAssetRegistry& EditorSceneDocument::GetMaterialAssetRegistry()
    {
        return m_materialAssetRegistry;
    }

    const Game::Assets::SpriteMaterialAssetRegistry& EditorSceneDocument::GetMaterialAssetRegistry() const
    {
        return m_materialAssetRegistry;
    }

    Game::Assets::Collider2DAssetRegistry& EditorSceneDocument::GetCollider2DAssetRegistry()
    {
        return m_collider2DAssetRegistry;
    }

    const Game::Assets::Collider2DAssetRegistry& EditorSceneDocument::GetCollider2DAssetRegistry() const
    {
        return m_collider2DAssetRegistry;
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
