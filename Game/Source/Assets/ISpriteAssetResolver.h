#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

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
		void RegisterSpriteAsset(Core::AssetId assetId, SpriteAsset spriteAsset)
		{
			m_spriteAssets[assetId.GetValue()] = std::move(spriteAsset);
		}

		/// <summary>
		/// 指定した AssetId に対応する SpriteAsset を取得する。
		/// </summary>
		/// <param name="assetId">取得対象の SpriteAsset 識別子。</param>
		/// <returns>解決できた SpriteAsset。未解決時は空。</returns>
		std::optional<SpriteAsset> ResolveSpriteAsset(const Core::AssetId& assetId) const override
		{
			const auto it = m_spriteAssets.find(assetId.GetValue());
			if (it == m_spriteAssets.end()) {
				return std::nullopt;
			}

			return it->second;
		}

	private:
		std::unordered_map<std::string, SpriteAsset> m_spriteAssets;
	};
}
