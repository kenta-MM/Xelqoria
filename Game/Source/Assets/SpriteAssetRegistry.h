#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "ISpriteAssetResolver.h"

namespace Xelqoria::Game::Assets
{
	/// <summary>
	/// AssetId と SpriteAsset の対応を保持する簡易レジストリを表す。
	/// </summary>
	class SpriteAssetRegistry final : public ISpriteAssetResolver
	{
	public:
		/// <summary>
		/// SpriteAsset を登録する。
		/// </summary>
		/// <param name="assetId">登録に使用する SpriteAsset 識別子。</param>
		/// <param name="spriteAsset">登録する SpriteAsset。</param>
		void RegisterSpriteAsset(Core::AssetId assetId, SpriteAsset spriteAsset);

		/// <summary>
		/// 指定した AssetId に対応する SpriteAsset を取得する。
		/// </summary>
		/// <param name="assetId">取得対象の SpriteAsset 識別子。</param>
		/// <returns>解決できた SpriteAsset。未解決時は空。</returns>
		std::optional<SpriteAsset> ResolveSpriteAsset(const Core::AssetId& assetId) const override;

	private:
		std::unordered_map<std::string, SpriteAsset> m_spriteAssets;
	};
}
