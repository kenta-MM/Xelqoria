#pragma once

#include <array>
#include <cstdint>
#include <cstddef>

#include "IGraphicsContext.h"
#include "SpriteRenderMath.h"

namespace Xelqoria::Graphics
{
    inline constexpr std::size_t QuadRenderConstantFloatCount = 20;

    /// <summary>
    /// Quad 描画パイプラインへ渡す Graphics 層の描画定数を表す。
    /// </summary>
    struct QuadRenderConstants
    {
        /// <summary>
        /// X 軸方向の拡大量を表す。
        /// </summary>
        float scaleX = 1.0f;

        /// <summary>
        /// Y 軸方向の拡大量を表す。
        /// </summary>
        float scaleY = 1.0f;

        /// <summary>
        /// 回転角度の cos 値を表す。
        /// </summary>
        float rotationCos = 1.0f;

        /// <summary>
        /// 回転角度の sin 値を表す。
        /// </summary>
        float rotationSin = 0.0f;

        /// <summary>
        /// X 軸方向の移動量を表す。
        /// </summary>
        float translateX = 0.0f;

        /// <summary>
        /// Y 軸方向の移動量を表す。
        /// </summary>
        float translateY = 0.0f;

        /// <summary>
        /// 外枠描画が有効な場合は 1.0f、それ以外は 0.0f を表す。
        /// </summary>
        float outlineEnabled = 0.0f;

        /// <summary>
        /// 外枠太さを画面ピクセル単位で表す。
        /// </summary>
        float outlineThickness = 0.0f;

        /// <summary>
        /// 外枠色の RGBA 成分を表す。
        /// </summary>
        std::array<float, 4> outlineColor{ 1.0f, 1.0f, 0.0f, 1.0f };

        /// <summary>
        /// 塗りつぶし色の RGBA 成分を表す。
        /// </summary>
        std::array<float, 4> fillColor{ 1.0f, 1.0f, 1.0f, 1.0f };

        /// <summary>
        /// テクスチャサンプリングを使用する場合は 1.0f、それ以外は 0.0f を表す。
        /// </summary>
        float textureEnabled = 1.0f;
    };

    /// <summary>
    /// Quad 描画定数を RHI へ渡す浮動小数点配列へ変換する。
    /// </summary>
    /// <param name="constants">変換対象の描画定数。</param>
    /// <returns>RHI に渡す定数列。</returns>
    [[nodiscard]] inline std::array<float, QuadRenderConstantFloatCount> PackQuadRenderConstants(
        const QuadRenderConstants& constants)
    {
        std::array<float, QuadRenderConstantFloatCount> result{};
        result[0] = constants.scaleX;
        result[1] = constants.scaleY;
        result[2] = constants.rotationCos;
        result[3] = constants.rotationSin;
        result[4] = constants.translateX;
        result[5] = constants.translateY;
        result[6] = constants.outlineEnabled;
        result[7] = constants.outlineThickness;
        result[8] = constants.outlineColor[0];
        result[9] = constants.outlineColor[1];
        result[10] = constants.outlineColor[2];
        result[11] = constants.outlineColor[3];
        result[12] = constants.fillColor[0];
        result[13] = constants.fillColor[1];
        result[14] = constants.fillColor[2];
        result[15] = constants.fillColor[3];
        result[16] = constants.textureEnabled;
        return result;
    }

    /// <summary>
    /// Quad 描画定数を RHI へ設定する。
    /// </summary>
    /// <param name="context">設定先の描画コンテキスト。</param>
    /// <param name="constants">設定する描画定数。</param>
    inline void ApplyQuadRenderConstants(RHI::IGraphicsContext& context, const QuadRenderConstants& constants)
    {
        const std::array<float, QuadRenderConstantFloatCount> packedConstants = PackQuadRenderConstants(constants);
        context.SetShaderConstants(packedConstants);
    }

    /// <summary>
    /// SpriteQuadTransform と描画設定から Quad 描画定数を構築する。
    /// </summary>
    /// <param name="quadTransform">Sprite の正規化済み変換値。</param>
    /// <param name="outlineEnabled">外枠描画を有効にする場合は true。</param>
    /// <param name="outlineThickness">外枠太さ。</param>
    /// <param name="outlineColor">RGBA 順の外枠色。</param>
    /// <returns>Quad 描画定数。</returns>
    [[nodiscard]] inline QuadRenderConstants MakeTexturedQuadTransform(
        const SpriteQuadTransform& quadTransform,
        bool outlineEnabled,
        float outlineThickness,
        const std::array<float, 4>& outlineColor)
    {
        QuadRenderConstants result{};
        result.scaleX = quadTransform.scaleX;
        result.scaleY = quadTransform.scaleY;
        result.rotationCos = quadTransform.rotationCos;
        result.rotationSin = quadTransform.rotationSin;
        result.translateX = quadTransform.translateX;
        result.translateY = quadTransform.translateY;
        result.outlineEnabled = outlineEnabled ? 1.0f : 0.0f;
        result.outlineThickness = outlineThickness;
        result.outlineColor = outlineColor;
        result.textureEnabled = 1.0f;
        return result;
    }

    /// <summary>
    /// ビューポート中心基準の単色クアッドから Quad 描画定数を構築する。
    /// </summary>
    /// <param name="centerX">描画中心の X 座標。</param>
    /// <param name="centerY">描画中心の Y 座標。</param>
    /// <param name="width">ピクセル単位の幅。</param>
    /// <param name="height">ピクセル単位の高さ。</param>
    /// <param name="viewportWidth">描画先ビューポート幅。</param>
    /// <param name="viewportHeight">描画先ビューポート高さ。</param>
    /// <param name="fillColor">RGBA 順の塗りつぶし色。</param>
    /// <returns>Quad 描画定数。</returns>
    [[nodiscard]] inline QuadRenderConstants MakeSolidQuadTransform(
        float centerX,
        float centerY,
        float width,
        float height,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight,
        const std::array<float, 4>& fillColor)
    {
        QuadRenderConstants result{};
        result.scaleX = (2.0f * width) / static_cast<float>(viewportWidth);
        result.scaleY = (2.0f * height) / static_cast<float>(viewportHeight);
        result.rotationCos = 1.0f;
        result.rotationSin = 0.0f;
        result.translateX = (2.0f * centerX) / static_cast<float>(viewportWidth);
        result.translateY = (-2.0f * centerY) / static_cast<float>(viewportHeight);
        result.fillColor = fillColor;
        result.textureEnabled = 0.0f;
        return result;
    }
}
