#pragma once

#include "IWindow.h"
#include "PlatformTypes.h"

#include <Windows.h>
#include <cstdint>
#include <string>

namespace Xelqoria::Platform::Win32
{
    /// <summary>
    /// Win32 API を使用してアプリケーションウィンドウを管理する。
    /// </summary>
    class Win32Window final : public IWindow
    {
    public:
        /// <summary>
        /// Win32 アプリケーションインスタンスに紐づくウィンドウ管理を生成する。
        /// </summary>
        /// <param name="applicationHandle">Win32 アプリケーションインスタンスハンドル。</param>
        explicit Win32Window(NativeApplicationHandle applicationHandle);

        /// <summary>
        /// ウィンドウリソースを解放する。
        /// </summary>
        ~Win32Window() override;

        Win32Window(const Win32Window&) = delete;
        Win32Window& operator=(const Win32Window&) = delete;
        Win32Window(Win32Window&&) = delete;
        Win32Window& operator=(Win32Window&&) = delete;

        bool Create(const std::wstring& title, std::uint32_t clientWidth, std::uint32_t clientHeight) override;
        void Show() override;
        void Close() override;
        [[nodiscard]] bool IsOpen() const override;
        [[nodiscard]] NativeWindowHandle GetNativeHandle() const override;
        [[nodiscard]] std::uint32_t GetClientWidth() const override;
        [[nodiscard]] std::uint32_t GetClientHeight() const override;
        void SetCommandHandler(CommandHandler handler) override;
        void SetNotifyHandler(NativeMessageHandler handler) override;
        void SetDrawItemHandler(NativeMessageHandler handler) override;
        void SetCloseRequestHandler(CloseRequestHandler handler) override;
        void SetResizeHandler(ResizeHandler handler) override;
        [[nodiscard]] int ConsumeMouseWheelDelta() override;

    private:
        /// <summary>
        /// Win32 API から呼び出される静的ウィンドウプロシージャ。
        /// </summary>
        static LRESULT CALLBACK StaticWndProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

        /// <summary>
        /// 各ウィンドウインスタンスのメッセージ処理を行う。
        /// </summary>
        LRESULT WndProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

        HINSTANCE m_applicationHandle = nullptr;
        HWND m_windowHandle = nullptr;
        CommandHandler m_commandHandler{};
        NativeMessageHandler m_notifyHandler{};
        NativeMessageHandler m_drawItemHandler{};
        CloseRequestHandler m_closeRequestHandler{};
        ResizeHandler m_resizeHandler{};
        int m_pendingMouseWheelDelta = 0;
        std::wstring m_className = L"XelqoriaWindowClass";
        std::wstring m_title = L"Xelqoria";
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
    };
}
