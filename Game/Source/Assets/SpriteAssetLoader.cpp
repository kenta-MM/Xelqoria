#include "Assets/SpriteAssetLoader.h"

#include <utility>
#include "SpriteAsset.h"
#include <optional>
#include <string_view>
#include <string>

namespace
{
	std::string_view Trim(std::string_view value)
	{
		const auto first = value.find_first_not_of(" \t\r");
		if (first == std::string_view::npos) {
			return {};
		}

		const auto last = value.find_last_not_of(" \t\r");
		return value.substr(first, last - first + 1);
	}

	Xelqoria::Game::Assets::SpriteAssetLoadResult MakeError(
		Xelqoria::Game::Assets::SpriteAssetLoadErrorCode code,
		std::size_t lineNumber,
		std::string_view fieldName,
		std::string message)
	{
		return {
			std::nullopt,
			Xelqoria::Game::Assets::SpriteAssetLoadError{
				code,
				lineNumber,
				std::string(fieldName),
				std::move(message)
			}
		};
	}
}

namespace Xelqoria::Game::Assets
{
	SpriteAssetLoadResult SpriteAssetLoader::LoadFromText(std::string_view source)
	{
		SpriteAsset asset{};
		bool hasTextureAssetId = false;
		std::size_t lineNumber = 0;
		std::size_t cursor = 0;

		while (cursor <= source.size())
		{
			++lineNumber;

			const std::size_t lineEnd = source.find('\n', cursor);
			const std::size_t lineLength = lineEnd == std::string_view::npos
				? source.size() - cursor
				: lineEnd - cursor;
			std::string_view line = source.substr(cursor, lineLength);
			line = Trim(line);

			if (!line.empty() && !line.starts_with('#')) {
				const std::size_t separator = line.find('=');
				if (separator == std::string_view::npos) {
					return MakeError(
						SpriteAssetLoadErrorCode::InvalidRecord,
						lineNumber,
						{},
						"SpriteAsset の各行は key=value 形式である必要があります。");
				}

				const std::string_view key = Trim(line.substr(0, separator));
				const std::string_view value = Trim(line.substr(separator + 1));
				if (key.empty()) {
					return MakeError(
						SpriteAssetLoadErrorCode::InvalidRecord,
						lineNumber,
						{},
						"SpriteAsset のキーが空です。");
				}

				if (key == "textureAssetId") {
					if (hasTextureAssetId) {
						return MakeError(
							SpriteAssetLoadErrorCode::DuplicateField,
							lineNumber,
							key,
							"textureAssetId が重複しています。");
					}

					if (value.empty()) {
						return MakeError(
							SpriteAssetLoadErrorCode::EmptyFieldValue,
							lineNumber,
							key,
							"textureAssetId には空でない値が必要です。");
					}

					asset.textureAssetId = Core::AssetId(value);
					hasTextureAssetId = true;
				}
				else {
					return MakeError(
						SpriteAssetLoadErrorCode::UnknownField,
						lineNumber,
						key,
						"未対応の SpriteAsset フィールドです。");
				}
			}

			if (lineEnd == std::string_view::npos) {
				break;
			}

			cursor = lineEnd + 1;
		}

		if (!hasTextureAssetId) {
			return MakeError(
				SpriteAssetLoadErrorCode::MissingRequiredField,
				0,
				"textureAssetId",
				"textureAssetId が不足しています。");
		}

		return { asset, std::nullopt };
	}
}
