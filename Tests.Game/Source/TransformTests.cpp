#include <memory>
#include <cmath>
#include <string>
#include <vector>

#include "AssetId.h"
#include "Assets/ISpriteAssetResolver.h"
#include "Assets/SpriteAssetLoader.h"
#include "ITextureAssetResolver.h"
#include "ITexture.h"
#include "Scene.h"
#include "SceneSaveFormat.h"
#include "SceneSerializer.h"
#include "Sprite.h"
#include "SpriteComponent.h"
#include "SpriteRenderMath.h"
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

/// <summary>
/// Transform と Scene の最小ランタイム API を検証する。
/// </summary>
/// <returns>成功時は 0、失敗時は 1 を返す。</returns>
int main()
{
	Xelqoria::Game::Transform transform;

	if (!IsEqual(transform.position.x, 0.0f) ||
		!IsEqual(transform.position.y, 0.0f) ||
		!IsEqual(transform.position.z, 0.0f)) {
		return 1;
	}

	if (!IsEqual(transform.rotation.x, 0.0f) ||
		!IsEqual(transform.rotation.y, 0.0f) ||
		!IsEqual(transform.rotation.z, 0.0f)) {
		return 1;
	}

	if (!IsEqual(transform.scale.x, 1.0f) ||
		!IsEqual(transform.scale.y, 1.0f) ||
		!IsEqual(transform.scale.z, 1.0f)) {
		return 1;
	}

	transform.SetPosition(10.0f, 20.0f, 30.0f);
	if (!IsEqual(transform.position.x, 10.0f) ||
		!IsEqual(transform.position.y, 20.0f) ||
		!IsEqual(transform.position.z, 30.0f)) {
		return 1;
	}

	transform.SetPosition(Xelqoria::Game::Vector3{ -1.0f, 2.5f, 0.5f });
	if (!IsEqual(transform.position.x, -1.0f) ||
		!IsEqual(transform.position.y, 2.5f) ||
		!IsEqual(transform.position.z, 0.5f)) {
		return 1;
	}

	Xelqoria::Game::Scene scene;
	const Xelqoria::Game::EntityId firstEntityId = scene.CreateEntity().GetId();
	const Xelqoria::Game::EntityId secondEntityId = scene.CreateEntity().GetId();
	auto secondEntity = scene.FindEntity(secondEntityId);
	const std::span<const Xelqoria::Game::Entity> entities = scene.GetEntities();

	if (scene.GetEntityCount() != 2 || entities.size() != 2) {
		return 1;
	}

	if (firstEntityId != 1 || secondEntityId != 2) {
		return 1;
	}

	if (entities[0].GetId() != 1 || entities[1].GetId() != 2) {
		return 1;
	}

	if (!secondEntity.has_value()) {
		return 1;
	}

	secondEntity->get().GetTransform().SetPosition(3.0f, 4.0f, 5.0f);
	if (!IsEqual(secondEntity->get().GetTransform().position.x, 3.0f) ||
		!IsEqual(secondEntity->get().GetTransform().position.y, 4.0f) ||
		!IsEqual(secondEntity->get().GetTransform().position.z, 5.0f)) {
		return 1;
	}

	if (!scene.DestroyEntity(firstEntityId)) {
		return 1;
	}

	if (scene.DestroyEntity(firstEntityId)) {
		return 1;
	}

	if (scene.GetEntityCount() != 1) {
		return 1;
	}

	const std::span<const Xelqoria::Game::Entity> remainingEntities = scene.GetEntities();
	if (remainingEntities.size() != 1 || remainingEntities[0].GetId() != secondEntityId) {
		return 1;
	}

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
		"ui/visible",
		{
			true,
			5,
			0.5f
		}
	});

	const auto renderItems = scene.CollectSpriteRenderItems();
	if (renderItems.size() != 1) {
		return 1;
	}

	if (renderItems[0].entityId != visibleSpriteEntity.GetId() ||
		renderItems[0].transform == nullptr ||
		renderItems[0].spriteComponent == nullptr) {
		return 1;
	}

	if (!IsEqual(renderItems[0].transform->position.x, 10.0f) ||
		!IsEqual(renderItems[0].transform->position.y, 20.0f) ||
		!IsEqual(renderItems[0].transform->position.z, 30.0f)) {
		return 1;
	}

	if (renderItems[0].spriteComponent->spriteAssetRef != "ui/visible" ||
		!renderItems[0].spriteComponent->renderSettings.visible ||
		renderItems[0].spriteComponent->renderSettings.sortOrder != 5 ||
		!IsEqual(renderItems[0].spriteComponent->renderSettings.opacity, 0.5f)) {
		return 1;
	}

	auto sprite = std::make_shared<Xelqoria::Graphics::Sprite>();
	scene.AddSprite(sprite);

	const auto sprites = scene.GetSprites();
	if (sprites.size() != 1 || sprites[0] != sprite) {
		return 1;
	}

	Xelqoria::Game::Entity spriteEntity(100);
	if (spriteEntity.HasSpriteComponent()) {
		return 1;
	}

	const Xelqoria::Game::SpriteComponent spriteComponent{
		"ui/title-logo",
		{
			true,
			10,
			0.75f
		}
	};
	spriteEntity.SetSpriteComponent(spriteComponent);

	if (!spriteEntity.HasSpriteComponent()) {
		return 1;
	}

	auto attachedSpriteComponent = spriteEntity.GetSpriteComponent();
	if (!attachedSpriteComponent.has_value()) {
		return 1;
	}

	if (attachedSpriteComponent->get().spriteAssetRef != "ui/title-logo") {
		return 1;
	}

	if (!attachedSpriteComponent->get().renderSettings.visible ||
		!IsEqual(attachedSpriteComponent->get().renderSettings.opacity, 0.75f) ||
		attachedSpriteComponent->get().renderSettings.sortOrder != 10) {
		return 1;
	}

	const Xelqoria::Game::SpriteComponent defaultSpriteComponent{};
	if (!defaultSpriteComponent.renderSettings.visible ||
		defaultSpriteComponent.renderSettings.sortOrder != 0 ||
		!IsEqual(defaultSpriteComponent.renderSettings.opacity, 1.0f) ||
		!defaultSpriteComponent.spriteAssetRef.IsEmpty()) {
		return 1;
	}

	Xelqoria::Graphics::Sprite spriteAssetReference;
	spriteAssetReference.SetTextureAssetId("textures/player-idle");
	if (spriteAssetReference.GetTextureAssetId() != Xelqoria::Core::AssetId("textures/player-idle")) {
		return 1;
	}

	auto renderTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
	renderTexture->SetRHITexture(std::make_shared<FakeTexture>(64, 32));

	Xelqoria::Graphics::Sprite positionedSprite;
	positionedSprite.SetTexture(renderTexture);
	positionedSprite.SetPosition(160.0f, -90.0f);
	positionedSprite.SetScale(2.0f, 0.5f);

	const auto quadTransform = Xelqoria::Graphics::ComputeSpriteQuadTransform(positionedSprite, 1280, 720);
	if (!IsEqual(quadTransform.scaleX, 0.2f) ||
		!IsEqual(quadTransform.scaleY, 0.044444446f) ||
		!IsEqual(quadTransform.translateX, 0.25f) ||
		!IsEqual(quadTransform.translateY, 0.25f)) {
		return 1;
	}

	const auto spriteAssetLoadResult = Xelqoria::Game::Assets::SpriteAssetLoader::LoadFromText(
		"# SpriteAsset\n"
		"textureAssetId = textures/player-idle\n");
	if (!spriteAssetLoadResult.IsSuccess() || !spriteAssetLoadResult.asset.has_value()) {
		return 1;
	}

	if (spriteAssetLoadResult.asset->textureAssetId != Xelqoria::Core::AssetId("textures/player-idle")) {
		return 1;
	}

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

	if (resolvedSprites.size() != 1) {
		return 1;
	}

	if (resolvedSprites[0].GetTexture() != renderTexture ||
		resolvedSprites[0].GetTextureAssetId() != Xelqoria::Core::AssetId("textures/player-idle")) {
		return 1;
	}

	if (!IsEqual(resolvedSprites[0].GetPosition().x, 10.0f) ||
		!IsEqual(resolvedSprites[0].GetPosition().y, 20.0f) ||
		!IsEqual(resolvedSprites[0].GetScale().x, 2.0f) ||
		!IsEqual(resolvedSprites[0].GetScale().y, 3.0f)) {
		return 1;
	}

	if (resolveLogs.size() != 1 ||
		resolveLogs[0].find("resolved entity") == std::string::npos ||
		resolveLogs[0].find("sprites/player-idle") == std::string::npos) {
		return 1;
	}

	const auto missingFieldLoadResult = Xelqoria::Game::Assets::SpriteAssetLoader::LoadFromText(
		"# textureAssetId is missing\n");
	if (missingFieldLoadResult.IsSuccess() || !missingFieldLoadResult.error.has_value()) {
		return 1;
	}

	if (missingFieldLoadResult.error->code != Xelqoria::Game::Assets::SpriteAssetLoadErrorCode::MissingRequiredField ||
		missingFieldLoadResult.error->fieldName != "textureAssetId") {
		return 1;
	}

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
	const auto unresolvedSprites = missingAssetScene.ResolveSprites(
		spriteAssetRegistry,
		textureAssetRegistry,
		[&missingAssetLogs](const std::string& message)
		{
			missingAssetLogs.push_back(message);
		});

	if (!unresolvedSprites.empty()) {
		return 1;
	}

	if (missingAssetLogs.size() != 1 ||
		missingAssetLogs[0].find("could not resolve SpriteAsset") == std::string::npos ||
		missingAssetLogs[0].find("sprites/missing") == std::string::npos) {
		return 1;
	}

	if (Xelqoria::Game::SceneSaveFormatMagic != std::string_view("xelqoria.scene") ||
		Xelqoria::Game::SceneSaveFormatVersion != 1 ||
		Xelqoria::Game::SceneSaveExtensionFieldPrefix != std::string_view("extensions.")) {
		return 1;
	}

	const std::string sceneSaveFormatDocumentation(Xelqoria::Game::SceneSaveFormatDocumentation);
	if (sceneSaveFormatDocumentation.find("entity.<index>.transform.position=<x>,<y>,<z>") == std::string::npos ||
		sceneSaveFormatDocumentation.find("entity.<index>.spriteRef=<SpriteAssetId>") == std::string::npos ||
		sceneSaveFormatDocumentation.find("entity.<index>.extensions.<name>=<reserved>") == std::string::npos) {
		return 1;
	}

	Xelqoria::Game::SceneEntitySaveRecord sceneSaveRecord{};
	sceneSaveRecord.entityId = 77;
	sceneSaveRecord.transform.SetPosition(4.0f, 5.0f, 6.0f);
	sceneSaveRecord.transform.rotation = { 0.0f, 0.0f, 45.0f };
	sceneSaveRecord.transform.scale = { 2.0f, 3.0f, 1.0f };
	sceneSaveRecord.spriteRef = Xelqoria::Game::SceneSpriteRefRecord{ "sprites/player" };

	if (sceneSaveRecord.entityId != 77 ||
		!IsEqual(sceneSaveRecord.transform.position.x, 4.0f) ||
		!IsEqual(sceneSaveRecord.transform.position.y, 5.0f) ||
		!IsEqual(sceneSaveRecord.transform.position.z, 6.0f) ||
		!sceneSaveRecord.spriteRef.has_value() ||
		sceneSaveRecord.spriteRef->spriteAssetRef != Xelqoria::Core::AssetId("sprites/player")) {
		return 1;
	}

	if (!VerifySceneSaveSerialization()) {
		return 1;
	}

	spriteEntity.RemoveSpriteComponent();
	if (spriteEntity.HasSpriteComponent() || spriteEntity.GetSpriteComponent().has_value()) {
		return 1;
	}

	return 0;
}
