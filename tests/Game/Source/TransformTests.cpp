#include <memory>
#include <cmath>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "AssetId.h"
#include "Assets/SpriteAssetLoader.h"
#include "Assets/SpriteAssetRegistry.h"
#include "Assets/Collider2DAssetLoader.h"
#include "ITexture.h"
#include "Scene.h"
#include "SceneSaveFormat.h"
#include "SceneSerializer.h"
#include "Sprite.h"
#include "SpriteComponent.h"
#include "TextureAssetRegistry.h"
#include "Texture2D.h"
#include "Assets/SpriteMaterialAssetLoader.h"
#include "Assets/SpriteMaterialAssetRegistry.h"
#include "Collider2DComponent.h"
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
		playerEntity.SetName("Player");
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
		backgroundEntity.SetName("Background Layer");
		backgroundEntity.GetTransform().SetPosition(-32.0f, 64.0f, 0.0f);
		backgroundEntity.GetTransform().rotation = { 0.0f, 0.0f, 0.0f };
		backgroundEntity.GetTransform().scale = { 4.0f, 4.0f, 1.0f };

		const std::string sceneSaveSnapshot =
			"magic=xelqoria.scene\n"
			"version=2\n"
			"entity.0.id=1\n"
			"entity.0.name=\"Player\"\n"
			"entity.0.transform.position=12.500000,-8.000000,3.000000\n"
			"entity.0.transform.rotation=0.000000,45.000000,90.000000\n"
			"entity.0.transform.scale=1.000000,2.000000,1.000000\n"
			"entity.0.hasSpriteComponent=true\n"
			"entity.0.spriteRef=sprites/player\n"
			"entity.0.hasCollider2DComponent=false\n"
			"entity.1.id=2\n"
			"entity.1.name=\"Background Layer\"\n"
			"entity.1.transform.position=-32.000000,64.000000,0.000000\n"
			"entity.1.transform.rotation=0.000000,0.000000,0.000000\n"
			"entity.1.transform.scale=4.000000,4.000000,1.000000\n"
			"entity.1.hasSpriteComponent=false\n"
			"entity.1.hasCollider2DComponent=false\n";

		return Xelqoria::Game::SceneSerializer::SaveToText(saveScene) == sceneSaveSnapshot;
	}

	bool VerifySceneSaveRoundTrip()
	{
		Xelqoria::Game::Scene sourceScene;
		auto& playerEntity = sourceScene.CreateEntity();
		playerEntity.SetName("Player \"Alpha\"");
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
		backgroundEntity.SetName("Background\\Layer");
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

		if (loadedPlayerEntity->get().GetName() != "Player \"Alpha\"" ||
			loadedBackgroundEntity->get().GetName() != "Background\\Layer") {
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

		transform.SetPosition(Xelqoria::Math::Vector3{ -1.0f, 2.5f, 0.5f });
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
			0.5f,
			{ 0.25f, 0.5f, 0.75f, 0.8f }
		}
	});
		const Xelqoria::Game::EntityId visibleSpriteEntityId = visibleSpriteEntity.GetId();

		const auto renderItems = scene.CollectSpriteRenderItems();
		ASSERT_EQ(static_cast<std::size_t>(1), renderItems.size());

		EXPECT_EQ(visibleSpriteEntityId, renderItems[0].entityId);
		ASSERT_NE(nullptr, renderItems[0].transform);
		ASSERT_NE(nullptr, renderItems[0].spriteComponent);

		EXPECT_TRUE(IsEqual(renderItems[0].transform->position.x, 10.0f));
		EXPECT_TRUE(IsEqual(renderItems[0].transform->position.y, 20.0f));
		EXPECT_TRUE(IsEqual(renderItems[0].transform->position.z, 30.0f));

		EXPECT_EQ(renderItems[0].spriteComponent->spriteAssetRef, Xelqoria::Core::AssetId("sprites/player-idle"));
		EXPECT_TRUE(renderItems[0].spriteComponent->renderSettings.visible);
		EXPECT_EQ(5, renderItems[0].spriteComponent->renderSettings.sortOrder);
		EXPECT_TRUE(IsEqual(renderItems[0].spriteComponent->renderSettings.opacity, 0.5f));
		EXPECT_TRUE(IsEqual(renderItems[0].spriteComponent->renderSettings.color[0], 0.25f));
		EXPECT_TRUE(IsEqual(renderItems[0].spriteComponent->renderSettings.color[3], 0.8f));

		Xelqoria::Game::Entity& backgroundSpriteEntity = scene.CreateEntity();
		backgroundSpriteEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
		"sprites/background",
		{
			true,
			-10,
			1.0f
		}
	});

		const auto sortedRenderItems = scene.CollectSpriteRenderItems();
		ASSERT_EQ(static_cast<std::size_t>(2), sortedRenderItems.size());
		EXPECT_EQ(backgroundSpriteEntity.GetId(), sortedRenderItems[0].entityId);
		EXPECT_EQ(visibleSpriteEntityId, sortedRenderItems[1].entityId);

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
		"textureAssetId = textures/player-idle\n"
		"scriptAssetId = scripts/Scripts/Player.script\n"
		"materialAssetId = materials/Player.material\n"
		"collider2DAssetId = colliders2d/Player.collider2d\n");
		EXPECT_TRUE(spriteAssetLoadResult.IsSuccess());
		ASSERT_TRUE(spriteAssetLoadResult.asset.has_value());

		EXPECT_EQ(spriteAssetLoadResult.asset->textureAssetId, Xelqoria::Core::AssetId("textures/player-idle"));
		EXPECT_EQ(spriteAssetLoadResult.asset->scriptAssetId, Xelqoria::Core::AssetId("scripts/Scripts/Player.script"));
		EXPECT_EQ(spriteAssetLoadResult.asset->materialAssetId, Xelqoria::Core::AssetId("materials/Player.material"));
		EXPECT_EQ(spriteAssetLoadResult.asset->collider2DAssetId, Xelqoria::Core::AssetId("colliders2d/Player.collider2d"));

		const auto editorSpriteAssetLoadResult = Xelqoria::Game::Assets::SpriteAssetLoader::LoadFromText(
		"magic=XelqoriaSpriteAsset\n"
		"version=1\n"
		"name=\"Sprite 1\"\n"
		"transform.position=0.000000,0.000000,0.000000\n"
		"transform.rotation=0.000000,0.000000,0.000000\n"
		"transform.scale=1.000000,1.000000,1.000000\n"
		"hasSpriteComponent=true\n"
		"spriteAssetRef=sprites/player-idle\n"
		"textureAssetId=textures/player-idle\n"
		"scriptAssetId=scripts/Scripts/Player.script\n"
		"materialAssetId=materials/Player.material\n"
		"collider2DAssetId=colliders2d/Player.collider2d\n"
		"texture.size=64,32\n"
		"render.visible=true\n"
		"render.sortOrder=0\n"
		"render.opacity=1.000000\n"
		"render.color=1.000000,1.000000,1.000000,1.000000\n");
		EXPECT_TRUE(editorSpriteAssetLoadResult.IsSuccess());
		ASSERT_TRUE(editorSpriteAssetLoadResult.asset.has_value());
		EXPECT_EQ(editorSpriteAssetLoadResult.asset->textureAssetId, Xelqoria::Core::AssetId("textures/player-idle"));
		EXPECT_EQ(editorSpriteAssetLoadResult.asset->scriptAssetId, Xelqoria::Core::AssetId("scripts/Scripts/Player.script"));
		EXPECT_EQ(editorSpriteAssetLoadResult.asset->materialAssetId, Xelqoria::Core::AssetId("materials/Player.material"));
		EXPECT_EQ(editorSpriteAssetLoadResult.asset->collider2DAssetId, Xelqoria::Core::AssetId("colliders2d/Player.collider2d"));

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
		EXPECT_TRUE(IsEqual(resolvedSprites[0].GetColor()[0], 0.25f));
		EXPECT_TRUE(IsEqual(resolvedSprites[0].GetColor()[1], 0.5f));
		EXPECT_TRUE(IsEqual(resolvedSprites[0].GetColor()[2], 0.75f));
		EXPECT_TRUE(IsEqual(resolvedSprites[0].GetColor()[3], 0.4f));

		ASSERT_EQ(static_cast<std::size_t>(2), resolveLogs.size());
		EXPECT_NE(resolveLogs[1].find("resolved entity"), std::string::npos);
		EXPECT_NE(resolveLogs[1].find("sprites/player-idle"), std::string::npos);

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
		EXPECT_EQ(Xelqoria::Game::SceneSaveFormatVersion, static_cast<std::uint32_t>(2));
		EXPECT_EQ(Xelqoria::Game::SceneSaveExtensionFieldPrefix, std::string_view("extensions."));

		const std::string sceneSaveFormatDocumentation(Xelqoria::Game::SceneSaveFormatDocumentation);
		EXPECT_NE(sceneSaveFormatDocumentation.find("entity.<index>.transform.position=<x>,<y>,<z>"), std::string::npos);
		EXPECT_NE(sceneSaveFormatDocumentation.find("entity.<index>.name=\"<EntityName>\""), std::string::npos);
		EXPECT_NE(sceneSaveFormatDocumentation.find("entity.<index>.hasSpriteComponent=<true|false>"), std::string::npos);
		EXPECT_NE(sceneSaveFormatDocumentation.find("entity.<index>.spriteRef=<SpriteAssetId>"), std::string::npos);
		EXPECT_NE(sceneSaveFormatDocumentation.find("entity.<index>.hasCollider2DComponent=<true|false>"), std::string::npos);
		EXPECT_NE(sceneSaveFormatDocumentation.find("entity.<index>.collider2D.shapeType=Box"), std::string::npos);
		EXPECT_NE(sceneSaveFormatDocumentation.find("entity.<index>.extensions.<name>=<reserved>"), std::string::npos);

		Xelqoria::Game::SceneEntitySaveRecord sceneSaveRecord{};
		sceneSaveRecord.entityId = 77;
		sceneSaveRecord.name = "Scene Save Record";
		sceneSaveRecord.hasSpriteComponent = true;
		sceneSaveRecord.hasCollider2DComponent = true;
		sceneSaveRecord.transform.SetPosition(4.0f, 5.0f, 6.0f);
		sceneSaveRecord.transform.rotation = { 0.0f, 0.0f, 45.0f };
		sceneSaveRecord.transform.scale = { 2.0f, 3.0f, 1.0f };
		sceneSaveRecord.spriteRef = Xelqoria::Game::SceneSpriteRefRecord{ "sprites/player" };
		sceneSaveRecord.collider2D = Xelqoria::Game::SceneCollider2DRecord{
			false,
			true,
			Xelqoria::Game::Collider2DShapeType::Box,
			{ 2.0f, -3.0f },
			{ 4.0f, 5.0f }
		};

	EXPECT_EQ(static_cast<Xelqoria::Game::EntityId>(77), sceneSaveRecord.entityId);
	EXPECT_EQ("Scene Save Record", sceneSaveRecord.name);
	EXPECT_FALSE(sceneSaveRecord.hasName);
	EXPECT_TRUE(sceneSaveRecord.hasSpriteComponent);
	EXPECT_TRUE(sceneSaveRecord.hasCollider2DComponent);
		EXPECT_TRUE(IsEqual(sceneSaveRecord.transform.position.x, 4.0f));
		EXPECT_TRUE(IsEqual(sceneSaveRecord.transform.position.y, 5.0f));
		EXPECT_TRUE(IsEqual(sceneSaveRecord.transform.position.z, 6.0f));
		ASSERT_TRUE(sceneSaveRecord.spriteRef.has_value());
		EXPECT_EQ(sceneSaveRecord.spriteRef->spriteAssetRef, Xelqoria::Core::AssetId("sprites/player"));
		ASSERT_TRUE(sceneSaveRecord.collider2D.has_value());
		EXPECT_FALSE(sceneSaveRecord.collider2D->enabled);
		EXPECT_TRUE(sceneSaveRecord.collider2D->isTrigger);
		EXPECT_EQ(sceneSaveRecord.collider2D->shapeType, Xelqoria::Game::Collider2DShapeType::Box);
		EXPECT_FLOAT_EQ(2.0f, sceneSaveRecord.collider2D->offset.x);
		EXPECT_FLOAT_EQ(-3.0f, sceneSaveRecord.collider2D->offset.y);
		EXPECT_FLOAT_EQ(4.0f, sceneSaveRecord.collider2D->size.x);
		EXPECT_FLOAT_EQ(5.0f, sceneSaveRecord.collider2D->size.y);

		EXPECT_TRUE(VerifySceneSaveSerialization());
		EXPECT_TRUE(VerifySceneSaveRoundTrip());

		spriteEntity.RemoveSpriteComponent();
		EXPECT_FALSE(spriteEntity.HasSpriteComponent());
		EXPECT_FALSE(spriteEntity.GetSpriteComponent().has_value());
}

TEST(TransformTests, SceneSerializerRoundTripPreservesMultipleSpritePlacementsAndMissingRefs)
{
	Xelqoria::Game::Scene sourceScene;

	auto& playerEntity = sourceScene.CreateEntity();
	playerEntity.GetTransform().SetPosition(24.0f, -12.0f, 0.0f);
	playerEntity.GetTransform().scale = { 1.5f, 1.5f, 1.0f };
	playerEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
		"sprites/player",
		{
			true,
			1,
			1.0f
		}
	});

	auto& backgroundEntity = sourceScene.CreateEntity();
	backgroundEntity.GetTransform().SetPosition(-320.0f, 180.0f, 0.0f);
	backgroundEntity.GetTransform().scale = { 4.0f, 4.0f, 1.0f };
	backgroundEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
		"sprites/background",
		{
			true,
			-5,
			0.9f
		}
	});

	auto& missingEntity = sourceScene.CreateEntity();
	missingEntity.GetTransform().SetPosition(512.0f, 96.0f, 0.0f);
	missingEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
		"sprites/missing",
		{
			true,
			3,
			0.75f
		}
	});

	const std::string serializedScene = Xelqoria::Game::SceneSerializer::SaveToText(sourceScene);
	const auto loadResult = Xelqoria::Game::SceneSerializer::LoadFromText(serializedScene);
	ASSERT_TRUE(loadResult.IsSuccess());
	ASSERT_TRUE(loadResult.scene.has_value());

	Xelqoria::Game::Scene loadedScene = *loadResult.scene;
	EXPECT_EQ(static_cast<std::size_t>(3), loadedScene.GetEntityCount());
	EXPECT_EQ(serializedScene, Xelqoria::Game::SceneSerializer::SaveToText(loadedScene));

	auto playerTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
	playerTexture->SetRHITexture(std::make_shared<FakeTexture>(64, 64));
	auto backgroundTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
	backgroundTexture->SetRHITexture(std::make_shared<FakeTexture>(512, 256));

	Xelqoria::Game::Assets::SpriteAssetRegistry spriteAssetRegistry;
	spriteAssetRegistry.RegisterSpriteAsset(
		"sprites/player",
		Xelqoria::Game::Assets::SpriteAsset{ "textures/player" });
	spriteAssetRegistry.RegisterSpriteAsset(
		"sprites/background",
		Xelqoria::Game::Assets::SpriteAsset{ "textures/background" });

	Xelqoria::Graphics::TextureAssetRegistry textureAssetRegistry;
	textureAssetRegistry.RegisterTexture("textures/player", playerTexture);
	textureAssetRegistry.RegisterTexture("textures/background", backgroundTexture);

	loadedScene.ValidateSpriteReferences(spriteAssetRegistry);

	const auto loadedMissingEntity = loadedScene.FindEntity(missingEntity.GetId());
	ASSERT_TRUE(loadedMissingEntity.has_value());
	const auto loadedMissingSpriteComponent = loadedMissingEntity->get().GetSpriteComponent();
	ASSERT_TRUE(loadedMissingSpriteComponent.has_value());
	EXPECT_EQ(Xelqoria::Game::SpriteAssetReferenceState::Missing, loadedMissingSpriteComponent->get().spriteAssetState);
	EXPECT_EQ(Xelqoria::Core::AssetId("sprites/missing"), loadedMissingSpriteComponent->get().missingSpriteAssetRef);

	const auto resolvedSprites = loadedScene.ResolveSprites(spriteAssetRegistry, textureAssetRegistry);
	ASSERT_EQ(static_cast<std::size_t>(2), resolvedSprites.size());

	EXPECT_EQ(Xelqoria::Core::AssetId("textures/player"), resolvedSprites[0].GetTextureAssetId());
	EXPECT_EQ(Xelqoria::Core::AssetId("textures/background"), resolvedSprites[1].GetTextureAssetId());
	EXPECT_TRUE(IsEqual(24.0f, resolvedSprites[0].GetPosition().x));
	EXPECT_TRUE(IsEqual(-12.0f, resolvedSprites[0].GetPosition().y));
	EXPECT_TRUE(IsEqual(-320.0f, resolvedSprites[1].GetPosition().x));
	EXPECT_TRUE(IsEqual(180.0f, resolvedSprites[1].GetPosition().y));
}

TEST(TransformTests, SceneSerializerRoundTripPreservesMaterialRef)
{
	Xelqoria::Game::Scene scene;
	auto& entity = scene.CreateEntity();
	Xelqoria::Game::SpriteComponent spriteComponent{
		Xelqoria::Core::AssetId("sprites/player"),
		{}
	};
	spriteComponent.materialAssetRef = Xelqoria::Core::AssetId("materials/player.material");
	entity.SetSpriteComponent(spriteComponent);

	const std::string saved = Xelqoria::Game::SceneSerializer::SaveToText(scene);
	const auto loaded = Xelqoria::Game::SceneSerializer::LoadFromText(saved);

	ASSERT_TRUE(loaded.IsSuccess());
	const auto loadedEntity = loaded.scene->FindEntity(entity.GetId());
	ASSERT_TRUE(loadedEntity.has_value());
	const auto loadedSpriteComponent = loadedEntity->get().GetSpriteComponent();
	ASSERT_TRUE(loadedSpriteComponent.has_value());
	EXPECT_EQ(
		Xelqoria::Core::AssetId("materials/player.material"),
		loadedSpriteComponent->get().materialAssetRef);
}

TEST(TransformTests, SceneSerializerRoundTripPreservesCollider2DComponent)
{
	Xelqoria::Game::Scene scene;
	auto& entity = scene.CreateEntity();
	entity.SetCollider2DComponent(Xelqoria::Game::Collider2DComponent{
		false,
		true,
		Xelqoria::Game::Collider2DShapeType::Box,
		{ 2.5f, -3.5f },
		{ 4.0f, 6.0f }
	});

	const std::string saved = Xelqoria::Game::SceneSerializer::SaveToText(scene);
	EXPECT_NE(std::string::npos, saved.find("version=2\n"));
	EXPECT_NE(std::string::npos, saved.find("entity.0.hasCollider2DComponent=true\n"));
	EXPECT_NE(std::string::npos, saved.find("entity.0.collider2D.enabled=false\n"));
	EXPECT_NE(std::string::npos, saved.find("entity.0.collider2D.isTrigger=true\n"));
	EXPECT_NE(std::string::npos, saved.find("entity.0.collider2D.shapeType=Box\n"));
	EXPECT_NE(std::string::npos, saved.find("entity.0.collider2D.offset=2.500000,-3.500000\n"));
	EXPECT_NE(std::string::npos, saved.find("entity.0.collider2D.size=4.000000,6.000000\n"));

	const auto loaded = Xelqoria::Game::SceneSerializer::LoadFromText(saved);

	ASSERT_TRUE(loaded.IsSuccess());
	const auto loadedEntity = loaded.scene->FindEntity(entity.GetId());
	ASSERT_TRUE(loadedEntity.has_value());
	const auto loadedCollider = loadedEntity->get().GetCollider2DComponent();
	ASSERT_TRUE(loadedCollider.has_value());
	EXPECT_FALSE(loadedCollider->get().enabled);
	EXPECT_TRUE(loadedCollider->get().isTrigger);
	EXPECT_EQ(Xelqoria::Game::Collider2DShapeType::Box, loadedCollider->get().shapeType);
	EXPECT_FLOAT_EQ(2.5f, loadedCollider->get().offset.x);
	EXPECT_FLOAT_EQ(-3.5f, loadedCollider->get().offset.y);
	EXPECT_FLOAT_EQ(4.0f, loadedCollider->get().size.x);
	EXPECT_FLOAT_EQ(6.0f, loadedCollider->get().size.y);
}

TEST(TransformTests, SceneSerializerLoadVersion1KeepsCollider2DAbsent)
{
	const std::string source =
		"magic=xelqoria.scene\n"
		"version=1\n"
		"entity.0.id=5\n"
		"entity.0.name=\"Legacy\"\n"
		"entity.0.transform.position=1.000000,2.000000,3.000000\n"
		"entity.0.transform.rotation=0.000000,0.000000,0.000000\n"
		"entity.0.transform.scale=1.000000,1.000000,1.000000\n"
		"entity.0.hasSpriteComponent=false\n";

	const auto loadResult = Xelqoria::Game::SceneSerializer::LoadFromText(source);

	ASSERT_TRUE(loadResult.IsSuccess());
	const auto loadedEntity = loadResult.scene->FindEntity(5);
	ASSERT_TRUE(loadedEntity.has_value());
	EXPECT_FALSE(loadedEntity->get().HasCollider2DComponent());
}

TEST(TransformTests, SceneSerializerLoadRejectsInvalidCollider2DFields)
{
	const auto loadInvalidColliderScene = [](std::string_view fieldLine)
	{
		const std::string source =
			"magic=xelqoria.scene\n"
			"version=2\n"
			"entity.0.id=5\n"
			"entity.0.name=\"Collider\"\n"
			"entity.0.transform.position=1.000000,2.000000,3.000000\n"
			"entity.0.transform.rotation=0.000000,0.000000,0.000000\n"
			"entity.0.transform.scale=1.000000,1.000000,1.000000\n"
			"entity.0.hasSpriteComponent=false\n"
			"entity.0.hasCollider2DComponent=true\n"
			+ std::string(fieldLine);
		return Xelqoria::Game::SceneSerializer::LoadFromText(source);
	};

	const auto invalidBool = loadInvalidColliderScene("entity.0.collider2D.enabled=yes\n");
	EXPECT_FALSE(invalidBool.IsSuccess());
	ASSERT_TRUE(invalidBool.error.has_value());
	EXPECT_EQ("entity.0.collider2D.enabled", invalidBool.error->fieldName);

	const auto invalidShapeType = loadInvalidColliderScene("entity.0.collider2D.shapeType=Circle\n");
	EXPECT_FALSE(invalidShapeType.IsSuccess());
	ASSERT_TRUE(invalidShapeType.error.has_value());
	EXPECT_EQ("entity.0.collider2D.shapeType", invalidShapeType.error->fieldName);

	const auto invalidOffset = loadInvalidColliderScene("entity.0.collider2D.offset=1,2,3\n");
	EXPECT_FALSE(invalidOffset.IsSuccess());
	ASSERT_TRUE(invalidOffset.error.has_value());
	EXPECT_EQ("entity.0.collider2D.offset", invalidOffset.error->fieldName);

	const auto invalidSizeX = loadInvalidColliderScene("entity.0.collider2D.size=0,1\n");
	EXPECT_FALSE(invalidSizeX.IsSuccess());
	ASSERT_TRUE(invalidSizeX.error.has_value());
	EXPECT_EQ("entity.0.collider2D.size", invalidSizeX.error->fieldName);

	const auto invalidSizeY = loadInvalidColliderScene("entity.0.collider2D.size=1,-2\n");
	EXPECT_FALSE(invalidSizeY.IsSuccess());
	ASSERT_TRUE(invalidSizeY.error.has_value());
	EXPECT_EQ("entity.0.collider2D.size", invalidSizeY.error->fieldName);
}

TEST(TransformTests, SpriteMaterialAssetLoaderReadsTextureColorAndOutline)
{
	const auto loadResult = Xelqoria::Game::Assets::SpriteMaterialAssetLoader::LoadFromText(
		"magic=XelqoriaSpriteMaterialAsset\n"
		"version=1\n"
		"textureAssetId=textures/player.png\n"
		"color=0.2,0.4,0.6,0.8\n"
		"outline.enabled=true\n"
		"outline.thickness=3.5\n"
		"outline.color=1.0,0.5,0.25,1.0\n");

	ASSERT_TRUE(loadResult.IsSuccess());
	ASSERT_TRUE(loadResult.asset.has_value());
	EXPECT_EQ(Xelqoria::Core::AssetId("textures/player.png"), loadResult.asset->textureAssetId);
	EXPECT_TRUE(IsEqual(0.2f, loadResult.asset->color[0]));
	EXPECT_TRUE(IsEqual(0.8f, loadResult.asset->color[3]));
	EXPECT_TRUE(loadResult.asset->outlineEnabled);
	EXPECT_TRUE(IsEqual(3.5f, loadResult.asset->outlineThickness));
	EXPECT_TRUE(IsEqual(0.25f, loadResult.asset->outlineColor[2]));
}

TEST(TransformTests, Collider2DAssetLoaderReadsBoxCollider)
{
	const auto loadResult = Xelqoria::Game::Assets::Collider2DAssetLoader::LoadFromText(
		"magic=XelqoriaCollider2DAsset\n"
		"version=1\n"
		"enabled=true\n"
		"isTrigger=true\n"
		"shapeType=Box\n"
		"offset=1.5,-2.0\n"
		"size=3.0,4.0\n");

	ASSERT_TRUE(loadResult.IsSuccess());
	ASSERT_TRUE(loadResult.asset.has_value());
	const Xelqoria::Game::Collider2DComponent& collider = loadResult.asset->collider;
	EXPECT_TRUE(collider.enabled);
	EXPECT_TRUE(collider.isTrigger);
	EXPECT_EQ(Xelqoria::Game::Collider2DShapeType::Box, collider.shapeType);
	EXPECT_FLOAT_EQ(1.5f, collider.offset.x);
	EXPECT_FLOAT_EQ(-2.0f, collider.offset.y);
	EXPECT_FLOAT_EQ(3.0f, collider.size.x);
	EXPECT_FLOAT_EQ(4.0f, collider.size.y);
}

TEST(TransformTests, Collider2DAssetLoaderRejectsNonPositiveSize)
{
	const auto loadResult = Xelqoria::Game::Assets::Collider2DAssetLoader::LoadFromText(
		"magic=XelqoriaCollider2DAsset\n"
		"version=1\n"
		"enabled=true\n"
		"isTrigger=false\n"
		"shapeType=Box\n"
		"offset=0.0,0.0\n"
		"size=0.0,1.0\n");

	EXPECT_FALSE(loadResult.IsSuccess());
	ASSERT_TRUE(loadResult.error.has_value());
	EXPECT_EQ(Xelqoria::Game::Assets::Collider2DAssetLoadErrorCode::InvalidRecord, loadResult.error->code);
	EXPECT_EQ("size", loadResult.error->fieldName);
}

TEST(TransformTests, SceneSerializerLoadAcceptsExtensionFieldsAndEmptyNames)
{
	const std::string source =
		"magic=xelqoria.scene\n"
		"version=1\n"
		"entity.0.id=5\n"
		"entity.0.name=\"\"\n"
		"entity.0.transform.position=1.000000,2.000000,3.000000\n"
		"entity.0.transform.rotation=0.000000,0.000000,0.000000\n"
		"entity.0.transform.scale=1.000000,1.000000,1.000000\n"
		"entity.0.hasSpriteComponent=true\n"
		"entity.0.spriteRef=sprites/player\n"
		"entity.0.extensions.comment=ignored\n";

	const auto loadResult = Xelqoria::Game::SceneSerializer::LoadFromText(source);
	ASSERT_TRUE(loadResult.IsSuccess());
	ASSERT_TRUE(loadResult.scene.has_value());

	const auto loadedEntity = loadResult.scene->FindEntity(5);
	ASSERT_TRUE(loadedEntity.has_value());
	EXPECT_TRUE(loadedEntity->get().GetName().empty());

	const auto spriteComponent = loadedEntity->get().GetSpriteComponent();
	ASSERT_TRUE(spriteComponent.has_value());
	EXPECT_EQ(Xelqoria::Core::AssetId("sprites/player"), spriteComponent->get().spriteAssetRef);
}

TEST(TransformTests, SceneSerializerLoadReturnsErrorWhenEntityIdIsMissing)
{
	const std::string source =
		"magic=xelqoria.scene\n"
		"version=1\n"
		"entity.0.name=\"Missing Id\"\n"
		"entity.0.transform.position=0.000000,0.000000,0.000000\n"
		"entity.0.transform.rotation=0.000000,0.000000,0.000000\n"
		"entity.0.transform.scale=1.000000,1.000000,1.000000\n"
		"entity.0.hasSpriteComponent=false\n";

	const auto loadResult = Xelqoria::Game::SceneSerializer::LoadFromText(source);
	EXPECT_FALSE(loadResult.IsSuccess());
	ASSERT_TRUE(loadResult.error.has_value());
	EXPECT_EQ("entity.0.id", loadResult.error->fieldName);
	EXPECT_NE(loadResult.error->message.find("不足"), std::string::npos);
}

TEST(TransformTests, SceneSerializerLoadReturnsErrorWhenEntityIdIsDuplicated)
{
	const std::string source =
		"magic=xelqoria.scene\n"
		"version=1\n"
		"entity.0.id=7\n"
		"entity.0.name=\"First\"\n"
		"entity.0.transform.position=0.000000,0.000000,0.000000\n"
		"entity.0.transform.rotation=0.000000,0.000000,0.000000\n"
		"entity.0.transform.scale=1.000000,1.000000,1.000000\n"
		"entity.0.hasSpriteComponent=false\n"
		"entity.1.id=7\n"
		"entity.1.name=\"Second\"\n"
		"entity.1.transform.position=0.000000,0.000000,0.000000\n"
		"entity.1.transform.rotation=0.000000,0.000000,0.000000\n"
		"entity.1.transform.scale=1.000000,1.000000,1.000000\n"
		"entity.1.hasSpriteComponent=false\n";

	const auto loadResult = Xelqoria::Game::SceneSerializer::LoadFromText(source);
	EXPECT_FALSE(loadResult.IsSuccess());
	ASSERT_TRUE(loadResult.error.has_value());
	EXPECT_EQ("entity.1.id", loadResult.error->fieldName);
	EXPECT_NE(loadResult.error->message.find("重複"), std::string::npos);
}

TEST(TransformTests, ResolveSceneSpritesPreservesEntityIdsAndResolvedTextures)
{
	Xelqoria::Game::Scene scene;

	auto& foregroundEntity = scene.CreateEntity();
	const Xelqoria::Game::EntityId foregroundEntityId = foregroundEntity.GetId();
	foregroundEntity.GetTransform().SetPosition(24.0f, -12.0f, 0.0f);
	foregroundEntity.GetTransform().scale = { 1.5f, 1.5f, 1.0f };
	foregroundEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
		"sprites/player",
		{
			true,
			1,
			1.0f
		}
	});

	auto& backgroundEntity = scene.CreateEntity();
	const Xelqoria::Game::EntityId backgroundEntityId = backgroundEntity.GetId();
	backgroundEntity.GetTransform().SetPosition(-320.0f, 180.0f, 0.0f);
	backgroundEntity.GetTransform().scale = { 4.0f, 4.0f, 1.0f };
	backgroundEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
		"sprites/background",
		{
			true,
			-5,
			0.9f
		}
	});

	auto playerTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
	playerTexture->SetRHITexture(std::make_shared<FakeTexture>(64, 64));
	auto backgroundTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
	backgroundTexture->SetRHITexture(std::make_shared<FakeTexture>(512, 256));

	Xelqoria::Game::Assets::SpriteAssetRegistry spriteAssetRegistry;
	spriteAssetRegistry.RegisterSpriteAsset(
		"sprites/player",
		Xelqoria::Game::Assets::SpriteAsset{ "textures/player" });
	spriteAssetRegistry.RegisterSpriteAsset(
		"sprites/background",
		Xelqoria::Game::Assets::SpriteAsset{ "textures/background" });

	Xelqoria::Graphics::TextureAssetRegistry textureAssetRegistry;
	textureAssetRegistry.RegisterTexture("textures/player", playerTexture);
	textureAssetRegistry.RegisterTexture("textures/background", backgroundTexture);

	const auto resolvedSceneSprites = scene.ResolveSceneSprites(spriteAssetRegistry, textureAssetRegistry);
	ASSERT_EQ(static_cast<std::size_t>(2), resolvedSceneSprites.size());

	EXPECT_EQ(backgroundEntityId, resolvedSceneSprites[0].entityId);
	EXPECT_EQ(Xelqoria::Core::AssetId("textures/background"), resolvedSceneSprites[0].sprite.GetTextureAssetId());
	EXPECT_EQ(backgroundTexture, resolvedSceneSprites[0].sprite.GetTexture());
	EXPECT_TRUE(IsEqual(-320.0f, resolvedSceneSprites[0].sprite.GetPosition().x));
	EXPECT_TRUE(IsEqual(180.0f, resolvedSceneSprites[0].sprite.GetPosition().y));

	EXPECT_EQ(foregroundEntityId, resolvedSceneSprites[1].entityId);
	EXPECT_EQ(Xelqoria::Core::AssetId("textures/player"), resolvedSceneSprites[1].sprite.GetTextureAssetId());
	EXPECT_EQ(playerTexture, resolvedSceneSprites[1].sprite.GetTexture());
	EXPECT_TRUE(IsEqual(24.0f, resolvedSceneSprites[1].sprite.GetPosition().x));
	EXPECT_TRUE(IsEqual(-12.0f, resolvedSceneSprites[1].sprite.GetPosition().y));
}

TEST(TransformTests, ResolveSceneSpritesUsesAssignedMaterialAsset)
{
	Xelqoria::Game::Scene scene;

	auto& entity = scene.CreateEntity();
	entity.GetTransform().SetPosition(8.0f, -16.0f, 0.0f);
	auto spriteComponent = Xelqoria::Game::SpriteComponent{
		"sprites/player",
		{
			true,
			0,
			1.0f
		}
	};
	spriteComponent.materialAssetRef = Xelqoria::Core::AssetId("materials/player.material");
	entity.SetSpriteComponent(spriteComponent);

	auto fallbackTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
	fallbackTexture->SetRHITexture(std::make_shared<FakeTexture>(16, 16));
	auto materialTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
	materialTexture->SetRHITexture(std::make_shared<FakeTexture>(64, 32));

	Xelqoria::Game::Assets::SpriteAssetRegistry spriteAssetRegistry;
	spriteAssetRegistry.RegisterSpriteAsset(
		"sprites/player",
		Xelqoria::Game::Assets::SpriteAsset{ "textures/fallback" });

	Xelqoria::Game::Assets::SpriteMaterialAssetRegistry materialAssetRegistry;
	materialAssetRegistry.RegisterMaterialAsset(
		"materials/player.material",
		Xelqoria::Game::Assets::SpriteMaterialAsset{
			"textures/player-material",
			{ 0.25f, 0.5f, 0.75f, 0.8f },
			true,
			3.5f,
			{ 1.0f, 0.0f, 0.5f, 0.6f }
		});

	Xelqoria::Graphics::TextureAssetRegistry textureAssetRegistry;
	textureAssetRegistry.RegisterTexture("textures/fallback", fallbackTexture);
	textureAssetRegistry.RegisterTexture("textures/player-material", materialTexture);

	std::vector<std::string> resolveLogs;
	const auto resolvedSceneSprites = scene.ResolveSceneSprites(
		spriteAssetRegistry,
		materialAssetRegistry,
		textureAssetRegistry,
		[&resolveLogs](const std::string& message)
		{
			resolveLogs.push_back(message);
		});

	ASSERT_EQ(static_cast<std::size_t>(1), resolvedSceneSprites.size());
	const auto& resolvedSprite = resolvedSceneSprites[0].sprite;
	EXPECT_EQ(Xelqoria::Core::AssetId("textures/player-material"), resolvedSprite.GetTextureAssetId());
	EXPECT_EQ(materialTexture, resolvedSprite.GetTexture());
	EXPECT_TRUE(IsEqual(0.25f, resolvedSprite.GetColor()[0]));
	EXPECT_TRUE(IsEqual(0.5f, resolvedSprite.GetColor()[1]));
	EXPECT_TRUE(IsEqual(0.75f, resolvedSprite.GetColor()[2]));
	EXPECT_TRUE(IsEqual(0.8f, resolvedSprite.GetColor()[3]));
	EXPECT_TRUE(resolvedSprite.IsOutlineEnabled());
	EXPECT_TRUE(IsEqual(3.5f, resolvedSprite.GetOutlineThickness()));
	EXPECT_TRUE(IsEqual(1.0f, resolvedSprite.GetOutlineColor()[0]));
	EXPECT_TRUE(IsEqual(0.0f, resolvedSprite.GetOutlineColor()[1]));
	EXPECT_TRUE(IsEqual(0.5f, resolvedSprite.GetOutlineColor()[2]));
	EXPECT_TRUE(IsEqual(0.6f, resolvedSprite.GetOutlineColor()[3]));

	ASSERT_EQ(static_cast<std::size_t>(1), resolveLogs.size());
	EXPECT_NE(std::string::npos, resolveLogs[0].find("materials/player.material"));
	EXPECT_NE(std::string::npos, resolveLogs[0].find("textures/player-material"));
}

TEST(TransformTests, ResolveSceneSpritesUsesMaterialAssetWithoutSpriteAssetRef)
{
	Xelqoria::Game::Scene scene;

	auto& entity = scene.CreateEntity();
	const Xelqoria::Game::EntityId entityId = entity.GetId();
	entity.GetTransform().SetPosition(-36.0f, -24.0f, 0.0f);
	Xelqoria::Game::SpriteComponent spriteComponent{};
	spriteComponent.materialAssetRef = Xelqoria::Core::AssetId("materials/NewMaterial1.material");
	entity.SetSpriteComponent(spriteComponent);

	auto materialTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
	materialTexture->SetRHITexture(std::make_shared<FakeTexture>(128, 64));

	Xelqoria::Game::Assets::SpriteAssetRegistry spriteAssetRegistry;
	Xelqoria::Game::Assets::SpriteMaterialAssetRegistry materialAssetRegistry;
	materialAssetRegistry.RegisterMaterialAsset(
		"materials/NewMaterial1.material",
		Xelqoria::Game::Assets::SpriteMaterialAsset{
			"textures/title.png",
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			false,
			1.0f,
			{ 1.0f, 1.0f, 0.0f, 1.0f }
		});

	Xelqoria::Graphics::TextureAssetRegistry textureAssetRegistry;
	textureAssetRegistry.RegisterTexture("textures/title.png", materialTexture);

	const auto resolvedSceneSprites = scene.ResolveSceneSprites(
		spriteAssetRegistry,
		materialAssetRegistry,
		textureAssetRegistry);

	ASSERT_EQ(static_cast<std::size_t>(1), resolvedSceneSprites.size());
	EXPECT_EQ(entityId, resolvedSceneSprites[0].entityId);
	EXPECT_EQ(Xelqoria::Core::AssetId("textures/title.png"), resolvedSceneSprites[0].sprite.GetTextureAssetId());
	EXPECT_EQ(materialTexture, resolvedSceneSprites[0].sprite.GetTexture());
	EXPECT_TRUE(IsEqual(-36.0f, resolvedSceneSprites[0].sprite.GetPosition().x));
	EXPECT_TRUE(IsEqual(-24.0f, resolvedSceneSprites[0].sprite.GetPosition().y));
}
