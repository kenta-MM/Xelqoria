#include "SpriteAssetRegistry.h"

#include <utility>
#include <optional>
#include <AssetId.h>
#include "SpriteAsset.h"

namespace Xelqoria::Game::Assets
{
	void SpriteAssetRegistry::RegisterSpriteAsset(Core::AssetId assetId, SpriteAsset spriteAsset)
	{
		m_spriteAssets[assetId.GetValue()] = std::move(spriteAsset);
	}

	std::optional<SpriteAsset> SpriteAssetRegistry::ResolveSpriteAsset(const Core::AssetId& assetId) const
	{
		const auto it = m_spriteAssets.find(assetId.GetValue());
		if (it == m_spriteAssets.end()) {
			return std::nullopt;
		}

		return it->second;
	}
}
