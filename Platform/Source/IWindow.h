#pragma once

#include "PlatformTypes.h"

#include <cstdint>
#include <functional>
#include <string>

namespace Xelqoria::Platform
{
    /// <summary>
    /// OS 固有のウィンドウ生成と基本操作を抽象化する。
    /// </summary>
    class IWindow
    {
    public:
        /// <summary>
        /// コマンド ID を受け取る関数型を表す。
        /// </summary>
        using CommandHandler = std::function<void(unsigned)>;

        /// <summary>
        /// OS 固有メッセージ引数を処理する関数型を表す。
        /// </summary>
        using NativeMessageHandler = std::function<bool(NativeMessageParameter)>;

        /// <summary>
        /// ウィンドウ終了要求を処理する関数型を表す。
        /// </summary>
        using CloseRequestHandler = std::function<bool()>;

        /// <summary>
        /// クライアント領域のサイズ変更を受け取る関数型を表す。
        /// </summary>
        using ResizeHandler = std::function<void(std::uint32_t, std::uint32_t)>;

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

        /// <summary>
        /// コマンド通知を受け取るハンドラを設定する。
        /// </summary>
        /// <param name="handler">コマンド ID を受け取るハンドラ。</param>
        virtual void SetCommandHandler(CommandHandler handler) = 0;

        /// <summary>
        /// OS 固有の通知メッセージを受け取るハンドラを設定する。
        /// </summary>
        /// <param name="handler">通知メッセージ引数を処理し、消費した場合は true を返すハンドラ。</param>
        virtual void SetNotifyHandler(NativeMessageHandler handler) = 0;

        /// <summary>
        /// OS 固有のオーナー描画メッセージを受け取るハンドラを設定する。
        /// </summary>
        /// <param name="handler">描画メッセージ引数を処理し、消費した場合は true を返すハンドラ。</param>
        virtual void SetDrawItemHandler(NativeMessageHandler handler) = 0;

        /// <summary>
        /// ウィンドウを閉じる前に呼ばれるハンドラを設定する。
        /// </summary>
        /// <param name="handler">閉じてよい場合は true を返すハンドラ。</param>
        virtual void SetCloseRequestHandler(CloseRequestHandler handler) = 0;

        /// <summary>
        /// クライアント領域サイズ変更時に呼ばれるハンドラを設定する。
        /// </summary>
        /// <param name="handler">新しいクライアント領域の幅と高さを受け取るハンドラ。</param>
        virtual void SetResizeHandler(ResizeHandler handler) = 0;

        /// <summary>
        /// 前回取得以降に蓄積されたマウスホイール差分を取得して消費する。
        /// </summary>
        /// <returns>蓄積されたマウスホイール差分。</returns>
        [[nodiscard]] virtual int ConsumeMouseWheelDelta() = 0;
    };
}
