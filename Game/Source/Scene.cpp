#include "Scene.h"

#include <algorithm>
#include <array>
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
#include "Assets/IMaterialAssetResolver.h"
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
		/// Sprite 描画で使用する Material 状態を表す。
		/// </summary>
		struct ResolvedSpriteMaterialState
		{
			Core::AssetId textureAssetId{};
			std::array<float, 4> color{ 1.0f, 1.0f, 1.0f, 1.0f };
			bool outlineEnabled = false;
			float outlineThickness = 1.0f;
			std::array<float, 4> outlineColor{ 1.0f, 1.0f, 0.0f, 1.0f };
			bool resolvedFromMaterialAsset = false;
			Core::AssetId materialAssetRef{};
		};

		/// <summary>
		/// 旧 SpriteAsset Texture 参照と SpriteComponent 描画設定から Material 状態を構成する。
		/// </summary>
		/// <param name="spriteAsset">参照元 SpriteAsset。</param>
		/// <param name="spriteComponent">参照元 SpriteComponent。</param>
		/// <returns>描画に使用する Material 状態。</returns>
		ResolvedSpriteMaterialState MakeSpriteAssetMaterialState(
			const Assets::SpriteAsset& spriteAsset,
			const SpriteComponent& spriteComponent)
		{
			ResolvedSpriteMaterialState materialState{};
			materialState.textureAssetId = spriteAsset.textureAssetId;
			materialState.color = spriteComponent.renderSettings.color;
			materialState.color[3] *= spriteComponent.renderSettings.opacity;
			return materialState;
		}

		/// <summary>
		/// Sprite 描画候補 1 件を解決済み Sprite へ変換する。
		/// </summary>
		/// <param name="renderItem">変換対象の描画候補。</param>
		/// <param name="spriteAssetResolver">SpriteAsset を解決する Resolver。</param>
		/// <param name="materialAssetResolver">MaterialAsset を解決する Resolver。未指定時は旧 SpriteAsset Texture を使用する。</param>
		/// <param name="textureAssetResolver">Texture2D を解決する Resolver。</param>
		/// <param name="logger">解決状況を受け取るロガー。</param>
		/// <returns>解決に成功した Sprite。失敗時は nullopt。</returns>
		std::optional<ResolvedSceneSprite> ResolveSceneRenderItem(
			const SceneSpriteRenderItem& renderItem,
			const Assets::ISpriteAssetResolver& spriteAssetResolver,
			const Assets::IMaterialAssetResolver* materialAssetResolver,
			const Graphics::ITextureAssetResolver& textureAssetResolver,
			const std::function<void(const std::string&)>& logger)
		{
			if (renderItem.transform == nullptr || renderItem.spriteComponent == nullptr) {
				LogMessage(logger, "Scene::ResolveSceneSprites skipped an item because required references were null.");
				return std::nullopt;
			}

			const auto& spriteAssetRef = renderItem.spriteComponent->spriteAssetRef;
			Core::AssetId materialAssetRef = renderItem.spriteComponent->materialAssetRef;
			std::optional<Assets::SpriteAsset> spriteAsset{};
			if (false == spriteAssetRef.IsEmpty())
			{
				spriteAsset = spriteAssetResolver.ResolveSpriteAsset(spriteAssetRef);
				if (false == spriteAsset.has_value()) {
					std::ostringstream message;
					message << "Scene::ResolveSceneSprites could not resolve SpriteAsset '"
						<< spriteAssetRef.GetValue() << "' for entity " << renderItem.entityId << ".";
					LogMessage(logger, message.str());
					return std::nullopt;
				}

				if (materialAssetRef.IsEmpty())
				{
					materialAssetRef = spriteAsset->materialAssetId;
				}
			}

			ResolvedSpriteMaterialState materialState{};
			if (false == materialAssetRef.IsEmpty())
			{
				if (nullptr == materialAssetResolver)
				{
					if (false == spriteAsset.has_value())
					{
						std::ostringstream message;
						message << "Scene::ResolveSceneSprites skipped entity " << renderItem.entityId
							<< " because materialAssetRef was set but no MaterialAsset resolver was provided.";
						LogMessage(logger, message.str());
						return std::nullopt;
					}

					std::ostringstream message;
					message << "Scene::ResolveSceneSprites ignored MaterialAsset '"
						<< materialAssetRef.GetValue() << "' for entity " << renderItem.entityId
						<< " because no MaterialAsset resolver was provided.";
					LogMessage(logger, message.str());
					materialState = MakeSpriteAssetMaterialState(*spriteAsset, *renderItem.spriteComponent);
				}
				else
				{
					const auto materialAsset = materialAssetResolver->ResolveMaterialAsset(materialAssetRef);
					if (false == materialAsset.has_value())
					{
						std::ostringstream message;
						message << "Scene::ResolveSceneSprites could not resolve MaterialAsset '"
							<< materialAssetRef.GetValue() << "' for entity " << renderItem.entityId << ".";
						LogMessage(logger, message.str());
						return std::nullopt;
					}

					materialState.textureAssetId = materialAsset->textureAssetId;
					materialState.color = materialAsset->color;
					materialState.color[3] *= renderItem.spriteComponent->renderSettings.opacity;
					materialState.outlineEnabled = materialAsset->outlineEnabled;
					materialState.outlineThickness = materialAsset->outlineThickness;
					materialState.outlineColor = materialAsset->outlineColor;
					materialState.resolvedFromMaterialAsset = true;
					materialState.materialAssetRef = materialAssetRef;
				}
			}
			else if (true == spriteAsset.has_value())
			{
				materialState = MakeSpriteAssetMaterialState(*spriteAsset, *renderItem.spriteComponent);
			}
			else
			{
				std::ostringstream message;
				message << "Scene::ResolveSceneSprites skipped entity " << renderItem.entityId
					<< " because both spriteAssetRef and materialAssetRef were empty.";
				LogMessage(logger, message.str());
				return std::nullopt;
			}

			const auto texture = textureAssetResolver.ResolveTexture(materialState.textureAssetId);
			if (!texture) {
				std::ostringstream message;
				message << "Scene::ResolveSceneSprites could not resolve Texture2D '"
					<< materialState.textureAssetId.GetValue() << "' for entity " << renderItem.entityId << ".";
				LogMessage(logger, message.str());
				return std::nullopt;
			}

			Graphics::Sprite sprite{};
			sprite.SetTexture(texture);
			sprite.SetTextureAssetId(materialState.textureAssetId);
			sprite.SetPosition(renderItem.transform->position.x, renderItem.transform->position.y);
			sprite.SetScale(renderItem.transform->scale.x, renderItem.transform->scale.y);
			sprite.SetRotationDegrees(renderItem.transform->rotation.z);
			sprite.SetColor(
				materialState.color[0],
				materialState.color[1],
				materialState.color[2],
				materialState.color[3]);
			sprite.SetOutlineEnabled(materialState.outlineEnabled);
			sprite.SetOutlineThickness(materialState.outlineThickness);
			sprite.SetOutlineColor(
				materialState.outlineColor[0],
				materialState.outlineColor[1],
				materialState.outlineColor[2],
				materialState.outlineColor[3]);

			std::ostringstream message;
			message << "Scene::ResolveSceneSprites resolved entity " << renderItem.entityId;
			if (false == spriteAssetRef.IsEmpty())
			{
				message << " from SpriteAsset '" << spriteAssetRef.GetValue() << "'";
			}
			if (true == materialState.resolvedFromMaterialAsset)
			{
				message << (false == spriteAssetRef.IsEmpty() ? " and" : " from")
					<< " MaterialAsset '" << materialState.materialAssetRef.GetValue() << "'";
			}
			message << " to Texture2D '" << materialState.textureAssetId.GetValue() << "'.";
			LogMessage(logger, message.str());

			return ResolvedSceneSprite{
				renderItem.entityId,
				std::move(sprite)
			};
		}

		/// <summary>
		/// Scene 内の Sprite 描画候補を解決済み Sprite 一覧へ変換する。
		/// </summary>
		/// <param name="scene">解決対象 Scene。</param>
		/// <param name="spriteAssetResolver">SpriteAsset を解決する Resolver。</param>
		/// <param name="materialAssetResolver">MaterialAsset を解決する Resolver。未指定時は旧 SpriteAsset Texture を使用する。</param>
		/// <param name="textureAssetResolver">Texture2D を解決する Resolver。</param>
		/// <param name="logger">解決状況を受け取るロガー。</param>
		/// <returns>描画可能な Sprite 一覧。</returns>
		std::vector<ResolvedSceneSprite> ResolveSceneSpritesCore(
			const Scene& scene,
			const Assets::ISpriteAssetResolver& spriteAssetResolver,
			const Assets::IMaterialAssetResolver* materialAssetResolver,
			const Graphics::ITextureAssetResolver& textureAssetResolver,
			const std::function<void(const std::string&)>& logger)
		{
			std::vector<ResolvedSceneSprite> resolvedSprites;
			const auto renderItems = scene.CollectSpriteRenderItems();
			resolvedSprites.reserve(renderItems.size());

			for (const SceneSpriteRenderItem& renderItem : renderItems) {
				const auto resolvedSprite = ResolveSceneRenderItem(
					renderItem,
					spriteAssetResolver,
					materialAssetResolver,
					textureAssetResolver,
					logger);
				if (false == resolvedSprite.has_value()) {
					continue;
				}

				resolvedSprites.push_back(std::move(*resolvedSprite));
			}

			return resolvedSprites;
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
		const Assets::IMaterialAssetResolver& materialAssetResolver,
		const Graphics::ITextureAssetResolver& textureAssetResolver,
		const std::function<void(const std::string&)>& logger) const
	{
		std::vector<Graphics::Sprite> resolvedSprites;
		const auto sceneSprites = ResolveSceneSprites(spriteAssetResolver, materialAssetResolver, textureAssetResolver, logger);
		resolvedSprites.reserve(sceneSprites.size());

		for (const ResolvedSceneSprite& sceneSprite : sceneSprites) {
			resolvedSprites.push_back(sceneSprite.sprite);
		}

		return resolvedSprites;
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
		const Assets::IMaterialAssetResolver& materialAssetResolver,
		const Graphics::ITextureAssetResolver& textureAssetResolver,
		const std::function<void(const std::string&)>& logger) const
	{
		return ResolveSceneSpritesCore(
			*this,
			spriteAssetResolver,
			&materialAssetResolver,
			textureAssetResolver,
			logger);
	}

	std::vector<ResolvedSceneSprite> Scene::ResolveSceneSprites(
		const Assets::ISpriteAssetResolver& spriteAssetResolver,
		const Graphics::ITextureAssetResolver& textureAssetResolver,
		const std::function<void(const std::string&)>& logger) const
	{
		return ResolveSceneSpritesCore(
			*this,
			spriteAssetResolver,
			nullptr,
			textureAssetResolver,
			logger);
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
