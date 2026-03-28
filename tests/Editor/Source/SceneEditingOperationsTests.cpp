#include <gtest/gtest.h>

#include "SceneEditingOperations.h"

TEST(SceneEditingOperationsTests, DuplicateSelectedEntityCopiesTransformAndSpriteComponent)
{
    Xelqoria::Game::Scene scene;
    auto& sourceEntity = scene.CreateEntity();
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
