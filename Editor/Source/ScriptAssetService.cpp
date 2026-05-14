#include "ScriptAssetService.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
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
        constexpr const wchar_t* ScriptApiHeaderFileName = L"XelqoriaScriptApi.h";
        constexpr const wchar_t* ScriptBuildDirectory = L".xelqoria/ScriptBuild";

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
        /// コマンドライン引数として渡す文字列を引用符で囲む。
        /// </summary>
        /// <param name="value">引用符で囲む値。</param>
        /// <returns>コマンドライン向け文字列。</returns>
        [[nodiscard]] std::wstring QuoteCommandArgument(const std::filesystem::path& value)
        {
            std::wstring quoted = L"\"";
            for (const wchar_t character : value.wstring())
            {
                if (L'"' == character)
                {
                    quoted += L'\\';
                }

                quoted += character;
            }

            quoted += L"\"";
            return quoted;
        }

        /// <summary>
        /// Script コンパイル用コマンドラインを構築する。
        /// </summary>
        /// <param name="options">Script ビルド設定。</param>
        /// <param name="sourcePath">コンパイル対象ソース。</param>
        /// <param name="objectPath">出力 object パス。</param>
        /// <param name="modulePath">出力 DLL パス。</param>
        /// <returns>実行するコマンドライン。</returns>
        [[nodiscard]] std::wstring BuildScriptCompileCommandLine(
            const ScriptBuildOptions& options,
            const std::filesystem::path& sourcePath,
            const std::filesystem::path& objectPath,
            const std::filesystem::path& modulePath)
        {
            std::wstring commandLine{};
            if (false == options.environmentSetupBatch.empty())
            {
                commandLine += L"call ";
                commandLine += QuoteCommandArgument(options.environmentSetupBatch);
                commandLine += L" -arch=amd64 >nul && ";
            }

            commandLine += L"call ";
            commandLine += QuoteCommandArgument(options.compilerExecutable);
            commandLine += L" /nologo /EHsc /std:c++20 /LD ";
            commandLine += QuoteCommandArgument(sourcePath);
            commandLine += L" /Fo";
            commandLine += QuoteCommandArgument(objectPath);
            commandLine += L" /Fe";
            commandLine += QuoteCommandArgument(modulePath);
            commandLine += L" 2>&1";
            return commandLine;
        }

        /// <summary>
        /// 外部コマンドを実行して標準出力と標準エラーを取得する。
        /// </summary>
        /// <param name="commandLine">実行するコマンドライン。</param>
        /// <param name="output">取得した出力。</param>
        /// <returns>プロセス終了コード。起動失敗時は -1。</returns>
        [[nodiscard]] int RunCommandAndCaptureOutput(const std::wstring& commandLine, std::wstring& output)
        {
#if defined(_WIN32)
            FILE* pipe = _popen(ToNarrowString(commandLine).c_str(), "r");
            if (nullptr == pipe)
            {
                output += L"コンパイラ プロセスを開始できませんでした。\n";
                return -1;
            }

            std::array<char, 512> buffer{};
            while (nullptr != fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
            {
                output += ToWideString(buffer.data());
            }

            return _pclose(pipe);
#else
            (void)commandLine;
            output += L"Script ビルドは Windows Editor でのみ実行できます。\n";
            return -1;
#endif
        }

        /// <summary>
        /// Script Asset ファイルをプロジェクト配下から列挙する。
        /// </summary>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <returns>Script Asset ファイル一覧。</returns>
        [[nodiscard]] std::vector<std::filesystem::path> EnumerateScriptAssetFiles(
            const std::filesystem::path& projectRootDirectory)
        {
            std::vector<std::filesystem::path> assetPaths{};
            std::error_code errorCode;
            for (const std::filesystem::directory_entry& entry :
                std::filesystem::recursive_directory_iterator(projectRootDirectory, errorCode))
            {
                if (errorCode)
                {
                    break;
                }

                if (entry.is_regular_file(errorCode)
                    && false == static_cast<bool>(errorCode)
                    && entry.path().extension() == ScriptAssetExtension)
                {
                    assetPaths.push_back(entry.path());
                }
            }

            std::sort(assetPaths.begin(), assetPaths.end());
            return assetPaths;
        }

        /// <summary>
        /// コンパイル出力先 object ファイルパスを作成する。
        /// </summary>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <param name="sourcePath">コンパイル対象ソース。</param>
        /// <param name="index">同名回避用インデックス。</param>
        /// <returns>object ファイルパス。</returns>
        [[nodiscard]] std::filesystem::path BuildObjectPath(
            const std::filesystem::path& projectRootDirectory,
            const std::filesystem::path& sourcePath,
            std::size_t index)
        {
            std::error_code errorCode;
            std::filesystem::path relativeSourcePath =
                std::filesystem::relative(sourcePath, projectRootDirectory, errorCode);
            if (errorCode || relativeSourcePath.empty())
            {
                relativeSourcePath = sourcePath.filename();
            }

            std::wstring objectStem = SanitizeManagedCodeStem(relativeSourcePath.generic_wstring());
            if (objectStem.empty())
            {
                objectStem = L"Script";
            }

            return projectRootDirectory
                / ScriptBuildDirectory
                / (std::to_wstring(index) + L"_" + objectStem + L".obj");
        }

        /// <summary>
        /// Script 実行モジュールの出力先パスを作成する。
        /// </summary>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <param name="sourcePath">コンパイル対象ソース。</param>
        /// <param name="index">同名回避用インデックス。</param>
        /// <returns>実行モジュールファイルパス。</returns>
        [[nodiscard]] std::filesystem::path BuildModulePath(
            const std::filesystem::path& projectRootDirectory,
            const std::filesystem::path& sourcePath,
            std::size_t index)
        {
            std::error_code errorCode;
            std::filesystem::path relativeSourcePath =
                std::filesystem::relative(sourcePath, projectRootDirectory, errorCode);
            if (errorCode || relativeSourcePath.empty())
            {
                relativeSourcePath = sourcePath.filename();
            }

            std::wstring moduleStem = SanitizeManagedCodeStem(relativeSourcePath.generic_wstring());
            if (moduleStem.empty())
            {
                moduleStem = L"Script";
            }

            return projectRootDirectory
                / ScriptBuildDirectory
                / (std::to_wstring(index) + L"_" + moduleStem + L".dll");
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
        /// `key=value` 行から値部分を取得する。
        /// </summary>
        /// <param name="line">解析対象行。</param>
        /// <param name="key">取得対象キー。</param>
        /// <returns>値。キーが一致しない場合は空。</returns>
        [[nodiscard]] std::optional<std::string> ReadValue(std::string_view line, std::string_view key)
        {
            if (line.size() <= key.size() + 1 || line.substr(0, key.size()) != key || line[key.size()] != '=')
            {
                return std::nullopt;
            }

            return std::string(line.substr(key.size() + 1));
        }

        /// <summary>
        /// 引用符付き文字列を復元する。
        /// </summary>
        /// <param name="value">復元対象文字列。</param>
        /// <returns>復元した文字列。失敗時は空。</returns>
        [[nodiscard]] std::optional<std::string> ParseQuotedString(std::string_view value)
        {
            std::istringstream stream{ std::string(value) };
            std::string parsed{};
            stream >> std::quoted(parsed);
            if (false == static_cast<bool>(stream))
            {
                return std::nullopt;
            }

            stream >> std::ws;
            if (std::char_traits<char>::eof() != stream.peek())
            {
                return std::nullopt;
            }

            return parsed;
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
        /// Script から Editor 側 Sprite API を呼び出すための C++ ヘッダを書き出す。
        /// </summary>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <returns>書き出しに成功した場合は true。</returns>
        [[nodiscard]] bool WriteScriptApiHeader(const std::filesystem::path& projectRootDirectory)
        {
            const std::filesystem::path headerPath =
                projectRootDirectory / ManagedScriptsDirectory / ScriptApiHeaderFileName;
            std::error_code errorCode;
            std::filesystem::create_directories(headerPath.parent_path(), errorCode);
            if (errorCode)
            {
                return false;
            }

            std::ofstream output(headerPath, std::ios::binary | std::ios::trunc);
            if (false == output.is_open())
            {
                return false;
            }

            output
                << "#pragma once\n"
                << "\n"
                << "struct XelqoriaScriptSpriteApi\n"
                << "{\n"
                << "    void* context;\n"
                << "    void (*ReportError)(void* context, const char* message);\n"
                << "    void (*SetPosition)(void* context, float x, float y, float z);\n"
                << "    void (*SetRotation)(void* context, float x, float y, float z);\n"
                << "    void (*SetScale)(void* context, float x, float y, float z);\n"
                << "    void (*SetVisible)(void* context, int visible);\n"
                << "    void (*SetColor)(void* context, float red, float green, float blue, float alpha);\n"
                << "};\n"
                << "\n"
                << "inline XelqoriaScriptSpriteApi* g_xelqoriaScriptSpriteApi = nullptr;\n"
                << "\n"
                << "extern \"C\" __declspec(dllexport) void Xelqoria_SetSpriteApi(XelqoriaScriptSpriteApi* api)\n"
                << "{\n"
                << "    g_xelqoriaScriptSpriteApi = api;\n"
                << "}\n"
                << "\n"
                << "inline void ReportScriptError(const char* message)\n"
                << "{\n"
                << "    if (nullptr != g_xelqoriaScriptSpriteApi && nullptr != g_xelqoriaScriptSpriteApi->ReportError)\n"
                << "    {\n"
                << "        g_xelqoriaScriptSpriteApi->ReportError(g_xelqoriaScriptSpriteApi->context, message);\n"
                << "    }\n"
                << "}\n"
                << "\n"
                << "inline void SetSpritePosition(float x, float y, float z)\n"
                << "{\n"
                << "    if (nullptr != g_xelqoriaScriptSpriteApi && nullptr != g_xelqoriaScriptSpriteApi->SetPosition)\n"
                << "    {\n"
                << "        g_xelqoriaScriptSpriteApi->SetPosition(g_xelqoriaScriptSpriteApi->context, x, y, z);\n"
                << "    }\n"
                << "}\n"
                << "\n"
                << "inline void SetSpriteRotation(float x, float y, float z)\n"
                << "{\n"
                << "    if (nullptr != g_xelqoriaScriptSpriteApi && nullptr != g_xelqoriaScriptSpriteApi->SetRotation)\n"
                << "    {\n"
                << "        g_xelqoriaScriptSpriteApi->SetRotation(g_xelqoriaScriptSpriteApi->context, x, y, z);\n"
                << "    }\n"
                << "}\n"
                << "\n"
                << "inline void SetSpriteScale(float x, float y, float z)\n"
                << "{\n"
                << "    if (nullptr != g_xelqoriaScriptSpriteApi && nullptr != g_xelqoriaScriptSpriteApi->SetScale)\n"
                << "    {\n"
                << "        g_xelqoriaScriptSpriteApi->SetScale(g_xelqoriaScriptSpriteApi->context, x, y, z);\n"
                << "    }\n"
                << "}\n"
                << "\n"
                << "inline void SetSpriteVisible(bool visible)\n"
                << "{\n"
                << "    if (nullptr != g_xelqoriaScriptSpriteApi && nullptr != g_xelqoriaScriptSpriteApi->SetVisible)\n"
                << "    {\n"
                << "        g_xelqoriaScriptSpriteApi->SetVisible(g_xelqoriaScriptSpriteApi->context, visible ? 1 : 0);\n"
                << "    }\n"
                << "}\n"
                << "\n"
                << "inline void SetSpriteColor(float red, float green, float blue, float alpha)\n"
                << "{\n"
                << "    if (nullptr != g_xelqoriaScriptSpriteApi && nullptr != g_xelqoriaScriptSpriteApi->SetColor)\n"
                << "    {\n"
                << "        g_xelqoriaScriptSpriteApi->SetColor(g_xelqoriaScriptSpriteApi->context, red, green, blue, alpha);\n"
                << "    }\n"
                << "}\n";
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
                << "#include \"" << ToNarrowString(ScriptApiHeaderFileName) << "\"\n"
                << "\n"
                << "extern \"C\" __declspec(dllexport) "
                << "void Start()\n"
                << "{\n"
                << "}\n"
                << "\n"
                << "extern \"C\" __declspec(dllexport) "
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

    Core::AssetId ScriptAssetService::BuildScriptAssetId(
        const std::filesystem::path& projectRootDirectory,
        const std::filesystem::path& assetPath)
    {
        if (projectRootDirectory.empty()
            || assetPath.empty()
            || false == IsScriptAssetFile(assetPath)
            || false == EditorPathSecurity::IsPathInsideOrEqual(assetPath, projectRootDirectory))
        {
            return {};
        }

        std::error_code errorCode;
        const std::filesystem::path relativeAssetPath =
            std::filesystem::relative(assetPath, projectRootDirectory, errorCode);
        if (errorCode || false == EditorPathSecurity::IsSafeRelativePath(relativeAssetPath))
        {
            return {};
        }

        return Xelqoria::Editor::BuildScriptAssetId(relativeAssetPath);
    }

    std::optional<std::filesystem::path> ScriptAssetService::ResolveSourcePath(
        const std::filesystem::path& projectRootDirectory,
        const std::filesystem::path& assetPath)
    {
        if (projectRootDirectory.empty()
            || assetPath.empty()
            || false == IsScriptAssetFile(assetPath)
            || false == EditorPathSecurity::IsPathInsideOrEqual(assetPath, projectRootDirectory))
        {
            return std::nullopt;
        }

        std::ifstream input(assetPath, std::ios::binary);
        if (false == input.is_open())
        {
            return std::nullopt;
        }

        std::optional<std::string> sourceValue{};
        std::string line{};
        while (std::getline(input, line))
        {
            if (const auto value = ReadValue(line, "source"))
            {
                sourceValue = ParseQuotedString(*value);
                break;
            }
        }

        if (false == sourceValue.has_value())
        {
            return std::nullopt;
        }

        const std::filesystem::path sourceRelativePath = ToWideString(*sourceValue);
        if (false == EditorPathSecurity::IsSafeRelativePath(sourceRelativePath))
        {
            return std::nullopt;
        }

        const std::filesystem::path sourcePath = projectRootDirectory / sourceRelativePath;
        if (false == EditorPathSecurity::IsPathInsideOrEqual(sourcePath, projectRootDirectory))
        {
            return std::nullopt;
        }

        return sourcePath;
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

        if (false == WriteScriptApiHeader(projectRootDirectory))
        {
            std::filesystem::remove(assetPath, errorCode);
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
        result.scriptAssetId = Xelqoria::Editor::BuildScriptAssetId(relativeAssetPath);
        return result;
    }

    ScriptBuildResult ScriptAssetService::BuildProjectScripts(
        const std::filesystem::path& projectRootDirectory,
        const ScriptBuildOptions& options)
    {
        ScriptBuildResult result{};
        if (projectRootDirectory.empty())
        {
            result.diagnostics = L"Script ビルド対象のプロジェクトが開かれていません。";
            return result;
        }

        std::error_code errorCode;
        if (false == std::filesystem::is_directory(projectRootDirectory, errorCode) || errorCode)
        {
            result.diagnostics = L"Script ビルド対象のプロジェクトルートを確認できません。";
            return result;
        }

        const std::vector<std::filesystem::path> scriptAssetPaths =
            EnumerateScriptAssetFiles(projectRootDirectory);
        if (scriptAssetPaths.empty())
        {
            result.succeeded = true;
            result.diagnostics = L"Script Asset はありません。";
            return result;
        }

        const std::filesystem::path buildDirectory = projectRootDirectory / ScriptBuildDirectory;
        std::filesystem::create_directories(buildDirectory, errorCode);
        if (errorCode)
        {
            result.diagnostics = L"Script ビルド出力フォルダを作成できません。";
            return result;
        }

        if (false == WriteScriptApiHeader(projectRootDirectory))
        {
            result.diagnostics = L"Script API ヘッダを作成できません。";
            return result;
        }

        std::wostringstream diagnostics{};
        bool allSucceeded = true;
        std::size_t compiledCount = 0;
        for (const std::filesystem::path& scriptAssetPath : scriptAssetPaths)
        {
            const Core::AssetId scriptAssetId = BuildScriptAssetId(projectRootDirectory, scriptAssetPath);
            if (scriptAssetId.IsEmpty())
            {
                allSucceeded = false;
                diagnostics << L"[Script] " << scriptAssetPath.wstring()
                    << L": Script AssetId を生成できません。\n";
                continue;
            }

            const std::optional<std::filesystem::path> sourcePath =
                ResolveSourcePath(projectRootDirectory, scriptAssetPath);
            if (false == sourcePath.has_value())
            {
                allSucceeded = false;
                diagnostics << L"[Script] " << scriptAssetPath.wstring()
                    << L": 管理 C++ ソースの参照を解決できません。\n";
                continue;
            }

            if (false == std::filesystem::exists(*sourcePath, errorCode) || errorCode)
            {
                allSucceeded = false;
                diagnostics << L"[Script] " << scriptAssetPath.wstring()
                    << L": 管理 C++ ソースが見つかりません: " << sourcePath->wstring() << L"\n";
                continue;
            }

            result.sourcePaths.push_back(*sourcePath);
            const std::filesystem::path objectPath =
                BuildObjectPath(projectRootDirectory, *sourcePath, compiledCount);
            const std::filesystem::path modulePath =
                BuildModulePath(projectRootDirectory, *sourcePath, compiledCount);
            ++compiledCount;

            const std::wstring commandLine =
                BuildScriptCompileCommandLine(options, *sourcePath, objectPath, modulePath);

            std::wstring compilerOutput{};
            const int exitCode = RunCommandAndCaptureOutput(commandLine, compilerOutput);
            diagnostics << L"[Script] " << sourcePath->wstring() << L"\n";
            if (false == compilerOutput.empty())
            {
                diagnostics << compilerOutput;
                if (compilerOutput.back() != L'\n')
                {
                    diagnostics << L"\n";
                }
            }

            if (0 != exitCode)
            {
                allSucceeded = false;
                diagnostics << L"終了コード: " << exitCode << L"\n";
            }
            else
            {
                result.artifacts.push_back(ScriptBuildArtifact{
                    scriptAssetId,
                    *sourcePath,
                    modulePath
                });
            }
        }

        result.succeeded = allSucceeded;
        if (allSucceeded)
        {
            diagnostics << L"Script ビルドに成功しました。対象: "
                << result.sourcePaths.size() << L" 件";
        }

        result.diagnostics = diagnostics.str();
        return result;
    }

}
