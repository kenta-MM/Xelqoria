#include "Entity.h"

#include <utility>
#include "Transform.h"
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
		if (!m_spriteComponent.has_value()) {
			return std::nullopt;
		}

		return *m_spriteComponent;
	}

	std::optional<std::reference_wrapper<const SpriteComponent>> Entity::GetSpriteComponent() const
	{
		if (!m_spriteComponent.has_value()) {
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
