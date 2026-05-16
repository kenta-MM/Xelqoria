#pragma once

#include <string>

namespace Xelqoria::Platform
{
    /// <summary>
    /// OS のクリップボード操作を抽象化する。
    /// </summary>
    class IClipboard
    {
    public:
        virtual ~IClipboard() = default;

        /// <summary>
        /// クリップボードにテキストを設定する。
        /// </summary>
        /// <param name="text">設定する Unicode テキスト。</param>
        /// <returns>設定に成功した場合は true。</returns>
        virtual bool SetText(const std::wstring& text) = 0;

        /// <summary>
        /// クリップボードからテキストを取得する。
        /// </summary>
        /// <returns>取得した Unicode テキスト。取得できない場合は空文字列。</returns>
        [[nodiscard]] virtual std::wstring GetText() const = 0;
    };
}
