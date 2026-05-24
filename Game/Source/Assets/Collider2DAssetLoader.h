#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "Collider2DAsset.h"

namespace Xelqoria::Game::Assets
{
	/// <summary>
	/// Collider2DAsset の読込失敗要因を分類する。
	/// </summary>
	enum class Collider2DAssetLoadErrorCode
	{
		None = 0,
		InvalidRecord,
		DuplicateField,
		UnknownField,
		MissingRequiredField
	};

	/// <summary>
	/// Collider2DAsset 読込失敗時の詳細情報を表す。
	/// </summary>
	struct Collider2DAssetLoadError
	{
		Collider2DAssetLoadErrorCode code = Collider2DAssetLoadErrorCode::None;
		std::size_t lineNumber = 0;
		std::string fieldName{};
		std::string message{};
	};

	/// <summary>
	/// Collider2DAsset 読込結果を表す。
	/// </summary>
	struct Collider2DAssetLoadResult
	{
		std::optional<Collider2DAsset> asset{};
		std::optional<Collider2DAssetLoadError> error{};

		/// <summary>
		/// 読込に成功したかを取得する。
		/// </summary>
		/// <returns>成功時は true。</returns>
		[[nodiscard]] bool IsSuccess() const
		{
			return asset.has_value() && false == error.has_value();
		}
	};

	/// <summary>
	/// `key=value` 形式のデータから Collider2DAsset を復元する。
	/// </summary>
	class Collider2DAssetLoader
	{
	public:
		/// <summary>
		/// テキストデータから Collider2DAsset を読込する。
		/// </summary>
		/// <param name="source">読込対象テキスト。</param>
		/// <returns>Collider2DAsset または失敗詳細。</returns>
		static Collider2DAssetLoadResult LoadFromText(std::string_view source);
	};
}
