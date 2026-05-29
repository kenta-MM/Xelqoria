#include "Script/ScriptRuntimeSession.h"

#include <filesystem>

#include <gtest/gtest.h>

#include "Assets/SpriteAssetRegistry.h"
#include "Scene.h"

TEST(ScriptRuntimeSessionTests, BuildInstancePlansCreatesSeparateModuleCopiesPerSpriteInstance)
{
    Xelqoria::Game::Scene scene{};
    Xelqoria::Game::Entity& firstEntity = scene.CreateEntity();
    firstEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
        Xelqoria::Core::AssetId("sprites/player"),
        {}
    });
    const Xelqoria::Game::EntityId firstEntityId = firstEntity.GetId();
    Xelqoria::Game::Entity& secondEntity = scene.CreateEntity();
    secondEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
        Xelqoria::Core::AssetId("sprites/player"),
        {}
    });
    const Xelqoria::Game::EntityId secondEntityId = secondEntity.GetId();

    Xelqoria::Game::Assets::SpriteAssetRegistry spriteAssetRegistry{};
    spriteAssetRegistry.RegisterSpriteAsset(
        Xelqoria::Core::AssetId("sprites/player"),
        Xelqoria::Game::Assets::SpriteAsset{
            Xelqoria::Core::AssetId("textures/player"),
            Xelqoria::Core::AssetId("scripts/Player.script")
        });

    const std::filesystem::path projectRoot = std::filesystem::temp_directory_path() / L"XelqoriaScriptRuntimeSessionTests";
    const Xelqoria::Editor::ScriptBuildResult buildResult{
        true,
        { projectRoot / L".xelqoria" / L"Scripts" / L"Player.cpp" },
        {
            Xelqoria::Editor::ScriptBuildArtifact{
                Xelqoria::Core::AssetId("scripts/Player.script"),
                projectRoot / L".xelqoria" / L"Scripts" / L"Player.cpp",
                projectRoot / L".xelqoria" / L"ScriptBuild" / L"Player.dll"
            }
        },
        L""
    };

    const std::vector<Xelqoria::Editor::ScriptRuntimeInstancePlan> plans =
        Xelqoria::Editor::ScriptRuntimeSession::BuildInstancePlans(
            scene,
            spriteAssetRegistry,
            buildResult,
            projectRoot);

    ASSERT_EQ(static_cast<std::size_t>(2), plans.size());
    EXPECT_EQ(firstEntityId, plans[0].entityId);
    EXPECT_EQ(secondEntityId, plans[1].entityId);
    EXPECT_EQ(plans[0].scriptAssetId, plans[1].scriptAssetId);
    EXPECT_EQ(plans[0].sourceModulePath, plans[1].sourceModulePath);
    EXPECT_NE(plans[0].instanceModulePath, plans[1].instanceModulePath);
}

TEST(ScriptRuntimeSessionTests, BuildInstancePlansSkipsSpritesWithoutScriptAsset)
{
    Xelqoria::Game::Scene scene{};
    Xelqoria::Game::Entity& entity = scene.CreateEntity();
    entity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
        Xelqoria::Core::AssetId("sprites/no-script"),
        {}
    });

    Xelqoria::Game::Assets::SpriteAssetRegistry spriteAssetRegistry{};
    spriteAssetRegistry.RegisterSpriteAsset(
        Xelqoria::Core::AssetId("sprites/no-script"),
        Xelqoria::Game::Assets::SpriteAsset{ Xelqoria::Core::AssetId("textures/player") });

    const Xelqoria::Editor::ScriptBuildResult buildResult{};
    const std::vector<Xelqoria::Editor::ScriptRuntimeInstancePlan> plans =
        Xelqoria::Editor::ScriptRuntimeSession::BuildInstancePlans(
            scene,
            spriteAssetRegistry,
            buildResult,
            std::filesystem::temp_directory_path());

    EXPECT_TRUE(plans.empty());
}

TEST(ScriptRuntimeSessionTests, SpriteApiMethodsUpdateTargetSpriteState)
{
    Xelqoria::Game::Scene scene{};
    Xelqoria::Game::Entity& entity = scene.CreateEntity();
    entity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
        Xelqoria::Core::AssetId("sprites/player"),
        {}
    });
    const Xelqoria::Game::EntityId entityId = entity.GetId();

    EXPECT_TRUE(Xelqoria::Editor::ScriptRuntimeSession::SetSpritePosition(
        scene,
        entityId,
        10.0f,
        20.0f,
        3.0f));
    EXPECT_TRUE(Xelqoria::Editor::ScriptRuntimeSession::SetSpriteRotation(
        scene,
        entityId,
        1.0f,
        2.0f,
        90.0f));
    EXPECT_TRUE(Xelqoria::Editor::ScriptRuntimeSession::SetSpriteScale(
        scene,
        entityId,
        2.0f,
        3.0f,
        1.0f));
    EXPECT_TRUE(Xelqoria::Editor::ScriptRuntimeSession::SetSpriteVisible(scene, entityId, false));
    EXPECT_TRUE(Xelqoria::Editor::ScriptRuntimeSession::SetSpriteColor(
        scene,
        entityId,
        0.25f,
        0.5f,
        0.75f,
        0.8f));

    const auto updatedEntity = scene.FindEntity(entityId);
    ASSERT_TRUE(updatedEntity.has_value());
    EXPECT_FLOAT_EQ(10.0f, updatedEntity->get().GetTransform().position.x);
    EXPECT_FLOAT_EQ(20.0f, updatedEntity->get().GetTransform().position.y);
    EXPECT_FLOAT_EQ(3.0f, updatedEntity->get().GetTransform().position.z);
    EXPECT_FLOAT_EQ(90.0f, updatedEntity->get().GetTransform().rotation.z);
    EXPECT_FLOAT_EQ(2.0f, updatedEntity->get().GetTransform().scale.x);
    EXPECT_FLOAT_EQ(3.0f, updatedEntity->get().GetTransform().scale.y);

    const auto spriteComponent = updatedEntity->get().GetSpriteComponent();
    ASSERT_TRUE(spriteComponent.has_value());
    EXPECT_FALSE(spriteComponent->get().renderSettings.visible);
    EXPECT_FLOAT_EQ(0.25f, spriteComponent->get().renderSettings.color[0]);
    EXPECT_FLOAT_EQ(0.5f, spriteComponent->get().renderSettings.color[1]);
    EXPECT_FLOAT_EQ(0.75f, spriteComponent->get().renderSettings.color[2]);
    EXPECT_FLOAT_EQ(0.8f, spriteComponent->get().renderSettings.color[3]);
}

TEST(ScriptRuntimeSessionTests, SpriteApiMethodsRejectEntitiesWithoutSpriteComponent)
{
    Xelqoria::Game::Scene scene{};
    const Xelqoria::Game::EntityId entityId = scene.CreateEntity().GetId();

    EXPECT_FALSE(Xelqoria::Editor::ScriptRuntimeSession::SetSpritePosition(
        scene,
        entityId,
        10.0f,
        20.0f,
        3.0f));
    EXPECT_FALSE(Xelqoria::Editor::ScriptRuntimeSession::SetSpriteVisible(scene, entityId, true));
}
