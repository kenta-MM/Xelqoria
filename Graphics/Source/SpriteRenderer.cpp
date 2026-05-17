#include "SpriteRenderer.h"

#include "SpriteCulling.h"
#include "SpriteRenderMath.h"
#include "QuadTransformFactory.h"
#include "Sprite.h"
#include "SpriteDrawInput.h"
#include "Texture2D.h"
#include "IGraphicsContext.h"

namespace Xelqoria::Graphics
{
    SpriteRenderer::SpriteRenderer(RHI::IGraphicsContext& context)
        : m_context(&context)
    {
    }

    void SpriteRenderer::Begin()
    {
        m_isDrawing = true;
    }

    void SpriteRenderer::Draw(const Sprite& sprite)
    {
        Draw(sprite.ToDrawInput());
    }

    void SpriteRenderer::Draw(const SpriteDrawInput& input)
    {
        if (!m_isDrawing || !m_context)
        {
            return;
        }

        const SpriteCullRect viewportCullRect = MakeViewportCullRect(
            m_context->GetViewportWidth(),
            m_context->GetViewportHeight());
        const SpriteCullRect& activeCullRect = m_cullingRect.has_value() ? m_cullingRect.value() : viewportCullRect;
        if (false == IsSpriteVisible(input, activeCullRect))
        {
            return;
        }

        const auto texture = input.texture;
        if (!texture)
        {
            return;
        }

        const auto rhiTexture = texture->GetRHITexture();
        if (!rhiTexture)
        {
            return;
        }

        m_context->BindTexture(0, rhiTexture.get());
        const SpriteQuadTransform quadTransform = ComputeSpriteQuadTransform(
            input,
            m_context->GetViewportWidth(),
            m_context->GetViewportHeight());
        const QuadRenderConstants quadRenderConstants = MakeTexturedQuadTransform(
            quadTransform,
            input.outlineEnabled,
            input.outlineThickness,
            input.outlineColor,
            input.color);
        ApplyQuadRenderConstants(*m_context, quadRenderConstants);

        // 1スプライトを2三角形(6頂点)で描画する前提。
        m_context->Draw(6, 0);
    }

    void SpriteRenderer::SetCullingRect(const SpriteCullRect& cullRect)
    {
        m_cullingRect = cullRect;
    }

    void SpriteRenderer::ClearCullingRect()
    {
        m_cullingRect.reset();
    }

    void SpriteRenderer::End()
    {
        if (!m_context)
        {
            return;
        }

        m_context->BindTexture(0, nullptr);
        m_isDrawing = false;
    }
}
