#pragma once

#include "AssetId.h"

namespace Xelqoria::Game::Assets
{
	/// <summary>
	/// 保存データとして扱う Sprite アセット定義を表す。
	/// </summary>
	struct SpriteAsset
	{
		/// <summary>
		/// スプライトに関連付けるテクスチャアセット識別子を表す。
		/// </summary>
		Core::AssetId textureAssetId{};
	};
}
