#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <vector>

#include "Entity.h"

namespace Xelqoria::Game
{
	/// <summary>
	/// Entity 群を保持して列挙する Scene コンテナ。
	/// </summary>
	class Scene
	{
	public:
		/// <summary>
		/// 新しい Entity を生成して Scene に追加する。
		/// </summary>
		/// <returns>追加された Entity。</returns>
		Entity& CreateEntity();

		/// <summary>
		/// 指定した Entity を Scene から削除する。
		/// </summary>
		/// <param name="entityId">削除対象の Entity ID。</param>
		/// <returns>削除できた場合は true。</returns>
		bool DestroyEntity(EntityId entityId);

		/// <summary>
		/// 指定した Entity を取得する。
		/// </summary>
		/// <param name="entityId">取得対象の Entity ID。</param>
		/// <returns>見つかった Entity への参照。見つからない場合は空。</returns>
		std::optional<std::reference_wrapper<Entity>> FindEntity(EntityId entityId);

		/// <summary>
		/// 指定した Entity を取得する。
		/// </summary>
		/// <param name="entityId">取得対象の Entity ID。</param>
		/// <returns>見つかった Entity への読み取り専用参照。見つからない場合は空。</returns>
		std::optional<std::reference_wrapper<const Entity>> FindEntity(EntityId entityId) const;

		/// <summary>
		/// Scene が保持している Entity 一覧を取得する。
		/// </summary>
		/// <returns>Entity 一覧の読み取り専用ビュー。</returns>
		std::span<const Entity> GetEntities() const;

		/// <summary>
		/// Scene が保持している Entity 数を取得する。
		/// </summary>
		/// <returns>Entity 数。</returns>
		std::size_t GetEntityCount() const;

	private:
		std::vector<Entity> m_entities;
		EntityId m_nextEntityId = 1;
	};
}
