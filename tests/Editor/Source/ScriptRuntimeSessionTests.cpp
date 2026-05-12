#include "ScriptRuntimeSession.h"

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
