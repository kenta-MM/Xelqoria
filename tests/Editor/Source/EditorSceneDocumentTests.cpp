#include "EditorSceneDocument.h"

#include <filesystem>
#include <fstream>
#include <memory>

#include <gtest/gtest.h>

#include "Assets/SpriteAsset.h"
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
