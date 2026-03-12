#pragma once

#include <cstdint>

namespace Xelqoria::RHI
{
	/// <summary>
	/// テクスチャサイズ取得を提供する最小 RHI テクスチャインターフェース。
	/// </summary>
	class ITexture
	{
	public:
		virtual ~ITexture() = default;

		/// <summary>
		/// テクスチャ幅を返す。
		/// </summary>
		/// <returns>
		/// テクスチャ幅（ピクセル）。
		/// </returns>
		virtual uint32_t GetWidth() const = 0;

		/// <summary>
		/// テクスチャ高さを返す。
		/// </summary>
		/// <returns>
		/// テクスチャ高さ（ピクセル）。
		/// </returns>
		virtual uint32_t GetHeight() const = 0;
	};
}
