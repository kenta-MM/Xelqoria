#include <gtest/gtest.h>

#include <string>

#include "Collider2DComponent.h"
#include "Commands/SceneEditingOperations.h"
#include "SceneSerializer.h"

TEST(SceneEditingOperationsTests, DuplicateSelectedEntityCopiesTransformAndSpriteComponent)
{
    Xelqoria::Game::Scene scene;
    auto& sourceEntity = scene.CreateEntity();
    sourceEntity.SetName("Player");
    sourceEntity.GetTransform().SetPosition(12.0f, -8.0f, 1.0f);
    sourceEntity.GetTransform().rotation = { 2.0f, 4.0f, 8.0f };
    sourceEntity.GetTransform().scale = { 3.0f, 5.0f, 1.0f };
    sourceEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
        Xelqoria::Core::AssetId("sprites/player"),
        {
            true,
            7,
            0.5f
        },
        Xelqoria::Game::SpriteAssetReferenceState::Resolved,
        {}
    });

    const auto expectedTransform = sourceEntity.GetTransform();
    const auto expectedSpriteComponent = sourceEntity.GetSpriteComponent()->get();

    const auto result =
        Xelqoria::Editor::SceneEditingOperations::DuplicateSelectedEntity(scene, sourceEntity.GetId());

    ASSERT_TRUE(result.changed);
    ASSERT_TRUE(result.selectedEntityId.has_value());
    ASSERT_NE(sourceEntity.GetId(), *result.selectedEntityId);
    ASSERT_EQ(static_cast<std::size_t>(2), scene.GetEntityCount());

    const auto duplicateEntity = scene.FindEntity(*result.selectedEntityId);
    ASSERT_TRUE(duplicateEntity.has_value());
    EXPECT_EQ("Player Copy", duplicateEntity->get().GetName());

    const auto& duplicateTransform = duplicateEntity->get().GetTransform();
    EXPECT_FLOAT_EQ(expectedTransform.position.x, duplicateTransform.position.x);
    EXPECT_FLOAT_EQ(expectedTransform.position.y, duplicateTransform.position.y);
    EXPECT_FLOAT_EQ(expectedTransform.position.z, duplicateTransform.position.z);
    EXPECT_FLOAT_EQ(expectedTransform.rotation.z, duplicateTransform.rotation.z);
    EXPECT_FLOAT_EQ(expectedTransform.scale.y, duplicateTransform.scale.y);

    const auto duplicateSpriteComponent = duplicateEntity->get().GetSpriteComponent();
    ASSERT_TRUE(duplicateSpriteComponent.has_value());
    EXPECT_EQ(expectedSpriteComponent.spriteAssetRef.GetValue(), duplicateSpriteComponent->get().spriteAssetRef.GetValue());
    EXPECT_EQ(expectedSpriteComponent.renderSettings.sortOrder, duplicateSpriteComponent->get().renderSettings.sortOrder);
    EXPECT_FLOAT_EQ(expectedSpriteComponent.renderSettings.opacity, duplicateSpriteComponent->get().renderSettings.opacity);
}

TEST(SceneEditingOperationsTests, CreateEntityAssignsDefaultNameAndSelectsCreatedEntity)
{
    Xelqoria::Game::Scene scene;

    const auto result = Xelqoria::Editor::SceneEditingOperations::CreateEntity(scene);

    ASSERT_TRUE(result.changed);
    ASSERT_TRUE(result.selectedEntityId.has_value());
    const auto createdEntity = scene.FindEntity(*result.selectedEntityId);
    ASSERT_TRUE(createdEntity.has_value());
    EXPECT_EQ("Entity 1", createdEntity->get().GetName());
}

TEST(SceneEditingOperationsTests, CreateUntexturedSpriteAddsEmptySpriteComponentAtRequestedPosition)
{
    Xelqoria::Game::Scene scene;

    const auto result = Xelqoria::Editor::SceneEditingOperations::CreateUntexturedSprite(scene, 10.0f, -20.0f);

    ASSERT_TRUE(result.changed);
    ASSERT_TRUE(result.selectedEntityId.has_value());

    const auto createdEntity = scene.FindEntity(*result.selectedEntityId);
    ASSERT_TRUE(createdEntity.has_value());
    EXPECT_EQ("Sprite 1", createdEntity->get().GetName());
    EXPECT_FLOAT_EQ(10.0f, createdEntity->get().GetTransform().position.x);
    EXPECT_FLOAT_EQ(-20.0f, createdEntity->get().GetTransform().position.y);

    const auto spriteComponent = createdEntity->get().GetSpriteComponent();
    ASSERT_TRUE(spriteComponent.has_value());
    EXPECT_TRUE(spriteComponent->get().spriteAssetRef.IsEmpty());

    const std::string serializedScene = Xelqoria::Game::SceneSerializer::SaveToText(scene);
    EXPECT_NE(std::string::npos, serializedScene.find("entity.0.spriteRef=\n"));

    const auto loadResult = Xelqoria::Game::SceneSerializer::LoadFromText(
        serializedScene);
    ASSERT_TRUE(loadResult.IsSuccess());
    ASSERT_TRUE(loadResult.scene.has_value());

    const auto loadedEntity = loadResult.scene->FindEntity(*result.selectedEntityId);
    ASSERT_TRUE(loadedEntity.has_value());
    ASSERT_TRUE(loadedEntity->get().HasSpriteComponent());
    EXPECT_TRUE(loadedEntity->get().GetSpriteComponent()->get().spriteAssetRef.IsEmpty());
}

TEST(SceneEditingOperationsTests, MoveEntityUpdatesPositionAndPreservesZ)
{
    Xelqoria::Game::Scene scene;
    auto& entity = scene.CreateEntity();
    entity.SetPosition(12.0f, -8.0f, 3.0f);

    const bool changed =
        Xelqoria::Editor::SceneEditingOperations::MoveEntity(scene, entity.GetId(), 48.0f, 96.0f);

    ASSERT_TRUE(changed);
    const auto movedEntity = scene.FindEntity(entity.GetId());
    ASSERT_TRUE(movedEntity.has_value());
    EXPECT_FLOAT_EQ(48.0f, movedEntity->get().GetTransform().position.x);
    EXPECT_FLOAT_EQ(96.0f, movedEntity->get().GetTransform().position.y);
    EXPECT_FLOAT_EQ(3.0f, movedEntity->get().GetTransform().position.z);
}

TEST(SceneEditingOperationsTests, MoveEntitySkipsWhenPositionIsUnchanged)
{
    Xelqoria::Game::Scene scene;
    auto& entity = scene.CreateEntity();
    entity.SetPosition(24.0f, 36.0f, 1.0f);

    const bool changed =
        Xelqoria::Editor::SceneEditingOperations::MoveEntity(scene, entity.GetId(), 24.0f, 36.0f);

    EXPECT_FALSE(changed);
}

TEST(SceneEditingOperationsTests, AdjustEntityUniformScaleUpdatesXYAndPreservesZ)
{
    Xelqoria::Game::Scene scene;
    auto& entity = scene.CreateEntity();
    entity.SetScale(2.0f, 3.0f, 4.0f);

    const bool changed =
        Xelqoria::Editor::SceneEditingOperations::AdjustEntityUniformScale(scene, entity.GetId(), 10.0f);

    ASSERT_TRUE(changed);
    const auto scaledEntity = scene.FindEntity(entity.GetId());
    ASSERT_TRUE(scaledEntity.has_value());
    EXPECT_FLOAT_EQ(12.0f, scaledEntity->get().GetTransform().scale.x);
    EXPECT_FLOAT_EQ(13.0f, scaledEntity->get().GetTransform().scale.y);
    EXPECT_FLOAT_EQ(4.0f, scaledEntity->get().GetTransform().scale.z);
}

TEST(SceneEditingOperationsTests, AdjustEntityRotationZUpdatesZAndPreservesXY)
{
    Xelqoria::Game::Scene scene;
    auto& entity = scene.CreateEntity();
    entity.SetRotation(2.0f, 3.0f, 4.0f);

    const bool changed =
        Xelqoria::Editor::SceneEditingOperations::AdjustEntityRotationZ(scene, entity.GetId(), -10.0f);

    ASSERT_TRUE(changed);
    const auto rotatedEntity = scene.FindEntity(entity.GetId());
    ASSERT_TRUE(rotatedEntity.has_value());
    EXPECT_FLOAT_EQ(2.0f, rotatedEntity->get().GetTransform().rotation.x);
    EXPECT_FLOAT_EQ(3.0f, rotatedEntity->get().GetTransform().rotation.y);
    EXPECT_FLOAT_EQ(-6.0f, rotatedEntity->get().GetTransform().rotation.z);
}

TEST(SceneEditingOperationsTests, DeleteSelectedEntityChoosesRemainingNeighbor)
{
    Xelqoria::Game::Scene scene;
    const auto firstId = scene.CreateEntity().GetId();
    const auto secondId = scene.CreateEntity().GetId();
    const auto thirdId = scene.CreateEntity().GetId();

    const auto result =
        Xelqoria::Editor::SceneEditingOperations::DeleteSelectedEntity(scene, secondId);

    ASSERT_TRUE(result.changed);
    ASSERT_EQ(static_cast<std::size_t>(2), scene.GetEntityCount());
    EXPECT_FALSE(scene.FindEntity(secondId).has_value());
    ASSERT_TRUE(result.selectedEntityId.has_value());
    EXPECT_EQ(thirdId, *result.selectedEntityId);
    EXPECT_TRUE(scene.FindEntity(firstId).has_value());
}

TEST(SceneEditingOperationsTests, DeleteLastEntityClearsSelection)
{
    Xelqoria::Game::Scene scene;
    const auto entityId = scene.CreateEntity().GetId();

    const auto result =
        Xelqoria::Editor::SceneEditingOperations::DeleteSelectedEntity(scene, entityId);

    ASSERT_TRUE(result.changed);
    EXPECT_EQ(static_cast<std::size_t>(0), scene.GetEntityCount());
    EXPECT_FALSE(result.selectedEntityId.has_value());
}

TEST(SceneEditingOperationsTests, AddSpriteComponentAttachesDefaultComponentAndSurvivesSerialization)
{
    Xelqoria::Game::Scene scene;
    auto& entity = scene.CreateEntity();

    const bool changed = Xelqoria::Editor::SceneEditingOperations::AddSpriteComponent(entity);

    ASSERT_TRUE(changed);
    ASSERT_TRUE(entity.HasSpriteComponent());
    const auto spriteComponent = entity.GetSpriteComponent();
    ASSERT_TRUE(spriteComponent.has_value());
    EXPECT_TRUE(spriteComponent->get().spriteAssetRef.IsEmpty());
    EXPECT_TRUE(spriteComponent->get().renderSettings.visible);
    EXPECT_EQ(0, spriteComponent->get().renderSettings.sortOrder);
    EXPECT_FLOAT_EQ(1.0f, spriteComponent->get().renderSettings.opacity);

    const auto loadResult = Xelqoria::Game::SceneSerializer::LoadFromText(
        Xelqoria::Game::SceneSerializer::SaveToText(scene));
    ASSERT_TRUE(loadResult.IsSuccess());
    ASSERT_TRUE(loadResult.scene.has_value());

    const auto loadedEntity = loadResult.scene->FindEntity(entity.GetId());
    ASSERT_TRUE(loadedEntity.has_value());
    EXPECT_TRUE(loadedEntity->get().HasSpriteComponent());
}

TEST(SceneEditingOperationsTests, RenameEntityTrimsWhitespaceAndFallsBackToDefaultName)
{
    Xelqoria::Game::Scene scene;
    auto& entity = scene.CreateEntity();

    EXPECT_TRUE(Xelqoria::Editor::SceneEditingOperations::RenameEntity(entity, "  Player  "));
    EXPECT_EQ("Player", entity.GetName());

    EXPECT_TRUE(Xelqoria::Editor::SceneEditingOperations::RenameEntity(entity, "   "));
    EXPECT_EQ("Entity 1", entity.GetName());

    EXPECT_FALSE(Xelqoria::Editor::SceneEditingOperations::RenameEntity(entity, "Entity 1"));
}

TEST(SceneEditingOperationsTests, RemoveSpriteComponentDetachesComponentAndSurvivesSerialization)
{
    Xelqoria::Game::Scene scene;
    auto& entity = scene.CreateEntity();
    entity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
        Xelqoria::Core::AssetId("sprites/player"),
        {
            true,
            2,
            0.75f
        }
    });

    const bool changed = Xelqoria::Editor::SceneEditingOperations::RemoveSpriteComponent(entity);

    ASSERT_TRUE(changed);
    EXPECT_FALSE(entity.HasSpriteComponent());

    const auto loadResult = Xelqoria::Game::SceneSerializer::LoadFromText(
        Xelqoria::Game::SceneSerializer::SaveToText(scene));
    ASSERT_TRUE(loadResult.IsSuccess());
    ASSERT_TRUE(loadResult.scene.has_value());

    const auto loadedEntity = loadResult.scene->FindEntity(entity.GetId());
    ASSERT_TRUE(loadedEntity.has_value());
    EXPECT_FALSE(loadedEntity->get().HasSpriteComponent());
}

TEST(SceneEditingOperationsTests, AddAndRemoveCollider2DComponentReturnWhetherChanged)
{
    Xelqoria::Game::Scene scene;
    auto& entity = scene.CreateEntity();

    EXPECT_TRUE(Xelqoria::Editor::SceneEditingOperations::AddCollider2DComponent(entity));
    EXPECT_TRUE(entity.HasCollider2DComponent());
    EXPECT_FALSE(Xelqoria::Editor::SceneEditingOperations::AddCollider2DComponent(entity));

    const auto collider = entity.GetCollider2DComponent();
    ASSERT_TRUE(collider.has_value());
    EXPECT_TRUE(collider->get().enabled);
    EXPECT_FALSE(collider->get().isTrigger);
    EXPECT_EQ(Xelqoria::Game::Collider2DShapeType::Box, collider->get().shapeType);

    EXPECT_TRUE(Xelqoria::Editor::SceneEditingOperations::RemoveCollider2DComponent(entity));
    EXPECT_FALSE(entity.HasCollider2DComponent());
    EXPECT_FALSE(Xelqoria::Editor::SceneEditingOperations::RemoveCollider2DComponent(entity));
}

TEST(SceneEditingOperationsTests, DuplicateSelectedEntityCopiesCollider2DComponent)
{
    Xelqoria::Game::Scene scene;
    auto& sourceEntity = scene.CreateEntity();
    sourceEntity.SetName("Collider");
    sourceEntity.SetCollider2DComponent(Xelqoria::Game::Collider2DComponent{
        false,
        true,
        Xelqoria::Game::Collider2DShapeType::Box,
        { 3.0f, -4.0f },
        { 5.0f, 6.0f }
    });

    const auto result =
        Xelqoria::Editor::SceneEditingOperations::DuplicateSelectedEntity(scene, sourceEntity.GetId());

    ASSERT_TRUE(result.changed);
    ASSERT_TRUE(result.selectedEntityId.has_value());
    const auto duplicateEntity = scene.FindEntity(*result.selectedEntityId);
    ASSERT_TRUE(duplicateEntity.has_value());

    const auto duplicateCollider = duplicateEntity->get().GetCollider2DComponent();
    ASSERT_TRUE(duplicateCollider.has_value());
    EXPECT_FALSE(duplicateCollider->get().enabled);
    EXPECT_TRUE(duplicateCollider->get().isTrigger);
    EXPECT_EQ(Xelqoria::Game::Collider2DShapeType::Box, duplicateCollider->get().shapeType);
    EXPECT_FLOAT_EQ(3.0f, duplicateCollider->get().offset.x);
    EXPECT_FLOAT_EQ(-4.0f, duplicateCollider->get().offset.y);
    EXPECT_FLOAT_EQ(5.0f, duplicateCollider->get().size.x);
    EXPECT_FLOAT_EQ(6.0f, duplicateCollider->get().size.y);
}
