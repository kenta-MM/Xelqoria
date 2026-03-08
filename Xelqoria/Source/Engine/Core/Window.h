#pragma once
#include <Windows.h>
#include <string>

namespace Xelqoria::Core
{
	/**
	 * @brief アプリケーションウィンドウを管理するクラス
	 *
	 * Win32 API を使用してアプリケーションウィンドウの生成、
	 * 表示、およびメッセージ処理を行う。
	 *
	 * エンジンの描画システムは、このクラスが生成した
	 * ウィンドウハンドル (HWND) を使用してレンダリングを行う。
	 */
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

		/**
		 * @brief ウィンドウを作成する
		 *
		 * 指定されたサイズとタイトルで Win32 ウィンドウを作成する。
		 *
		 * @param hInstance アプリケーションインスタンスハンドル
		 * @param title ウィンドウタイトル
		 * @param clientWidth クライアント領域の幅
		 * @param clientHeight クライアント領域の高さ
		 * @return 作成に成功した場合 true
		 */
		bool Create(HINSTANCE hInstance, const wchar_t* title, uint32_t clientWidth, uint32_t clientHeight);

		/**
		 * @brief ウィンドウを表示する
		 *
		 * @param nCmdShow 表示方法 (SW_SHOW など)
		 */
		void Show(int nCmdShow = SW_SHOW);

		/**
		 * @brief ウィンドウメッセージを処理する
		 *
		 * Win32 メッセージキューからメッセージを取得し処理する。
		 *
		 * @return アプリケーション終了要求があった場合 false
		 */
		bool PumpMessages();

		/// ウィンドウハンドルを取得する
		[[nodiscard]] HWND GetHwnd() const;

		/// クライアント領域の幅を取得する
		[[nodiscard]] uint32_t GetWidth() const;

		/// クライアント領域の高さを取得する
		[[nodiscard]] uint32_t GetHeight() const;

	private:
		/**
		 * @brief Win32 の静的ウィンドウプロシージャ
		 *
		 * Win32 API から呼び出されるコールバック関数。
		 * 実際の処理はインスタンスメンバ関数 WndProc に委譲する。
		 */
		static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

		/**
		 * @brief インスタンス用ウィンドウプロシージャ
		 *
		 * 各ウィンドウインスタンスのメッセージ処理を行う。
		 */
		LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

		HINSTANCE m_hInstance = nullptr;
		HWND m_hWnd = nullptr;

		std::wstring m_className = L"XelqoriaWindowClass";
		std::wstring m_title = L"Xelqoria";

		uint32_t m_width = 0;
		uint32_t m_height = 0;
	};
}