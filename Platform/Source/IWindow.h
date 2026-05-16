#pragma once

#include "PlatformTypes.h"

#include <cstdint>
#include <string>

namespace Xelqoria::Platform
{
    /// <summary>
    /// OS 固有のウィンドウ生成と基本操作を抽象化する。
    /// </summary>
    class IWindow
    {
    public:
        virtual ~IWindow() = default;

        /// <summary>
        /// 指定サイズとタイトルでウィンドウを作成する。
        /// </summary>
        /// <param name="title">ウィンドウタイトル。</param>
        /// <param name="clientWidth">クライアント領域の幅。</param>
        /// <param name="clientHeight">クライアント領域の高さ。</param>
        /// <returns>作成に成功した場合は true。</returns>
        virtual bool Create(const std::wstring& title, std::uint32_t clientWidth, std::uint32_t clientHeight) = 0;

        /// <summary>
        /// ウィンドウを表示する。
        /// </summary>
        virtual void Show() = 0;

        /// <summary>
        /// ウィンドウを閉じる。
        /// </summary>
        virtual void Close() = 0;

        /// <summary>
        /// ウィンドウが有効かを取得する。
        /// </summary>
        /// <returns>有効な場合は true。</returns>
        [[nodiscard]] virtual bool IsOpen() const = 0;

        /// <summary>
        /// OS 固有のウィンドウハンドルを取得する。
        /// </summary>
        /// <returns>不透明なネイティブウィンドウハンドル。</returns>
        [[nodiscard]] virtual NativeWindowHandle GetNativeHandle() const = 0;

        /// <summary>
        /// クライアント領域の幅を取得する。
        /// </summary>
        /// <returns>クライアント領域の幅。</returns>
        [[nodiscard]] virtual std::uint32_t GetClientWidth() const = 0;

        /// <summary>
        /// クライアント領域の高さを取得する。
        /// </summary>
        /// <returns>クライアント領域の高さ。</returns>
        [[nodiscard]] virtual std::uint32_t GetClientHeight() const = 0;
    };
}
