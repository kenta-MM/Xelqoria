#include "SceneSerializer.h"

#include <charconv>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <string_view>

#include "SceneSaveFormat.h"

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
		if (first == std::string_view::npos) {
			return {};
		}

		const auto last = value.find_last_not_of(" \t\r");
		return value.substr(first, last - first + 1);
	}

	/// <summary>
	/// Vector3 をシーン保存形式の `x,y,z` 文字列として追記する。
	/// </summary>
	/// <param name="stream">出力先ストリーム。</param>
	/// <param name="value">追記するベクトル値。</param>
	void AppendVector3(std::ostringstream& stream, const Xelqoria::Game::Vector3& value)
	{
		stream << value.x << "," << value.y << "," << value.z;
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
		if (ec != std::errc{} || ptr != end) {
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
		if (!stream || !stream.eof()) {
			return std::nullopt;
		}

		return parsedValue;
	}

	/// <summary>
	/// `x,y,z` 形式の文字列を Vector3 へ変換する。
	/// </summary>
	/// <param name="value">変換対象の文字列。</param>
	/// <returns>変換に成功した Vector3。失敗時は nullopt。</returns>
	std::optional<Xelqoria::Game::Vector3> ParseVector3(std::string_view value)
	{
		Xelqoria::Game::Vector3 vector{};
		std::size_t cursor = 0;
		float* components[3] = { &vector.x, &vector.y, &vector.z };

		for (int index = 0; index < 3; ++index)
		{
			const std::size_t separator = value.find(',', cursor);
			const std::string_view token = separator == std::string_view::npos
				? value.substr(cursor)
				: value.substr(cursor, separator - cursor);
			const auto parsedComponent = ParseFloat(Trim(token));
			if (!parsedComponent.has_value()) {
				return std::nullopt;
			}

			*components[index] = *parsedComponent;
			if (separator == std::string_view::npos) {
				return index == 2 ? std::optional<Xelqoria::Game::Vector3>(vector) : std::nullopt;
			}

			cursor = separator + 1;
		}

		if (value.find(',', cursor) != std::string_view::npos) {
			return std::nullopt;
		}

		return vector;
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
}

namespace Xelqoria::Game
{
	std::string SceneSerializer::SaveToText(const Scene& scene)
	{
		std::ostringstream stream;
		stream << std::fixed << std::setprecision(6);
		stream << "magic=" << SceneSaveFormatMagic << "\n";
		stream << "version=" << SceneSaveFormatVersion << "\n";

		const auto entities = scene.GetEntities();
		for (std::size_t entityIndex = 0; entityIndex < entities.size(); ++entityIndex)
		{
			const auto& entity = entities[entityIndex];
			const auto& transform = entity.GetTransform();
			const auto prefix = "entity." + std::to_string(entityIndex);

			stream << prefix << ".id=" << entity.GetId() << "\n";
			stream << prefix << ".transform.position=";
			AppendVector3(stream, transform.position);
			stream << "\n";
			stream << prefix << ".transform.rotation=";
			AppendVector3(stream, transform.rotation);
			stream << "\n";
			stream << prefix << ".transform.scale=";
			AppendVector3(stream, transform.scale);
			stream << "\n";

			const auto spriteComponent = entity.GetSpriteComponent();
			if (spriteComponent.has_value() && !spriteComponent->get().spriteAssetRef.IsEmpty()) {
				stream << prefix << ".spriteRef=" << spriteComponent->get().spriteAssetRef.GetValue() << "\n";
			}
		}

		return stream.str();
	}

	SceneLoadResult SceneSerializer::LoadFromText(std::string_view source)
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

			if (!line.empty()) {
				const std::size_t separator = line.find('=');
				if (separator == std::string_view::npos) {
					return MakeError(lineNumber, {}, "Scene の各行は key=value 形式である必要があります。");
				}

				const std::string_view key = Trim(line.substr(0, separator));
				const std::string_view value = Trim(line.substr(separator + 1));
				if (key == "magic") {
					magic = std::string(value);
				}
				else if (key == "version") {
					version = ParseUnsigned(value);
					if (!version.has_value()) {
						return MakeError(lineNumber, key, "version は符号なし整数である必要があります。");
					}
				}
				else if (key.starts_with("entity.")) {
					const std::string_view remainder = key.substr(std::string_view("entity.").size());
					const std::size_t nextDot = remainder.find('.');
					if (nextDot == std::string_view::npos) {
						return MakeError(lineNumber, key, "entity フィールド名が不正です。");
					}

					const auto entityIndex = ParseUnsigned(remainder.substr(0, nextDot));
					if (!entityIndex.has_value()) {
						return MakeError(lineNumber, key, "entity index は符号なし整数である必要があります。");
					}

					auto& record = records[*entityIndex];
					const std::string_view fieldKey = remainder.substr(nextDot + 1);
					if (fieldKey == "id") {
						const auto entityId = ParseUnsigned(value);
						if (!entityId.has_value()) {
							return MakeError(lineNumber, key, "entity id は符号なし整数である必要があります。");
						}

						record.entityId = *entityId;
					}
					else if (fieldKey == "transform.position") {
						const auto parsedVector = ParseVector3(value);
						if (!parsedVector.has_value()) {
							return MakeError(lineNumber, key, "position は x,y,z 形式である必要があります。");
						}

						record.transform.position = *parsedVector;
					}
					else if (fieldKey == "transform.rotation") {
						const auto parsedVector = ParseVector3(value);
						if (!parsedVector.has_value()) {
							return MakeError(lineNumber, key, "rotation は x,y,z 形式である必要があります。");
						}

						record.transform.rotation = *parsedVector;
					}
					else if (fieldKey == "transform.scale") {
						const auto parsedVector = ParseVector3(value);
						if (!parsedVector.has_value()) {
							return MakeError(lineNumber, key, "scale は x,y,z 形式である必要があります。");
						}

						record.transform.scale = *parsedVector;
					}
					else if (fieldKey == "spriteRef") {
						record.spriteRef = SceneSpriteRefRecord{ Core::AssetId(value) };
					}
					else if (!fieldKey.starts_with(SceneSaveExtensionFieldPrefix)) {
						return MakeError(lineNumber, key, "未対応の Scene フィールドです。");
					}
				}
				else {
					return MakeError(lineNumber, key, "未対応の Scene フィールドです。");
				}
			}

			if (lineEnd == std::string_view::npos) {
				break;
			}

			cursor = lineEnd + 1;
		}

		if (!magic.has_value() || *magic != SceneSaveFormatMagic) {
			return MakeError(0, "magic", "SceneSaveFormatMagic と一致する magic が必要です。");
		}

		if (!version.has_value() || *version != SceneSaveFormatVersion) {
			return MakeError(0, "version", "対応していない Scene 保存バージョンです。");
		}

		Scene scene;
		for (const auto& [entityIndex, record] : records)
		{
			if (record.entityId == 0) {
				return MakeError(0, "entity." + std::to_string(entityIndex) + ".id", "entity id が不足しています。");
			}

			auto& entity = scene.CreateEntity(record.entityId);
			entity.GetTransform() = record.transform;
			if (record.spriteRef.has_value()) {
				entity.SetSpriteComponent(SpriteComponent{
					record.spriteRef->spriteAssetRef,
					{}
				});
			}
		}

		return { std::move(scene), std::nullopt };
	}
}
