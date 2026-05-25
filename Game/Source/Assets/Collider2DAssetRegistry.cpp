#include "Assets/Collider2DAssetRegistry.h"

#include <utility>

namespace Xelqoria::Game::Assets
{
	void Collider2DAssetRegistry::RegisterCollider2DAsset(Core::AssetId assetId, Collider2DAsset collider2DAsset)
	{
		m_collider2DAssets[assetId.GetValue()] = std::move(collider2DAsset);
	}

	std::optional<Collider2DAsset> Collider2DAssetRegistry::ResolveCollider2DAsset(const Core::AssetId& assetId) const
	{
		const auto it = m_collider2DAssets.find(assetId.GetValue());
		if (it == m_collider2DAssets.end())
		{
			return std::nullopt;
		}

		return it->second;
	}
}
