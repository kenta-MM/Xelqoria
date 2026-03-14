#include "EntityManager.h"

namespace Xelqoria::Game
{
	EntityId EntityManager::CreateEntity()
	{
		const EntityId entityId = m_nextEntityId++;
		m_entities.insert(entityId);
		return entityId;
	}

	bool EntityManager::DestroyEntity(EntityId entityId)
	{
		if (!m_entities.erase(entityId))
		{
			return false;
		}

		m_transforms.erase(entityId);
		return true;
	}

	bool EntityManager::HasEntity(EntityId entityId) const
	{
		return m_entities.contains(entityId);
	}

	std::size_t EntityManager::GetEntityCount() const
	{
		return m_entities.size();
	}

	bool EntityManager::SetTransform(EntityId entityId, const Transform& transform)
	{
		if (!HasEntity(entityId))
		{
			return false;
		}

		m_transforms[entityId] = transform;
		return true;
	}

	Transform* EntityManager::FindTransform(EntityId entityId)
	{
		auto it = m_transforms.find(entityId);
		if (it == m_transforms.end())
		{
			return nullptr;
		}

		return &it->second;
	}

	const Transform* EntityManager::FindTransform(EntityId entityId) const
	{
		auto it = m_transforms.find(entityId);
		if (it == m_transforms.end())
		{
			return nullptr;
		}

		return &it->second;
	}
}
