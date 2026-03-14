#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "Transform.h"

namespace Xelqoria::Game
{
	/// <summary>
	/// Entity を識別するための最小 ID 型。
	/// 0 は無効値として扱う。
	/// </summary>
	using EntityId = std::uint32_t;

	/// <summary>
	/// 最小構成の Entity 管理を提供する。
	/// Entity の生成・破棄と Transform の関連付けを担当する。
	/// </summary>
	class EntityManager
	{
	public:
		/// <summary>
		/// 新しい Entity を生成し、識別子を返す。
		/// </summary>
		/// <returns>生成した EntityId。</returns>
		EntityId CreateEntity();

		/// <summary>
		/// Entity を破棄し、関連付けられた Transform も削除する。
		/// </summary>
		/// <param name="entityId">破棄する EntityId。</param>
		/// <returns>Entity が存在して削除された場合は true。</returns>
		bool DestroyEntity(EntityId entityId);

		/// <summary>
		/// 指定した Entity が存在するか確認する。
		/// </summary>
		/// <param name="entityId">確認対象の EntityId。</param>
		/// <returns>存在する場合は true。</returns>
		bool HasEntity(EntityId entityId) const;

		/// <summary>
		/// 管理中の Entity 数を返す。
		/// </summary>
		/// <returns>現在保持している Entity 数。</returns>
		std::size_t GetEntityCount() const;

		/// <summary>
		/// Entity に Transform を関連付ける。
		/// </summary>
		/// <param name="entityId">Transform を設定する EntityId。</param>
		/// <param name="transform">設定する Transform。</param>
		/// <returns>Entity が存在する場合は true。</returns>
		bool SetTransform(EntityId entityId, const Transform& transform);

		/// <summary>
		/// Entity に関連付けられた Transform を取得する。
		/// </summary>
		/// <param name="entityId">取得対象の EntityId。</param>
		/// <returns>Transform が存在する場合はそのポインタ。存在しない場合は nullptr。</returns>
		Transform* FindTransform(EntityId entityId);

		/// <summary>
		/// Entity に関連付けられた Transform を読み取り専用で取得する。
		/// </summary>
		/// <param name="entityId">取得対象の EntityId。</param>
		/// <returns>Transform が存在する場合はそのポインタ。存在しない場合は nullptr。</returns>
		const Transform* FindTransform(EntityId entityId) const;

	private:
		/// <summary>
		/// 次に発行する EntityId。
		/// </summary>
		EntityId m_nextEntityId = 1;

		/// <summary>
		/// 生存中の Entity 一覧。
		/// </summary>
		std::unordered_set<EntityId> m_entities;

		/// <summary>
		/// Entity に紐付く Transform 一覧。
		/// </summary>
		std::unordered_map<EntityId, Transform> m_transforms;
	};
}
