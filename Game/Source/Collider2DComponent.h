#pragma once

#include "Vector2.h"

namespace Xelqoria::Game
{
	struct AabbCollider2D;
	struct Transform;

	/// <summary>
	/// Collider2DComponent が表す 2D 形状種別。
	/// </summary>
	enum class Collider2DShapeType
	{
		/// <summary>
		/// 軸平行矩形として扱う Box Collider。
		/// </summary>
		Box
	};

	/// <summary>
	/// Entity に付与する 2D 当たり判定情報。
	/// </summary>
	struct Collider2DComponent
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
	/// Transform と Collider2DComponent から AABB Collider を生成する。
	/// </summary>
	/// <param name="transform">Entity の Transform。</param>
	/// <param name="collider">AABB に変換する Collider2DComponent。</param>
	/// <returns>Physics2D で扱う AABB Collider。</returns>
	[[nodiscard]] AabbCollider2D BuildAabbCollider2D(
		const Transform& transform,
		const Collider2DComponent& collider);
}
