#include "Scene.h"

#include <algorithm>
#include <limits>
#include <sstream>

#include "Entity.h"
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "Assets/ISpriteAssetResolver.h"
#include "SpriteComponent.h"
#include <ITextureAssetResolver.h>
#include <Sprite.h>

namespace Xelqoria::Game
{
	namespace
	{
		/// <summary>
		/// ロガーが設定されている場合だけメッセージを出力する。
		/// </summary>
		/// <param name="logger">出力先ロガー。</param>
		/// <param name="message">出力するメッセージ。</param>
		void LogMessage(const std::function<void(const std::string&)>& logger, std::string message)
		{
			if (logger) {
				logger(message);
			}
		}

		/// <summary>
		/// Sprite 描画候補 1 件を解決済み Sprite へ変換する。
		/// </summary>
		/// <param name="renderItem">変換対象の描画候補。</param>
		/// <param name="spriteAssetResolver">SpriteAsset を解決する Resolver。</param>
		/// <param name="textureAssetResolver">Texture2D を解決する Resolver。</param>
		/// <param name="logger">解決状況を受け取るロガー。</param>
		/// <returns>解決に成功した Sprite。失敗時は nullopt。</returns>
		std::optional<ResolvedSceneSprite> ResolveSceneRenderItem(
			const SceneSpriteRenderItem& renderItem,
			const Assets::ISpriteAssetResolver& spriteAssetResolver,
			const Graphics::ITextureAssetResolver& textureAssetResolver,
			const std::function<void(const std::string&)>& logger)
		{
			if (renderItem.transform == nullptr || renderItem.spriteComponent == nullptr) {
				LogMessage(logger, "Scene::ResolveSceneSprites skipped an item because required references were null.");
				return std::nullopt;
			}

			const auto& spriteAssetRef = renderItem.spriteComponent->spriteAssetRef;
			if (spriteAssetRef.IsEmpty()) {
				std::ostringstream message;
				message << "Scene::ResolveSceneSprites skipped entity " << renderItem.entityId
					<< " because spriteAssetRef was empty.";
				LogMessage(logger, message.str());
				return std::nullopt;
			}

			const auto spriteAsset = spriteAssetResolver.ResolveSpriteAsset(spriteAssetRef);
			if (!spriteAsset.has_value()) {
				std::ostringstream message;
				message << "Scene::ResolveSceneSprites could not resolve SpriteAsset '"
					<< spriteAssetRef.GetValue() << "' for entity " << renderItem.entityId << ".";
				LogMessage(logger, message.str());
				return std::nullopt;
			}

			const auto texture = textureAssetResolver.ResolveTexture(spriteAsset->textureAssetId);
			if (!texture) {
				std::ostringstream message;
				message << "Scene::ResolveSceneSprites could not resolve Texture2D '"
					<< spriteAsset->textureAssetId.GetValue() << "' for entity " << renderItem.entityId << ".";
				LogMessage(logger, message.str());
				return std::nullopt;
			}

			Graphics::Sprite sprite{};
			sprite.SetTexture(texture);
			sprite.SetTextureAssetId(spriteAsset->textureAssetId);
			sprite.SetPosition(renderItem.transform->position.x, renderItem.transform->position.y);
			sprite.SetScale(renderItem.transform->scale.x, renderItem.transform->scale.y);
			sprite.SetRotationDegrees(renderItem.transform->rotation.z);

			std::ostringstream message;
			message << "Scene::ResolveSceneSprites resolved entity " << renderItem.entityId
				<< " from SpriteAsset '" << spriteAssetRef.GetValue()
				<< "' to Texture2D '" << spriteAsset->textureAssetId.GetValue() << "'.";
			LogMessage(logger, message.str());

			return ResolvedSceneSprite{
				renderItem.entityId,
				std::move(sprite)
			};
		}
	}

	Scene::Scene() = default;

	Scene::~Scene() = default;

	Entity& Scene::CreateEntity()
	{
		while (FindEntity(m_nextEntityId).has_value()
			&& m_nextEntityId < (std::numeric_limits<EntityId>::max)())
		{
			++m_nextEntityId;
		}

		const auto existingEntity = FindEntity(m_nextEntityId);
		if (existingEntity.has_value())
		{
			return existingEntity->get();
		}

		const EntityId entityId = m_nextEntityId;
		m_entities.emplace_back(entityId);
		if (m_nextEntityId < (std::numeric_limits<EntityId>::max)())
		{
			++m_nextEntityId;
		}

		return m_entities.back();
	}

	Entity& Scene::CreateEntity(EntityId entityId)
	{
		const auto existingEntity = FindEntity(entityId);
		if (existingEntity.has_value())
		{
			return existingEntity->get();
		}

		m_entities.emplace_back(entityId);
		if (entityId >= m_nextEntityId && entityId < (std::numeric_limits<EntityId>::max)()) {
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

		std::stable_sort(
			renderItems.begin(),
			renderItems.end(),
			[](const SceneSpriteRenderItem& lhs, const SceneSpriteRenderItem& rhs)
			{
				const std::int32_t lhsSortOrder = lhs.spriteComponent != nullptr
					? lhs.spriteComponent->renderSettings.sortOrder
					: 0;
				const std::int32_t rhsSortOrder = rhs.spriteComponent != nullptr
					? rhs.spriteComponent->renderSettings.sortOrder
					: 0;
				return lhsSortOrder < rhsSortOrder;
			});

		return renderItems;
	}

	std::vector<Graphics::Sprite> Scene::ResolveSprites(
		const Assets::ISpriteAssetResolver& spriteAssetResolver,
		const Graphics::ITextureAssetResolver& textureAssetResolver,
		const std::function<void(const std::string&)>& logger) const
	{
		std::vector<Graphics::Sprite> resolvedSprites;
		const auto sceneSprites = ResolveSceneSprites(spriteAssetResolver, textureAssetResolver, logger);
		resolvedSprites.reserve(sceneSprites.size());

		for (const ResolvedSceneSprite& sceneSprite : sceneSprites) {
			resolvedSprites.push_back(sceneSprite.sprite);
		}

		return resolvedSprites;
	}

	std::vector<ResolvedSceneSprite> Scene::ResolveSceneSprites(
		const Assets::ISpriteAssetResolver& spriteAssetResolver,
		const Graphics::ITextureAssetResolver& textureAssetResolver,
		const std::function<void(const std::string&)>& logger) const
	{
		std::vector<ResolvedSceneSprite> resolvedSprites;
		const auto renderItems = CollectSpriteRenderItems();
		resolvedSprites.reserve(renderItems.size());

		for (const SceneSpriteRenderItem& renderItem : renderItems) {
			const auto resolvedSprite = ResolveSceneRenderItem(
				renderItem,
				spriteAssetResolver,
				textureAssetResolver,
				logger);
			if (false == resolvedSprite.has_value()) {
				continue;
			}

			resolvedSprites.push_back(std::move(*resolvedSprite));
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
