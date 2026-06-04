#include "Project/EditorProject.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cwctype>
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

        struct ProjectMigrationResult
        {
            bool attempted = false;
            bool succeeded = true;
            std::filesystem::path startupScenePath{};
            std::filesystem::path projectSettingsFilePath{};
            std::vector<std::wstring> messages{};
        };

        [[nodiscard]] std::optional<std::string> ReadValue(std::string_view line, std::string_view key)
        {
            if (line.size() <= key.size() + 1 || line.substr(0, key.size()) != key || line[key.size()] != '=')
            {
                return std::nullopt;
            }

            return std::string(line.substr(key.size() + 1));
        }

        [[nodiscard]] std::optional<std::uint32_t> ParseUnsignedValue(const std::optional<std::string>& value)
        {
            if (false == value.has_value())
            {
                return std::nullopt;
            }

            try
            {
                return static_cast<std::uint32_t>(std::stoul(*value));
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        [[nodiscard]] std::wstring ToLowerPathPart(std::wstring text)
        {
            for (wchar_t& character : text)
            {
                character = static_cast<wchar_t>(std::towlower(character));
            }

            return text;
        }

        [[nodiscard]] bool IsExcludedRootDirectory(const std::filesystem::path& directory)
        {
            const std::wstring name = ToLowerPathPart(directory.filename().wstring());
            return name == L"assets"
                || name == L"projectsettings"
                || name == L".xelqoria"
                || name == L".git"
                || name == L".vs"
                || name == L"build"
                || name == L"bin"
                || name == L"obj"
                || name == L"out";
        }

        [[nodiscard]] bool IsMaterialJsonFile(const std::filesystem::path& path)
        {
            const std::wstring fileName = ToLowerPathPart(path.filename().wstring());
            return ToLowerPathPart(path.extension().wstring()) == L".json"
                && ((fileName.size() > std::wstring(L"_material.json").size()
                        && fileName.ends_with(L"_material.json"))
                    || fileName.starts_with(L"material_"));
        }

        [[nodiscard]] std::optional<std::filesystem::path> ResolveRootFileMigrationDirectory(
            const std::filesystem::path& rootDirectory,
            const std::filesystem::path& filePath)
        {
            const std::wstring fileName = ToLowerPathPart(filePath.filename().wstring());
            const std::wstring extension = ToLowerPathPart(filePath.extension().wstring());
            if (extension == L".scene")
            {
                return rootDirectory / AssetsDirectoryName / ScenesDirectoryName;
            }

            if (extension == L".png"
                || extension == L".jpg"
                || extension == L".jpeg"
                || extension == L".bmp"
                || extension == L".tga")
            {
                return rootDirectory / AssetsDirectoryName / TexturesDirectoryName;
            }

            if (extension == L".material" || extension == L".mat" || IsMaterialJsonFile(filePath))
            {
                return rootDirectory / AssetsDirectoryName / MaterialsDirectoryName;
            }

            if (extension == L".cpp" || extension == L".h" || extension == L".hpp" || extension == L".cs")
            {
                return rootDirectory / AssetsDirectoryName / ScriptsDirectoryName;
            }

            if (fileName == L"project_settings.json")
            {
                return rootDirectory / ProjectSettingsDirectoryName;
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::filesystem::path> ResolveRootDirectoryMigrationDirectory(
            const std::filesystem::path& rootDirectory,
            const std::filesystem::path& directoryPath)
        {
            const std::wstring name = ToLowerPathPart(directoryPath.filename().wstring());
            if (name == L"scenes")
            {
                return rootDirectory / AssetsDirectoryName / ScenesDirectoryName;
            }

            if (name == L"textures")
            {
                return rootDirectory / AssetsDirectoryName / TexturesDirectoryName;
            }

            if (name == L"materials")
            {
                return rootDirectory / AssetsDirectoryName / MaterialsDirectoryName;
            }

            if (name == L"scripts")
            {
                return rootDirectory / AssetsDirectoryName / ScriptsDirectoryName;
            }

            return std::nullopt;
        }

        [[nodiscard]] std::filesystem::path BuildUniquePath(const std::filesystem::path& targetPath)
        {
            if (false == std::filesystem::exists(targetPath))
            {
                return targetPath;
            }

            const std::filesystem::path parentPath = targetPath.parent_path();
            const std::wstring stem = targetPath.stem().wstring();
            const std::wstring extension = targetPath.extension().wstring();
            for (int index = 1; ; ++index)
            {
                const std::filesystem::path candidate =
                    parentPath / (stem + L"_" + std::to_wstring(index) + extension);
                if (false == std::filesystem::exists(candidate))
                {
                    return candidate;
                }
            }
        }

        [[nodiscard]] bool IsSamePath(
            const std::filesystem::path& lhs,
            const std::filesystem::path& rhs)
        {
            if (lhs.empty() || rhs.empty())
            {
                return false;
            }

            return EditorPathSecurity::NormalizeForContainment(lhs)
                == EditorPathSecurity::NormalizeForContainment(rhs);
        }

        [[nodiscard]] std::optional<std::filesystem::path> TryMovePath(
            const std::filesystem::path& sourcePath,
            const std::filesystem::path& targetDirectory,
            ProjectMigrationResult& result,
            const std::filesystem::path& oldStartupScenePath,
            std::filesystem::path& newStartupScenePath)
        {
            std::error_code errorCode;
            std::filesystem::create_directories(targetDirectory, errorCode);
            if (errorCode)
            {
                result.succeeded = false;
                result.messages.push_back(L"Project migration error: failed to create " + targetDirectory.wstring());
                return std::nullopt;
            }

            const std::filesystem::path targetPath = BuildUniquePath(targetDirectory / sourcePath.filename());
            std::filesystem::rename(sourcePath, targetPath, errorCode);
            if (errorCode)
            {
                result.succeeded = false;
                result.messages.push_back(L"Project migration error: failed to move " + sourcePath.wstring());
                return std::nullopt;
            }

            if (IsSamePath(sourcePath, oldStartupScenePath))
            {
                newStartupScenePath = targetPath;
            }
            else if (std::filesystem::is_directory(targetPath, errorCode)
                && false == static_cast<bool>(errorCode)
                && EditorPathSecurity::IsPathInsideOrEqual(oldStartupScenePath, sourcePath))
            {
                errorCode.clear();
                const std::filesystem::path relativeStartupScene =
                    std::filesystem::relative(oldStartupScenePath, sourcePath, errorCode);
                if (false == static_cast<bool>(errorCode))
                {
                    newStartupScenePath = targetPath / relativeStartupScene;
                }
            }

            result.messages.push_back(L"Project migration: moved " + sourcePath.filename().wstring()
                + L" -> " + targetPath.wstring());
            return targetPath;
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

        [[nodiscard]] bool WriteProjectMetadataFile(const EditorProjectInfo& info)
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

        [[nodiscard]] ProjectMigrationResult MigrateProjectStructure(
            EditorProjectInfo& info,
            const std::filesystem::path& sourceProjectFilePath,
            const std::filesystem::path& oldStartupSceneRelativePath)
        {
            ProjectMigrationResult result{};
            result.attempted = true;
            result.startupScenePath = info.rootDirectory / oldStartupSceneRelativePath;
            result.projectSettingsFilePath = info.projectSettingsFilePath;
            result.messages.push_back(L"Project migration: started");

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
                    result.succeeded = false;
                    result.messages.push_back(L"Project migration error: failed to create " + directory.wstring());
                    return result;
                }
            }

            const std::filesystem::path oldStartupScenePath = info.rootDirectory / oldStartupSceneRelativePath;
            std::filesystem::path newStartupScenePath = oldStartupScenePath;
            const std::filesystem::path newProjectFilePath = info.projectFilePath;

            for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(info.rootDirectory, errorCode))
            {
                if (errorCode)
                {
                    result.succeeded = false;
                    result.messages.push_back(L"Project migration error: failed to enumerate project root");
                    return result;
                }

                const std::filesystem::path entryPath = entry.path();
                if (IsSamePath(entryPath, sourceProjectFilePath) || IsSamePath(entryPath, newProjectFilePath))
                {
                    continue;
                }

                errorCode.clear();
                if (entry.is_directory(errorCode) && false == static_cast<bool>(errorCode))
                {
                    if (IsExcludedRootDirectory(entryPath))
                    {
                        continue;
                    }

                    const std::optional<std::filesystem::path> targetDirectory =
                        ResolveRootDirectoryMigrationDirectory(info.rootDirectory, entryPath);
                    if (targetDirectory.has_value())
                    {
                        std::filesystem::create_directories(*targetDirectory, errorCode);
                        if (errorCode)
                        {
                            result.succeeded = false;
                            result.messages.push_back(L"Project migration error: failed to create " + targetDirectory->wstring());
                            return result;
                        }

                        for (const std::filesystem::directory_entry& childEntry : std::filesystem::directory_iterator(entryPath, errorCode))
                        {
                            if (errorCode)
                            {
                                result.succeeded = false;
                                result.messages.push_back(L"Project migration error: failed to enumerate " + entryPath.wstring());
                                return result;
                            }

                            (void)TryMovePath(childEntry.path(), *targetDirectory, result, oldStartupScenePath, newStartupScenePath);
                        }

                        errorCode.clear();
                        std::filesystem::remove(entryPath, errorCode);
                    }

                    continue;
                }

                errorCode.clear();
                if (entry.is_regular_file(errorCode) && false == static_cast<bool>(errorCode))
                {
                    const std::optional<std::filesystem::path> targetDirectory =
                        ResolveRootFileMigrationDirectory(info.rootDirectory, entryPath);
                    if (targetDirectory.has_value())
                    {
                        const std::filesystem::path beforeMovePath = entryPath;
                        const std::optional<std::filesystem::path> movedPath =
                            TryMovePath(entryPath, *targetDirectory, result, oldStartupScenePath, newStartupScenePath);
                        if (ToLowerPathPart(beforeMovePath.filename().wstring()) == L"project_settings.json")
                        {
                            if (movedPath.has_value())
                            {
                                result.projectSettingsFilePath = *movedPath;
                            }
                        }
                    }
                }
            }

            if (false == std::filesystem::exists(result.projectSettingsFilePath))
            {
                result.projectSettingsFilePath = info.projectSettingsFilePath;
                if (false == WriteTextFile(result.projectSettingsFilePath, BuildDefaultProjectSettingsText(info.name)))
                {
                    result.succeeded = false;
                    result.messages.push_back(L"Project migration error: failed to create project settings");
                    return result;
                }
            }

            if (false == std::filesystem::exists(newStartupScenePath))
            {
                newStartupScenePath = info.scenesDirectory / InitialSceneFileName;
            }

            info.activeScenePath = newStartupScenePath;
            info.projectSettingsFilePath = result.projectSettingsFilePath;
            if (false == WriteProjectMetadataFile(info))
            {
                result.succeeded = false;
                result.messages.push_back(L"Project migration error: failed to write project file");
                return result;
            }

            result.startupScenePath = newStartupScenePath;
            result.messages.push_back(L"Project migration: completed");
            return result;
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
        m_lastMigrationMessages.clear();

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
        std::optional<std::string> projectStructureVersion{};
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
            else if (const auto value = ReadValue(line, "projectStructureVersion"))
            {
                projectStructureVersion = value;
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

        bool isModernProjectFile = projectFilePath.extension() == ProjectFileExtension;
        const std::optional<std::uint32_t> parsedProjectStructureVersion =
            ParseUnsignedValue(projectStructureVersion);
        const bool needsMigration = false == parsedProjectStructureVersion.has_value()
            || *parsedProjectStructureVersion < CurrentProjectStructureVersion;
        std::filesystem::path effectiveProjectFilePath = projectFilePath;
        std::filesystem::path assetRootRelativePath = assetRoot.has_value()
            ? std::filesystem::path(*assetRoot)
            : std::filesystem::path(AssetsDirectoryName);
        std::filesystem::path projectSettingsRelativePath = projectSettings.has_value()
            ? std::filesystem::path(*projectSettings)
            : std::filesystem::path(ProjectSettingsDirectoryName) / ProjectSettingsFileName;
        if ((isModernProjectFile && false == needsMigration && false == assetRoot.has_value())
            || false == EditorPathSecurity::IsSafeRelativePath(assetRootRelativePath)
            || false == EditorPathSecurity::IsSafeRelativePath(projectSettingsRelativePath))
        {
            return false;
        }

        EditorProjectInfo info{};
        info.name = ToWideString(*projectName);
        info.projectFilePath = effectiveProjectFilePath;
        info.rootDirectory = projectFilePath.parent_path();
        info.assetRootDirectory = info.rootDirectory / assetRootRelativePath;
        info.scenesDirectory = info.assetRootDirectory / ScenesDirectoryName;
        info.projectSettingsFilePath = info.rootDirectory / projectSettingsRelativePath;
        info.internalDirectory = info.rootDirectory / InternalDirectoryName;
        info.cacheDirectory = info.internalDirectory / CacheDirectoryName;
        info.activeScenePath = info.rootDirectory / activeSceneRelativePath;

        if (needsMigration)
        {
            info.projectFilePath = info.rootDirectory / (info.name + ProjectFileExtension);
            info.assetRootDirectory = info.rootDirectory / AssetsDirectoryName;
            info.scenesDirectory = info.assetRootDirectory / ScenesDirectoryName;
            info.projectSettingsFilePath = info.rootDirectory / ProjectSettingsDirectoryName / ProjectSettingsFileName;
            info.internalDirectory = info.rootDirectory / InternalDirectoryName;
            info.cacheDirectory = info.internalDirectory / CacheDirectoryName;

            ProjectMigrationResult migrationResult =
                MigrateProjectStructure(info, projectFilePath, activeSceneRelativePath);
            m_lastMigrationMessages = std::move(migrationResult.messages);
            if (false == migrationResult.succeeded)
            {
                return false;
            }

            isModernProjectFile = true;
            effectiveProjectFilePath = info.projectFilePath;
            assetRootRelativePath = AssetsDirectoryName;
            info.activeScenePath = migrationResult.startupScenePath;
        }

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

    const std::vector<std::wstring>& EditorProject::GetLastMigrationMessages() const
    {
        return m_lastMigrationMessages;
    }

    bool EditorProject::WriteProjectFile(const EditorProjectInfo& info) const
    {
        return WriteProjectMetadataFile(info);
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
