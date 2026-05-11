#include "RecentProjectsStore.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <system_error>

#include "EditorStringUtils.h"

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr std::size_t MaxRecentProjectCount = 5;
    }

    std::vector<EditorProjectInfo> RecentProjectsStore::Load() const
    {
        std::vector<EditorProjectInfo> projects{};
        std::ifstream input(GetStorePath(), std::ios::binary);
        if (false == input.is_open())
        {
            return projects;
        }

        std::string line{};
        while (std::getline(input, line) && projects.size() < MaxRecentProjectCount)
        {
            const std::size_t separator = line.find('|');
            if (separator == std::string::npos)
            {
                continue;
            }

            const std::wstring name = ToWideString(line.substr(0, separator));
            const std::filesystem::path projectFilePath = ToWideString(line.substr(separator + 1));
            if (false == std::filesystem::exists(projectFilePath))
            {
                continue;
            }

            EditorProjectInfo info{};
            info.name = name;
            info.projectFilePath = projectFilePath;
            info.rootDirectory = projectFilePath.parent_path();
            info.scenesDirectory = info.rootDirectory / L"Scenes";
            projects.emplace_back(info);
        }

        return projects;
    }

    bool RecentProjectsStore::Record(const EditorProjectInfo& projectInfo) const
    {
        if (projectInfo.name.empty() || projectInfo.projectFilePath.empty())
        {
            return false;
        }

        std::vector<EditorProjectInfo> projects = Load();
        projects.erase(
            std::remove_if(
                projects.begin(),
                projects.end(),
                [&projectInfo](const EditorProjectInfo& existingProject)
                {
                    return existingProject.projectFilePath == projectInfo.projectFilePath;
                }),
            projects.end());
        projects.insert(projects.begin(), projectInfo);
        if (projects.size() > MaxRecentProjectCount)
        {
            projects.resize(MaxRecentProjectCount);
        }

        const std::filesystem::path storePath = GetStorePath();
        std::error_code errorCode;
        std::filesystem::create_directories(storePath.parent_path(), errorCode);
        if (errorCode)
        {
            return false;
        }

        std::ofstream output(storePath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return false;
        }

        for (const EditorProjectInfo& project : projects)
        {
            output << ToNarrowString(project.name)
                << '|'
                << ToNarrowString(project.projectFilePath.generic_wstring())
                << '\n';
        }

        return output.good();
    }

    std::filesystem::path RecentProjectsStore::GetStorePath() const
    {
        return std::filesystem::path("Saved") / "RecentProjects.txt";
    }
}
