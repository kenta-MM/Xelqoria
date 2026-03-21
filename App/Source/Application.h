#pragma once
#include <Windows.h>
#include <memory>

#include "Assets/ISpriteAssetResolver.h"
#include "ITextureAssetResolver.h"
#include "Scene.h"
#include "Window.h"
#include "IGraphicsContext.h"

namespace Xelqoria::Graphics
{
	class SpriteRenderer;
}

namespace Xelqoria::App
{
	/// <summary>
	/// アプリケーションのメインクラス。
	/// 
	/// ウィンドウの生成、グラフィックスコンテキストの初期化、
	/// メインループ（Update / Render）の実行を管理する。
	/// エンジンのエントリーポイントとなるクラス。
	/// </summary>
	class Application
	{
	public:

		/// <summary>
		/// Application を生成する。
		/// </summary>
		/// <param name="hInstance">
		/// Windowsアプリケーションのインスタンスハンドル。
		/// WinMain から渡される。
		/// </param>
		explicit Application(HINSTANCE hInstance);

		/// <summary>
		/// Application のデストラクタ。
		/// 内部リソースの解放を行う。
		/// </summary>
		~Application();

		/// <summary>
		/// コピーコンストラクタは禁止。
		/// </summary>
		Application(const Application&) = delete;

		/// <summary>
		/// コピー代入は禁止。
		/// </summary>
		Application& operator=(const Application&) = delete;

		/// <summary>
		/// ムーブコンストラクタは禁止。
		/// </summary>
		Application(Application&&) = delete;

		/// <summary>
		/// ムーブ代入は禁止。
		/// </summary>
		Application& operator=(Application&&) = delete;

		/// <summary>
		/// アプリケーションを実行する。
		/// メインループを開始し、Update と Render を繰り返す。
		/// </summary>
		/// <returns>
		/// プロセス終了コード。
		/// </returns>
		int Run();

	private:

		/// <summary>
		/// アプリケーションの初期化処理。
		/// ウィンドウ生成やグラフィックス初期化を行う。
		/// </summary>
		/// <returns>
		/// 初期化に成功した場合 true。
		/// </returns>
		bool Initialize();

		/// <summary>
		/// アプリケーション終了時のクリーンアップ処理。
		/// </summary>
		void Shutdown();

		/// <summary>
		/// ゲームロジックの更新処理。
		/// </summary>
		/// <param name="dt">
		/// 前フレームからの経過時間（秒）。
		/// </param>
		void Update(float dt);

		/// <summary>
		/// 描画処理を実行する。
		/// </summary>
		void Render();

	private:

		/// <summary>
		/// Windows アプリケーションインスタンス。
		/// </summary>
		HINSTANCE m_hInstance = nullptr;

		/// <summary>
		/// メインウィンドウ。
		/// </summary>
		Core::Window m_window{};

		/// <summary>
		/// グラフィックスコンテキスト。
		/// DirectX / Vulkan などの実装を抽象化する。
		/// </summary>
		std::unique_ptr<RHI::IGraphicsContext> m_graphics;

		/// <summary>
		/// 実行中の描画対象と Entity を保持する Scene。
		/// </summary>
		std::unique_ptr<Game::Scene> m_scene;

		/// <summary>
		/// Scene から収集した Sprite を描画するレンダラー。
		/// </summary>
		std::unique_ptr<Graphics::SpriteRenderer> m_spriteRenderer;

		/// <summary>
		/// Scene が参照する SpriteAsset を解決するレジストリ。
		/// </summary>
		Game::Assets::SpriteAssetRegistry m_spriteAssetRegistry{};

		/// <summary>
		/// Scene が参照する Texture2D を解決するレジストリ。
		/// </summary>
		Graphics::TextureAssetRegistry m_textureAssetRegistry{};

		/// <summary>
		/// Scene ベースのランタイムへ移行するための準備状態。
		/// </summary>
		bool m_sceneRuntimeReady = false;

		/// <summary>
		/// Scene の Asset 解決ログを初回描画で出力済みかを表す。
		/// </summary>
		bool m_hasLoggedSceneResolution = false;

		/// <summary>
		/// アプリケーション実行フラグ。
		/// </summary>
		bool m_running = true;

		/// <summary>
		/// 目標FPS。
		/// </summary>
		unsigned int m_fps = 60;

		/// <summary>
		/// フレームカウンタ。
		/// </summary>
		unsigned int m_count = 0;

		/// <summary>
		/// FPS計測開始時間。
		/// </summary>
		DWORD m_startTime = 0;

		/// <summary>
		/// 前フレームからの経過時間（秒）。
		/// </summary>
		float m_deltaTime = 1.0f / 60.0f;
	};
}
