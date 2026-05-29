#pragma once

#include <algorithm>
#include <cwchar>
#include <cwctype>
#include <filesystem>
#include <string>
#include <system_error>

namespace Xelqoria::Editor::EditorPathSecurity
{
    /// <summary>
    /// パス要素を大文字小文字差を吸収して比較できる文字列へ変換する。
    /// </summary>
    /// <param name="part">変換対象のパス要素。</param>
    /// <returns>比較用文字列。</returns>
    [[nodiscard]] inline std::wstring ToComparablePathPart(const std::filesystem::path& part)
    {
        std::wstring value = part.generic_wstring();
        std::transform(value.begin(), value.end(), value.begin(), [](wchar_t character)
            {
                return static_cast<wchar_t>(std::towlower(character));
            });
        return value;
    }

    /// <summary>
    /// 存在する範囲を解決したうえでパスを正規化する。
    /// </summary>
    /// <param name="path">正規化対象パス。</param>
    /// <returns>正規化したパス。</returns>
    [[nodiscard]] inline std::filesystem::path NormalizeForContainment(const std::filesystem::path& path)
    {
        std::error_code errorCode;
        std::filesystem::path normalized = std::filesystem::weakly_canonical(path, errorCode);
        if (errorCode)
        {
            normalized = std::filesystem::absolute(path, errorCode);
            if (errorCode)
            {
                normalized = path;
            }
        }

        return normalized.lexically_normal();
    }

    /// <summary>
    /// 指定パスがルートディレクトリ配下または同一パスかを判定する。
    /// </summary>
    /// <param name="path">判定対象パス。</param>
    /// <param name="rootDirectory">許可するルートディレクトリ。</param>
    /// <returns>ルート配下または同一の場合は true。</returns>
    [[nodiscard]] inline bool IsPathInsideOrEqual(
        const std::filesystem::path& path,
        const std::filesystem::path& rootDirectory)
    {
        if (path.empty() || rootDirectory.empty())
        {
            return false;
        }

        const std::filesystem::path normalizedPath = NormalizeForContainment(path);
        const std::filesystem::path normalizedRoot = NormalizeForContainment(rootDirectory);

        auto pathIterator = normalizedPath.begin();
        auto rootIterator = normalizedRoot.begin();
        for (; rootIterator != normalizedRoot.end(); ++rootIterator, ++pathIterator)
        {
            if (pathIterator == normalizedPath.end()
                || ToComparablePathPart(*pathIterator) != ToComparablePathPart(*rootIterator))
            {
                return false;
            }
        }

        return true;
    }

    /// <summary>
    /// 相対パスが上位ディレクトリへ脱出しない形式かを判定する。
    /// </summary>
    /// <param name="path">判定対象パス。</param>
    /// <returns>安全な相対パスの場合は true。</returns>
    [[nodiscard]] inline bool IsSafeRelativePath(const std::filesystem::path& path)
    {
        if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory())
        {
            return false;
        }

        const std::filesystem::path normalizedPath = path.lexically_normal();
        if (normalizedPath.empty())
        {
            return false;
        }

        for (const std::filesystem::path& part : normalizedPath)
        {
            const std::wstring partText = part.generic_wstring();
            if (partText == L"." || partText == L"..")
            {
                return false;
            }
        }

        return true;
    }

    /// <summary>
    /// プロジェクト名や生成ファイル名として安全に扱える名前かを判定する。
    /// </summary>
    /// <param name="projectName">判定対象名。</param>
    /// <returns>安全な名前の場合は true。</returns>
    [[nodiscard]] inline bool IsValidProjectName(const std::wstring& projectName)
    {
        if (projectName.empty() || projectName == L"." || projectName == L"..")
        {
            return false;
        }

        if (projectName.back() == L'.' || projectName.back() == L' ')
        {
            return false;
        }

        constexpr const wchar_t* reservedCharacters = L"<>:\"/\\|?*";
        for (const wchar_t character : projectName)
        {
            if (character < 0x20 || std::wcschr(reservedCharacters, character) != nullptr)
            {
                return false;
            }
        }

        std::wstring comparableName = projectName;
        std::transform(comparableName.begin(), comparableName.end(), comparableName.begin(), [](wchar_t character)
            {
                return static_cast<wchar_t>(std::towupper(character));
            });

        const std::size_t extensionSeparator = comparableName.find(L'.');
        const std::wstring baseName = comparableName.substr(0, extensionSeparator);
        constexpr const wchar_t* reservedNames[] =
        {
            L"CON", L"PRN", L"AUX", L"NUL",
            L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
            L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"
        };

        for (const wchar_t* reservedName : reservedNames)
        {
            if (baseName == reservedName)
            {
                return false;
            }
        }

        return true;
    }
}
