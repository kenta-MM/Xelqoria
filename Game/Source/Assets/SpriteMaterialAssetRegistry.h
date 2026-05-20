#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "IMaterialAssetResolver.h"

namespace Xelqoria::Game::Assets
{
	/// <summary>
	/// AssetId と SpriteMaterialAsset の対応を保持する簡易レジストリを表す。
	/// </summary>
	class SpriteMaterialAssetRegistry final : public IMaterialAssetResolver
	{
	public:
		/// <summary>
		/// SpriteMaterialAsset を登録する。
		/// </summary>
		/// <param name="assetId">登録に使用する MaterialAssetId。</param>
		/// <param name="materialAsset">登録する Material アセット。</param>
		void RegisterMaterialAsset(Core::AssetId assetId, SpriteMaterialAsset materialAsset);

		/// <summary>
		/// 指定した AssetId に対応する SpriteMaterialAsset を取得する。
		/// </summary>
		/// <param name="assetId">取得対象の MaterialAssetId。</param>
		/// <returns>解決できた SpriteMaterialAsset。未解決時は空。</returns>
		std::optional<SpriteMaterialAsset> ResolveMaterialAsset(const Core::AssetId& assetId) const override;

	private:
		std::unordered_map<std::string, SpriteMaterialAsset> m_materialAssets;
	};
}
