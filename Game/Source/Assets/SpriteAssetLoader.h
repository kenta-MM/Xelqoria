#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "SpriteAsset.h"

namespace Xelqoria::Game::Assets
{
	/// <summary>
	/// SpriteAsset の読込失敗要因を分類する。
	/// </summary>
	enum class SpriteAssetLoadErrorCode
	{
		None = 0,
		MissingRequiredField,
		EmptyFieldValue,
		DuplicateField,
		UnknownField,
		InvalidRecord
	};

	/// <summary>
	/// SpriteAsset 読込失敗時の詳細情報を表す。
	/// </summary>
	struct SpriteAssetLoadError
	{
		/// <summary>
		/// 失敗要因の種類を表す。
		/// </summary>
		SpriteAssetLoadErrorCode code = SpriteAssetLoadErrorCode::None;

		/// <summary>
		/// 問題が発生した入力行番号を表す。
		/// </summary>
		std::size_t lineNumber = 0;

		/// <summary>
		/// 問題が発生したフィールド名を表す。
		/// </summary>
		std::string fieldName{};

		/// <summary>
		/// 呼び出し側がログ出力に利用できる説明文を表す。
		/// </summary>
		std::string message{};
	};

	/// <summary>
	/// SpriteAsset 読込結果を表す。
	/// </summary>
	struct SpriteAssetLoadResult
	{
		/// <summary>
		/// 読込に成功した SpriteAsset を表す。
		/// </summary>
		std::optional<SpriteAsset> asset{};

		/// <summary>
		/// 読込に失敗した場合の詳細情報を表す。
		/// </summary>
		std::optional<SpriteAssetLoadError> error{};

		/// <summary>
		/// 読込に成功したかどうかを返す。
		/// </summary>
		/// <returns>成功時は true。</returns>
		bool IsSuccess() const
		{
			return asset.has_value();
		}
	};

	/// <summary>
	/// `key=value` 形式のデータから SpriteAsset を復元する。
	/// </summary>
	class SpriteAssetLoader
	{
	public:
		/// <summary>
		/// テキストデータから SpriteAsset を読込する。
		/// </summary>
		/// <param name="source">読込対象のテキスト全体。</param>
		/// <returns>SpriteAsset または失敗詳細。</returns>
		static SpriteAssetLoadResult LoadFromText(std::string_view source);
	};
}
