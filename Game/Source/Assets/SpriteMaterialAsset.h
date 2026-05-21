#pragma once

#include <array>

#include "AssetId.h"

namespace Xelqoria::Game::Assets
{
	/// <summary>
	/// 保存データとして扱う Sprite Material アセット定義を表す。
	/// </summary>
	struct SpriteMaterialAsset
	{
		/// <summary>
		/// Material が参照する Texture アセット識別子を表す。
		/// </summary>
		Core::AssetId textureAssetId{};

		/// <summary>
		/// Sprite 全体に乗算する色を RGBA 順で表す。
		/// </summary>
		std::array<float, 4> color{ 1.0f, 1.0f, 1.0f, 1.0f };

		/// <summary>
		/// 外枠描画を有効化するかを表す。
		/// </summary>
		bool outlineEnabled = false;

		/// <summary>
		/// 外枠太さを画面ピクセル単位で表す。
		/// </summary>
		float outlineThickness = 1.0f;

		/// <summary>
		/// 外枠色を RGBA 順で表す。
		/// </summary>
		std::array<float, 4> outlineColor{ 1.0f, 1.0f, 0.0f, 1.0f };
	};
}
