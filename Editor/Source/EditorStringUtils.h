#pragma once

#include <string>
#include <string_view>

namespace Xelqoria::Editor
{
    /// <summary>
    /// 狭い文字列を単純なワイド文字列へ変換する。
    /// </summary>
    /// <param name="value">変換対象の文字列。</param>
    /// <returns>各文字をそのまま拡張したワイド文字列。</returns>
    inline std::wstring ToWideString(std::string_view value)
    {
        return std::wstring(value.begin(), value.end());
    }

    /// <summary>
    /// ワイド文字列から ASCII 範囲のみを狭い文字列へ変換する。
    /// </summary>
    /// <param name="value">変換対象のワイド文字列。</param>
    /// <returns>ASCII 文字だけを保持した文字列。</returns>
    inline std::string ToNarrowString(std::wstring_view value)
    {
        std::string result;
        result.reserve(value.size());

        for (const wchar_t character : value)
        {
            if (character >= 0 && character <= 0x7f)
            {
                result.push_back(static_cast<char>(character));
            }
        }

        return result;
    }
}
