#pragma once

#include <string_view>

#include "SceneSerializer.h"

namespace Xelqoria::Game
{
	/// <summary>
	/// 保存テキストから Scene を復元する。
	/// </summary>
	class SceneTextReader
	{
	public:
		/// <summary>
		/// `key=value` 形式の保存テキストから Scene を復元する。
		/// </summary>
		/// <param name="source">読込対象のテキスト全体。</param>
		/// <returns>復元した Scene または失敗詳細。</returns>
		static SceneLoadResult Read(std::string_view source);
	};
}
