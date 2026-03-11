#pragma once

namespace Xelqoria::RHI
{
    class IGraphicsContext;
}

namespace Xelqoria::Graphics
{
    class Sprite;

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
        /// 描画バッチを終了して送信する。
        /// </summary>
        void End();

    private:
        RHI::IGraphicsContext* m_context = nullptr;
        bool m_isDrawing = false;
    };
}
