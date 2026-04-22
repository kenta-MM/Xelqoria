#include "RecentProjectsStore.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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
}

TEST(RecentProjectsStoreTests, RecordKeepsMostRecentFiveExistingProjects)
{
    const std::filesystem::path originalCurrentPath = std::filesystem::current_path();
    const std::filesystem::path tempDirectory = MakeTempDirectory(L"XelqoriaRecentProjectsStoreTests");
    std::filesystem::current_path(tempDirectory);

    Xelqoria::Editor::RecentProjectsStore store{};
    for (int index = 0; index < 6; ++index)
    {
        const std::wstring projectName = L"Project" + std::to_wstring(index);
        const std::filesystem::path projectRoot = tempDirectory / projectName;
        std::filesystem::create_directories(projectRoot);
        const std::filesystem::path projectFilePath = projectRoot / (projectName + L".proj");
        std::ofstream(projectFilePath).put('\n');

        Xelqoria::Editor::EditorProjectInfo info{};
        info.name = projectName;
        info.projectFilePath = projectFilePath;
        info.rootDirectory = projectRoot;
        EXPECT_TRUE(store.Record(info));
    }

    const std::vector<Xelqoria::Editor::EditorProjectInfo> projects = store.Load();
    ASSERT_EQ(5u, projects.size());
    EXPECT_EQ(L"Project5", projects[0].name);
    EXPECT_EQ(L"Project1", projects[4].name);

    std::filesystem::current_path(originalCurrentPath);
    std::filesystem::remove_all(tempDirectory);
}
