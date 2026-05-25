#pragma once

#include "Collider2DComponent.h"

namespace Xelqoria::Game::Assets
{
	/// <summary>
	/// 保存データとして扱う Collider2D アセット定義を表す。
	/// </summary>
	struct Collider2DAsset
	{
		/// <summary>
		/// Collider2D の初期 Component 値を表す。
		/// </summary>
		Collider2DComponent collider{};
	};
}
