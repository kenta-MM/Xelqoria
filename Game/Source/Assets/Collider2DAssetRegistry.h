#pragma once

#include <string>
#include <unordered_map>

#include "ICollider2DAssetResolver.h"

namespace Xelqoria::Game::Assets
{
	/// <summary>
	/// AssetId と Collider2DAsset の対応を保持する簡易レジストリを表す。
	/// </summary>
	class Collider2DAssetRegistry final : public ICollider2DAssetResolver
	{
	public:
		/// <summary>
		/// Collider2DAsset を登録する。
		/// </summary>
		/// <param name="assetId">登録に使用する Collider2DAsset 識別子。</param>
		/// <param name="collider2DAsset">登録する Collider2DAsset。</param>
		void RegisterCollider2DAsset(Core::AssetId assetId, Collider2DAsset collider2DAsset);

		/// <summary>
		/// 指定した AssetId に対応する Collider2DAsset を取得する。
		/// </summary>
		/// <param name="assetId">取得対象の Collider2DAsset 識別子。</param>
		/// <returns>解決できた Collider2DAsset。未解決時は空。</returns>
		std::optional<Collider2DAsset> ResolveCollider2DAsset(const Core::AssetId& assetId) const override;

	private:
		std::unordered_map<std::string, Collider2DAsset> m_collider2DAssets{};
	};
}
