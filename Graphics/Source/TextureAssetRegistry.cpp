#include "TextureAssetRegistry.h"

#include <utility>
#include <memory>
#include <AssetId.h>
#include "ITextureAssetResolver.h"

namespace Xelqoria::Graphics
{
	void TextureAssetRegistry::RegisterTexture(Core::AssetId assetId, std::shared_ptr<Texture2D> texture)
	{
		m_textures[assetId.GetValue()] = std::move(texture);
	}

	std::shared_ptr<Texture2D> TextureAssetRegistry::ResolveTexture(const Core::AssetId& assetId) const
	{
		const auto it = m_textures.find(assetId.GetValue());
		if (it == m_textures.end()) {
			return nullptr;
		}

		return it->second;
	}
}
