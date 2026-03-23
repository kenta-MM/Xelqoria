#pragma once

#include <optional>

#include "AssetId.h"
#include "SpriteAsset.h"

namespace Xelqoria::Game::Assets
{
	/// <summary>
	/// SpriteAsset を AssetId から解決する抽象インターフェースを表す。
	/// </summary>
	class ISpriteAssetResolver
	{
	public:
		/// <summary>
		/// Resolver を破棄する。
		/// </summary>
		virtual ~ISpriteAssetResolver() = default;

		/// <summary>
		/// 指定した AssetId に対応する SpriteAsset を取得する。
		/// </summary>
		/// <param name="assetId">取得対象の SpriteAsset 識別子。</param>
		/// <returns>解決できた SpriteAsset。未解決時は空。</returns>
		virtual std::optional<SpriteAsset> ResolveSpriteAsset(const Core::AssetId& assetId) const = 0;
	};
}
