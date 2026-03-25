#include <gtest/gtest.h>

#include "SceneCommandHistory.h"
#include "SceneSerializer.h"

namespace
{
    Xelqoria::Editor::SceneCommandHistoryEntry CreateEntry(
        const Xelqoria::Game::Scene& scene,
        std::optional<Xelqoria::Game::EntityId> selectedEntityId)
    {
        return Xelqoria::Editor::SceneCommandHistoryEntry{
            Xelqoria::Game::SceneSerializer::SaveToText(scene),
            selectedEntityId
        };
    }
}

TEST(SceneCommandHistoryTests, StoresOneDropAsSingleUndoRedoUnit)
{
    Xelqoria::Editor::SceneCommandHistory history;

    Xelqoria::Game::Scene initialScene;
    history.Reset(CreateEntry(initialScene, std::nullopt));

    Xelqoria::Game::Scene droppedScene;
    auto& droppedEntity = droppedScene.CreateEntity();
    droppedEntity.GetTransform().SetPosition(32.0f, -16.0f, 0.0f);
    droppedEntity.SetSpriteComponent(Xelqoria::Game::SpriteComponent{
        Xelqoria::Core::AssetId("sprites/player"),
        {
            true,
            0,
            1.0f
        }
    });
    history.Push(CreateEntry(droppedScene, droppedEntity.GetId()));

    ASSERT_TRUE(history.CanUndo());

    const auto undone = history.Undo();
    ASSERT_TRUE(undone.has_value());
    const auto undoneScene = Xelqoria::Game::SceneSerializer::LoadFromText(undone->serializedScene);
    ASSERT_TRUE(undoneScene.IsSuccess());
    ASSERT_TRUE(undoneScene.scene.has_value());
    EXPECT_EQ(static_cast<std::size_t>(0), undoneScene.scene->GetEntityCount());
    EXPECT_FALSE(undone->selectedEntityId.has_value());

    ASSERT_TRUE(history.CanRedo());

    const auto redone = history.Redo();
    ASSERT_TRUE(redone.has_value());
    const auto redoneScene = Xelqoria::Game::SceneSerializer::LoadFromText(redone->serializedScene);
    ASSERT_TRUE(redoneScene.IsSuccess());
    ASSERT_TRUE(redoneScene.scene.has_value());
    ASSERT_EQ(static_cast<std::size_t>(1), redoneScene.scene->GetEntityCount());
    EXPECT_EQ(droppedEntity.GetId(), redone->selectedEntityId);
}

TEST(SceneCommandHistoryTests, ClearsRedoBranchWhenNewEditIsPushedAfterUndo)
{
    Xelqoria::Editor::SceneCommandHistory history;

    Xelqoria::Game::Scene initialScene;
    history.Reset(CreateEntry(initialScene, std::nullopt));

    Xelqoria::Game::Scene firstScene;
    firstScene.CreateEntity();
    history.Push(CreateEntry(firstScene, static_cast<Xelqoria::Game::EntityId>(1)));

    Xelqoria::Game::Scene secondScene = firstScene;
    secondScene.CreateEntity();
    history.Push(CreateEntry(secondScene, static_cast<Xelqoria::Game::EntityId>(2)));

    ASSERT_TRUE(history.Undo().has_value());
    ASSERT_TRUE(history.CanRedo());

    Xelqoria::Game::Scene replacementScene = firstScene;
    auto& replacementEntity = replacementScene.CreateEntity();
    replacementEntity.GetTransform().SetPosition(10.0f, 20.0f, 0.0f);
    history.Push(CreateEntry(replacementScene, replacementEntity.GetId()));

    EXPECT_FALSE(history.CanRedo());
    EXPECT_EQ(static_cast<std::size_t>(3), history.GetCount());
}
