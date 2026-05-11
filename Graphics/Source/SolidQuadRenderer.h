#pragma once

#include <array>
#include <cstdint>

#include "IGraphicsContext.h"
#include "QuadTransformFactory.h"

namespace Xelqoria::Graphics
{
    /// <summary>
    /// SceneView オーバーレイなどで使用する単色クアッドを表す。
    /// </summary>
    struct SolidQuad
    {
        /// <summary>
        /// ビューポート中心基準の X 座標を表す。
        /// </summary>
        float centerX = 0.0f;

        /// <summary>
        /// ビューポート中心基準の Y 座標を表す。
        /// </summary>
        float centerY = 0.0f;

        /// <summary>
        /// ピクセル単位の幅を表す。
        /// </summary>
        float width = 1.0f;

        /// <summary>
        /// ピクセル単位の高さを表す。
        /// </summary>
        float height = 1.0f;

        /// <summary>
        /// RGBA 順の塗りつぶし色を表す。
        /// </summary>
        std::array<float, 4> color{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    /// <summary>
    /// テクスチャを使わない単色クアッドを描画する軽量レンダラー。
    /// </summary>
    class SolidQuadRenderer
    {
    public:
        /// <summary>
        /// レンダラーを生成する。
        /// </summary>
        /// <param name="context">描画に使用するグラフィックスコンテキスト。</param>
        explicit SolidQuadRenderer(RHI::IGraphicsContext& context)
            : m_context(&context)
        {
        }

        /// <summary>
        /// 単色クアッドを描画する。
        /// </summary>
        /// <param name="quad">描画するクアッド情報。</param>
        void Draw(const SolidQuad& quad)
        {
            if (m_context == nullptr)
            {
                return;
            }

            const std::uint32_t viewportWidth = m_context->GetViewportWidth();
            const std::uint32_t viewportHeight = m_context->GetViewportHeight();
            if (viewportWidth == 0 || viewportHeight == 0 || quad.width <= 0.0f || quad.height <= 0.0f)
            {
                return;
            }

            m_context->BindTexture(0, nullptr);
            const QuadRenderConstants quadRenderConstants = MakeSolidQuadTransform(
                quad.centerX,
                quad.centerY,
                quad.width,
                quad.height,
                viewportWidth,
                viewportHeight,
                quad.color);
            ApplyQuadRenderConstants(*m_context, quadRenderConstants);
            m_context->Draw(6, 0);
        }

    private:
        RHI::IGraphicsContext* m_context = nullptr;
    };
}
