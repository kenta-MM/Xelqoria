#pragma once

#include <cstdint>

namespace Xelqoria::RHI
{
    /// <summary>
    /// インデックスバッファの要素型を表す。
    /// </summary>
    enum class IndexType
    {
        UInt16,
        UInt32
    };

    /// <summary>
    /// インデックスバッファの基本情報を提供する RHI インターフェース。
    /// </summary>
    class IIndexBuffer
    {
    public:
        virtual ~IIndexBuffer() = default;

        /// <summary>
        /// 保持しているインデックス数を返す。
        /// </summary>
        /// <returns>
        /// インデックス数。
        /// </returns>
        virtual uint32_t GetCount() const = 0;

        /// <summary>
        /// インデックス要素型を返す。
        /// </summary>
        /// <returns>
        /// インデックス要素型。
        /// </returns>
        virtual IndexType GetIndexType() const = 0;
    };
}
