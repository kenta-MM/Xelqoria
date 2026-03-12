#pragma once

#include <cstdint>

namespace Xelqoria::RHI
{
	/// <summary>
	/// 頂点バッファのサイズ情報を提供する RHI インターフェース。
	/// </summary>
	class IVertexBuffer
	{
	public:
		virtual ~IVertexBuffer() = default;

		/// <summary>
		/// バッファ全体のバイト数を返す。
		/// </summary>
		/// <returns>
		/// バッファサイズ（バイト）。
		/// </returns>
		virtual uint32_t GetBufferSize() const = 0;

		/// <summary>
		/// 1 頂点あたりのバイト数を返す。
		/// </summary>
		/// <returns>
		/// 頂点ストライド（バイト）。
		/// </returns>
		virtual uint32_t GetStrideSize() const = 0;
	};
}
