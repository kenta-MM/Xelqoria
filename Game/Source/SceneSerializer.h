#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "Scene.h"

namespace Xelqoria::Game
{
	/// <summary>
	/// Scene 読込失敗時の詳細情報を表す。
	/// </summary>
	struct SceneLoadError
	{
		/// <summary>
		/// 問題が発生した入力行番号を表す。
		/// </summary>
		std::size_t lineNumber = 0;

		/// <summary>
		/// 問題が発生したフィールド名を表す。
		/// </summary>
		std::string fieldName{};

		/// <summary>
		/// 読込失敗理由を表す。
		/// </summary>
		std::string message{};
	};

	/// <summary>
	/// Scene 読込結果を表す。
	/// </summary>
	struct SceneLoadResult
	{
		/// <summary>
		/// 読込に成功した Scene を表す。
		/// </summary>
		std::optional<Scene> scene{};

		/// <summary>
		/// 読込に失敗した場合の詳細情報を表す。
		/// </summary>
		std::optional<SceneLoadError> error{};

		/// <summary>
		/// 読込に成功したかどうかを返す。
		/// </summary>
		/// <returns>成功時は true。</returns>
		bool IsSuccess() const
		{
			return scene.has_value();
		}
	};

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

		/// <summary>
		/// `key=value` 形式の保存テキストから Scene を復元する。
		/// </summary>
		/// <param name="source">読込対象のテキスト全体。</param>
		/// <returns>復元した Scene または失敗詳細。</returns>
		static SceneLoadResult LoadFromText(std::string_view source);
	};
}
