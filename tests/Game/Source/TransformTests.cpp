#include <memory>
#include <cmath>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "AssetId.h"
#include "Assets/SpriteAssetLoader.h"
#include "Assets/SpriteAssetRegistry.h"
#include "ITexture.h"
#include "Scene.h"
#include "SceneSaveFormat.h"
#include "SceneSerializer.h"
#include "Sprite.h"
#include "SpriteComponent.h"
#include "TextureAssetRegistry.h"
#include "Texture2D.h"
#include "Transform.h"

namespace
{
	bool IsEqual(float lhs, float rhs)
	{
		return std::fabs(lhs - rhs) < 0.0001f;
	}

	bool VerifySceneSaveSerialization()
	{
		Xelqoria::Game::Scene saveScene;
		auto& playerEntity = saveScene.CreateEntity();
		playerEntity.GetTransform().SetPosition(12.5f, -8.0f, 3.0f);
		playerEntity.GetTransform().rotation = { 0.0f, 45.0f, 90.0f };
		playerEntity.GetTransform().scale = { 1.0f, 2.0f, 1.0f };
		playerEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
			"sprites/player",
			{
				true,
				0,
				1.0f
			}
		});

		auto& backgroundEntity = saveScene.CreateEntity();
		backgroundEntity.GetTransform().SetPosition(-32.0f, 64.0f, 0.0f);
		backgroundEntity.GetTransform().rotation = { 0.0f, 0.0f, 0.0f };
		backgroundEntity.GetTransform().scale = { 4.0f, 4.0f, 1.0f };

		const std::string sceneSaveSnapshot =
			"magic=xelqoria.scene\n"
			"version=1\n"
			"entity.0.id=1\n"
			"entity.0.transform.position=12.500000,-8.000000,3.000000\n"
			"entity.0.transform.rotation=0.000000,45.000000,90.000000\n"
			"entity.0.transform.scale=1.000000,2.000000,1.000000\n"
			"entity.0.spriteRef=sprites/player\n"
			"entity.1.id=2\n"
			"entity.1.transform.position=-32.000000,64.000000,0.000000\n"
			"entity.1.transform.rotation=0.000000,0.000000,0.000000\n"
			"entity.1.transform.scale=4.000000,4.000000,1.000000\n";

		return Xelqoria::Game::SceneSerializer::SaveToText(saveScene) == sceneSaveSnapshot;
	}

	bool VerifySceneSaveRoundTrip()
	{
		Xelqoria::Game::Scene sourceScene;
		auto& playerEntity = sourceScene.CreateEntity();
		playerEntity.GetTransform().SetPosition(1.0f, 2.0f, 3.0f);
		playerEntity.GetTransform().rotation = { 4.0f, 5.0f, 6.0f };
		playerEntity.GetTransform().scale = { 7.0f, 8.0f, 9.0f };
		playerEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
			"sprites/player",
			{
				true,
				3,
				0.8f
			}
		});

		auto& backgroundEntity = sourceScene.CreateEntity();
		backgroundEntity.GetTransform().SetPosition(-10.0f, 20.0f, -30.0f);
		backgroundEntity.GetTransform().rotation = { 0.0f, 15.0f, 30.0f };
		backgroundEntity.GetTransform().scale = { 2.0f, 2.0f, 1.0f };

		const std::string serializedScene = Xelqoria::Game::SceneSerializer::SaveToText(sourceScene);
		const auto loadResult = Xelqoria::Game::SceneSerializer::LoadFromText(serializedScene);
		if (!loadResult.IsSuccess() || !loadResult.scene.has_value()) {
			return false;
		}

		const auto& loadedScene = *loadResult.scene;
		if (loadedScene.GetEntityCount() != 2) {
			return false;
		}

		const auto loadedPlayerEntity = loadedScene.FindEntity(1);
		const auto loadedBackgroundEntity = loadedScene.FindEntity(2);
		if (!loadedPlayerEntity.has_value() || !loadedBackgroundEntity.has_value()) {
			return false;
		}

		const auto& loadedPlayerTransform = loadedPlayerEntity->get().GetTransform();
		const auto& loadedBackgroundTransform = loadedBackgroundEntity->get().GetTransform();
		if (!IsEqual(loadedPlayerTransform.position.x, 1.0f) ||
			!IsEqual(loadedPlayerTransform.position.y, 2.0f) ||
			!IsEqual(loadedPlayerTransform.position.z, 3.0f) ||
			!IsEqual(loadedPlayerTransform.rotation.x, 4.0f) ||
			!IsEqual(loadedPlayerTransform.rotation.y, 5.0f) ||
			!IsEqual(loadedPlayerTransform.rotation.z, 6.0f) ||
			!IsEqual(loadedPlayerTransform.scale.x, 7.0f) ||
			!IsEqual(loadedPlayerTransform.scale.y, 8.0f) ||
			!IsEqual(loadedPlayerTransform.scale.z, 9.0f)) {
			return false;
		}

		if (!IsEqual(loadedBackgroundTransform.position.x, -10.0f) ||
			!IsEqual(loadedBackgroundTransform.position.y, 20.0f) ||
			!IsEqual(loadedBackgroundTransform.position.z, -30.0f) ||
			!IsEqual(loadedBackgroundTransform.rotation.x, 0.0f) ||
			!IsEqual(loadedBackgroundTransform.rotation.y, 15.0f) ||
			!IsEqual(loadedBackgroundTransform.rotation.z, 30.0f) ||
			!IsEqual(loadedBackgroundTransform.scale.x, 2.0f) ||
			!IsEqual(loadedBackgroundTransform.scale.y, 2.0f) ||
			!IsEqual(loadedBackgroundTransform.scale.z, 1.0f)) {
			return false;
		}

		const auto loadedPlayerSpriteComponent = loadedPlayerEntity->get().GetSpriteComponent();
		if (!loadedPlayerSpriteComponent.has_value() ||
			loadedPlayerSpriteComponent->get().spriteAssetRef != Xelqoria::Core::AssetId("sprites/player")) {
			return false;
		}

		if (loadedBackgroundEntity->get().GetSpriteComponent().has_value()) {
			return false;
		}

		return Xelqoria::Game::SceneSerializer::SaveToText(loadedScene) == serializedScene;
	}

	class FakeTexture final : public Xelqoria::RHI::ITexture
	{
	public:
		FakeTexture(std::uint32_t width, std::uint32_t height)
			: m_width(width)
			, m_height(height)
		{
		}

		std::uint32_t GetWidth() const override
		{
			return m_width;
		}

		std::uint32_t GetHeight() const override
		{
			return m_height;
		}

	private:
		std::uint32_t m_width = 0;
		std::uint32_t m_height = 0;
	};
}

TEST(TransformTests, TransformAndSceneRuntimeApiWorks)
{
		Xelqoria::Game::Transform transform;

		EXPECT_TRUE(IsEqual(transform.position.x, 0.0f));
		EXPECT_TRUE(IsEqual(transform.position.y, 0.0f));
		EXPECT_TRUE(IsEqual(transform.position.z, 0.0f));

		EXPECT_TRUE(IsEqual(transform.rotation.x, 0.0f));
		EXPECT_TRUE(IsEqual(transform.rotation.y, 0.0f));
		EXPECT_TRUE(IsEqual(transform.rotation.z, 0.0f));

		EXPECT_TRUE(IsEqual(transform.scale.x, 1.0f));
		EXPECT_TRUE(IsEqual(transform.scale.y, 1.0f));
		EXPECT_TRUE(IsEqual(transform.scale.z, 1.0f));

		transform.SetPosition(10.0f, 20.0f, 30.0f);
		EXPECT_TRUE(IsEqual(transform.position.x, 10.0f));
		EXPECT_TRUE(IsEqual(transform.position.y, 20.0f));
		EXPECT_TRUE(IsEqual(transform.position.z, 30.0f));

		transform.SetPosition(Xelqoria::Game::Vector3{ -1.0f, 2.5f, 0.5f });
		EXPECT_TRUE(IsEqual(transform.position.x, -1.0f));
		EXPECT_TRUE(IsEqual(transform.position.y, 2.5f));
		EXPECT_TRUE(IsEqual(transform.position.z, 0.5f));

		Xelqoria::Game::Scene scene;
		const Xelqoria::Game::EntityId firstEntityId = scene.CreateEntity().GetId();
		const Xelqoria::Game::EntityId secondEntityId = scene.CreateEntity().GetId();
		auto secondEntity = scene.FindEntity(secondEntityId);
		const std::span<const Xelqoria::Game::Entity> entities = scene.GetEntities();

		EXPECT_EQ(static_cast<std::size_t>(2), scene.GetEntityCount());
		EXPECT_EQ(static_cast<std::size_t>(2), entities.size());

		EXPECT_EQ(static_cast<Xelqoria::Game::EntityId>(1), firstEntityId);
		EXPECT_EQ(static_cast<Xelqoria::Game::EntityId>(2), secondEntityId);

		EXPECT_EQ(static_cast<Xelqoria::Game::EntityId>(1), entities[0].GetId());
		EXPECT_EQ(static_cast<Xelqoria::Game::EntityId>(2), entities[1].GetId());

		ASSERT_TRUE(secondEntity.has_value());

		secondEntity->get().GetTransform().SetPosition(3.0f, 4.0f, 5.0f);
		EXPECT_TRUE(IsEqual(secondEntity->get().GetTransform().position.x, 3.0f));
		EXPECT_TRUE(IsEqual(secondEntity->get().GetTransform().position.y, 4.0f));
		EXPECT_TRUE(IsEqual(secondEntity->get().GetTransform().position.z, 5.0f));

		EXPECT_TRUE(scene.DestroyEntity(firstEntityId));
		EXPECT_FALSE(scene.DestroyEntity(firstEntityId));

		EXPECT_EQ(static_cast<std::size_t>(1), scene.GetEntityCount());

		const std::span<const Xelqoria::Game::Entity> remainingEntities = scene.GetEntities();
		EXPECT_EQ(static_cast<std::size_t>(1), remainingEntities.size());
		EXPECT_EQ(secondEntityId, remainingEntities[0].GetId());

		Xelqoria::Game::Entity& hiddenSpriteEntity = scene.CreateEntity();
		hiddenSpriteEntity.GetTransform().SetPosition(50.0f, 60.0f, 70.0f);
		hiddenSpriteEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
		"ui/hidden",
		{
			false,
			2,
			1.0f
		}
	});

		Xelqoria::Game::Entity& visibleSpriteEntity = scene.CreateEntity();
		visibleSpriteEntity.GetTransform().SetPosition(10.0f, 20.0f, 30.0f);
		visibleSpriteEntity.GetTransform().scale = { 2.0f, 3.0f, 1.0f };
		visibleSpriteEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
		"sprites/player-idle",
		{
			true,
			5,
			0.5f
		}
	});

		const auto renderItems = scene.CollectSpriteRenderItems();
		ASSERT_EQ(static_cast<std::size_t>(1), renderItems.size());

		EXPECT_EQ(visibleSpriteEntity.GetId(), renderItems[0].entityId);
		ASSERT_NE(nullptr, renderItems[0].transform);
		ASSERT_NE(nullptr, renderItems[0].spriteComponent);

		EXPECT_TRUE(IsEqual(renderItems[0].transform->position.x, 10.0f));
		EXPECT_TRUE(IsEqual(renderItems[0].transform->position.y, 20.0f));
		EXPECT_TRUE(IsEqual(renderItems[0].transform->position.z, 30.0f));

		EXPECT_EQ(renderItems[0].spriteComponent->spriteAssetRef, Xelqoria::Core::AssetId("sprites/player-idle"));
		EXPECT_TRUE(renderItems[0].spriteComponent->renderSettings.visible);
		EXPECT_EQ(5, renderItems[0].spriteComponent->renderSettings.sortOrder);
		EXPECT_TRUE(IsEqual(renderItems[0].spriteComponent->renderSettings.opacity, 0.5f));

		auto sprite = std::make_shared<Xelqoria::Graphics::Sprite>();
		scene.AddSprite(sprite);

		const auto sprites = scene.GetSprites();
		ASSERT_EQ(static_cast<std::size_t>(1), sprites.size());
		EXPECT_EQ(sprites[0], sprite);

		Xelqoria::Game::Entity spriteEntity(100);
		EXPECT_FALSE(spriteEntity.HasSpriteComponent());

		const Xelqoria::Game::SpriteComponent spriteComponent{
		"ui/title-logo",
		{
			true,
			10,
			0.75f
		}
		};
		spriteEntity.SetSpriteComponent(spriteComponent);
		EXPECT_TRUE(spriteEntity.HasSpriteComponent());

		auto attachedSpriteComponent = spriteEntity.GetSpriteComponent();
		ASSERT_TRUE(attachedSpriteComponent.has_value());
		EXPECT_EQ(attachedSpriteComponent->get().spriteAssetRef, Xelqoria::Core::AssetId("ui/title-logo"));
		EXPECT_TRUE(attachedSpriteComponent->get().renderSettings.visible);
		EXPECT_TRUE(IsEqual(attachedSpriteComponent->get().renderSettings.opacity, 0.75f));
		EXPECT_EQ(10, attachedSpriteComponent->get().renderSettings.sortOrder);
		EXPECT_EQ(Xelqoria::Game::SpriteAssetReferenceState::Unknown, attachedSpriteComponent->get().spriteAssetState);
		EXPECT_TRUE(attachedSpriteComponent->get().missingSpriteAssetRef.IsEmpty());

		const Xelqoria::Game::SpriteComponent defaultSpriteComponent{};
		EXPECT_TRUE(defaultSpriteComponent.renderSettings.visible);
		EXPECT_EQ(0, defaultSpriteComponent.renderSettings.sortOrder);
		EXPECT_TRUE(IsEqual(defaultSpriteComponent.renderSettings.opacity, 1.0f));
		EXPECT_TRUE(defaultSpriteComponent.spriteAssetRef.IsEmpty());
		EXPECT_EQ(Xelqoria::Game::SpriteAssetReferenceState::Unknown, defaultSpriteComponent.spriteAssetState);
		EXPECT_TRUE(defaultSpriteComponent.missingSpriteAssetRef.IsEmpty());

		auto renderTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
		renderTexture->SetRHITexture(std::make_shared<FakeTexture>(64, 32));

		const auto spriteAssetLoadResult = Xelqoria::Game::Assets::SpriteAssetLoader::LoadFromText(
		"# SpriteAsset\n"
		"textureAssetId = textures/player-idle\n");
		EXPECT_TRUE(spriteAssetLoadResult.IsSuccess());
		ASSERT_TRUE(spriteAssetLoadResult.asset.has_value());

		EXPECT_EQ(spriteAssetLoadResult.asset->textureAssetId, Xelqoria::Core::AssetId("textures/player-idle"));

		Xelqoria::Game::Assets::SpriteAssetRegistry spriteAssetRegistry;
		spriteAssetRegistry.RegisterSpriteAsset(
		"sprites/player-idle",
		Xelqoria::Game::Assets::SpriteAsset{ "textures/player-idle" });

		Xelqoria::Graphics::TextureAssetRegistry textureAssetRegistry;
		textureAssetRegistry.RegisterTexture("textures/player-idle", renderTexture);

		std::vector<std::string> resolveLogs;
		const auto resolvedSprites = scene.ResolveSprites(
		spriteAssetRegistry,
		textureAssetRegistry,
		[&resolveLogs](const std::string& message)
		{
			resolveLogs.push_back(message);
		});

		ASSERT_EQ(static_cast<std::size_t>(1), resolvedSprites.size());

		EXPECT_EQ(resolvedSprites[0].GetTexture(), renderTexture);
		EXPECT_EQ(resolvedSprites[0].GetTextureAssetId(), Xelqoria::Core::AssetId("textures/player-idle"));

		EXPECT_TRUE(IsEqual(resolvedSprites[0].GetPosition().x, 10.0f));
		EXPECT_TRUE(IsEqual(resolvedSprites[0].GetPosition().y, 20.0f));
		EXPECT_TRUE(IsEqual(resolvedSprites[0].GetScale().x, 2.0f));
		EXPECT_TRUE(IsEqual(resolvedSprites[0].GetScale().y, 3.0f));

		ASSERT_EQ(static_cast<std::size_t>(1), resolveLogs.size());
		EXPECT_NE(resolveLogs[0].find("resolved entity"), std::string::npos);
		EXPECT_NE(resolveLogs[0].find("sprites/player-idle"), std::string::npos);

		const auto missingFieldLoadResult = Xelqoria::Game::Assets::SpriteAssetLoader::LoadFromText(
		"# textureAssetId is missing\n");
		EXPECT_FALSE(missingFieldLoadResult.IsSuccess());
		ASSERT_TRUE(missingFieldLoadResult.error.has_value());

		EXPECT_EQ(Xelqoria::Game::Assets::SpriteAssetLoadErrorCode::MissingRequiredField, missingFieldLoadResult.error->code);
		EXPECT_EQ(missingFieldLoadResult.error->fieldName, "textureAssetId");

		Xelqoria::Game::Scene missingAssetScene;
		auto& missingAssetEntity = missingAssetScene.CreateEntity();
		missingAssetEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
		"sprites/missing",
		{
			true,
			0,
			1.0f
		}
	});

		std::vector<std::string> missingAssetLogs;
		missingAssetScene.ValidateSpriteReferences(
		spriteAssetRegistry,
		[&missingAssetLogs](const std::string& message)
		{
			missingAssetLogs.push_back(message);
		});

		const auto missingAssetComponent = missingAssetEntity.GetSpriteComponent();
		ASSERT_TRUE(missingAssetComponent.has_value());
		EXPECT_EQ(Xelqoria::Game::SpriteAssetReferenceState::Missing, missingAssetComponent->get().spriteAssetState);
		EXPECT_EQ(missingAssetComponent->get().missingSpriteAssetRef, Xelqoria::Core::AssetId("sprites/missing"));

		const auto unresolvedSprites = missingAssetScene.ResolveSprites(
		spriteAssetRegistry,
		textureAssetRegistry,
		[&missingAssetLogs](const std::string& message)
		{
			missingAssetLogs.push_back(message);
		});

		EXPECT_TRUE(unresolvedSprites.empty());

		ASSERT_EQ(static_cast<std::size_t>(2), missingAssetLogs.size());
		EXPECT_NE(missingAssetLogs[0].find("detected missing SpriteAsset"), std::string::npos);
		EXPECT_NE(missingAssetLogs[0].find("sprites/missing"), std::string::npos);
		EXPECT_NE(missingAssetLogs[1].find("could not resolve SpriteAsset"), std::string::npos);
		EXPECT_NE(missingAssetLogs[1].find("sprites/missing"), std::string::npos);

		EXPECT_EQ(Xelqoria::Game::SceneSaveFormatMagic, std::string_view("xelqoria.scene"));
		EXPECT_EQ(Xelqoria::Game::SceneSaveFormatVersion, static_cast<std::uint32_t>(1));
		EXPECT_EQ(Xelqoria::Game::SceneSaveExtensionFieldPrefix, std::string_view("extensions."));

		const std::string sceneSaveFormatDocumentation(Xelqoria::Game::SceneSaveFormatDocumentation);
		EXPECT_NE(sceneSaveFormatDocumentation.find("entity.<index>.transform.position=<x>,<y>,<z>"), std::string::npos);
		EXPECT_NE(sceneSaveFormatDocumentation.find("entity.<index>.spriteRef=<SpriteAssetId>"), std::string::npos);
		EXPECT_NE(sceneSaveFormatDocumentation.find("entity.<index>.extensions.<name>=<reserved>"), std::string::npos);

		Xelqoria::Game::SceneEntitySaveRecord sceneSaveRecord{};
		sceneSaveRecord.entityId = 77;
		sceneSaveRecord.transform.SetPosition(4.0f, 5.0f, 6.0f);
		sceneSaveRecord.transform.rotation = { 0.0f, 0.0f, 45.0f };
		sceneSaveRecord.transform.scale = { 2.0f, 3.0f, 1.0f };
		sceneSaveRecord.spriteRef = Xelqoria::Game::SceneSpriteRefRecord{ "sprites/player" };

		EXPECT_EQ(static_cast<Xelqoria::Game::EntityId>(77), sceneSaveRecord.entityId);
		EXPECT_TRUE(IsEqual(sceneSaveRecord.transform.position.x, 4.0f));
		EXPECT_TRUE(IsEqual(sceneSaveRecord.transform.position.y, 5.0f));
		EXPECT_TRUE(IsEqual(sceneSaveRecord.transform.position.z, 6.0f));
		ASSERT_TRUE(sceneSaveRecord.spriteRef.has_value());
		EXPECT_EQ(sceneSaveRecord.spriteRef->spriteAssetRef, Xelqoria::Core::AssetId("sprites/player"));

		EXPECT_TRUE(VerifySceneSaveSerialization());
		EXPECT_TRUE(VerifySceneSaveRoundTrip());

		spriteEntity.RemoveSpriteComponent();
		EXPECT_FALSE(spriteEntity.HasSpriteComponent());
		EXPECT_FALSE(spriteEntity.GetSpriteComponent().has_value());
}
