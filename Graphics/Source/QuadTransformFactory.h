#pragma once

#include <array>
#include <cstdint>

#include "IGraphicsContext.h"
#include "SpriteRenderMath.h"

namespace Xelqoria::Graphics
{
    /// <summary>
    /// SpriteQuadTransform と描画設定から RHI 用 QuadTransform2D を構築する。
    /// </summary>
    /// <param name="quadTransform">Sprite の正規化済み変換値。</param>
    /// <param name="outlineEnabled">外枠描画を有効にする場合は true。</param>
    /// <param name="outlineThickness">外枠太さ。</param>
    /// <param name="outlineColor">RGBA 順の外枠色。</param>
    /// <returns>RHI に渡す QuadTransform2D。</returns>
    [[nodiscard]] inline RHI::QuadTransform2D MakeTexturedQuadTransform(
        const SpriteQuadTransform& quadTransform,
        bool outlineEnabled,
        float outlineThickness,
        const std::array<float, 4>& outlineColor)
    {
        RHI::QuadTransform2D result{};
        result.scaleX = quadTransform.scaleX;
        result.scaleY = quadTransform.scaleY;
        result.rotationCos = quadTransform.rotationCos;
        result.rotationSin = quadTransform.rotationSin;
        result.translateX = quadTransform.translateX;
        result.translateY = quadTransform.translateY;
        result.outlineEnabled = outlineEnabled ? 1.0f : 0.0f;
        result.outlineThickness = outlineThickness;
        result.outlineColorR = outlineColor[0];
        result.outlineColorG = outlineColor[1];
        result.outlineColorB = outlineColor[2];
        result.outlineColorA = outlineColor[3];
        result.textureEnabled = 1.0f;
        return result;
    }

    /// <summary>
    /// ビューポート中心基準の単色クアッドから RHI 用 QuadTransform2D を構築する。
    /// </summary>
    /// <param name="centerX">描画中心の X 座標。</param>
    /// <param name="centerY">描画中心の Y 座標。</param>
    /// <param name="width">ピクセル単位の幅。</param>
    /// <param name="height">ピクセル単位の高さ。</param>
    /// <param name="viewportWidth">描画先ビューポート幅。</param>
    /// <param name="viewportHeight">描画先ビューポート高さ。</param>
    /// <param name="fillColor">RGBA 順の塗りつぶし色。</param>
    /// <returns>RHI に渡す QuadTransform2D。</returns>
    [[nodiscard]] inline RHI::QuadTransform2D MakeSolidQuadTransform(
        float centerX,
        float centerY,
        float width,
        float height,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight,
        const std::array<float, 4>& fillColor)
    {
        RHI::QuadTransform2D result{};
        result.scaleX = (2.0f * width) / static_cast<float>(viewportWidth);
        result.scaleY = (2.0f * height) / static_cast<float>(viewportHeight);
        result.rotationCos = 1.0f;
        result.rotationSin = 0.0f;
        result.translateX = (2.0f * centerX) / static_cast<float>(viewportWidth);
        result.translateY = (-2.0f * centerY) / static_cast<float>(viewportHeight);
        result.fillColorR = fillColor[0];
        result.fillColorG = fillColor[1];
        result.fillColorB = fillColor[2];
        result.fillColorA = fillColor[3];
        result.textureEnabled = 0.0f;
        return result;
    }
}
