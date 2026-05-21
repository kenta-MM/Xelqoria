#include "SpriteMaterialAssetLoader.h"

#include <array>
#include <charconv>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "AssetId.h"

namespace
{
	std::string_view Trim(std::string_view value)
	{
		const auto first = value.find_first_not_of(" \t\r");
		if (first == std::string_view::npos)
		{
			return {};
		}

		const auto last = value.find_last_not_of(" \t\r");
		return value.substr(first, last - first + 1);
	}

	std::optional<float> ParseFloat(std::string_view value)
	{
		std::istringstream stream{ std::string(value) };
		float parsedValue = 0.0f;
		stream >> parsedValue;
		if (false == static_cast<bool>(stream) || false == stream.eof())
		{
			return std::nullopt;
		}

		return parsedValue;
	}

	std::optional<bool> ParseBool(std::string_view value)
	{
		if (value == "true")
		{
			return true;
		}

		if (value == "false")
		{
			return false;
		}

		return std::nullopt;
	}

	std::optional<std::array<float, 4>> ParseColor(std::string_view value)
	{
		std::array<float, 4> color{};
		std::size_t cursor = 0;
		for (int index = 0; index < 4; ++index)
		{
			const std::size_t separator = value.find(',', cursor);
			const std::string_view token = separator == std::string_view::npos
				? value.substr(cursor)
				: value.substr(cursor, separator - cursor);
			const auto parsed = ParseFloat(Trim(token));
			if (false == parsed.has_value())
			{
				return std::nullopt;
			}

			color[static_cast<std::size_t>(index)] = *parsed;
			if (separator == std::string_view::npos)
			{
				return index == 3 ? std::optional<std::array<float, 4>>(color) : std::nullopt;
			}

			cursor = separator + 1;
		}

		return value.find(',', cursor) == std::string_view::npos
			? std::optional<std::array<float, 4>>(color)
			: std::nullopt;
	}

	Xelqoria::Game::Assets::SpriteMaterialAssetLoadResult MakeError(
		Xelqoria::Game::Assets::SpriteMaterialAssetLoadErrorCode code,
		std::size_t lineNumber,
		std::string_view fieldName,
		std::string message)
	{
		return {
			std::nullopt,
			Xelqoria::Game::Assets::SpriteMaterialAssetLoadError{
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
	SpriteMaterialAssetLoadResult SpriteMaterialAssetLoader::LoadFromText(std::string_view source)
	{
		SpriteMaterialAsset asset{};
		bool hasTextureAssetId = false;
		bool hasColor = false;
		bool hasOutlineEnabled = false;
		bool hasOutlineThickness = false;
		bool hasOutlineColor = false;
		std::size_t lineNumber = 0;
		std::size_t cursor = 0;

		while (cursor <= source.size())
		{
			++lineNumber;
			const std::size_t lineEnd = source.find('\n', cursor);
			const std::size_t lineLength = lineEnd == std::string_view::npos
				? source.size() - cursor
				: lineEnd - cursor;
			std::string_view line = Trim(source.substr(cursor, lineLength));

			if (false == line.empty() && false == line.starts_with('#'))
			{
				const std::size_t separator = line.find('=');
				if (separator == std::string_view::npos)
				{
					return MakeError(SpriteMaterialAssetLoadErrorCode::InvalidRecord, lineNumber, {}, "MaterialAsset の各行は key=value 形式である必要があります。");
				}

				const std::string_view key = Trim(line.substr(0, separator));
				const std::string_view value = Trim(line.substr(separator + 1));
				if (key == "magic" || key == "version" || key == "name")
				{
					// Editor 生成のメタデータは実行時 MaterialAsset では使用しない。
				}
				else if (key == "textureAssetId")
				{
					if (hasTextureAssetId)
					{
						return MakeError(SpriteMaterialAssetLoadErrorCode::DuplicateField, lineNumber, key, "textureAssetId が重複しています。");
					}

					asset.textureAssetId = Core::AssetId(value);
					hasTextureAssetId = true;
				}
				else if (key == "color")
				{
					if (hasColor)
					{
						return MakeError(SpriteMaterialAssetLoadErrorCode::DuplicateField, lineNumber, key, "color が重複しています。");
					}

					const auto parsedColor = ParseColor(value);
					if (false == parsedColor.has_value())
					{
						return MakeError(SpriteMaterialAssetLoadErrorCode::InvalidRecord, lineNumber, key, "color は r,g,b,a 形式である必要があります。");
					}

					asset.color = *parsedColor;
					hasColor = true;
				}
				else if (key == "outline.enabled")
				{
					if (hasOutlineEnabled)
					{
						return MakeError(SpriteMaterialAssetLoadErrorCode::DuplicateField, lineNumber, key, "outline.enabled が重複しています。");
					}

					const auto parsedBool = ParseBool(value);
					if (false == parsedBool.has_value())
					{
						return MakeError(SpriteMaterialAssetLoadErrorCode::InvalidRecord, lineNumber, key, "outline.enabled は true または false である必要があります。");
					}

					asset.outlineEnabled = *parsedBool;
					hasOutlineEnabled = true;
				}
				else if (key == "outline.thickness")
				{
					if (hasOutlineThickness)
					{
						return MakeError(SpriteMaterialAssetLoadErrorCode::DuplicateField, lineNumber, key, "outline.thickness が重複しています。");
					}

					const auto parsedFloat = ParseFloat(value);
					if (false == parsedFloat.has_value())
					{
						return MakeError(SpriteMaterialAssetLoadErrorCode::InvalidRecord, lineNumber, key, "outline.thickness は数値である必要があります。");
					}

					asset.outlineThickness = *parsedFloat;
					hasOutlineThickness = true;
				}
				else if (key == "outline.color")
				{
					if (hasOutlineColor)
					{
						return MakeError(SpriteMaterialAssetLoadErrorCode::DuplicateField, lineNumber, key, "outline.color が重複しています。");
					}

					const auto parsedColor = ParseColor(value);
					if (false == parsedColor.has_value())
					{
						return MakeError(SpriteMaterialAssetLoadErrorCode::InvalidRecord, lineNumber, key, "outline.color は r,g,b,a 形式である必要があります。");
					}

					asset.outlineColor = *parsedColor;
					hasOutlineColor = true;
				}
				else
				{
					return MakeError(SpriteMaterialAssetLoadErrorCode::UnknownField, lineNumber, key, "未対応の MaterialAsset フィールドです。");
				}
			}

			if (lineEnd == std::string_view::npos)
			{
				break;
			}

			cursor = lineEnd + 1;
		}

		if (false == hasTextureAssetId)
		{
			return MakeError(SpriteMaterialAssetLoadErrorCode::MissingRequiredField, 0, "textureAssetId", "textureAssetId が不足しています。");
		}

		return { asset, std::nullopt };
	}
}
