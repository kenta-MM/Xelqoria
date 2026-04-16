#pragma once

#include <memory>
#include <optional>

#include "Assets/SpriteAssetRegistry.h"
#include "EditorCamera2D.h"
#include "GraphicsAPI.h"
#include "IGraphicsContext.h"
#include "Scene.h"
#include "SceneViewController.h"
#include "SolidQuadRenderer.h"
#include "SpriteRenderer.h"
#include "TextureAssetRegistry.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// SceneView の描画初期化とフレーム描画を管理する。
    /// </summary>
    class SceneViewRenderer
    {
    public:
        /// <summary>
        /// SceneView 描画コンテキストを初期化する。
        /// </summary>
        /// <param name="hInstance">Windows アプリケーションインスタンス。</param>
        /// <param name="hostWindow">描画先 child window。</param>
        /// <param name="width">描画幅。</param>
        /// <param name="height">描画高さ。</param>
        /// <param name="api">使用する GraphicsAPI。</param>
        /// <returns>初期化に成功した場合は true。</returns>
        bool Initialize(HINSTANCE hInstance, HWND hostWindow, std::uint32_t width, std::uint32_t height, RHI::GraphicsAPI api);

        /// <summary>
        /// 保持している描画コンテキストを解放する。
        /// </summary>
        void Shutdown();

        /// <summary>
        /// SceneView サイズ変更を描画コンテキストへ反映する。
        /// </summary>
        /// <param name="width">新しい描画幅。</param>
        /// <param name="height">新しい描画高さ。</param>
        void Resize(std::uint32_t width, std::uint32_t height);

        /// <summary>
        /// 現在の描画コンテキストを取得する。
        /// </summary>
        /// <returns>描画コンテキスト。</returns>
        [[nodiscard]] RHI::IGraphicsContext& GetGraphicsContext();

        /// <summary>
        /// SceneView の 1 フレーム描画を行う。
        /// </summary>
        /// <param name="scene">描画対象の Scene。</param>
        /// <param name="spriteAssetRegistry">SpriteAsset レジストリ。</param>
        /// <param name="textureAssetRegistry">Texture レジストリ。</param>
        /// <param name="camera">SceneView 用カメラ。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        /// <param name="dragPreviewState">現在のドラッグプレビュー状態。</param>
        void RenderFrame(
            Game::Scene* scene,
            Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
            Graphics::TextureAssetRegistry& textureAssetRegistry,
            const EditorCamera2D& camera,
            std::optional<Game::EntityId> selectedEntityId,
            const SceneDragPreviewState& dragPreviewState);

    private:
        std::unique_ptr<RHI::IGraphicsContext> m_graphics;
        std::unique_ptr<Graphics::SpriteRenderer> m_spriteRenderer;
        std::unique_ptr<Graphics::SolidQuadRenderer> m_solidQuadRenderer;
        std::uint32_t m_sceneViewWidth = 0;
        std::uint32_t m_sceneViewHeight = 0;
    };
}
