#pragma once

#include <optional>

#include "SpriteCulling.h"

namespace Xelqoria::RHI
{
    class IGraphicsContext;
}

namespace Xelqoria::Graphics
{
    class Sprite;
    struct SpriteDrawInput;

    /// <summary>
    /// Sprite を描画するレンダラー。
    /// </summary>
    class SpriteRenderer
    {
    public:
        /// <summary>
        /// レンダラーを生成する。
        /// </summary>
        /// <param name="context">描画に使用するグラフィックスコンテキスト。</param>
        explicit SpriteRenderer(RHI::IGraphicsContext& context);

        /// <summary>
        /// 描画バッチを開始する。
        /// </summary>
        void Begin();

        /// <summary>
        /// スプライトを描画キューへ追加する。
        /// </summary>
        /// <param name="sprite">描画対象のスプライト。</param>
        void Draw(const Sprite& sprite);

        /// <summary>
        /// 共通 Sprite 入力データを描画キューへ追加する。
        /// </summary>
        /// <param name="input">描画対象の共通 Sprite 入力データ。</param>
        void Draw(const SpriteDrawInput& input);

        /// <summary>
        /// Sprite 描画で使用するカリング範囲を設定する。
        /// </summary>
        /// <param name="cullRect">描画対象として扱う範囲。</param>
        void SetCullingRect(const SpriteCullRect& cullRect);

        /// <summary>
        /// 明示的に設定したカリング範囲を解除し、ビューポート範囲を使用する。
        /// </summary>
        void ClearCullingRect();

        /// <summary>
        /// 描画バッチを終了して送信する。
        /// </summary>
        void End();

    private:
        RHI::IGraphicsContext* m_context = nullptr;
        std::optional<SpriteCullRect> m_cullingRect{};
        bool m_isDrawing = false;
    };

    /// <summary>
    /// CPU SpriteBatch として利用する Sprite 描画 API を表す。
    /// </summary>
    using SpriteBatch = SpriteRenderer;

    /// <summary>
    /// GPU InstancedSpriteRenderer として利用する Sprite 描画 API を表す。
    /// </summary>
    using InstancedSpriteRenderer = SpriteRenderer;
}
