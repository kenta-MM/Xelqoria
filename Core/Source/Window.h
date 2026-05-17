#pragma once

#include "IEventLoop.h"
#include "IWindow.h"
#include "PlatformTypes.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace Xelqoria::Core
{
	/// <summary>
	/// Platform のウィンドウ実装を通じてアプリケーションウィンドウを管理する。
	/// </summary>
	class Window
	{
	public:
		/// <summary>
		/// 空の Window を生成する。使用前に SetPlatformWindow を呼び出す必要がある。
		/// </summary>
		Window() = default;

		/// <summary>
		/// Platform 実装を受け取って Window を生成する。
		/// </summary>
		/// <param name="window">OS 固有のウィンドウ実装。</param>
		/// <param name="eventLoop">OS 固有のイベントループ実装。</param>
		Window(std::unique_ptr<Platform::IWindow> window, std::unique_ptr<Platform::IEventLoop> eventLoop);

		/// <summary>
		/// ウィンドウリソースを解放する。
		/// </summary>
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;
		Window(Window&&) = delete;
		Window& operator=(Window&&) = delete;

		/// <summary>
		/// 使用する Platform ウィンドウ実装を設定する。
		/// </summary>
		/// <param name="window">OS 固有のウィンドウ実装。</param>
		/// <param name="eventLoop">OS 固有のイベントループ実装。</param>
		void SetPlatformWindow(std::unique_ptr<Platform::IWindow> window, std::unique_ptr<Platform::IEventLoop> eventLoop);

		/// <summary>
		/// 指定サイズとタイトルでウィンドウを作成する。
		/// </summary>
		/// <param name="title">ウィンドウタイトル。</param>
		/// <param name="clientWidth">クライアント領域の幅。</param>
		/// <param name="clientHeight">クライアント領域の高さ。</param>
		/// <returns>作成に成功した場合は true。</returns>
		bool Create(const wchar_t* title, uint32_t clientWidth, uint32_t clientHeight);

		/// <summary>
		/// ウィンドウを表示する。
		/// </summary>
		/// <param name="showCommand">互換性のために受け取る表示指定。Platform 実装が表示方法を決定する。</param>
		void Show(int showCommand = 1);

		/// <summary>
		/// Platform のイベントキューからイベントを取得して処理する。
		/// </summary>
		/// <returns>アプリケーション終了要求があった場合は false。</returns>
		bool PumpMessages();

		/// <summary>
		/// OS 固有のウィンドウハンドルを取得する。
		/// </summary>
		/// <returns>不透明なネイティブウィンドウハンドル。</returns>
		[[nodiscard]] Platform::NativeWindowHandle GetHwnd() const;

		/// <summary>
		/// コマンド通知を受け取るハンドラを設定する。
		/// </summary>
		/// <param name="handler">コマンド ID を受け取るハンドラ。</param>
		void SetCommandHandler(std::function<void(unsigned)> handler);

		/// <summary>
		/// OS 固有の通知メッセージを受け取るハンドラを設定する。
		/// </summary>
		/// <param name="handler">通知メッセージ引数を処理し、消費した場合は true を返すハンドラ。</param>
		void SetNotifyHandler(std::function<bool(Platform::NativeMessageParameter)> handler);

		/// <summary>
		/// OS 固有のオーナー描画メッセージを受け取るハンドラを設定する。
		/// </summary>
		/// <param name="handler">描画メッセージ引数を処理し、消費した場合は true を返すハンドラ。</param>
		void SetDrawItemHandler(std::function<bool(Platform::NativeMessageParameter)> handler);

		/// <summary>
		/// ウィンドウを閉じる前に呼ばれるハンドラを設定する。
		/// </summary>
		/// <param name="handler">閉じてよい場合は true を返すハンドラ。</param>
		void SetCloseRequestHandler(std::function<bool()> handler);

		/// <summary>
		/// クライアント領域サイズ変更時に呼ばれるハンドラを設定する。
		/// </summary>
		/// <param name="handler">新しいクライアント領域の幅と高さを受け取るハンドラ。</param>
		void SetResizeHandler(std::function<void(uint32_t, uint32_t)> handler);

		/// <summary>
		/// クライアント領域の幅を取得する。
		/// </summary>
		/// <returns>クライアント領域の幅。</returns>
		[[nodiscard]] uint32_t GetWidth() const;

		/// <summary>
		/// クライアント領域の高さを取得する。
		/// </summary>
		/// <returns>クライアント領域の高さ。</returns>
		[[nodiscard]] uint32_t GetHeight() const;

		/// <summary>
		/// 前回取得以降に蓄積されたマウスホイール差分を取得して消費する。
		/// </summary>
		/// <returns>蓄積されたマウスホイール差分。</returns>
		[[nodiscard]] int ConsumeMouseWheelDelta();

	private:
		std::unique_ptr<Platform::IWindow> m_window{};
		std::unique_ptr<Platform::IEventLoop> m_eventLoop{};
	};
}
