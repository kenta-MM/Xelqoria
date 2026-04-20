#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Xelqoria::Backends::D3D11
{
    /// <summary>
    /// Direct3D 11 バックエンド向けの画像ファイル読込を提供する。
    /// </summary>
    class D3D11TextureLoader
    {
    public:
        /// <summary>
        /// 画像ファイルを RGBA8 ピクセル配列として読み込む。
        /// </summary>
        /// <param name="filePath">読み込む画像ファイルパス。</param>
        /// <param name="outPixels">読み込み結果のピクセル配列。</param>
        /// <param name="outWidth">読み込んだ画像の幅。</param>
        /// <param name="outHeight">読み込んだ画像の高さ。</param>
        /// <returns>読み込みに成功した場合は true。</returns>
        static bool LoadRgbaPixelsFromFile(
            const std::wstring& filePath,
            std::vector<std::uint8_t>& outPixels,
            std::uint32_t& outWidth,
            std::uint32_t& outHeight);
    };
}
