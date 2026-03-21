#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "AssetId.h"

namespace Xelqoria::Graphics
{
	class Texture2D;

	/// <summary>
	/// Texture2D を AssetId から解決する抽象インターフェースを表す。
	/// </summary>
	class ITextureAssetResolver
	{
	public:
		/// <summary>
		/// Resolver を破棄する。
		/// </summary>
		virtual ~ITextureAssetResolver() = default;

		/// <summary>
		/// 指定した AssetId に対応する Texture2D を取得する。
		/// </summary>
		/// <param name="assetId">取得対象のテクスチャアセット識別子。</param>
		/// <returns>解決できた Texture2D。未解決時は空。</returns>
		virtual std::shared_ptr<Texture2D> ResolveTexture(const Core::AssetId& assetId) const = 0;
	};

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
		void RegisterTexture(Core::AssetId assetId, std::shared_ptr<Texture2D> texture)
		{
			m_textures[assetId.GetValue()] = std::move(texture);
		}

		/// <summary>
		/// 指定した AssetId に対応する Texture2D を取得する。
		/// </summary>
		/// <param name="assetId">取得対象のテクスチャアセット識別子。</param>
		/// <returns>解決できた Texture2D。未解決時は空。</returns>
		std::shared_ptr<Texture2D> ResolveTexture(const Core::AssetId& assetId) const override
		{
			const auto it = m_textures.find(assetId.GetValue());
			if (it == m_textures.end()) {
				return nullptr;
			}

			return it->second;
		}

	private:
		std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_textures;
	};
}
