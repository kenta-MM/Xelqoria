#pragma once

#include <string>
#include <string_view>
#include <utility>

namespace Xelqoria::Core
{
    /// <summary>
    /// アセット参照を文字列パスから切り離して扱うための最小識別子。
    /// </summary>
    class AssetId
    {
    public:
        /// <summary>
        /// 空の AssetId を生成する。
        /// </summary>
        AssetId() = default;

        /// <summary>
        /// C 文字列から AssetId を生成する。
        /// </summary>
        /// <param name="value">識別子として保持する文字列。</param>
        AssetId(const char* value)
            : m_value(value != nullptr ? value : "")
        {
        }

        /// <summary>
        /// 文字列ビューから AssetId を生成する。
        /// </summary>
        /// <param name="value">識別子として保持する文字列。</param>
        AssetId(std::string_view value)
            : m_value(value)
        {
        }

        /// <summary>
        /// 所有する文字列から AssetId を生成する。
        /// </summary>
        /// <param name="value">識別子として保持する文字列。</param>
        AssetId(std::string value)
            : m_value(std::move(value))
        {
        }

        /// <summary>
        /// 識別子文字列が空かどうかを取得する。
        /// </summary>
        /// <returns>空の場合は true。</returns>
        bool IsEmpty() const
        {
            return m_value.empty();
        }

        /// <summary>
        /// 保持している識別子文字列を取得する。
        /// </summary>
        /// <returns>識別子文字列。</returns>
        const std::string& GetValue() const
        {
            return m_value;
        }

        /// <summary>
        /// 2 つの AssetId が同一かどうかを判定する。
        /// </summary>
        /// <param name="lhs">左辺の AssetId。</param>
        /// <param name="rhs">右辺の AssetId。</param>
        /// <returns>同一の場合は true。</returns>
        friend bool operator==(const AssetId& lhs, const AssetId& rhs) = default;

    private:
        std::string m_value{};
    };
}
