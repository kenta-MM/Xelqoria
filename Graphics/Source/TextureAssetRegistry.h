#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "ITextureAssetResolver.h"

namespace Xelqoria::Graphics
{
	/// <summary>
	/// AssetId と Texture2D の対応を保持する簡易レジストリを表す。
	/// </summary>
	class TextureAssetRegistry final : public ITextureAssetResolver
	{
	public:
		/// <summary>
		/// テクスチャアセットを登録する。
		/// </summary>
		/// <param name="assetId">登録に使用するテクスチャアセット識別子。</param>
		/// <param name="texture">登録する Texture2D。</param>
		void RegisterTexture(Core::AssetId assetId, std::shared_ptr<Texture2D> texture);

		/// <summary>
		/// 指定した AssetId に対応する Texture2D を取得する。
		/// </summary>
		/// <param name="assetId">取得対象のテクスチャアセット識別子。</param>
		/// <returns>解決できた Texture2D。未解決時は空。</returns>
		std::shared_ptr<Texture2D> ResolveTexture(const Core::AssetId& assetId) const override;

	private:
		std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_textures;
	};
}
