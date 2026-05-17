#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

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
		/// Entity 名を取得する。
		/// </summary>
		/// <returns>現在の Entity 名。</returns>
		const std::string& GetName() const;

		/// <summary>
		/// Entity 名を設定する。
		/// </summary>
		/// <param name="name">設定する Entity 名。</param>
		void SetName(const std::string& name);

		/// <summary>
		/// Entity 名を設定する。
		/// </summary>
		/// <param name="name">設定する Entity 名。</param>
		void SetName(std::string&& name);

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
		/// Transform を Entity に設定する。
		/// </summary>
		/// <param name="transform">設定する Transform。</param>
		void SetTransform(const Transform& transform);

		/// <summary>
		/// Transform を Entity に設定する。
		/// </summary>
		/// <param name="transform">設定する Transform。</param>
		void SetTransform(Transform&& transform);

		/// <summary>
		/// Transform の位置を更新する。
		/// </summary>
		/// <param name="position">設定する位置。</param>
		void SetPosition(const Xelqoria::Math::Vector3& position);

		/// <summary>
		/// Transform の位置を更新する。
		/// </summary>
		/// <param name="x">X 座標。</param>
		/// <param name="y">Y 座標。</param>
		/// <param name="z">Z 座標。</param>
		void SetPosition(float x, float y, float z);

		/// <summary>
		/// Transform の回転量を更新する。
		/// </summary>
		/// <param name="rotation">設定する回転量。</param>
		void SetRotation(const Xelqoria::Math::Vector3& rotation);

		/// <summary>
		/// Transform の回転量を更新する。
		/// </summary>
		/// <param name="x">X 軸回転量。</param>
		/// <param name="y">Y 軸回転量。</param>
		/// <param name="z">Z 軸回転量。</param>
		void SetRotation(float x, float y, float z);

		/// <summary>
		/// Transform の拡大率を更新する。
		/// </summary>
		/// <param name="scale">設定する拡大率。</param>
		void SetScale(const Xelqoria::Math::Vector3& scale);

		/// <summary>
		/// Transform の拡大率を更新する。
		/// </summary>
		/// <param name="x">X 軸拡大率。</param>
		/// <param name="y">Y 軸拡大率。</param>
		/// <param name="z">Z 軸拡大率。</param>
		void SetScale(float x, float y, float z);

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
		/// <returns>SpriteComponent。未設定の場合は空。</returns>
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
		std::string m_name{};
		Transform m_transform{};
		std::optional<SpriteComponent> m_spriteComponent{};
	};
}
