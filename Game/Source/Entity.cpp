#include "Entity.h"

#include <optional>
#include <utility>

#include "SpriteComponent.h"

namespace Xelqoria::Game
{
	Entity::Entity(EntityId id)
		: m_id(id)
	{
	}

	EntityId Entity::GetId() const
	{
		return m_id;
	}

	Transform& Entity::GetTransform()
	{
		return m_transform;
	}

	const Transform& Entity::GetTransform() const
	{
		return m_transform;
	}

	void Entity::SetTransform(const Transform& transform)
	{
		m_transform = transform;
	}

	void Entity::SetTransform(Transform&& transform)
	{
		m_transform = std::move(transform);
	}

	void Entity::SetPosition(const Xelqoria::Math::Vector3& position)
	{
		m_transform.SetPosition(position);
	}

	void Entity::SetPosition(float x, float y, float z)
	{
		m_transform.SetPosition(x, y, z);
	}

	void Entity::SetRotation(const Xelqoria::Math::Vector3& rotation)
	{
		m_transform.SetRotation(rotation);
	}

	void Entity::SetRotation(float x, float y, float z)
	{
		m_transform.SetRotation(x, y, z);
	}

	void Entity::SetScale(const Xelqoria::Math::Vector3& scale)
	{
		m_transform.SetScale(scale);
	}

	void Entity::SetScale(float x, float y, float z)
	{
		m_transform.SetScale(x, y, z);
	}

	void Entity::SetSpriteComponent(const SpriteComponent& spriteComponent)
	{
		m_spriteComponent = spriteComponent;
	}

	void Entity::SetSpriteComponent(SpriteComponent&& spriteComponent)
	{
		m_spriteComponent = std::move(spriteComponent);
	}

	std::optional<std::reference_wrapper<SpriteComponent>> Entity::GetSpriteComponent()
	{
		if (false == m_spriteComponent.has_value()) {
			return std::nullopt;
		}

		return *m_spriteComponent;
	}

	std::optional<std::reference_wrapper<const SpriteComponent>> Entity::GetSpriteComponent() const
	{
		if (false == m_spriteComponent.has_value()) {
			return std::nullopt;
		}

		return *m_spriteComponent;
	}

	bool Entity::HasSpriteComponent() const
	{
		return m_spriteComponent.has_value();
	}

	void Entity::RemoveSpriteComponent()
	{
		m_spriteComponent.reset();
	}
}
