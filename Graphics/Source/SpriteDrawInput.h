#pragma once

#include "AssetId.h"
#include "Vector2.h"

#include <array>
#include <memory>

namespace Xelqoria::Graphics
{
	class Texture2D;

	/// <summary>
	/// CPU/GPU の Sprite 描画経路へ渡す共通入力データを表す。
	/// </summary>
	struct SpriteDrawInput
	{
		/// <summary>
		/// 描画に使用する解決済みテクスチャを表す。
		/// </summary>
		std::shared_ptr<Texture2D> texture{};

		/// <summary>
		/// 描画に使用するテクスチャアセット識別子を表す。
		/// </summary>
		Core::AssetId textureAssetId{};

		/// <summary>
		/// スプライトの描画位置を表す。
		/// </summary>
		Xelqoria::Math::Vector2 position{};

		/// <summary>
		/// スプライトの拡大率を表す。
		/// </summary>
		Xelqoria::Math::Vector2 scale{ 1.0f, 1.0f };

		/// <summary>
		/// 度数法で表したスプライトの回転角度を表す。
		/// </summary>
		float rotationDegrees = 0.0f;

		/// <summary>
		/// RGBA 順の色乗算値を表す。
		/// </summary>
		std::array<float, 4> color{ 1.0f, 1.0f, 1.0f, 1.0f };

		/// <summary>
		/// スプライト外枠描画が有効かを表す。
		/// </summary>
		bool outlineEnabled = false;

		/// <summary>
		/// スプライト外枠の太さを画面ピクセル単位で表す。
		/// </summary>
		float outlineThickness = 1.0f;

		/// <summary>
		/// RGBA 順のスプライト外枠色を表す。
		/// </summary>
		std::array<float, 4> outlineColor{ 1.0f, 1.0f, 0.0f, 1.0f };
	};
}
