#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "AssetId.h"
#include "Collider2DComponent.h"
#include "Entity.h"
#include "Transform.h"

namespace Xelqoria::Game
{
	/// <summary>
	/// Scene 保存フォーマットの識別名を表す。
	/// </summary>
	inline constexpr std::string_view SceneSaveFormatMagic = "xelqoria.scene";

	/// <summary>
	/// Scene 保存フォーマットの現在バージョンを表す。
	/// </summary>
	inline constexpr std::uint32_t SceneSaveFormatVersion = 2;

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
	/// Scene 保存時に 1 Entity 分の Collider2DComponent を表す。
	/// </summary>
	struct SceneCollider2DRecord
	{
		/// <summary>
		/// Collider が有効かを表す。
		/// </summary>
		bool enabled = true;

		/// <summary>
		/// Trigger として扱うかを表す。
		/// </summary>
		bool isTrigger = false;

		/// <summary>
		/// Collider の形状種別。
		/// </summary>
		Collider2DShapeType shapeType = Collider2DShapeType::Box;

		/// <summary>
		/// Transform 位置からの 2D オフセット。
		/// </summary>
		Xelqoria::Math::Vector2 offset{};

		/// <summary>
		/// Collider の幅と高さ。
		/// </summary>
		Xelqoria::Math::Vector2 size{ 1.0f, 1.0f };
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

		/// <summary>
		/// Collider2DComponent を保持しているかを表す。
		/// </summary>
		bool hasCollider2DComponent = false;

		/// <summary>
		/// 保存対象の Collider2DComponent を表す。
		/// </summary>
		std::optional<SceneCollider2DRecord> collider2D{};
	};

	/// <summary>
	/// Scene 保存フォーマット v1 の項目構成を説明する。
	/// </summary>
	inline constexpr std::string_view SceneSaveFormatDocumentation =
		R"(magic=xelqoria.scene
version=1
version=2
entity.<index>.id=<EntityId>
entity.<index>.name="<EntityName>"
entity.<index>.hasSpriteComponent=<true|false>
entity.<index>.transform.position=<x>,<y>,<z>
entity.<index>.transform.rotation=<x>,<y>,<z>
entity.<index>.transform.scale=<x>,<y>,<z>
entity.<index>.spriteRef=<SpriteAssetId>
entity.<index>.materialRef=<MaterialAssetId>
entity.<index>.hasCollider2DComponent=<true|false>
entity.<index>.collider2D.enabled=<true|false>
entity.<index>.collider2D.isTrigger=<true|false>
entity.<index>.collider2D.shapeType=Box
entity.<index>.collider2D.offset=<x>,<y>
entity.<index>.collider2D.size=<x>,<y>
entity.<index>.extensions.<name>=<reserved>)";
}
