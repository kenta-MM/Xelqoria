#include "Scene.h"

#include <algorithm>
#include <sstream>

#include "Entity.h"

namespace Xelqoria::Game
{
	namespace
	{
		void LogMessage(const std::function<void(const std::string&)>& logger, std::string message)
		{
			if (logger) {
				logger(message);
			}
		}
	}

	Scene::Scene() = default;

	Scene::~Scene() = default;

	Entity& Scene::CreateEntity()
	{
		m_entities.emplace_back(m_nextEntityId++);
		return m_entities.back();
	}

	Entity& Scene::CreateEntity(EntityId entityId)
	{
		m_entities.emplace_back(entityId);
		if (entityId >= m_nextEntityId) {
			m_nextEntityId = entityId + 1;
		}

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

	std::vector<SceneSpriteRenderItem> Scene::CollectSpriteRenderItems() const
	{
		std::vector<SceneSpriteRenderItem> renderItems;
		renderItems.reserve(m_entities.size());

		for (const auto& entity : m_entities) {
			const auto spriteComponent = entity.GetSpriteComponent();
			if (!spriteComponent.has_value()) {
				continue;
			}

			const auto& spriteComponentValue = spriteComponent->get();
			if (!spriteComponentValue.renderSettings.visible) {
				continue;
			}

			renderItems.push_back(SceneSpriteRenderItem{
				entity.GetId(),
				&entity.GetTransform(),
				&spriteComponentValue
			});
		}

		return renderItems;
	}

	std::vector<Graphics::Sprite> Scene::ResolveSprites(
		const Assets::ISpriteAssetResolver& spriteAssetResolver,
		const Graphics::ITextureAssetResolver& textureAssetResolver,
		const std::function<void(const std::string&)>& logger) const
	{
		std::vector<Graphics::Sprite> resolvedSprites;
		const auto renderItems = CollectSpriteRenderItems();
		resolvedSprites.reserve(renderItems.size());

		for (const auto& renderItem : renderItems) {
			if (renderItem.transform == nullptr || renderItem.spriteComponent == nullptr) {
				LogMessage(logger, "Scene::ResolveSprites skipped an item because required references were null.");
				continue;
			}

			const auto& spriteAssetRef = renderItem.spriteComponent->spriteAssetRef;
			if (spriteAssetRef.IsEmpty()) {
				std::ostringstream message;
				message << "Scene::ResolveSprites skipped entity " << renderItem.entityId
					<< " because spriteAssetRef was empty.";
				LogMessage(logger, message.str());
				continue;
			}

			const auto spriteAsset = spriteAssetResolver.ResolveSpriteAsset(spriteAssetRef);
			if (!spriteAsset.has_value()) {
				std::ostringstream message;
				message << "Scene::ResolveSprites could not resolve SpriteAsset '"
					<< spriteAssetRef.GetValue() << "' for entity " << renderItem.entityId << ".";
				LogMessage(logger, message.str());
				continue;
			}

			const auto texture = textureAssetResolver.ResolveTexture(spriteAsset->textureAssetId);
			if (!texture) {
				std::ostringstream message;
				message << "Scene::ResolveSprites could not resolve Texture2D '"
					<< spriteAsset->textureAssetId.GetValue() << "' for entity " << renderItem.entityId << ".";
				LogMessage(logger, message.str());
				continue;
			}

			Graphics::Sprite sprite{};
			sprite.SetTexture(texture);
			sprite.SetTextureAssetId(spriteAsset->textureAssetId);
			sprite.SetPosition(renderItem.transform->position.x, renderItem.transform->position.y);
			sprite.SetScale(renderItem.transform->scale.x, renderItem.transform->scale.y);
			resolvedSprites.push_back(std::move(sprite));

			std::ostringstream message;
			message << "Scene::ResolveSprites resolved entity " << renderItem.entityId
				<< " from SpriteAsset '" << spriteAssetRef.GetValue()
				<< "' to Texture2D '" << spriteAsset->textureAssetId.GetValue() << "'.";
			LogMessage(logger, message.str());
		}

		return resolvedSprites;
	}

	void Scene::ValidateSpriteReferences(
		const Assets::ISpriteAssetResolver& spriteAssetResolver,
		const std::function<void(const std::string&)>& logger)
	{
		for (auto& entity : m_entities) {
			auto spriteComponent = entity.GetSpriteComponent();
			if (!spriteComponent.has_value()) {
				continue;
			}

			auto& spriteComponentValue = spriteComponent->get();
			if (spriteComponentValue.spriteAssetRef.IsEmpty()) {
				spriteComponentValue.spriteAssetState = SpriteAssetReferenceState::Unknown;
				spriteComponentValue.missingSpriteAssetRef = {};
				continue;
			}

			const auto spriteAsset = spriteAssetResolver.ResolveSpriteAsset(spriteComponentValue.spriteAssetRef);
			if (!spriteAsset.has_value()) {
				spriteComponentValue.spriteAssetState = SpriteAssetReferenceState::Missing;
				spriteComponentValue.missingSpriteAssetRef = spriteComponentValue.spriteAssetRef;

				std::ostringstream message;
				message << "Scene::ValidateSpriteReferences detected missing SpriteAsset '"
					<< spriteComponentValue.spriteAssetRef.GetValue() << "' for entity " << entity.GetId() << ".";
				LogMessage(logger, message.str());
				continue;
			}

			spriteComponentValue.spriteAssetState = SpriteAssetReferenceState::Resolved;
			spriteComponentValue.missingSpriteAssetRef = {};
		}
	}
}
