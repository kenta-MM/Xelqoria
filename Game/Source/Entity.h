#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "SpriteComponent.h"
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

		/// <summary>
		/// SpriteComponent を Entity に設定する。
		/// </summary>
		/// <param name="spriteComponent">設定する SpriteComponent。</param>
		void SetSpriteComponent(const SpriteComponent& spriteComponent);

		/// <summary>
		/// SpriteComponent を Entity に設定する。
		/// </summary>
		/// <param name="spriteComponent">設定する SpriteComponent。</param>
		void SetSpriteComponent(SpriteComponent&& spriteComponent);

		/// <summary>
		/// Entity に設定されている SpriteComponent を取得する。
		/// </summary>
		/// <returns>SpriteComponent。未設定時は空。</returns>
		std::optional<std::reference_wrapper<SpriteComponent>> GetSpriteComponent();

		/// <summary>
		/// Entity に設定されている SpriteComponent を取得する。
		/// </summary>
		/// <returns>読み取り専用の SpriteComponent。未設定時は空。</returns>
		std::optional<std::reference_wrapper<const SpriteComponent>> GetSpriteComponent() const;

		/// <summary>
		/// Entity に SpriteComponent が設定されているかを取得する。
		/// </summary>
		/// <returns>設定済みの場合は true。</returns>
		bool HasSpriteComponent() const;

		/// <summary>
		/// Entity から SpriteComponent を取り外す。
		/// </summary>
		void RemoveSpriteComponent();

	private:
		EntityId m_id = 0;
		Transform m_transform{};
		std::optional<SpriteComponent> m_spriteComponent{};
	};
}
