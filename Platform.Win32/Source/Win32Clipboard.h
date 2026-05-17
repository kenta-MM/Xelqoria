#pragma once

#include "IClipboard.h"
#include "PlatformTypes.h"

namespace Xelqoria::Platform::Win32
{
    /// <summary>
    /// Win32 API を使用するクリップボード実装。
    /// </summary>
    class Win32Clipboard final : public IClipboard
    {
    public:
        /// <summary>
        /// Win32Clipboard を生成する。
        /// </summary>
        /// <param name="ownerWindow">クリップボードを開く時に使う親ウィンドウ。</param>
        explicit Win32Clipboard(NativeWindowHandle ownerWindow = nullptr);

        /// <summary>
        /// クリップボードにテキストを設定する。
        /// </summary>
        /// <param name="text">設定する Unicode テキスト。</param>
        /// <returns>設定に成功した場合は true。</returns>
        bool SetText(const std::wstring& text) override;

        /// <summary>
        /// クリップボードからテキストを取得する。
        /// </summary>
        /// <returns>取得した Unicode テキスト。取得できない場合は空文字列。</returns>
        [[nodiscard]] std::wstring GetText() const override;

    private:
        NativeWindowHandle m_ownerWindow = nullptr;
    };
}
