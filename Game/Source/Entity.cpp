#include "Entity.h"
#include "Transform.h"

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
}
