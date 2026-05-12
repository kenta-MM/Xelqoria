#include "ScriptAssetService.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>

#include "EditorPathSecurity.h"
#include "EditorStringUtils.h"

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr const wchar_t* DefaultScriptName = L"NewScript";
        constexpr const wchar_t* ScriptAssetExtension = L".script";
        constexpr const wchar_t* ManagedScriptsDirectory = L".xelqoria/Scripts";

        /// <summary>
        /// Script Asset の既定名から未使用ファイルパスを取得する。
        /// </summary>
        /// <param name="targetDirectory">作成先ディレクトリ。</param>
        /// <returns>未使用の Script Asset ファイルパス。</returns>
        [[nodiscard]] std::filesystem::path FindAvailableScriptAssetPath(
            const std::filesystem::path& targetDirectory)
        {
            std::filesystem::path candidate = targetDirectory / (std::wstring(DefaultScriptName) + ScriptAssetExtension);
            for (int index = 1; std::filesystem::exists(candidate); ++index)
            {
                candidate = targetDirectory
                    / (std::wstring(DefaultScriptName) + std::to_wstring(index) + ScriptAssetExtension);
            }

            return candidate;
        }

        /// <summary>
        /// 管理コードファイル名に使えない文字を置換する。
        /// </summary>
        /// <param name="value">変換対象文字列。</param>
        /// <returns>安全なファイル名要素。</returns>
        [[nodiscard]] std::wstring SanitizeManagedCodeStem(std::wstring value)
        {
            for (wchar_t& character : value)
            {
                if (character == L'/'
                    || character == L'\\'
                    || character == L':'
                    || character == L'*'
                    || character == L'?'
                    || character == L'"'
                    || character == L'<'
                    || character == L'>'
                    || character == L'|')
                {
                    character = L'_';
                }
            }

            return value;
        }

        /// <summary>
        /// Script Asset への相対パスから管理 C++ ソースの相対パスを構築する。
        /// </summary>
        /// <param name="relativeAssetPath">プロジェクトルート基準の Script Asset パス。</param>
        /// <returns>プロジェクトルート基準の C++ ソースパス。</returns>
        [[nodiscard]] std::filesystem::path BuildManagedSourceRelativePath(
            const std::filesystem::path& relativeAssetPath)
        {
            std::filesystem::path sourceStem = relativeAssetPath;
            sourceStem.replace_extension();
            const std::wstring sourceFileName =
                SanitizeManagedCodeStem(sourceStem.generic_wstring()) + L".cpp";
            return std::filesystem::path(ManagedScriptsDirectory) / sourceFileName;
        }

        /// <summary>
        /// Script AssetId を構築する。
        /// </summary>
        /// <param name="relativeAssetPath">プロジェクトルート基準の Script Asset パス。</param>
        /// <returns>Script AssetId。</returns>
        [[nodiscard]] Core::AssetId BuildScriptAssetId(const std::filesystem::path& relativeAssetPath)
        {
            return Core::AssetId("scripts/" + ToNarrowString(relativeAssetPath.generic_wstring()));
        }

        /// <summary>
        /// Script Asset マニフェストを書き出す。
        /// </summary>
        /// <param name="assetPath">書き出し先 Script Asset ファイル。</param>
        /// <param name="scriptName">Script 表示名。</param>
        /// <param name="sourceRelativePath">管理 C++ ソースの相対パス。</param>
        /// <returns>書き出しに成功した場合は true。</returns>
        [[nodiscard]] bool WriteScriptAssetManifest(
            const std::filesystem::path& assetPath,
            const std::wstring& scriptName,
            const std::filesystem::path& sourceRelativePath)
        {
            std::ofstream output(assetPath, std::ios::binary | std::ios::trunc);
            if (false == output.is_open())
            {
                return false;
            }

            output << "magic=XelqoriaScriptAsset\n";
            output << "version=1\n";
            output << "language=cpp\n";
            output << "name=" << std::quoted(ToNarrowString(scriptName)) << "\n";
            output << "source=" << std::quoted(ToNarrowString(sourceRelativePath.generic_wstring())) << "\n";
            return output.good();
        }

        /// <summary>
        /// Start / Update を含む初期 C++ コードを書き出す。
        /// </summary>
        /// <param name="sourcePath">書き出し先 C++ ソースファイル。</param>
        /// <returns>書き出しに成功した場合は true。</returns>
        [[nodiscard]] bool WriteInitialScriptSource(const std::filesystem::path& sourcePath)
        {
            std::ofstream output(sourcePath, std::ios::binary | std::ios::trunc);
            if (false == output.is_open())
            {
                return false;
            }

            output
                << "void Start()\n"
                << "{\n"
                << "}\n"
                << "\n"
                << "void Update(float deltaTime)\n"
                << "{\n"
                << "    (void)deltaTime;\n"
                << "}\n";
            return output.good();
        }
    }

    bool ScriptAssetService::IsScriptAssetFile(const std::filesystem::path& path)
    {
        return path.extension() == ScriptAssetExtension;
    }

    ScriptAssetCreationResult ScriptAssetService::CreateScriptAsset(
        const std::filesystem::path& projectRootDirectory,
        const std::filesystem::path& targetDirectory)
    {
        ScriptAssetCreationResult result{};
        if (projectRootDirectory.empty() || targetDirectory.empty())
        {
            return result;
        }

        if (false == EditorPathSecurity::IsPathInsideOrEqual(targetDirectory, projectRootDirectory))
        {
            return result;
        }

        std::error_code errorCode;
        if (false == std::filesystem::is_directory(targetDirectory, errorCode) || errorCode)
        {
            return result;
        }

        const std::filesystem::path assetPath = FindAvailableScriptAssetPath(targetDirectory);
        const std::filesystem::path relativeAssetPath =
            std::filesystem::relative(assetPath, projectRootDirectory, errorCode);
        if (errorCode || false == EditorPathSecurity::IsSafeRelativePath(relativeAssetPath))
        {
            return result;
        }

        const std::filesystem::path sourceRelativePath = BuildManagedSourceRelativePath(relativeAssetPath);
        if (false == EditorPathSecurity::IsSafeRelativePath(sourceRelativePath))
        {
            return result;
        }

        const std::filesystem::path sourcePath = projectRootDirectory / sourceRelativePath;
        std::filesystem::create_directories(sourcePath.parent_path(), errorCode);
        if (errorCode)
        {
            return result;
        }

        const std::wstring scriptName = assetPath.stem().wstring();
        if (false == WriteScriptAssetManifest(assetPath, scriptName, sourceRelativePath))
        {
            return result;
        }

        if (false == WriteInitialScriptSource(sourcePath))
        {
            std::filesystem::remove(assetPath, errorCode);
            return result;
        }

        result.succeeded = true;
        result.assetPath = assetPath;
        result.sourcePath = sourcePath;
        result.scriptAssetId = BuildScriptAssetId(relativeAssetPath);
        return result;
    }
}
