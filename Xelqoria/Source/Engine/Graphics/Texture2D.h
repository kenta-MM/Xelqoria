#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace Xelqoria::RHI
{
    class IGraphicsContext;
    class ITexture;
}

namespace Xelqoria::Graphics
{
    /// <summary>
    /// Graphics 層で扱う 2D テクスチャ抽象。
    /// </summary>
    class Texture2D
    {
    public:
        /// <summary>
        /// 空のテクスチャオブジェクトを生成する。
        /// </summary>
        Texture2D() = default;

        /// <summary>
        /// テクスチャオブジェクトを破棄する。
        /// </summary>
        ~Texture2D() = default;

        /// <summary>
        /// ファイルからテクスチャを読み込み、RHI テクスチャを作成する。
        /// </summary>
        /// <param name="filePath">読み込むテクスチャファイルパス。</param>
        /// <param name="graphicsContext">テクスチャ生成に使用するグラフィックスコンテキスト。</param>
        /// <returns>読み込みと作成に成功した場合は true。</returns>
        bool LoadFromFile(const std::wstring& filePath, RHI::IGraphicsContext& graphicsContext);

        /// <summary>
        /// テクスチャ幅を取得する。
        /// </summary>
        /// <returns>テクスチャ幅（ピクセル）。</returns>
        std::uint32_t GetWidth() const { return m_width; }

        /// <summary>
        /// テクスチャ高さを取得する。
        /// </summary>
        /// <returns>テクスチャ高さ（ピクセル）。</returns>
        std::uint32_t GetHeight() const { return m_height; }

        /// <summary>
        /// 内部で参照する RHI テクスチャを設定する。
        /// </summary>
        /// <param name="texture">設定する RHI テクスチャ。</param>
        void SetRHITexture(std::shared_ptr<RHI::ITexture> texture);

        /// <summary>
        /// 内部で参照している RHI テクスチャを取得する。
        /// </summary>
        /// <returns>保持中の RHI テクスチャ。</returns>
        std::shared_ptr<RHI::ITexture> GetRHITexture() const { return m_texture; }

    private:
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;

        std::shared_ptr<RHI::ITexture> m_texture;
    };
}
