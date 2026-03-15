#pragma once

#include <cstdint>

#include "Transform.h"

namespace Xelqoria::Game
{
	/// <summary>
	/// Scene 内でオブジェクトを識別するための ID。
	/// </summary>
	using EntityId = std::uint32_t;

	/// <summary>
	/// Scene が保持する最小単位のエンティティ。
	/// </summary>
	class Entity
	{
	public:
		/// <summary>
		/// Entity を生成する。
		/// </summary>
		/// <param name="id">割り当てる Entity ID。</param>
		explicit Entity(EntityId id);

		/// <summary>
		/// Entity ID を取得する。
		/// </summary>
		/// <returns>Entity ID。</returns>
		EntityId GetId() const;

		/// <summary>
		/// Transform を取得する。
		/// </summary>
		/// <returns>保持している Transform。</returns>
		Transform& GetTransform();

		/// <summary>
		/// Transform を取得する。
		/// </summary>
		/// <returns>保持している Transform の読み取り専用参照。</returns>
		const Transform& GetTransform() const;

	private:
		EntityId m_id = 0;
		Transform m_transform{};
	};
}
