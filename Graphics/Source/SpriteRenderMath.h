#pragma once

#include <cmath>
#include <cstdint>

#include "Sprite.h"
#include "Texture2D.h"

namespace Xelqoria::Graphics
{
	/// <summary>
	/// 単位クアッドへ適用する 2D 変換値を表す。
	/// </summary>
	struct SpriteQuadTransform
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
		/// X 軸方向の移動量を表す。
		/// </summary>
		float translateX = 0.0f;

		/// <summary>
		/// Y 軸方向の移動量を表す。
		/// </summary>
		float translateY = 0.0f;

		/// <summary>
		/// 回転角度の cos 値を表す。
		/// </summary>
		float rotationCos = 1.0f;

		/// <summary>
		/// 回転角度の sin 値を表す。
		/// </summary>
		float rotationSin = 0.0f;
	};

	/// <summary>
	/// Sprite の位置と拡大率からクアッド描画用の正規化変換を計算する。
	/// </summary>
	/// <param name="sprite">変換対象の Sprite。</param>
	/// <param name="viewportWidth">描画先ビューポート幅。</param>
	/// <param name="viewportHeight">描画先ビューポート高さ。</param>
	/// <returns>単位クアッドに適用する変換値。</returns>
	inline SpriteQuadTransform ComputeSpriteQuadTransform(const Sprite& sprite, std::uint32_t viewportWidth, std::uint32_t viewportHeight)
	{
		SpriteQuadTransform transform{};
		if (viewportWidth == 0 || viewportHeight == 0) {
			return transform;
		}

		const auto texture = sprite.GetTexture();
		if (!texture) {
			return transform;
		}

		const Vector2 position = sprite.GetPosition();
		const Vector2 scale = sprite.GetScale();

		transform.scaleX = (2.0f * static_cast<float>(texture->GetWidth()) * scale.x) / static_cast<float>(viewportWidth);
		transform.scaleY = (2.0f * static_cast<float>(texture->GetHeight()) * scale.y) / static_cast<float>(viewportHeight);
		transform.translateX = (2.0f * position.x) / static_cast<float>(viewportWidth);
		transform.translateY = (-2.0f * position.y) / static_cast<float>(viewportHeight);
		const float rotationRadians = sprite.GetRotationDegrees() * (3.14159265358979323846f / 180.0f);
		transform.rotationCos = std::cos(rotationRadians);
		transform.rotationSin = std::sin(rotationRadians);
		return transform;
	}
}
