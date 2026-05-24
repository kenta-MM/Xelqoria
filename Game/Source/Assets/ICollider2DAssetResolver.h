#pragma once

#include <optional>

#include "AssetId.h"
#include "Collider2DAsset.h"

namespace Xelqoria::Game::Assets
{
	/// <summary>
	/// Collider2DAsset を AssetId から解決する抽象インターフェースを表す。
	/// </summary>
	class ICollider2DAssetResolver
	{
	public:
		virtual ~ICollider2DAssetResolver() = default;

		/// <summary>
		/// 指定した AssetId に対応する Collider2DAsset を取得する。
		/// </summary>
		/// <param name="assetId">取得対象の Collider2DAsset 識別子。</param>
		/// <returns>解決できた Collider2DAsset。未解決時は空。</returns>
		virtual std::optional<Collider2DAsset> ResolveCollider2DAsset(const Core::AssetId& assetId) const = 0;
	};
}
