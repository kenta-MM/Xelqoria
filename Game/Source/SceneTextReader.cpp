#include "SceneTextReader.h"

#include <charconv>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string_view>

#include "SceneSaveFormat.h"
#include <cstdint>
#include <string>
#include <system_error>
#include <utility>
#include <AssetId.h>
#include "Collider2DComponent.h"
#include "Scene.h"
#include "SpriteComponent.h"
#include "Transform.h"

namespace
{
	/// <summary>
	/// 行単位で読み込んだ文字列の前後にある空白を取り除く。
	/// </summary>
	/// <param name="value">整形対象の文字列ビュー。</param>
	/// <returns>前後の空白を除いた文字列ビュー。</returns>
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

	/// <summary>
	/// 10 進文字列を符号なし整数へ変換する。
	/// </summary>
	/// <param name="value">変換対象の文字列。</param>
	/// <returns>変換に成功した値。失敗時は nullopt。</returns>
	std::optional<std::uint32_t> ParseUnsigned(std::string_view value)
	{
		std::uint32_t parsedValue = 0;
		const auto* begin = value.data();
		const auto* end = value.data() + value.size();
		const auto [ptr, ec] = std::from_chars(begin, end, parsedValue);
		if (ec != std::errc{} || ptr != end)
		{
			return std::nullopt;
		}

		return parsedValue;
	}

	/// <summary>
	/// 文字列を単精度浮動小数点値へ変換する。
	/// </summary>
	/// <param name="value">変換対象の文字列。</param>
	/// <returns>変換に成功した値。失敗時は nullopt。</returns>
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

	/// <summary>
	/// 真偽値文字列を bool へ変換する。
	/// </summary>
	/// <param name="value">変換対象の文字列。</param>
	/// <returns>変換に成功した bool 値。失敗時は nullopt。</returns>
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

	/// <summary>
	/// 引用符付き文字列を復元する。
	/// </summary>
	/// <param name="value">変換対象の文字列。</param>
	/// <returns>変換に成功した文字列。失敗時は nullopt。</returns>
	std::optional<std::string> ParseQuotedString(std::string_view value)
	{
		std::istringstream stream{ std::string(value) };
		std::string parsedValue{};
		stream >> std::quoted(parsedValue);
		if (false == static_cast<bool>(stream))
		{
			return std::nullopt;
		}

		stream >> std::ws;
		if (std::char_traits<char>::eof() != stream.peek())
		{
			return std::nullopt;
		}

		return parsedValue;
	}

	/// <summary>
	/// `x,y,z` 形式の文字列を Vector3 へ変換する。
	/// </summary>
	/// <param name="value">変換対象の文字列。</param>
	/// <returns>変換に成功した Vector3。失敗時は nullopt。</returns>
	std::optional<Xelqoria::Math::Vector3> ParseVector3(std::string_view value)
	{
		Xelqoria::Math::Vector3 vector{};
		std::size_t cursor = 0;
		float* components[3] = { &vector.x, &vector.y, &vector.z };

		for (int index = 0; index < 3; ++index)
		{
			const std::size_t separator = value.find(',', cursor);
			const std::string_view token = separator == std::string_view::npos
				? value.substr(cursor)
				: value.substr(cursor, separator - cursor);
			const auto parsedComponent = ParseFloat(Trim(token));
			if (false == parsedComponent.has_value())
			{
				return std::nullopt;
			}

			*components[index] = *parsedComponent;
			if (separator == std::string_view::npos)
			{
				return index == 2 ? std::optional<Xelqoria::Math::Vector3>(vector) : std::nullopt;
			}

			if (index == 2)
			{
				return std::nullopt;
			}

			cursor = separator + 1;
		}

		if (value.find(',', cursor) != std::string_view::npos)
		{
			return std::nullopt;
		}

		return vector;
	}

	/// <summary>
	/// `x,y` 形式の文字列を Vector2 へ変換する。
	/// </summary>
	/// <param name="value">変換対象の文字列。</param>
	/// <returns>変換に成功した Vector2。失敗時は nullopt。</returns>
	std::optional<Xelqoria::Math::Vector2> ParseVector2(std::string_view value)
	{
		Xelqoria::Math::Vector2 vector{};
		std::size_t cursor = 0;
		float* components[2] = { &vector.x, &vector.y };

		for (int index = 0; index < 2; ++index)
		{
			const std::size_t separator = value.find(',', cursor);
			const std::string_view token = separator == std::string_view::npos
				? value.substr(cursor)
				: value.substr(cursor, separator - cursor);
			const auto parsedComponent = ParseFloat(Trim(token));
			if (false == parsedComponent.has_value())
			{
				return std::nullopt;
			}

			*components[index] = *parsedComponent;
			if (separator == std::string_view::npos)
			{
				return index == 1 ? std::optional<Xelqoria::Math::Vector2>(vector) : std::nullopt;
			}

			if (index == 1)
			{
				return std::nullopt;
			}

			cursor = separator + 1;
		}

		if (value.find(',', cursor) != std::string_view::npos)
		{
			return std::nullopt;
		}

		return vector;
	}

	/// <summary>
	/// Collider2D の shapeType 文字列を変換する。
	/// </summary>
	/// <param name="value">変換対象の文字列。</param>
	/// <returns>変換に成功した shapeType。失敗時は nullopt。</returns>
	std::optional<Xelqoria::Game::Collider2DShapeType> ParseCollider2DShapeType(std::string_view value)
	{
		if (value == "Box")
		{
			return Xelqoria::Game::Collider2DShapeType::Box;
		}

		return std::nullopt;
	}

	/// <summary>
	/// シーン読み込み失敗時の共通エラー結果を構築する。
	/// </summary>
	/// <param name="lineNumber">エラーが発生した行番号。</param>
	/// <param name="fieldName">問題のあったフィールド名。</param>
	/// <param name="message">ユーザー向けエラーメッセージ。</param>
	/// <returns>エラー情報を含む SceneLoadResult。</returns>
	Xelqoria::Game::SceneLoadResult MakeError(
		std::size_t lineNumber,
		std::string_view fieldName,
		std::string message)
	{
		return {
			std::nullopt,
			Xelqoria::Game::SceneLoadError{
				lineNumber,
				std::string(fieldName),
				std::move(message)
			}
		};
	}

	/// <summary>
	/// entity 系フィールドを Scene 保存レコードへ反映する。
	/// </summary>
	/// <param name="lineNumber">現在の入力行番号。</param>
	/// <param name="key">エラーメッセージ用のキー名。</param>
	/// <param name="fieldKey">entity プレフィックス以降のフィールド名。</param>
	/// <param name="value">フィールド値。</param>
	/// <param name="record">反映先レコード。</param>
	/// <returns>失敗した場合はエラー結果。</returns>
	std::optional<Xelqoria::Game::SceneLoadResult> ParseEntityField(
		std::size_t lineNumber,
		std::string_view key,
		std::string_view fieldKey,
		std::string_view value,
		Xelqoria::Game::SceneEntitySaveRecord& record)
	{
		if (fieldKey == "id")
		{
			const auto entityId = ParseUnsigned(value);
			if (false == entityId.has_value())
			{
				return MakeError(lineNumber, key, "entity id は符号なし整数である必要があります。");
			}

			record.entityId = *entityId;
			return std::nullopt;
		}

		if (fieldKey == "name")
		{
			const auto entityName = ParseQuotedString(value);
			if (false == entityName.has_value())
			{
				return MakeError(lineNumber, key, "name は引用符付き文字列である必要があります。");
			}

			record.name = *entityName;
			record.hasName = true;
			return std::nullopt;
		}

		if (fieldKey == "hasSpriteComponent")
		{
			const auto hasSpriteComponent = ParseBool(value);
			if (false == hasSpriteComponent.has_value())
			{
				return MakeError(lineNumber, key, "hasSpriteComponent は true または false である必要があります。");
			}

			record.hasSpriteComponent = *hasSpriteComponent;
			return std::nullopt;
		}

		if (fieldKey == "hasCollider2DComponent")
		{
			const auto hasCollider2DComponent = ParseBool(value);
			if (false == hasCollider2DComponent.has_value())
			{
				return MakeError(lineNumber, key, "hasCollider2DComponent は true または false である必要があります。");
			}

			record.hasCollider2DComponent = *hasCollider2DComponent;
			if (*hasCollider2DComponent && false == record.collider2D.has_value())
			{
				record.collider2D = Xelqoria::Game::SceneCollider2DRecord{};
			}
			return std::nullopt;
		}

		if (fieldKey == "transform.position")
		{
			const auto parsedVector = ParseVector3(value);
			if (false == parsedVector.has_value())
			{
				return MakeError(lineNumber, key, "position は x,y,z 形式である必要があります。");
			}

			record.transform.position = *parsedVector;
			return std::nullopt;
		}

		if (fieldKey == "transform.rotation")
		{
			const auto parsedVector = ParseVector3(value);
			if (false == parsedVector.has_value())
			{
				return MakeError(lineNumber, key, "rotation は x,y,z 形式である必要があります。");
			}

			record.transform.rotation = *parsedVector;
			return std::nullopt;
		}

		if (fieldKey == "transform.scale")
		{
			const auto parsedVector = ParseVector3(value);
			if (false == parsedVector.has_value())
			{
				return MakeError(lineNumber, key, "scale は x,y,z 形式である必要があります。");
			}

			record.transform.scale = *parsedVector;
			return std::nullopt;
		}

		if (fieldKey == "spriteRef")
		{
			record.hasSpriteComponent = true;
			record.spriteRef = Xelqoria::Game::SceneSpriteRefRecord{ Xelqoria::Core::AssetId(value) };
			return std::nullopt;
		}

		if (fieldKey == "materialRef")
		{
			record.hasSpriteComponent = true;
			record.materialRef = Xelqoria::Game::SceneMaterialRefRecord{ Xelqoria::Core::AssetId(value) };
			return std::nullopt;
		}

		if (fieldKey == "collider2D.enabled")
		{
			const auto enabled = ParseBool(value);
			if (false == enabled.has_value())
			{
				return MakeError(lineNumber, key, "collider2D.enabled は true または false である必要があります。");
			}

			record.hasCollider2DComponent = true;
			if (false == record.collider2D.has_value())
			{
				record.collider2D = Xelqoria::Game::SceneCollider2DRecord{};
			}
			record.collider2D->enabled = *enabled;
			return std::nullopt;
		}

		if (fieldKey == "collider2D.isTrigger")
		{
			const auto isTrigger = ParseBool(value);
			if (false == isTrigger.has_value())
			{
				return MakeError(lineNumber, key, "collider2D.isTrigger は true または false である必要があります。");
			}

			record.hasCollider2DComponent = true;
			if (false == record.collider2D.has_value())
			{
				record.collider2D = Xelqoria::Game::SceneCollider2DRecord{};
			}
			record.collider2D->isTrigger = *isTrigger;
			return std::nullopt;
		}

		if (fieldKey == "collider2D.shapeType")
		{
			const auto shapeType = ParseCollider2DShapeType(value);
			if (false == shapeType.has_value())
			{
				return MakeError(lineNumber, key, "collider2D.shapeType は Box である必要があります。");
			}

			record.hasCollider2DComponent = true;
			if (false == record.collider2D.has_value())
			{
				record.collider2D = Xelqoria::Game::SceneCollider2DRecord{};
			}
			record.collider2D->shapeType = *shapeType;
			return std::nullopt;
		}

		if (fieldKey == "collider2D.offset")
		{
			const auto offset = ParseVector2(value);
			if (false == offset.has_value())
			{
				return MakeError(lineNumber, key, "collider2D.offset は x,y 形式である必要があります。");
			}

			record.hasCollider2DComponent = true;
			if (false == record.collider2D.has_value())
			{
				record.collider2D = Xelqoria::Game::SceneCollider2DRecord{};
			}
			record.collider2D->offset = *offset;
			return std::nullopt;
		}

		if (fieldKey == "collider2D.size")
		{
			const auto size = ParseVector2(value);
			if (false == size.has_value())
			{
				return MakeError(lineNumber, key, "collider2D.size は x,y 形式である必要があります。");
			}

			if (size->x <= 0.0f || size->y <= 0.0f)
			{
				return MakeError(lineNumber, key, "collider2D.size は 0 より大きい必要があります。");
			}

			record.hasCollider2DComponent = true;
			if (false == record.collider2D.has_value())
			{
				record.collider2D = Xelqoria::Game::SceneCollider2DRecord{};
			}
			record.collider2D->size = *size;
			return std::nullopt;
		}

		if (false == fieldKey.starts_with(Xelqoria::Game::SceneSaveExtensionFieldPrefix))
		{
			return MakeError(lineNumber, key, "未対応の Scene フィールドです。");
		}

		return std::nullopt;
	}
}

namespace Xelqoria::Game
{
	SceneLoadResult SceneTextReader::Read(std::string_view source)
	{
		std::optional<std::string> magic;
		std::optional<std::uint32_t> version;
		std::map<std::size_t, SceneEntitySaveRecord> records;
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

			if (false == line.empty())
			{
				const std::size_t separator = line.find('=');
				if (separator == std::string_view::npos)
				{
					return MakeError(lineNumber, {}, "Scene の各行は key=value 形式である必要があります。");
				}

				const std::string_view key = Trim(line.substr(0, separator));
				const std::string_view value = Trim(line.substr(separator + 1));
				if (key == "magic")
				{
					magic = std::string(value);
				}
				else if (key == "version")
				{
					version = ParseUnsigned(value);
					if (false == version.has_value())
					{
						return MakeError(lineNumber, key, "version は符号なし整数である必要があります。");
					}
				}
				else if (key.starts_with("entity."))
				{
					const std::string_view remainder = key.substr(std::string_view("entity.").size());
					const std::size_t nextDot = remainder.find('.');
					if (nextDot == std::string_view::npos)
					{
						return MakeError(lineNumber, key, "entity フィールド名が不正です。");
					}

					const auto entityIndex = ParseUnsigned(remainder.substr(0, nextDot));
					if (false == entityIndex.has_value())
					{
						return MakeError(lineNumber, key, "entity index は符号なし整数である必要があります。");
					}

					auto& record = records[*entityIndex];
					const std::string_view fieldKey = remainder.substr(nextDot + 1);
					const auto parseError = ParseEntityField(lineNumber, key, fieldKey, value, record);
					if (parseError.has_value())
					{
						return *parseError;
					}
				}
				else
				{
					return MakeError(lineNumber, key, "未対応の Scene フィールドです。");
				}
			}

			if (lineEnd == std::string_view::npos)
			{
				break;
			}

			cursor = lineEnd + 1;
		}

		if (false == magic.has_value() || *magic != SceneSaveFormatMagic)
		{
			return MakeError(0, "magic", "SceneSaveFormatMagic と一致する magic が必要です。");
		}

		if (false == version.has_value() || (*version != 1 && *version != SceneSaveFormatVersion))
		{
			return MakeError(0, "version", "対応していない Scene 保存バージョンです。");
		}

		Scene scene;
		std::set<EntityId> usedEntityIds;
		for (const auto& [entityIndex, record] : records)
		{
			if (record.entityId == 0)
			{
				return MakeError(0, "entity." + std::to_string(entityIndex) + ".id", "entity id が不足しています。");
			}

			if (false == usedEntityIds.insert(record.entityId).second)
			{
				return MakeError(0, "entity." + std::to_string(entityIndex) + ".id", "entity id が重複しています。");
			}

			auto& entity = scene.CreateEntity(record.entityId);
			entity.SetTransform(record.transform);
			if (record.hasName)
			{
				entity.SetName(record.name);
			}
			if (record.hasSpriteComponent || record.spriteRef.has_value() || record.materialRef.has_value())
			{
				const Core::AssetId spriteAssetRef = record.spriteRef.has_value()
					? record.spriteRef->spriteAssetRef
					: Core::AssetId{};
				SpriteComponent spriteComponent{
					spriteAssetRef,
					{}
				};
				if (record.materialRef.has_value())
				{
					spriteComponent.materialAssetRef = record.materialRef->materialAssetRef;
				}

				entity.SetSpriteComponent(spriteComponent);
			}
			if (record.hasCollider2DComponent || record.collider2D.has_value())
			{
				const SceneCollider2DRecord colliderRecord = record.collider2D.value_or(SceneCollider2DRecord{});
				entity.SetCollider2DComponent(Collider2DComponent{
					colliderRecord.enabled,
					colliderRecord.isTrigger,
					colliderRecord.shapeType,
					colliderRecord.offset,
					colliderRecord.size
				});
			}
		}

		return { std::move(scene), std::nullopt };
	}
}
