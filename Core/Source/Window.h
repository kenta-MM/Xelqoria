#pragma once
#include <Windows.h>
#include <cstdint>
#include <functional>
#include <string>

namespace Xelqoria::Core
{
	/// <summary>
	/// Win32 API を使用してアプリケーションウィンドウを管理する。
	/// </summary>
	class Window
	{
	public:
		/// デフォルトコンストラクタ
		Window() = default;

		/// ウィンドウリソースを解放する
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;
		Window(Window&&) = delete;
		Window& operator=(Window&&) = delete;

		/// <summary>
		/// 指定サイズとタイトルで Win32 ウィンドウを作成する。
		/// </summary>
		/// <param name="hInstance">アプリケーションインスタンスハンドル。</param>
		/// <param name="title">ウィンドウタイトル。</param>
		/// <param name="clientWidth">クライアント領域の幅。</param>
		/// <param name="clientHeight">クライアント領域の高さ。</param>
		/// <returns>作成に成功した場合は true。</returns>
		bool Create(HINSTANCE hInstance, const wchar_t* title, uint32_t clientWidth, uint32_t clientHeight);

		/// <summary>
		/// ウィンドウを表示する。
		/// </summary>
		/// <param name="nCmdShow">表示方法（SW_SHOW など）。</param>
		void Show(int nCmdShow = SW_SHOW);

		/// <summary>
		/// Win32 メッセージキューからメッセージを取得して処理する。
		/// </summary>
		/// <returns>アプリケーション終了要求があった場合は false。</returns>
		bool PumpMessages();

		/// ウィンドウハンドルを取得する
		[[nodiscard]] HWND GetHwnd() const;

		/// <summary>
		/// WM_COMMAND を受け取るハンドラを設定する。
		/// </summary>
		/// <param name="handler">コマンド ID を受け取るハンドラ。</param>
		void SetCommandHandler(std::function<void(unsigned)> handler);

		/// <summary>
		/// WM_NOTIFY を受け取るハンドラを設定する。
		/// </summary>
		/// <param name="handler">通知 LPARAM を処理し、消費した場合は true を返すハンドラ。</param>
		void SetNotifyHandler(std::function<bool(LPARAM)> handler);

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

		/// クライアント領域の幅を取得する
		[[nodiscard]] uint32_t GetWidth() const;

		/// クライアント領域の高さを取得する
		[[nodiscard]] uint32_t GetHeight() const;

		/// <summary>
		/// 前回取得以降に蓄積されたマウスホイール差分を取得して消費する。
		/// </summary>
		/// <returns>蓄積されたマウスホイール差分。</returns>
		[[nodiscard]] int ConsumeMouseWheelDelta();

	private:
		/// <summary>
		/// Win32 API から呼び出される静的ウィンドウプロシージャ。
		/// 実際の処理はインスタンスメンバ関数 WndProc に委譲する。
		/// </summary>
		static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

		/// <summary>
		/// 各ウィンドウインスタンスのメッセージ処理を行う。
		/// </summary>
		LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

		HINSTANCE m_hInstance = nullptr;
		HWND m_hWnd = nullptr;
		std::function<void(unsigned)> m_commandHandler{};
		std::function<bool(LPARAM)> m_notifyHandler{};
		std::function<bool()> m_closeRequestHandler{};
		std::function<void(uint32_t, uint32_t)> m_resizeHandler{};
		int m_pendingMouseWheelDelta = 0;

		std::wstring m_className = L"XelqoriaWindowClass";
		std::wstring m_title = L"Xelqoria";

		uint32_t m_width = 0;
		uint32_t m_height = 0;
	};
}
