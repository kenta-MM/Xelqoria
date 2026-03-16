#pragma once

#include <cstdint>
#include <string>

namespace Xelqoria::Game
{
	/// <summary>
	/// SpriteComponent が保持する描画設定を表す。
	/// </summary>
	struct SpriteRenderSettings
	{
		/// <summary>
		/// スプライトを表示対象に含めるかを表す。
		/// </summary>
		bool visible = true;

		/// <summary>
		/// 描画順制御に使用するソートキーを表す。
		/// </summary>
		std::int32_t sortOrder = 0;

		/// <summary>
		/// スプライト全体に適用する不透明度を表す。
		/// </summary>
		float opacity = 1.0f;
	};

	/// <summary>
	/// Entity に付与する Sprite アセット参照と描画設定を保持する。
	/// </summary>
	struct SpriteComponent
	{
		/// <summary>
		/// 描画に使用する Sprite アセット参照名を表す。
		/// </summary>
		std::string spriteAssetRef{};

		/// <summary>
		/// 描画時に使用する設定値を表す。
		/// </summary>
		SpriteRenderSettings renderSettings{};
	};
}
