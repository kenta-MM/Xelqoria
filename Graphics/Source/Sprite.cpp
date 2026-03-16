#include "Sprite.h"

#include <utility>

namespace Xelqoria::Graphics
{
	void Sprite::SetTexture(std::shared_ptr <Texture2D> texture)
	{
		m_texture = std::move(texture);
	}

	std::shared_ptr<Texture2D> Sprite::GetTexture() const
	{
		return m_texture;
	}

	void Sprite::SetTextureAssetId(Core::AssetId assetId)
	{
		m_textureAssetId = std::move(assetId);
	}

	const Core::AssetId& Sprite::GetTextureAssetId() const
	{
		return m_textureAssetId;
	}
}
