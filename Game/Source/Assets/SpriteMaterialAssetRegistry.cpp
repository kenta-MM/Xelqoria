#include "SpriteMaterialAssetRegistry.h"

#include <utility>

#include "AssetId.h"
#include "SpriteMaterialAsset.h"

namespace Xelqoria::Game::Assets
{
	void SpriteMaterialAssetRegistry::RegisterMaterialAsset(Core::AssetId assetId, SpriteMaterialAsset materialAsset)
	{
		m_materialAssets[assetId.GetValue()] = std::move(materialAsset);
	}

	std::optional<SpriteMaterialAsset> SpriteMaterialAssetRegistry::ResolveMaterialAsset(const Core::AssetId& assetId) const
	{
		const auto it = m_materialAssets.find(assetId.GetValue());
		if (it == m_materialAssets.end())
		{
			return std::nullopt;
		}

		return it->second;
	}
}
