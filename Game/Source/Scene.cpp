#include "Scene.h"

#include <algorithm>
#include "Sprite.h"
#include "Entity.h"
#include <optional>
#include <type_traits>

namespace Xelqoria::Game
{
	Scene::Scene() = default;

	Scene::~Scene() = default;

	Entity& Scene::CreateEntity()
	{
		m_entities.emplace_back(m_nextEntityId++);
		return m_entities.back();
	}

	bool Scene::DestroyEntity(EntityId entityId)
	{
		const auto it = std::find_if(
			m_entities.begin(),
			m_entities.end(),
			[entityId](const Entity& entity)
			{
				return entity.GetId() == entityId;
			});

		if (it == m_entities.end()) {
			return false;
		}

		m_entities.erase(it);
		return true;
	}

	std::optional<std::reference_wrapper<Entity>> Scene::FindEntity(EntityId entityId)
	{
		const auto it = std::find_if(
			m_entities.begin(),
			m_entities.end(),
			[entityId](const Entity& entity)
			{
				return entity.GetId() == entityId;
			});

		if (it == m_entities.end()) {
			return std::nullopt;
		}

		return *it;
	}

	std::optional<std::reference_wrapper<const Entity>> Scene::FindEntity(EntityId entityId) const
	{
		const auto it = std::find_if(
			m_entities.cbegin(),
			m_entities.cend(),
			[entityId](const Entity& entity)
			{
				return entity.GetId() == entityId;
			});

		if (it == m_entities.cend()) {
			return std::nullopt;
		}

		return *it;
	}

	std::span<const Entity> Scene::GetEntities() const
	{
		return std::span<const Entity>(m_entities.data(), m_entities.size());
	}

	std::size_t Scene::GetEntityCount() const
	{
		return m_entities.size();
	}

	void Scene::AddSprite(std::shared_ptr<Graphics::Sprite> sprite)
	{
		if (!sprite) {
			return;
		}

		m_sprites.push_back(std::move(sprite));
	}

	std::span<const std::shared_ptr<Graphics::Sprite>> Scene::GetSprites() const
	{
		return std::span<const std::shared_ptr<Graphics::Sprite>>(m_sprites.data(), m_sprites.size());
	}
}
