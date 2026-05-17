#pragma once

#include <memory>

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
}
