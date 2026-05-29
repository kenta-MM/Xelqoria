#include "Project/EditorProject.h"

#include <filesystem>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>

#include "Scene.h"
#include "SceneSerializer.h"

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

TEST(EditorProjectTests, OpenPreservesUtf8ProjectName)
{
    const std::filesystem::path parentDirectory = MakeTempDirectory(L"XelqoriaEditorProjectTests_Utf8Name");
    Xelqoria::Editor::EditorProject createdProject{};
    ASSERT_TRUE(createdProject.Create(L"日本語Project", parentDirectory, MakeScene()));

    Xelqoria::Editor::EditorProject openedProject{};
    EXPECT_TRUE(openedProject.Open(createdProject.GetInfo()->projectFilePath));
    ASSERT_TRUE(openedProject.GetInfo().has_value());
    EXPECT_EQ(L"日本語Project", openedProject.GetInfo()->name);

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

TEST(EditorProjectTests, CreateRejectsUnsafeProjectNames)
{
    const std::filesystem::path parentDirectory = MakeTempDirectory(L"XelqoriaEditorProjectTests_UnsafeName");
    Xelqoria::Editor::EditorProject project{};

    EXPECT_FALSE(project.Create(L"..\\Outside", parentDirectory, MakeScene()));
    EXPECT_FALSE(project.Create(L"CON", parentDirectory, MakeScene()));
    EXPECT_FALSE(project.Create(L"Project.", parentDirectory, MakeScene()));

    std::filesystem::remove_all(parentDirectory);
}

TEST(EditorProjectTests, OpenRejectsActiveSceneOutsideProjectScenesDirectory)
{
    const std::filesystem::path parentDirectory = MakeTempDirectory(L"XelqoriaEditorProjectTests_RejectOutsideScene");
    const std::filesystem::path projectRoot = parentDirectory / L"UnsafeProject";
    const std::filesystem::path scenesDirectory = projectRoot / L"Scenes";
    std::filesystem::create_directories(scenesDirectory);

    const std::filesystem::path outsideScenePath = parentDirectory / L"Outside.xelqoria.scene";
    {
        std::ofstream outsideScene(outsideScenePath, std::ios::binary | std::ios::trunc);
        outsideScene << Xelqoria::Game::SceneSerializer::SaveToText(MakeScene());
    }

    const std::filesystem::path projectFilePath = projectRoot / L"UnsafeProject.proj";
    {
        std::ofstream projectFile(projectFilePath, std::ios::binary | std::ios::trunc);
        projectFile << "XelqoriaProject=1\n";
        projectFile << "Name=UnsafeProject\n";
        projectFile << "ActiveScene=../Outside.xelqoria.scene\n";
    }

    Xelqoria::Editor::EditorProject project{};
    EXPECT_FALSE(project.Open(projectFilePath));

    std::filesystem::remove_all(parentDirectory);
}

TEST(EditorProjectTests, SelectSceneFileRejectsOutsideSceneDirectory)
{
    const std::filesystem::path parentDirectory = MakeTempDirectory(L"XelqoriaEditorProjectTests_SelectOutsideScene");
    Xelqoria::Editor::EditorProject project{};
    ASSERT_TRUE(project.Create(L"SceneProject", parentDirectory, MakeScene()));

    const std::filesystem::path outsideScenePath = parentDirectory / L"Outside.xelqoria.scene";
    std::filesystem::copy_file(project.GetInfo()->activeScenePath, outsideScenePath);

    EXPECT_FALSE(project.SelectSceneFile(outsideScenePath));

    std::filesystem::remove_all(parentDirectory);
}
