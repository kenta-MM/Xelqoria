#pragma once

#include <optional>

#include "AssetId.h"
#include "SpriteMaterialAsset.h"

namespace Xelqoria::Game::Assets
{
	/// <summary>
	/// MaterialAssetId から SpriteMaterialAsset を解決するインターフェース。
	/// </summary>
	class IMaterialAssetResolver
	{
	public:
		/// <summary>
		/// MaterialAssetId に対応する SpriteMaterialAsset を取得する。
		/// </summary>
		/// <param name="assetId">取得対象の MaterialAssetId。</param>
		/// <returns>解決できた SpriteMaterialAsset。未解決時は空。</returns>
		virtual std::optional<SpriteMaterialAsset> ResolveMaterialAsset(const Core::AssetId& assetId) const = 0;

	protected:
		/// <summary>
		/// 派生型経由で破棄されることを許可する。
		/// </summary>
		virtual ~IMaterialAssetResolver() = default;
	};
}
