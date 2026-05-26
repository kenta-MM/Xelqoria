#include "Project/EditorSceneDocument.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

#include <gtest/gtest.h>

#include "Assets/SpriteAsset.h"
#include "Collider2DComponent.h"
#include "Scene.h"

namespace
{
    [[nodiscard]] std::filesystem::path MakeTempDirectory(const std::wstring& name)
    {
        const std::filesystem::path directory = std::filesystem::temp_directory_path() / name;
        std::filesystem::remove_all(directory);
        std::filesystem::create_directories(directory);
        return directory;
    }

    void WriteSpriteAsset(
        const std::filesystem::path& path,
        const char* textureAssetId,
        const char* scriptAssetId)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "magic=XelqoriaSpriteAsset\n";
        output << "version=1\n";
        output << "name=\"Player\"\n";
        output << "textureAssetId=" << textureAssetId << "\n";
        output << "scriptAssetId=" << scriptAssetId << "\n";
    }

    void WriteMaterialAsset(
        const std::filesystem::path& path,
        const char* textureAssetId)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "magic=XelqoriaMaterialAsset\n";
        output << "version=1\n";
        output << "name=\"Player\"\n";
        output << "textureAssetId=" << textureAssetId << "\n";
        output << "color=1,1,1,1\n";
        output << "outline.enabled=false\n";
        output << "outline.thickness=1\n";
        output << "outline.color=1,1,0,1\n";
    }

    [[nodiscard]] Xelqoria::Editor::EditorSceneDocument MakeProjectDocument(
        const std::filesystem::path& parentDirectory)
    {
        auto scene = std::make_unique<Xelqoria::Game::Scene>();
        scene->CreateEntity();

        Xelqoria::Editor::EditorSceneDocument document{};
        document.ReplaceScene(std::move(scene));
        EXPECT_TRUE(document.CreateProject(L"ScriptInspectorProject", parentDirectory));
        return document;
    }
}

TEST(EditorSceneDocumentTests, ResolveSpriteAssetPathRequiresRegisteredProjectSpriteAsset)
{
    const std::filesystem::path parentDirectory =
        MakeTempDirectory(L"XelqoriaEditorSceneDocumentTests_ResolveSpriteAsset");
    Xelqoria::Editor::EditorSceneDocument document = MakeProjectDocument(parentDirectory);
    const std::filesystem::path projectRoot = parentDirectory / L"ScriptInspectorProject";
    const std::filesystem::path spriteAssetPath = projectRoot / L"Player.sprite";
    WriteSpriteAsset(spriteAssetPath, "textures/player", "");

    ASSERT_TRUE(document.RegisterSpriteAssetFile(spriteAssetPath));

    const auto resolvedPath =
        document.ResolveSpriteAssetPath(Xelqoria::Core::AssetId("sprites/Player.sprite"));
    ASSERT_TRUE(resolvedPath.has_value());
    EXPECT_EQ(spriteAssetPath, *resolvedPath);

    EXPECT_FALSE(document.ResolveSpriteAssetPath(Xelqoria::Core::AssetId("sprites/Missing.sprite")).has_value());
    EXPECT_FALSE(document.ResolveSpriteAssetPath(Xelqoria::Core::AssetId("textures/Player.sprite")).has_value());

    std::filesystem::remove_all(parentDirectory);
}

TEST(EditorSceneDocumentTests, ClearScriptAssetFromSpriteAssetUpdatesRegistryAndFile)
{
    const std::filesystem::path parentDirectory =
        MakeTempDirectory(L"XelqoriaEditorSceneDocumentTests_ClearScriptAsset");
    Xelqoria::Editor::EditorSceneDocument document = MakeProjectDocument(parentDirectory);
    const std::filesystem::path projectRoot = parentDirectory / L"ScriptInspectorProject";
    const std::filesystem::path spriteAssetPath = projectRoot / L"Player.sprite";
    const Xelqoria::Core::AssetId spriteAssetId("sprites/Player.sprite");
    WriteSpriteAsset(spriteAssetPath, "textures/player", "scripts/Player.script");

    ASSERT_TRUE(document.RegisterSpriteAssetFile(spriteAssetPath));
    ASSERT_TRUE(document.ClearScriptAssetFromSpriteAsset(spriteAssetId));

    const auto spriteAsset = document.GetSpriteAssetRegistry().ResolveSpriteAsset(spriteAssetId);
    ASSERT_TRUE(spriteAsset.has_value());
    EXPECT_TRUE(spriteAsset->scriptAssetId.IsEmpty());

    std::filesystem::remove_all(parentDirectory);
}

TEST(EditorSceneDocumentTests, CreateAndAssignScriptAssetToSpriteAssetWritesScriptReference)
{
    const std::filesystem::path parentDirectory =
        MakeTempDirectory(L"XelqoriaEditorSceneDocumentTests_CreateAndAssignScript");
    Xelqoria::Editor::EditorSceneDocument document = MakeProjectDocument(parentDirectory);
    const std::filesystem::path projectRoot = parentDirectory / L"ScriptInspectorProject";
    const std::filesystem::path spriteAssetPath = projectRoot / L"Player.sprite";
    const Xelqoria::Core::AssetId spriteAssetId("sprites/Player.sprite");
    WriteSpriteAsset(spriteAssetPath, "textures/player", "");

    ASSERT_TRUE(document.RegisterSpriteAssetFile(spriteAssetPath));
    const Xelqoria::Editor::ScriptAssetCreationResult result =
        document.CreateAndAssignScriptAssetToSpriteAsset(spriteAssetId);

    ASSERT_TRUE(result.succeeded);
    EXPECT_TRUE(std::filesystem::exists(result.assetPath));
    EXPECT_TRUE(std::filesystem::exists(result.sourcePath));

    const auto spriteAsset = document.GetSpriteAssetRegistry().ResolveSpriteAsset(spriteAssetId);
    ASSERT_TRUE(spriteAsset.has_value());
    EXPECT_EQ(result.scriptAssetId, spriteAsset->scriptAssetId);

    std::filesystem::remove_all(parentDirectory);
}

TEST(EditorSceneDocumentTests, EnsureMaterialAssetTextureFillsEmptyMaterialTexture)
{
    const std::filesystem::path parentDirectory =
        MakeTempDirectory(L"XelqoriaEditorSceneDocumentTests_EnsureMaterialTexture");
    Xelqoria::Editor::EditorSceneDocument document = MakeProjectDocument(parentDirectory);
    const std::filesystem::path projectRoot = parentDirectory / L"ScriptInspectorProject";
    const std::filesystem::path materialAssetPath = projectRoot / L"Player.material";
    const Xelqoria::Core::AssetId materialAssetId("materials/Player.material");
    const Xelqoria::Core::AssetId textureAssetId("textures/player.png");
    WriteMaterialAsset(materialAssetPath, "");

    ASSERT_TRUE(document.RegisterMaterialAssetFile(materialAssetPath));
    ASSERT_TRUE(document.EnsureMaterialAssetTexture(materialAssetId, textureAssetId));

    const auto materialAsset = document.GetMaterialAssetRegistry().ResolveMaterialAsset(materialAssetId);
    ASSERT_TRUE(materialAsset.has_value());
    EXPECT_EQ(textureAssetId, materialAsset->textureAssetId);

    std::ifstream input(materialAssetPath, std::ios::binary);
    ASSERT_TRUE(input.is_open());
    std::ostringstream buffer;
    buffer << input.rdbuf();
    EXPECT_NE(std::string::npos, buffer.str().find("textureAssetId=textures/player.png"));
    input.close();

    std::filesystem::remove_all(parentDirectory);
}

TEST(EditorSceneDocumentTests, CreateAndAssignCollider2DAssetToSpriteAssetWritesColliderReference)
{
    const std::filesystem::path parentDirectory =
        MakeTempDirectory(L"XelqoriaEditorSceneDocumentTests_CreateAndAssignCollider2D");
    Xelqoria::Editor::EditorSceneDocument document = MakeProjectDocument(parentDirectory);
    const std::filesystem::path projectRoot = parentDirectory / L"ScriptInspectorProject";
    const std::filesystem::path spriteAssetPath = projectRoot / L"Player.sprite";
    const Xelqoria::Core::AssetId spriteAssetId("sprites/Player.sprite");
    WriteSpriteAsset(spriteAssetPath, "textures/player", "");

    Xelqoria::Game::Collider2DComponent collider{};
    collider.isTrigger = true;
    collider.offset = { 2.0f, 3.0f };
    collider.size = { 4.0f, 5.0f };

    ASSERT_TRUE(document.RegisterSpriteAssetFile(spriteAssetPath));
    const std::optional<Xelqoria::Core::AssetId> collider2DAssetId =
        document.CreateAndAssignCollider2DAssetToSpriteAsset(spriteAssetId, collider);

    ASSERT_TRUE(collider2DAssetId.has_value());
    ASSERT_TRUE(document.ResolveCollider2DAssetPath(*collider2DAssetId).has_value());

    const auto spriteAsset = document.GetSpriteAssetRegistry().ResolveSpriteAsset(spriteAssetId);
    ASSERT_TRUE(spriteAsset.has_value());
    EXPECT_EQ(*collider2DAssetId, spriteAsset->collider2DAssetId);

    const auto collider2DAsset = document.GetCollider2DAssetRegistry().ResolveCollider2DAsset(*collider2DAssetId);
    ASSERT_TRUE(collider2DAsset.has_value());
    EXPECT_TRUE(collider2DAsset->collider.isTrigger);
    EXPECT_FLOAT_EQ(2.0f, collider2DAsset->collider.offset.x);
    EXPECT_FLOAT_EQ(5.0f, collider2DAsset->collider.size.y);

    std::filesystem::remove_all(parentDirectory);
}

TEST(EditorSceneDocumentTests, EnsureSpriteAssetFileForEntityCreatesFileAndRepointsImageSpriteRef)
{
    const std::filesystem::path parentDirectory =
        MakeTempDirectory(L"XelqoriaEditorSceneDocumentTests_EnsureSpriteAssetFile");
    Xelqoria::Editor::EditorSceneDocument document = MakeProjectDocument(parentDirectory);
    Xelqoria::Game::Scene* scene = document.GetScene();
    ASSERT_NE(nullptr, scene);

    auto& entity = scene->CreateEntity();
    entity.SetName("Player");
    entity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
        Xelqoria::Core::AssetId("sprites/player.png"),
        {
            true,
            0,
            1.0f
        }
    });
    document.GetSpriteAssetRegistry().RegisterSpriteAsset(
        Xelqoria::Core::AssetId("sprites/player.png"),
        Xelqoria::Game::Assets::SpriteAsset{ Xelqoria::Core::AssetId("textures/player.png") });

    const std::optional<Xelqoria::Core::AssetId> spriteAssetId =
        document.EnsureSpriteAssetFileForEntity(entity, {});

    ASSERT_TRUE(spriteAssetId.has_value());
    EXPECT_EQ(Xelqoria::Core::AssetId("sprites/Player.sprite"), *spriteAssetId);
    ASSERT_TRUE(entity.GetSpriteComponent().has_value());
    EXPECT_EQ(*spriteAssetId, entity.GetSpriteComponent()->get().spriteAssetRef);

    const auto spriteAsset = document.GetSpriteAssetRegistry().ResolveSpriteAsset(*spriteAssetId);
    ASSERT_TRUE(spriteAsset.has_value());
    EXPECT_EQ(Xelqoria::Core::AssetId("textures/player.png"), spriteAsset->textureAssetId);
    EXPECT_TRUE(std::filesystem::exists(parentDirectory / L"ScriptInspectorProject" / L"Player.sprite"));

    std::filesystem::remove_all(parentDirectory);
}
