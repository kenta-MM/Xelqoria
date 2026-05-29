#include "Project/EditorProject.h"

#include <algorithm>
#include <fstream>
#include <optional>
#include <string_view>
#include <system_error>

#include "Utils/EditorPathSecurity.h"
#include "Utils/EditorStringUtils.h"
#include "SceneSerializer.h"

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr const char* ProjectFileHeader = "XelqoriaProject=1";
        constexpr const wchar_t* InitialSceneFileName = L"Main.xelqoria.scene";

        [[nodiscard]] std::optional<std::string> ReadValue(std::string_view line, std::string_view key)
        {
            if (line.size() <= key.size() + 1 || line.substr(0, key.size()) != key || line[key.size()] != '=')
            {
                return std::nullopt;
            }

            return std::string(line.substr(key.size() + 1));
        }
    }

    bool EditorProject::Create(
        const std::wstring& projectName,
        const std::filesystem::path& parentDirectory,
        const Game::Scene& initialScene)
    {
        if (false == EditorPathSecurity::IsValidProjectName(projectName) || parentDirectory.empty())
        {
            return false;
        }

        EditorProjectInfo info{};
        info.name = projectName;
        info.rootDirectory = parentDirectory / projectName;
        info.projectFilePath = info.rootDirectory / (projectName + L".proj");
        info.scenesDirectory = info.rootDirectory / L"Scenes";
        info.activeScenePath = info.scenesDirectory / InitialSceneFileName;

        std::error_code errorCode;
        std::filesystem::create_directories(info.scenesDirectory, errorCode);
        if (errorCode)
        {
            return false;
        }

        if (false == WriteSceneFile(info.activeScenePath, initialScene))
        {
            return false;
        }

        if (false == WriteProjectFile(info))
        {
            return false;
        }

        m_info = info;
        return true;
    }

    bool EditorProject::Open(const std::filesystem::path& projectFilePath)
    {
        std::ifstream input(projectFilePath, std::ios::binary);
        if (false == input.is_open())
        {
            return false;
        }

        std::string header{};
        std::getline(input, header);
        if (header != ProjectFileHeader)
        {
            return false;
        }

        std::optional<std::string> projectName{};
        std::optional<std::string> activeScene{};
        std::string line{};
        while (std::getline(input, line))
        {
            if (const auto value = ReadValue(line, "Name"))
            {
                projectName = value;
            }
            else if (const auto value = ReadValue(line, "ActiveScene"))
            {
                activeScene = value;
            }
        }

        if (false == projectName.has_value() || false == activeScene.has_value())
        {
            return false;
        }

        if (false == EditorPathSecurity::IsValidProjectName(ToWideString(*projectName)))
        {
            return false;
        }

        const std::filesystem::path activeSceneRelativePath(*activeScene);
        if (false == EditorPathSecurity::IsSafeRelativePath(activeSceneRelativePath)
            || activeSceneRelativePath.extension() != ".scene")
        {
            return false;
        }

        EditorProjectInfo info{};
        info.name = ToWideString(*projectName);
        info.projectFilePath = projectFilePath;
        info.rootDirectory = projectFilePath.parent_path();
        info.scenesDirectory = info.rootDirectory / L"Scenes";
        info.activeScenePath = info.rootDirectory / activeSceneRelativePath;

        if (false == std::filesystem::exists(info.activeScenePath))
        {
            return false;
        }

        if (false == EditorPathSecurity::IsPathInsideOrEqual(info.activeScenePath, info.scenesDirectory))
        {
            return false;
        }

        m_info = info;
        return true;
    }

    bool EditorProject::Save(const Game::Scene& scene) const
    {
        if (false == m_info.has_value())
        {
            return false;
        }

        if (false == WriteSceneFile(m_info->activeScenePath, scene))
        {
            return false;
        }

        return WriteProjectFile(*m_info);
    }

    bool EditorProject::SaveAs(
        const std::wstring& projectName,
        const std::filesystem::path& parentDirectory,
        const Game::Scene& scene)
    {
        return Create(projectName, parentDirectory, scene);
    }

    bool EditorProject::SelectSceneFile(const std::filesystem::path& scenePath)
    {
        if (false == m_info.has_value())
        {
            return false;
        }

        std::filesystem::path selectedScenePath = scenePath;
        if (false == selectedScenePath.is_absolute())
        {
            selectedScenePath = m_info->rootDirectory / selectedScenePath;
        }

        if (false == std::filesystem::is_regular_file(selectedScenePath)
            || selectedScenePath.extension() != ".scene"
            || false == EditorPathSecurity::IsPathInsideOrEqual(selectedScenePath, m_info->scenesDirectory))
        {
            return false;
        }

        m_info->activeScenePath = selectedScenePath;
        return true;
    }

    std::vector<std::filesystem::path> EditorProject::EnumerateSceneFiles() const
    {
        std::vector<std::filesystem::path> sceneFiles{};
        if (false == m_info.has_value() || false == std::filesystem::exists(m_info->scenesDirectory))
        {
            return sceneFiles;
        }

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_info->scenesDirectory))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".scene")
            {
                sceneFiles.emplace_back(entry.path());
            }
        }

        std::sort(sceneFiles.begin(), sceneFiles.end());
        return sceneFiles;
    }

    bool EditorProject::HasProject() const
    {
        return m_info.has_value();
    }

    const std::optional<EditorProjectInfo>& EditorProject::GetInfo() const
    {
        return m_info;
    }

    bool EditorProject::WriteProjectFile(const EditorProjectInfo& info) const
    {
        if (false == EditorPathSecurity::IsValidProjectName(info.name)
            || false == EditorPathSecurity::IsPathInsideOrEqual(info.activeScenePath, info.scenesDirectory))
        {
            return false;
        }

        std::error_code errorCode;
        std::filesystem::create_directories(info.rootDirectory, errorCode);
        if (errorCode)
        {
            return false;
        }

        std::ofstream output(info.projectFilePath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return false;
        }

        const std::filesystem::path relativeScenePath = std::filesystem::relative(info.activeScenePath, info.rootDirectory, errorCode);
        if (errorCode)
        {
            return false;
        }

        output << ProjectFileHeader << '\n';
        output << "Name=" << ToNarrowString(info.name) << '\n';
        output << "ActiveScene=" << relativeScenePath.generic_string() << '\n';
        return output.good();
    }

    bool EditorProject::WriteSceneFile(
        const std::filesystem::path& scenePath,
        const Game::Scene& scene) const
    {
        std::error_code errorCode;
        std::filesystem::create_directories(scenePath.parent_path(), errorCode);
        if (errorCode)
        {
            return false;
        }

        std::ofstream output(scenePath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return false;
        }

        output << Game::SceneSerializer::SaveToText(scene);
        return output.good();
    }
}
