#pragma once

#include <string>

#include "Scene.h"

namespace Xelqoria::Game
{
	/// <summary>
	/// Scene を保存用テキストへ変換する。
	/// </summary>
	class SceneSerializer
	{
	public:
		/// <summary>
		/// Scene を `key=value` 形式の保存テキストへ変換する。
		/// </summary>
		/// <param name="scene">保存対象の Scene。</param>
		/// <returns>保存フォーマットに従ったテキスト。</returns>
		static std::string SaveToText(const Scene& scene);
	};
}
