#include <gtest/gtest.h>

#include "InspectorPanelController.h"

TEST(InspectorPanelControllerTests, ComputeActionStateShowsEnabledRemoveWhenSpriteComponentAttached)
{
    const auto actionState =
        Xelqoria::Editor::InspectorPanelController::ComputeSpriteComponentActionState(true, false);

    EXPECT_TRUE(actionState.showSpriteRefControls);
    EXPECT_STREQ(L"SpriteComponent", actionState.sectionLabel);
    EXPECT_STREQ(L"Remove SpriteComponent", actionState.buttonLabel);
    EXPECT_TRUE(actionState.enableActionButton);
}

TEST(InspectorPanelControllerTests, ComputeActionStateShowsEnabledAddWhenSpriteAssetCanBeAdded)
{
    const auto actionState =
        Xelqoria::Editor::InspectorPanelController::ComputeSpriteComponentActionState(false, true);

    EXPECT_FALSE(actionState.showSpriteRefControls);
    EXPECT_STREQ(L"SpriteComponent (not attached)", actionState.sectionLabel);
    EXPECT_STREQ(L"Add SpriteComponent", actionState.buttonLabel);
    EXPECT_TRUE(actionState.enableActionButton);
}

TEST(InspectorPanelControllerTests, ComputeActionStateDisablesAddWhenNoVisibleSpriteAssetExists)
{
    const auto actionState =
        Xelqoria::Editor::InspectorPanelController::ComputeSpriteComponentActionState(false, false);

    EXPECT_FALSE(actionState.showSpriteRefControls);
    EXPECT_STREQ(L"SpriteComponent (not attached)", actionState.sectionLabel);
    EXPECT_STREQ(L"Add SpriteComponent", actionState.buttonLabel);
    EXPECT_FALSE(actionState.enableActionButton);
}

TEST(InspectorPanelControllerTests, ApplySpriteComponentActionAddsComponentOnlyWhenSpriteAssetCanBeAdded)
{
    Xelqoria::Game::Scene scene;
    auto& entity = scene.CreateEntity();

    const bool changed = Xelqoria::Editor::InspectorPanelController::ApplySpriteComponentAction(entity, true);

    EXPECT_TRUE(changed);
    EXPECT_TRUE(entity.HasSpriteComponent());
}

TEST(InspectorPanelControllerTests, ApplySpriteComponentActionRemovesExistingComponentEvenWithoutAddableSpriteAsset)
{
    Xelqoria::Game::Scene scene;
    auto& entity = scene.CreateEntity();
    entity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
        Xelqoria::Core::AssetId("sprites/player"),
        {
            true,
            0,
            1.0f
        }
    });

    const bool changed = Xelqoria::Editor::InspectorPanelController::ApplySpriteComponentAction(entity, false);

    EXPECT_TRUE(changed);
    EXPECT_FALSE(entity.HasSpriteComponent());
}

TEST(InspectorPanelControllerTests, ApplySpriteComponentActionSkipsAddWhenNoVisibleSpriteAssetExists)
{
    Xelqoria::Game::Scene scene;
    auto& entity = scene.CreateEntity();

    const bool changed = Xelqoria::Editor::InspectorPanelController::ApplySpriteComponentAction(entity, false);

    EXPECT_FALSE(changed);
    EXPECT_FALSE(entity.HasSpriteComponent());
}
