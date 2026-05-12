#include "ScriptAssetService.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <gtest/gtest.h>

namespace
{
    [[nodiscard]] std::filesystem::path MakeTempDirectory(const std::wstring& name)
    {
        const std::filesystem::path directory = std::filesystem::temp_directory_path() / name;
        std::filesystem::remove_all(directory);
        std::filesystem::create_directories(directory);
        return directory;
    }

    [[nodiscard]] std::string ReadTextFile(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        std::ostringstream buffer;
        buffer << input.rdbuf();
        return buffer.str();
    }
}

TEST(ScriptAssetServiceTests, CreateScriptAssetWritesManifestAndInitialCode)
{
    const std::filesystem::path projectRoot = MakeTempDirectory(L"XelqoriaScriptAssetServiceTests_Create");
    const std::filesystem::path targetDirectory = projectRoot / L"Scripts";
    std::filesystem::create_directories(targetDirectory);

    const Xelqoria::Editor::ScriptAssetCreationResult result =
        Xelqoria::Editor::ScriptAssetService::CreateScriptAsset(projectRoot, targetDirectory);

    ASSERT_TRUE(result.succeeded);
    EXPECT_EQ(targetDirectory / L"NewScript.script", result.assetPath);
    EXPECT_EQ(projectRoot / L".xelqoria" / L"Scripts" / L"Scripts_NewScript.cpp", result.sourcePath);
    EXPECT_EQ("scripts/Scripts/NewScript.script", result.scriptAssetId.GetValue());

    const std::string manifest = ReadTextFile(result.assetPath);
    EXPECT_NE(std::string::npos, manifest.find("magic=XelqoriaScriptAsset\n"));
    EXPECT_NE(std::string::npos, manifest.find("language=cpp\n"));
    EXPECT_NE(std::string::npos, manifest.find("name=\"NewScript\"\n"));
    EXPECT_NE(std::string::npos, manifest.find("source=\".xelqoria/Scripts/Scripts_NewScript.cpp\"\n"));

    const std::string source = ReadTextFile(result.sourcePath);
    EXPECT_NE(std::string::npos, source.find("void Start()"));
    EXPECT_NE(std::string::npos, source.find("void Update(float deltaTime)"));

    std::filesystem::remove_all(projectRoot);
}

TEST(ScriptAssetServiceTests, CreateScriptAssetUsesUniqueNames)
{
    const std::filesystem::path projectRoot = MakeTempDirectory(L"XelqoriaScriptAssetServiceTests_Unique");
    std::filesystem::create_directories(projectRoot);

    const Xelqoria::Editor::ScriptAssetCreationResult first =
        Xelqoria::Editor::ScriptAssetService::CreateScriptAsset(projectRoot, projectRoot);
    const Xelqoria::Editor::ScriptAssetCreationResult second =
        Xelqoria::Editor::ScriptAssetService::CreateScriptAsset(projectRoot, projectRoot);

    ASSERT_TRUE(first.succeeded);
    ASSERT_TRUE(second.succeeded);
    EXPECT_EQ(projectRoot / L"NewScript.script", first.assetPath);
    EXPECT_EQ(projectRoot / L"NewScript1.script", second.assetPath);
    EXPECT_TRUE(std::filesystem::exists(first.sourcePath));
    EXPECT_TRUE(std::filesystem::exists(second.sourcePath));

    std::filesystem::remove_all(projectRoot);
}

TEST(ScriptAssetServiceTests, CreateScriptAssetRejectsOutsideTargetDirectory)
{
    const std::filesystem::path projectRoot = MakeTempDirectory(L"XelqoriaScriptAssetServiceTests_Project");
    const std::filesystem::path outsideRoot = MakeTempDirectory(L"XelqoriaScriptAssetServiceTests_Outside");

    const Xelqoria::Editor::ScriptAssetCreationResult result =
        Xelqoria::Editor::ScriptAssetService::CreateScriptAsset(projectRoot, outsideRoot);

    EXPECT_FALSE(result.succeeded);
    EXPECT_FALSE(std::filesystem::exists(outsideRoot / L"NewScript.script"));

    std::filesystem::remove_all(projectRoot);
    std::filesystem::remove_all(outsideRoot);
}

TEST(ScriptAssetServiceTests, ResolveSourcePathReadsManagedCodePathFromManifest)
{
    const std::filesystem::path projectRoot = MakeTempDirectory(L"XelqoriaScriptAssetServiceTests_Resolve");
    const std::filesystem::path targetDirectory = projectRoot / L"Assets";
    std::filesystem::create_directories(targetDirectory);

    const Xelqoria::Editor::ScriptAssetCreationResult created =
        Xelqoria::Editor::ScriptAssetService::CreateScriptAsset(projectRoot, targetDirectory);

    ASSERT_TRUE(created.succeeded);
    const auto sourcePath =
        Xelqoria::Editor::ScriptAssetService::ResolveSourcePath(projectRoot, created.assetPath);

    ASSERT_TRUE(sourcePath.has_value());
    EXPECT_EQ(created.sourcePath, *sourcePath);

    std::filesystem::remove_all(projectRoot);
}

TEST(ScriptAssetServiceTests, ResolveSourcePathRejectsUnsafeManifestSource)
{
    const std::filesystem::path projectRoot = MakeTempDirectory(L"XelqoriaScriptAssetServiceTests_UnsafeSource");
    const std::filesystem::path assetPath = projectRoot / L"Unsafe.script";
    {
        std::ofstream output(assetPath, std::ios::binary | std::ios::trunc);
        output << "magic=XelqoriaScriptAsset\n";
        output << "version=1\n";
        output << "source=\"../Outside.cpp\"\n";
    }

    const auto sourcePath =
        Xelqoria::Editor::ScriptAssetService::ResolveSourcePath(projectRoot, assetPath);

    EXPECT_FALSE(sourcePath.has_value());

    std::filesystem::remove_all(projectRoot);
}
