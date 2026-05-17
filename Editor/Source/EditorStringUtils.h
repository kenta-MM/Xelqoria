#pragma once

#include <Windows.h>

#include <string>
#include <string_view>

namespace Xelqoria::Editor
{
    /// <summary>
    /// UTF-8 文字列をワイド文字列へ変換する。
    /// </summary>
    /// <param name="value">変換対象の文字列。</param>
    /// <returns>変換後のワイド文字列。</returns>
    inline std::wstring ToWideString(std::string_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const int wideLength = MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            value.data(),
            static_cast<int>(value.size()),
            nullptr,
            0);
        if (wideLength <= 0)
        {
            return std::wstring(value.begin(), value.end());
        }

        std::wstring result(static_cast<std::size_t>(wideLength), L'\0');
        const int convertedLength = MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            value.data(),
            static_cast<int>(value.size()),
            result.data(),
            wideLength);
        if (convertedLength <= 0)
        {
            return {};
        }

        return result;
    }

    /// <summary>
    /// ワイド文字列を UTF-8 文字列へ変換する。
    /// </summary>
    /// <param name="value">変換対象のワイド文字列。</param>
    /// <returns>変換後の UTF-8 文字列。</returns>
    inline std::string ToNarrowString(std::wstring_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const int narrowLength = WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            value.data(),
            static_cast<int>(value.size()),
            nullptr,
            0,
            nullptr,
            nullptr);
        if (narrowLength <= 0)
        {
            return {};
        }

        std::string result(static_cast<std::size_t>(narrowLength), '\0');
        const int convertedLength = WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            value.data(),
            static_cast<int>(value.size()),
            result.data(),
            narrowLength,
            nullptr,
            nullptr);
        if (convertedLength <= 0)
        {
            return {};
        }

        return result;
    }
}
