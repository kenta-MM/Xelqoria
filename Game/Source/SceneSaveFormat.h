#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "AssetId.h"
#include "Entity.h"
#include "Transform.h"

namespace Xelqoria::Game
{
	/// <summary>
	/// Scene 保存フォーマットの識別名を表す。
	/// </summary>
	inline constexpr std::string_view SceneSaveFormatMagic = "xelqoria.scene";

	/// <summary>
	/// Scene 保存フォーマットの初期バージョンを表す。
	/// </summary>
	inline constexpr std::uint32_t SceneSaveFormatVersion = 1;

	/// <summary>
	/// 将来拡張用フィールドを配置する接頭辞を表す。
	/// </summary>
	inline constexpr std::string_view SceneSaveExtensionFieldPrefix = "extensions.";

	/// <summary>
	/// Scene 保存時に 1 Entity 分の SpriteRef を表す。
	/// </summary>
	struct SceneSpriteRefRecord
	{
		/// <summary>
		/// Entity に関連付ける Sprite アセット参照を表す。
		/// </summary>
		Core::AssetId spriteAssetRef{};
	};

	/// <summary>
	/// Scene 保存時に 1 Entity 分の MaterialRef を表す。
	/// </summary>
	struct SceneMaterialRefRecord
	{
		/// <summary>
		/// Entity に関連付ける Material アセット参照を表す。
		/// </summary>
		Core::AssetId materialAssetRef{};
	};

	/// <summary>
	/// Scene 保存時に 1 Entity 分の保存対象を表す。
	/// </summary>
	struct SceneEntitySaveRecord
	{
		/// <summary>
		/// 保存対象の Entity ID を表す。
		/// </summary>
		EntityId entityId = 0;

		/// <summary>
		/// 保存対象の Transform を表す。
		/// </summary>
		Transform transform{};

		/// <summary>
		/// 保存対象の Entity 名を表す。
		/// </summary>
		std::string name{};

		/// <summary>
		/// Entity 名フィールドが明示的に保存されていたかを表す。
		/// </summary>
		bool hasName = false;

		/// <summary>
		/// SpriteComponent を保持しているかを表す。
		/// </summary>
		bool hasSpriteComponent = false;

		/// <summary>
		/// 保存対象の SpriteRef を表す。
		/// 未設定の Entity では空を許容する。
		/// </summary>
		std::optional<SceneSpriteRefRecord> spriteRef{};

		/// <summary>
		/// 保存対象の MaterialRef を表す。
		/// 未設定の Entity では空を許容する。
		/// </summary>
		std::optional<SceneMaterialRefRecord> materialRef{};
	};

	/// <summary>
	/// Scene 保存フォーマット v1 の項目構成を説明する。
	/// </summary>
	inline constexpr std::string_view SceneSaveFormatDocumentation =
		R"(magic=xelqoria.scene
version=1
entity.<index>.id=<EntityId>
entity.<index>.name="<EntityName>"
entity.<index>.hasSpriteComponent=<true|false>
entity.<index>.transform.position=<x>,<y>,<z>
entity.<index>.transform.rotation=<x>,<y>,<z>
entity.<index>.transform.scale=<x>,<y>,<z>
entity.<index>.spriteRef=<SpriteAssetId>
entity.<index>.materialRef=<MaterialAssetId>
entity.<index>.extensions.<name>=<reserved>)";
}
