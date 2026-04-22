#include "EditorProject.h"

#include <filesystem>
#include <vector>

#include <gtest/gtest.h>

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

    [[nodiscard]] Xelqoria::Game::Scene MakeScene()
    {
        Xelqoria::Game::Scene scene{};
        scene.CreateEntity();
        return scene;
    }
}

TEST(EditorProjectTests, CreateBuildsProjectFileScenesDirectoryAndInitialScene)
{
    const std::filesystem::path parentDirectory = MakeTempDirectory(L"XelqoriaEditorProjectTests_Create");
    Xelqoria::Editor::EditorProject project{};

    EXPECT_TRUE(project.Create(L"SampleProject", parentDirectory, MakeScene()));

    const std::filesystem::path projectRoot = parentDirectory / L"SampleProject";
    EXPECT_TRUE(std::filesystem::exists(projectRoot / L"SampleProject.proj"));
    EXPECT_TRUE(std::filesystem::is_directory(projectRoot / L"Scenes"));
    EXPECT_TRUE(std::filesystem::exists(projectRoot / L"Scenes" / L"Main.xelqoria.scene"));
    EXPECT_TRUE(project.HasProject());

    std::filesystem::remove_all(parentDirectory);
}

TEST(EditorProjectTests, SaveAsSwitchesCurrentProject)
{
    const std::filesystem::path parentDirectory = MakeTempDirectory(L"XelqoriaEditorProjectTests_SaveAs");
    Xelqoria::Editor::EditorProject project{};

    EXPECT_TRUE(project.SaveAs(L"RenamedProject", parentDirectory, MakeScene()));

    ASSERT_TRUE(project.GetInfo().has_value());
    EXPECT_EQ(L"RenamedProject", project.GetInfo()->name);
    EXPECT_TRUE(std::filesystem::exists(parentDirectory / L"RenamedProject" / L"RenamedProject.proj"));

    std::filesystem::remove_all(parentDirectory);
}

TEST(EditorProjectTests, OpenReadsProjectAndEnumeratesScenes)
{
    const std::filesystem::path parentDirectory = MakeTempDirectory(L"XelqoriaEditorProjectTests_Open");
    Xelqoria::Editor::EditorProject createdProject{};
    ASSERT_TRUE(createdProject.Create(L"OpenProject", parentDirectory, MakeScene()));

    Xelqoria::Editor::EditorProject openedProject{};
    EXPECT_TRUE(openedProject.Open(parentDirectory / L"OpenProject" / L"OpenProject.proj"));

    const std::vector<std::filesystem::path> sceneFiles = openedProject.EnumerateSceneFiles();
    ASSERT_EQ(1u, sceneFiles.size());
    EXPECT_EQ(L"Main.xelqoria.scene", sceneFiles[0].filename().wstring());

    std::filesystem::remove_all(parentDirectory);
}

TEST(EditorProjectTests, SelectSceneFileUpdatesActiveScene)
{
    const std::filesystem::path parentDirectory = MakeTempDirectory(L"XelqoriaEditorProjectTests_SelectScene");
    Xelqoria::Editor::EditorProject project{};
    ASSERT_TRUE(project.Create(L"SceneProject", parentDirectory, MakeScene()));

    const std::filesystem::path mainScenePath = project.GetInfo()->activeScenePath;
    const std::filesystem::path secondScenePath = project.GetInfo()->scenesDirectory / L"Second.xelqoria.scene";
    std::filesystem::copy_file(mainScenePath, secondScenePath);

    EXPECT_TRUE(project.SelectSceneFile(secondScenePath));
    ASSERT_TRUE(project.GetInfo().has_value());
    EXPECT_EQ(secondScenePath, project.GetInfo()->activeScenePath);

    std::filesystem::remove_all(parentDirectory);
}
