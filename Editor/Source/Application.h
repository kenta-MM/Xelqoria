#pragma once

#include <Windows.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "AssetId.h"
#include "Assets/ISpriteAssetResolver.h"
#include "IGraphicsContext.h"
#include "ITextureAssetResolver.h"
#include "Scene.h"
#include "Texture2D.h"
#include "Window.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// SceneView へ Runtime 描画を埋め込む方式を表す。
    /// </summary>
    enum class SceneViewEmbeddingMode
    {
        /// <summary>
        /// SceneView 専用 child HWND を swap chain の描画先にする。
        /// </summary>
        ChildWindowSwapChainHost = 0
    };

    /// <summary>
    /// Editor ターゲットの最小実行ループを管理する。
    /// </summary>
    class Application
    {
    public:
        /// <summary>
        /// Editor 用 Application を生成する。
        /// </summary>
        /// <param name="hInstance">Windows アプリケーションのインスタンスハンドル。</param>
        explicit Application(HINSTANCE hInstance);

        /// <summary>
        /// 内部リソースを解放する。
        /// </summary>
        ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        /// <summary>
        /// Editor のメインループを実行する。
        /// </summary>
        /// <returns>プロセス終了コード。</returns>
        int Run();

    private:
        /// <summary>
        /// Editor ターゲットを初期化する。
        /// </summary>
        /// <returns>初期化に成功した場合は true。</returns>
        bool Initialize();

        /// <summary>
        /// 終了時のクリーンアップを行う。
        /// </summary>
        void Shutdown();

        /// <summary>
        /// Editor 用のフレーム更新処理を行う。
        /// </summary>
        /// <param name="deltaTime">前フレームからの経過時間（秒）。</param>
        void Update(float deltaTime);

        /// <summary>
        /// Editor 用のフレーム描画を行う。
        /// </summary>
        void Render();

        /// <summary>
        /// Editor の固定パネル UI を生成する。
        /// </summary>
        /// <returns>生成に成功した場合は true。</returns>
        bool CreateEditorShell();

        /// <summary>
        /// 現在のクライアントサイズに合わせてパネル配置を更新する。
        /// </summary>
        void UpdateLayout();

        /// <summary>
        /// SceneView 用描画領域へグラフィックスコンテキストを初期化する。
        /// </summary>
        /// <returns>初期化に成功した場合は true。</returns>
        bool InitializeSceneViewGraphics();

        /// <summary>
        /// Editor で扱う最小サンプルデータを初期化する。
        /// </summary>
        /// <returns>初期化に成功した場合は true。</returns>
        bool InitializeDocument();

        /// <summary>
        /// Assets パネルの表示内容を更新する。
        /// </summary>
        void RefreshAssetsPanel();

        /// <summary>
        /// Assets パネルの選択状態を同期する。
        /// </summary>
        void SyncAssetSelection();

        /// <summary>
        /// Hierarchy パネルの表示内容を更新する。
        /// </summary>
        void RefreshHierarchyPanel();

        /// <summary>
        /// Hierarchy パネルの選択状態を同期する。
        /// </summary>
        void SyncHierarchySelection();

        /// <summary>
        /// 共通設定を適用した子ウィンドウを生成する。
        /// </summary>
        /// <param name="className">生成する Win32 クラス名。</param>
        /// <param name="text">初期表示文字列。</param>
        /// <param name="style">適用するウィンドウスタイル。</param>
        /// <param name="exStyle">適用する拡張ウィンドウスタイル。</param>
        /// <returns>生成した子ウィンドウハンドル。失敗時は nullptr。</returns>
        HWND CreateChildWindow(const wchar_t* className, const wchar_t* text, DWORD style, DWORD exStyle = 0) const;

    private:
        /// <summary>
        /// Windows アプリケーションインスタンスを保持する。
        /// </summary>
        HINSTANCE m_hInstance = nullptr;

        /// <summary>
        /// Editor のメインウィンドウを保持する。
        /// </summary>
        Core::Window m_window{};

        /// <summary>
        /// SceneView 描画に使用するグラフィックスコンテキストを保持する。
        /// </summary>
        std::unique_ptr<RHI::IGraphicsContext> m_graphics;

        /// <summary>
        /// メインループの継続状態を表す。
        /// </summary>
        bool m_running = true;

        /// <summary>
        /// Hierarchy パネルのグループボックスを保持する。
        /// </summary>
        HWND m_hierarchyPanel = nullptr;

        /// <summary>
        /// Assets パネルのグループボックスを保持する。
        /// </summary>
        HWND m_assetsPanel = nullptr;

        /// <summary>
        /// Inspector パネルのグループボックスを保持する。
        /// </summary>
        HWND m_inspectorPanel = nullptr;

        /// <summary>
        /// SceneView パネルのグループボックスを保持する。
        /// </summary>
        HWND m_sceneViewPanel = nullptr;

        /// <summary>
        /// SceneView 埋め込み方針を説明するラベルを保持する。
        /// </summary>
        HWND m_sceneViewPlanLabel = nullptr;

        /// <summary>
        /// SceneView 描画対象の child HWND を保持する。
        /// </summary>
        HWND m_sceneViewHost = nullptr;

        /// <summary>
        /// SceneView 現在サイズを説明するラベルを保持する。
        /// </summary>
        HWND m_sceneViewSizeLabel = nullptr;

        /// <summary>
        /// 既定 GUI フォントを保持する。
        /// </summary>
        HFONT m_defaultFont = nullptr;

        /// <summary>
        /// SceneView へ採用する埋め込み方針を表す。
        /// </summary>
        SceneViewEmbeddingMode m_sceneViewEmbeddingMode = SceneViewEmbeddingMode::ChildWindowSwapChainHost;

        /// <summary>
        /// 前回レイアウト時の SceneView 幅を保持する。
        /// </summary>
        std::uint32_t m_sceneViewWidth = 0;

        /// <summary>
        /// 前回レイアウト時の SceneView 高さを保持する。
        /// </summary>
        std::uint32_t m_sceneViewHeight = 0;

        /// <summary>
        /// Assets パネルの一覧表示に使う ListBox を保持する。
        /// </summary>
        HWND m_assetsListBox = nullptr;

        /// <summary>
        /// Assets パネルの要約表示ラベルを保持する。
        /// </summary>
        HWND m_assetsSummaryLabel = nullptr;

        /// <summary>
        /// Hierarchy パネルの要約表示ラベルを保持する。
        /// </summary>
        HWND m_hierarchySummaryLabel = nullptr;

        /// <summary>
        /// Hierarchy パネルの一覧表示に使う ListBox を保持する。
        /// </summary>
        HWND m_hierarchyListBox = nullptr;

        /// <summary>
        /// Editor が編集中の Scene を保持する。
        /// </summary>
        std::unique_ptr<Game::Scene> m_scene;

        /// <summary>
        /// Editor が参照する SpriteAsset レジストリを保持する。
        /// </summary>
        Game::Assets::SpriteAssetRegistry m_spriteAssetRegistry{};

        /// <summary>
        /// Editor が参照する Texture レジストリを保持する。
        /// </summary>
        Graphics::TextureAssetRegistry m_textureAssetRegistry{};

        /// <summary>
        /// 登録済み SpriteAssetId 一覧を保持する。
        /// </summary>
        std::vector<Core::AssetId> m_registeredSpriteAssetIds{};

        /// <summary>
        /// Assets パネルへ表示する SpriteAssetId 一覧を保持する。
        /// </summary>
        std::vector<Core::AssetId> m_visibleSpriteAssetIds{};

        /// <summary>
        /// Assets パネルで現在選択中の SpriteAssetId を保持する。
        /// </summary>
        Core::AssetId m_selectedSpriteAssetId{};

        /// <summary>
        /// Hierarchy パネルへ表示する EntityId 一覧を保持する。
        /// </summary>
        std::vector<Game::EntityId> m_visibleEntityIds{};

        /// <summary>
        /// Hierarchy パネルで現在選択中の EntityId を保持する。
        /// </summary>
        std::optional<Game::EntityId> m_selectedEntityId{};
    };
}
