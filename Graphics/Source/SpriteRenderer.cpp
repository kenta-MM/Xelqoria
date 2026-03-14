#include "SpriteRenderer.h"

#include "Sprite.h"
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
        if (!m_isDrawing || !m_context)
        {
            return;
        }

        const auto texture = sprite.GetTexture();
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

        // 1スプライトを2三角形(6頂点)で描画する前提。
        m_context->Draw(6, 0);
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
