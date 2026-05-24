#include "Assets/Collider2DAssetLoader.h"

#include <cstdlib>
#include <optional>
#include <string>
#include <utility>

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

	Xelqoria::Game::Assets::Collider2DAssetLoadResult MakeError(
		Xelqoria::Game::Assets::Collider2DAssetLoadErrorCode code,
		std::size_t lineNumber,
		std::string_view fieldName,
		std::string message)
	{
		return {
			std::nullopt,
			Xelqoria::Game::Assets::Collider2DAssetLoadError{
				code,
				lineNumber,
				std::string(fieldName),
				std::move(message)
			}
		};
	}

	std::optional<bool> ParseBool(std::string_view value)
	{
		if (value == "true" || value == "1")
		{
			return true;
		}

		if (value == "false" || value == "0")
		{
			return false;
		}

		return std::nullopt;
	}

	std::optional<float> ParseFloat(std::string_view value)
	{
		const std::string text(value);
		char* end = nullptr;
		const float parsed = std::strtof(text.c_str(), &end);
		if (end == text.c_str() || nullptr == end || '\0' != *end)
		{
			return std::nullopt;
		}

		return parsed;
	}

	std::optional<Xelqoria::Math::Vector2> ParseVector2(std::string_view value)
	{
		const std::size_t separator = value.find(',');
		if (separator == std::string_view::npos || value.find(',', separator + 1) != std::string_view::npos)
		{
			return std::nullopt;
		}

		const auto x = ParseFloat(Trim(value.substr(0, separator)));
		const auto y = ParseFloat(Trim(value.substr(separator + 1)));
		if (false == x.has_value() || false == y.has_value())
		{
			return std::nullopt;
		}

		return Xelqoria::Math::Vector2{ *x, *y };
	}
}

namespace Xelqoria::Game::Assets
{
	Collider2DAssetLoadResult Collider2DAssetLoader::LoadFromText(std::string_view source)
	{
		Collider2DAsset asset{};
		bool hasMagic = false;
		bool hasShapeType = false;
		bool hasOffset = false;
		bool hasSize = false;
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
					return MakeError(Collider2DAssetLoadErrorCode::InvalidRecord, lineNumber, {}, "Collider2DAsset の各行は key=value 形式である必要があります。");
				}

				const std::string_view key = Trim(line.substr(0, separator));
				const std::string_view value = Trim(line.substr(separator + 1));
				if (key == "magic")
				{
					if (value != "XelqoriaCollider2DAsset")
					{
						return MakeError(Collider2DAssetLoadErrorCode::InvalidRecord, lineNumber, key, "Collider2DAsset の magic が不正です。");
					}

					hasMagic = true;
				}
				else if (key == "version")
				{
					if (value != "1")
					{
						return MakeError(Collider2DAssetLoadErrorCode::InvalidRecord, lineNumber, key, "Collider2DAsset の version が不正です。");
					}
				}
				else if (key == "enabled")
				{
					const auto parsed = ParseBool(value);
					if (false == parsed.has_value())
					{
						return MakeError(Collider2DAssetLoadErrorCode::InvalidRecord, lineNumber, key, "enabled は true または false である必要があります。");
					}

					asset.collider.enabled = *parsed;
				}
				else if (key == "isTrigger")
				{
					const auto parsed = ParseBool(value);
					if (false == parsed.has_value())
					{
						return MakeError(Collider2DAssetLoadErrorCode::InvalidRecord, lineNumber, key, "isTrigger は true または false である必要があります。");
					}

					asset.collider.isTrigger = *parsed;
				}
				else if (key == "shapeType")
				{
					if (hasShapeType)
					{
						return MakeError(Collider2DAssetLoadErrorCode::DuplicateField, lineNumber, key, "shapeType が重複しています。");
					}
					if (value != "Box")
					{
						return MakeError(Collider2DAssetLoadErrorCode::InvalidRecord, lineNumber, key, "shapeType は Box である必要があります。");
					}

					asset.collider.shapeType = Collider2DShapeType::Box;
					hasShapeType = true;
				}
				else if (key == "offset")
				{
					if (hasOffset)
					{
						return MakeError(Collider2DAssetLoadErrorCode::DuplicateField, lineNumber, key, "offset が重複しています。");
					}
					const auto parsed = ParseVector2(value);
					if (false == parsed.has_value())
					{
						return MakeError(Collider2DAssetLoadErrorCode::InvalidRecord, lineNumber, key, "offset は x,y 形式である必要があります。");
					}

					asset.collider.offset = *parsed;
					hasOffset = true;
				}
				else if (key == "size")
				{
					if (hasSize)
					{
						return MakeError(Collider2DAssetLoadErrorCode::DuplicateField, lineNumber, key, "size が重複しています。");
					}
					const auto parsed = ParseVector2(value);
					if (false == parsed.has_value() || parsed->x <= 0.0f || parsed->y <= 0.0f)
					{
						return MakeError(Collider2DAssetLoadErrorCode::InvalidRecord, lineNumber, key, "size は正の x,y 形式である必要があります。");
					}

					asset.collider.size = *parsed;
					hasSize = true;
				}
				else
				{
					return MakeError(Collider2DAssetLoadErrorCode::UnknownField, lineNumber, key, "未対応の Collider2DAsset フィールドです。");
				}
			}

			if (lineEnd == std::string_view::npos)
			{
				break;
			}

			cursor = lineEnd + 1;
		}

		if (false == hasMagic)
		{
			return MakeError(Collider2DAssetLoadErrorCode::MissingRequiredField, 0, "magic", "magic が不足しています。");
		}
		if (false == hasShapeType || false == hasOffset || false == hasSize)
		{
			return MakeError(Collider2DAssetLoadErrorCode::MissingRequiredField, 0, {}, "Collider2DAsset の必須フィールドが不足しています。");
		}

		return { asset, std::nullopt };
	}
}
