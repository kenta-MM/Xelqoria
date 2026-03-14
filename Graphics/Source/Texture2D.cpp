#include "Texture2D.h"

#include <utility>

#include "IGraphicsContext.h"
#include "ITexture.h"

namespace Xelqoria::Graphics
{
    bool Texture2D::LoadFromFile(const std::wstring& filePath, RHI::IGraphicsContext& graphicsContext)
    {
        SetRHITexture(graphicsContext.CreateTextureFromFile(filePath));
        return static_cast<bool>(m_texture);
    }

    void Texture2D::SetRHITexture(std::shared_ptr<RHI::ITexture> texture)
    {
        m_texture = std::move(texture);

        if (m_texture)
        {
            m_width = m_texture->GetWidth();
            m_height = m_texture->GetHeight();
        }
        else
        {
            m_width = 0;
            m_height = 0;
        }
    }
}
