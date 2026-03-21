#pragma once

#include <cstdint>
#include <optional>
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
		/// 保存対象の SpriteRef を表す。
		/// 未設定の Entity では空を許容する。
		/// </summary>
		std::optional<SceneSpriteRefRecord> spriteRef{};
	};

	/// <summary>
	/// Scene 保存フォーマット v1 の項目構成を説明する。
	/// </summary>
	inline constexpr std::string_view SceneSaveFormatDocumentation =
		R"(magic=xelqoria.scene
version=1
entity.<index>.id=<EntityId>
entity.<index>.transform.position=<x>,<y>,<z>
entity.<index>.transform.rotation=<x>,<y>,<z>
entity.<index>.transform.scale=<x>,<y>,<z>
entity.<index>.spriteRef=<SpriteAssetId>
entity.<index>.extensions.<name>=<reserved>)";
}
