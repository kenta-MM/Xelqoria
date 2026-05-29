#include <gtest/gtest.h>

#include <string>

#include "Utils/EditorStringUtils.h"
#include "Panels/Inspector/InspectorPanelController.h"

TEST(EditorStringUtilsTests, Utf8ConversionRoundTripsJapaneseFileName)
{
    const std::wstring fileName = L"タイトルなし.png";
    const std::string utf8FileName = Xelqoria::Editor::ToNarrowString(fileName);

    EXPECT_FALSE(utf8FileName.empty());
    EXPECT_EQ(fileName, Xelqoria::Editor::ToWideString(utf8FileName));
}

TEST(InspectorPanelControllerTests, FormatTextureDisplayTextShowsFileNameOnly)
{
    const std::wstring displayText =
        Xelqoria::Editor::InspectorPanelController::FormatTextureDisplayText(
            Xelqoria::Core::AssetId("textures/タイトルなし.png"));

    EXPECT_EQ(L"タイトルなし.png", displayText);
}

TEST(InspectorPanelControllerTests, FormatTextureDisplayTextShowsNestedFileNameOnly)
{
    const std::wstring displayText =
        Xelqoria::Editor::InspectorPanelController::FormatTextureDisplayText(
            Xelqoria::Core::AssetId("textures/Images/タイトルなし.png"));

    EXPECT_EQ(L"タイトルなし.png", displayText);
}

TEST(InspectorPanelControllerTests, FormatTextureDisplayTextKeepsSpriteAssetFileNameOutOfTextureField)
{
    const std::wstring displayText =
        Xelqoria::Editor::InspectorPanelController::FormatTextureDisplayText(
            Xelqoria::Core::AssetId("textures/Characters/Player.png"));

    EXPECT_EQ(L"Player.png", displayText);
}

TEST(InspectorPanelControllerTests, FormatScriptDisplayTextShowsFileNameOrNone)
{
    EXPECT_EQ(
        L"Player.script",
        Xelqoria::Editor::InspectorPanelController::FormatScriptDisplayText(
            Xelqoria::Core::AssetId("scripts/Scripts/Player.script")));
    EXPECT_EQ(
        L"None",
        Xelqoria::Editor::InspectorPanelController::FormatScriptDisplayText({}));
}

TEST(InspectorPanelControllerTests, FormatMaterialDisplayTextShowsFileNameOrNone)
{
    EXPECT_EQ(
        L"Player.material",
        Xelqoria::Editor::InspectorPanelController::FormatMaterialDisplayText(
            Xelqoria::Core::AssetId("materials/Characters/Player.material")));
    EXPECT_EQ(
        L"None",
        Xelqoria::Editor::InspectorPanelController::FormatMaterialDisplayText({}));
}

TEST(InspectorPanelControllerTests, ComputeScriptActionStateReflectsSpriteAndScriptAvailability)
{
    const auto unavailableState =
        Xelqoria::Editor::InspectorPanelController::ComputeScriptActionState(
            false,
            {},
            false,
            {});
    EXPECT_FALSE(unavailableState.showScriptControls);
    EXPECT_FALSE(unavailableState.enableCreateButton);
    EXPECT_FALSE(unavailableState.enableAssignButton);
    EXPECT_FALSE(unavailableState.enableClearButton);

    const auto unresolvedState =
        Xelqoria::Editor::InspectorPanelController::ComputeScriptActionState(
            true,
            Xelqoria::Core::AssetId("sprites/player.sprite"),
            false,
            {});
    EXPECT_TRUE(unresolvedState.showScriptControls);
    EXPECT_FALSE(unresolvedState.enableCreateButton);
    EXPECT_FALSE(unresolvedState.enableAssignButton);
    EXPECT_FALSE(unresolvedState.enableClearButton);

    const auto assignedState =
        Xelqoria::Editor::InspectorPanelController::ComputeScriptActionState(
            true,
            Xelqoria::Core::AssetId("sprites/player.sprite"),
            true,
            Xelqoria::Core::AssetId("scripts/Player.script"));
    EXPECT_TRUE(assignedState.showScriptControls);
    EXPECT_TRUE(assignedState.enableCreateButton);
    EXPECT_TRUE(assignedState.enableAssignButton);
    EXPECT_TRUE(assignedState.enableClearButton);
}

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

TEST(InspectorPanelControllerTests, ComputeCollider2DComponentActionStateReflectsAttachment)
{
    const auto detachedState =
        Xelqoria::Editor::InspectorPanelController::ComputeCollider2DComponentActionState(false);

    EXPECT_FALSE(detachedState.showColliderControls);
    EXPECT_STREQ(L"Collider2DComponent (not attached)", detachedState.sectionLabel);
    EXPECT_STREQ(L"Add Collider2DComponent", detachedState.buttonLabel);

    const auto attachedState =
        Xelqoria::Editor::InspectorPanelController::ComputeCollider2DComponentActionState(true);

    EXPECT_TRUE(attachedState.showColliderControls);
    EXPECT_STREQ(L"Collider2DComponent", attachedState.sectionLabel);
    EXPECT_STREQ(L"Remove Collider2DComponent", attachedState.buttonLabel);
}

TEST(InspectorPanelControllerTests, ApplyCollider2DComponentActionTogglesComponent)
{
    Xelqoria::Game::Scene scene;
    auto& entity = scene.CreateEntity();

    EXPECT_TRUE(Xelqoria::Editor::InspectorPanelController::ApplyCollider2DComponentAction(entity));
    EXPECT_TRUE(entity.HasCollider2DComponent());

    EXPECT_TRUE(Xelqoria::Editor::InspectorPanelController::ApplyCollider2DComponentAction(entity));
    EXPECT_FALSE(entity.HasCollider2DComponent());
}
