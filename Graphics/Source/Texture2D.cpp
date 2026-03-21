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

}
