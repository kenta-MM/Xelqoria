#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "SpriteDrawInput.h"
#include "Texture2D.h"

namespace Xelqoria::Graphics
{
	/// <summary>
	/// Sprite カリングで使用する 2D 矩形を表す。
	/// </summary>
	struct SpriteCullRect
	{
		/// <summary>
		/// 矩形の左端座標。
		/// </summary>
		float left = 0.0f;

		/// <summary>
		/// 矩形の上端座標。
		/// </summary>
		float top = 0.0f;

		/// <summary>
		/// 矩形の右端座標。
		/// </summary>
		float right = 0.0f;

		/// <summary>
		/// 矩形の下端座標。
		/// </summary>
		float bottom = 0.0f;
	};

	/// <summary>
	/// Sprite の描画入力から回転を含む保守的な外接矩形を計算する。
	/// </summary>
	/// <param name="input">外接矩形を求める共通 Sprite 入力データ。</param>
	/// <returns>Sprite の外接矩形。テクスチャがない場合はゼロサイズ矩形。</returns>
	[[nodiscard]] inline SpriteCullRect ComputeSpriteCullRect(const SpriteDrawInput& input)
	{
		if (false == static_cast<bool>(input.texture))
		{
			return SpriteCullRect{
				input.position.x,
				input.position.y,
				input.position.x,
				input.position.y
			};
		}

		const float halfWidth = static_cast<float>(input.texture->GetWidth()) * std::abs(input.scale.x) * 0.5f;
		const float halfHeight = static_cast<float>(input.texture->GetHeight()) * std::abs(input.scale.y) * 0.5f;
		const float rotationRadians = input.rotationDegrees * (3.14159265358979323846f / 180.0f);
		const float rotationCos = std::abs(std::cos(rotationRadians));
		const float rotationSin = std::abs(std::sin(rotationRadians));
		const float extentX = (halfWidth * rotationCos) + (halfHeight * rotationSin);
		const float extentY = (halfWidth * rotationSin) + (halfHeight * rotationCos);

		return SpriteCullRect{
			input.position.x - extentX,
			input.position.y - extentY,
			input.position.x + extentX,
			input.position.y + extentY
		};
	}

	/// <summary>
	/// 2つのカリング矩形が交差しているかを判定する。
	/// </summary>
	/// <param name="lhs">判定対象の矩形。</param>
	/// <param name="rhs">比較対象の矩形。</param>
	/// <returns>矩形が交差している場合は true。</returns>
	[[nodiscard]] inline bool IntersectsSpriteCullRect(const SpriteCullRect& lhs, const SpriteCullRect& rhs)
	{
		return lhs.left <= rhs.right
			&& rhs.left <= lhs.right
			&& lhs.top <= rhs.bottom
			&& rhs.top <= lhs.bottom;
	}

	/// <summary>
	/// Sprite が指定された表示範囲に含まれるかを判定する。
	/// </summary>
	/// <param name="input">判定する共通 Sprite 入力データ。</param>
	/// <param name="viewRect">表示対象の矩形。</param>
	/// <returns>表示範囲と交差している場合は true。</returns>
	[[nodiscard]] inline bool IsSpriteVisible(const SpriteDrawInput& input, const SpriteCullRect& viewRect)
	{
		if (false == static_cast<bool>(input.texture))
		{
			return false;
		}

		if (input.texture->GetWidth() == 0 || input.texture->GetHeight() == 0)
		{
			return false;
		}

		const SpriteCullRect spriteRect = ComputeSpriteCullRect(input);
		return IntersectsSpriteCullRect(spriteRect, viewRect);
	}

	/// <summary>
	/// ビューポートサイズから画面座標系のカリング矩形を作成する。
	/// </summary>
	/// <param name="viewportWidth">ビューポート幅。</param>
	/// <param name="viewportHeight">ビューポート高さ。</param>
	/// <returns>原点を中心とする画面座標系のカリング矩形。</returns>
	[[nodiscard]] inline SpriteCullRect MakeViewportCullRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight)
	{
		const float halfWidth = static_cast<float>(viewportWidth) * 0.5f;
		const float halfHeight = static_cast<float>(viewportHeight) * 0.5f;
		return SpriteCullRect{
			-halfWidth,
			-halfHeight,
			halfWidth,
			halfHeight
		};
	}
}
