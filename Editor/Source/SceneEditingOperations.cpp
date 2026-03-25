#include "SceneEditingOperations.h"

#include <span>

namespace Xelqoria::Editor
{
    namespace
    {
        /// <summary>
        /// 複製に必要な Entity 状態のスナップショットを表す。
        /// </summary>
        struct EntitySnapshot
        {
            /// <summary>
            /// Entity の Transform を保持する。
            /// </summary>
            Game::Transform transform{};

            /// <summary>
            /// Entity の SpriteComponent を保持する。
            /// </summary>
            std::optional<Game::SpriteComponent> spriteComponent{};
        };

        /// <summary>
        /// Entity の Transform と SpriteComponent を複製用に退避する。
        /// </summary>
        /// <param name="source">複製元 Entity。</param>
        /// <returns>複製用スナップショット。</returns>
        EntitySnapshot CaptureEntitySnapshot(const Game::Entity& source)
        {
            EntitySnapshot snapshot{};
            snapshot.transform = source.GetTransform();

            if (const auto spriteComponent = source.GetSpriteComponent(); spriteComponent.has_value())
            {
                snapshot.spriteComponent = spriteComponent->get();
            }

            return snapshot;
        }

        /// <summary>
        /// 退避済みスナップショットを Entity へ復元する。
        /// </summary>
        /// <param name="snapshot">復元する複製用スナップショット。</param>
        /// <param name="destination">複製先 Entity。</param>
        void ApplyEntitySnapshot(const EntitySnapshot& snapshot, Game::Entity& destination)
        {
            destination.GetTransform() = snapshot.transform;

            if (snapshot.spriteComponent.has_value())
            {
                destination.SetSpriteComponent(*snapshot.spriteComponent);
                return;
            }

            destination.RemoveSpriteComponent();
        }
    }

    SceneEditResult SceneEditingOperations::DeleteSelectedEntity(
        Game::Scene& scene,
        std::optional<Game::EntityId> selectedEntityId)
    {
        if (!selectedEntityId.has_value())
        {
            return SceneEditResult{};
        }

        const std::span<const Game::Entity> entities = scene.GetEntities();
        std::optional<std::size_t> selectedIndex{};
        for (std::size_t index = 0; index < entities.size(); ++index)
        {
            if (entities[index].GetId() == *selectedEntityId)
            {
                selectedIndex = index;
                break;
            }
        }

        if (!selectedIndex.has_value())
        {
            return SceneEditResult{};
        }

        std::optional<Game::EntityId> nextSelection{};
        if (entities.size() > 1)
        {
            const std::size_t nextIndex = (*selectedIndex + 1 < entities.size())
                ? *selectedIndex + 1
                : *selectedIndex - 1;
            nextSelection = entities[nextIndex].GetId();
        }

        if (!scene.DestroyEntity(*selectedEntityId))
        {
            return SceneEditResult{};
        }

        return SceneEditResult{
            true,
            nextSelection
        };
    }

    SceneEditResult SceneEditingOperations::DuplicateSelectedEntity(
        Game::Scene& scene,
        std::optional<Game::EntityId> selectedEntityId)
    {
        if (!selectedEntityId.has_value())
        {
            return SceneEditResult{};
        }

        const auto sourceEntity = scene.FindEntity(*selectedEntityId);
        if (!sourceEntity.has_value())
        {
            return SceneEditResult{};
        }

        const EntitySnapshot snapshot = CaptureEntitySnapshot(sourceEntity->get());
        auto& duplicateEntity = scene.CreateEntity();
        ApplyEntitySnapshot(snapshot, duplicateEntity);

        return SceneEditResult{
            true,
            duplicateEntity.GetId()
        };
    }
}
