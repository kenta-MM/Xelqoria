#include "Project/EditorProject.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <optional>
#include <sstream>
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
        constexpr std::uint32_t CurrentProjectStructureVersion = 1;
        constexpr const wchar_t* ProjectFileExtension = L".xelqoria";
        constexpr const wchar_t* AssetsDirectoryName = L"Assets";
        constexpr const wchar_t* ScenesDirectoryName = L"Scenes";
        constexpr const wchar_t* TexturesDirectoryName = L"Textures";
        constexpr const wchar_t* MaterialsDirectoryName = L"Materials";
        constexpr const wchar_t* ScriptsDirectoryName = L"Scripts";
        constexpr const wchar_t* ProjectSettingsDirectoryName = L"ProjectSettings";
        constexpr const wchar_t* ProjectSettingsFileName = L"project_settings.json";
        constexpr const wchar_t* InternalDirectoryName = L".xelqoria";
        constexpr const wchar_t* CacheDirectoryName = L"cache";
        constexpr const wchar_t* InitialSceneFileName = L"Main.scene";
        constexpr const char* ProjectVersion = "1";

        [[nodiscard]] std::optional<std::string> ReadValue(std::string_view line, std::string_view key)
        {
            if (line.size() <= key.size() + 1 || line.substr(0, key.size()) != key || line[key.size()] != '=')
            {
                return std::nullopt;
            }

            return std::string(line.substr(key.size() + 1));
        }

        [[nodiscard]] std::string ToProjectRelativeGenericString(
            const std::filesystem::path& path,
            const std::filesystem::path& rootDirectory,
            std::error_code& errorCode)
        {
            const std::filesystem::path relativePath = std::filesystem::relative(path, rootDirectory, errorCode);
            if (errorCode)
            {
                return {};
            }

            return relativePath.generic_string();
        }

        [[nodiscard]] std::string BuildDefaultProjectSettingsText(const std::wstring& projectName)
        {
            std::ostringstream text;
            text << "{\n";
            text << "  \"projectName\": \"" << ToNarrowString(projectName) << "\"\n";
            text << "}\n";
            return text.str();
        }

        [[nodiscard]] bool WriteTextFile(
            const std::filesystem::path& filePath,
            std::string_view text)
        {
            std::error_code errorCode;
            std::filesystem::create_directories(filePath.parent_path(), errorCode);
            if (errorCode)
            {
                return false;
            }

            std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
            if (false == output.is_open())
            {
                return false;
            }

            output << text;
            return output.good();
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
        info.projectFilePath = info.rootDirectory / (projectName + ProjectFileExtension);
        info.assetRootDirectory = info.rootDirectory / AssetsDirectoryName;
        info.scenesDirectory = info.assetRootDirectory / ScenesDirectoryName;
        info.projectSettingsFilePath = info.rootDirectory / ProjectSettingsDirectoryName / ProjectSettingsFileName;
        info.internalDirectory = info.rootDirectory / InternalDirectoryName;
        info.cacheDirectory = info.internalDirectory / CacheDirectoryName;
        info.activeScenePath = info.scenesDirectory / InitialSceneFileName;

        std::error_code errorCode;
        const std::array<std::filesystem::path, 6> requiredDirectories =
        {
            info.scenesDirectory,
            info.assetRootDirectory / TexturesDirectoryName,
            info.assetRootDirectory / MaterialsDirectoryName,
            info.assetRootDirectory / ScriptsDirectoryName,
            info.projectSettingsFilePath.parent_path(),
            info.cacheDirectory
        };

        for (const std::filesystem::path& directory : requiredDirectories)
        {
            std::filesystem::create_directories(directory, errorCode);
            if (errorCode)
            {
                return false;
            }
        }

        if (false == WriteTextFile(info.projectSettingsFilePath, BuildDefaultProjectSettingsText(projectName)))
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
        std::optional<std::string> assetRoot{};
        std::optional<std::string> projectSettings{};
        std::optional<std::string> startupScene{};
        std::string line{};
        while (std::getline(input, line))
        {
            if (const auto value = ReadValue(line, "projectName"))
            {
                projectName = value;
            }
            else if (const auto value = ReadValue(line, "Name"))
            {
                projectName = value;
            }
            else if (const auto value = ReadValue(line, "startupScene"))
            {
                startupScene = value;
            }
            else if (const auto value = ReadValue(line, "ActiveScene"))
            {
                activeScene = value;
            }
            else if (const auto value = ReadValue(line, "assetRoot"))
            {
                assetRoot = value;
            }
            else if (const auto value = ReadValue(line, "projectSettings"))
            {
                projectSettings = value;
            }
        }

        const std::optional<std::string>& scenePathText = startupScene.has_value()
            ? startupScene
            : activeScene;
        if (false == projectName.has_value() || false == scenePathText.has_value())
        {
            return false;
        }

        if (false == EditorPathSecurity::IsValidProjectName(ToWideString(*projectName)))
        {
            return false;
        }

        const std::filesystem::path activeSceneRelativePath(*scenePathText);
        if (false == EditorPathSecurity::IsSafeRelativePath(activeSceneRelativePath)
            || activeSceneRelativePath.extension() != ".scene")
        {
            return false;
        }

        const bool isModernProjectFile = projectFilePath.extension() == ProjectFileExtension;
        const std::filesystem::path assetRootRelativePath = assetRoot.has_value()
            ? std::filesystem::path(*assetRoot)
            : std::filesystem::path(AssetsDirectoryName);
        const std::filesystem::path projectSettingsRelativePath = projectSettings.has_value()
            ? std::filesystem::path(*projectSettings)
            : std::filesystem::path(ProjectSettingsDirectoryName) / ProjectSettingsFileName;
        if ((isModernProjectFile && false == assetRoot.has_value())
            || false == EditorPathSecurity::IsSafeRelativePath(assetRootRelativePath)
            || false == EditorPathSecurity::IsSafeRelativePath(projectSettingsRelativePath))
        {
            return false;
        }

        EditorProjectInfo info{};
        info.name = ToWideString(*projectName);
        info.projectFilePath = projectFilePath;
        info.rootDirectory = projectFilePath.parent_path();
        info.assetRootDirectory = info.rootDirectory / assetRootRelativePath;
        info.scenesDirectory = info.assetRootDirectory / ScenesDirectoryName;
        info.projectSettingsFilePath = info.rootDirectory / projectSettingsRelativePath;
        info.internalDirectory = info.rootDirectory / InternalDirectoryName;
        info.cacheDirectory = info.internalDirectory / CacheDirectoryName;
        info.activeScenePath = info.rootDirectory / activeSceneRelativePath;

        if (false == std::filesystem::exists(info.activeScenePath))
        {
            return false;
        }

        const std::filesystem::path expectedSceneRoot = isModernProjectFile
            ? info.scenesDirectory
            : info.rootDirectory / ScenesDirectoryName;
        if (false == EditorPathSecurity::IsPathInsideOrEqual(info.activeScenePath, expectedSceneRoot))
        {
            return false;
        }

        if (false == isModernProjectFile)
        {
            info.assetRootDirectory = info.rootDirectory;
            info.scenesDirectory = expectedSceneRoot;
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
            || false == EditorPathSecurity::IsPathInsideOrEqual(info.activeScenePath, info.scenesDirectory)
            || false == EditorPathSecurity::IsPathInsideOrEqual(info.scenesDirectory, info.assetRootDirectory)
            || false == EditorPathSecurity::IsPathInsideOrEqual(info.projectSettingsFilePath, info.rootDirectory))
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

        const std::string relativeScenePath = ToProjectRelativeGenericString(info.activeScenePath, info.rootDirectory, errorCode);
        if (errorCode)
        {
            return false;
        }

        const std::string relativeAssetRoot = ToProjectRelativeGenericString(info.assetRootDirectory, info.rootDirectory, errorCode);
        if (errorCode)
        {
            return false;
        }

        const std::string relativeProjectSettings = ToProjectRelativeGenericString(info.projectSettingsFilePath, info.rootDirectory, errorCode);
        if (errorCode)
        {
            return false;
        }

        output << ProjectFileHeader << '\n';
        output << "projectName=" << ToNarrowString(info.name) << '\n';
        output << "version=" << ProjectVersion << '\n';
        output << "projectStructureVersion=" << CurrentProjectStructureVersion << '\n';
        output << "assetRoot=" << relativeAssetRoot << '\n';
        output << "projectSettings=" << relativeProjectSettings << '\n';
        output << "startupScene=" << relativeScenePath << '\n';
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
