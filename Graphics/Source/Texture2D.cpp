#include "Texture2D.h"

#include "IGraphicsContext.h"
#include <string>

namespace Xelqoria::Graphics
{
    bool Texture2D::LoadFromFile(const std::wstring& filePath, RHI::IGraphicsContext& graphicsContext)
    {
        SetRHITexture(graphicsContext.CreateTextureFromFile(filePath));
        return static_cast<bool>(m_texture);
    }

}
