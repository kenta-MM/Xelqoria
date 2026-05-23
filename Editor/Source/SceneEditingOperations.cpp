#include "SceneEditingOperations.h"

#include <cctype>
#include <span>
#include <optional>
#include <string>
#include <string_view>
#include <Collider2DComponent.h>
#include <Entity.h>
#include <Scene.h>
#include <SpriteComponent.h>
#include <Transform.h>

namespace Xelqoria::Editor
{
    namespace
    {
        /// <summary>
        /// Entity ID から既定の Entity 名を生成する。
        /// </summary>
        /// <param name="entityId">名前に使用する Entity ID。</param>
        /// <returns>既定の Entity 名。</returns>
        std::string BuildDefaultEntityName(Game::EntityId entityId)
        {
            return "Entity " + std::to_string(entityId);
        }

        /// <summary>
        /// 入力された Entity 名の前後空白を除去する。
        /// </summary>
        /// <param name="value">整形対象の Entity 名。</param>
        /// <returns>前後空白を除去した Entity 名。</returns>
        std::string TrimEntityName(std::string_view value)
        {
            std::size_t start = 0;
            while (start < value.size()
                && 0 != std::isspace(static_cast<unsigned char>(value[start])))
            {
                ++start;
            }

            std::size_t end = value.size();
            while (end > start
                && 0 != std::isspace(static_cast<unsigned char>(value[end - 1])))
            {
                --end;
            }

            return std::string(value.substr(start, end - start));
        }

        /// <summary>
        /// 複製先に割り当てる Entity 名を生成する。
        /// </summary>
        /// <param name="sourceName">複製元の Entity 名。</param>
        /// <param name="duplicateEntityId">複製先の Entity ID。</param>
        /// <returns>複製先の Entity 名。</returns>
        std::string BuildDuplicateEntityName(std::string_view sourceName, Game::EntityId duplicateEntityId)
        {
            const std::string trimmedSourceName = TrimEntityName(sourceName);
            if (true == trimmedSourceName.empty())
            {
                return BuildDefaultEntityName(duplicateEntityId);
            }

            return trimmedSourceName + " Copy";
        }

        /// <summary>
        /// 複製に必要な Entity 状態のスナップショットを表す。
        /// </summary>
        struct EntitySnapshot
        {
            /// <summary>
            /// Entity 名を保持する。
            /// </summary>
            std::string name{};

            /// <summary>
            /// Entity の Transform を保持する。
            /// </summary>
            Game::Transform transform{};

            /// <summary>
            /// Entity の SpriteComponent を保持する。
            /// </summary>
            std::optional<Game::SpriteComponent> spriteComponent{};

            /// <summary>
            /// Entity の Collider2DComponent を保持する。
            /// </summary>
            std::optional<Game::Collider2DComponent> collider2DComponent{};
        };

        /// <summary>
        /// Entity の Transform と SpriteComponent を複製用に退避する。
        /// </summary>
        /// <param name="source">複製元 Entity。</param>
        /// <returns>複製用スナップショット。</returns>
        EntitySnapshot CaptureEntitySnapshot(const Game::Entity& source)
        {
            EntitySnapshot snapshot{};
            snapshot.name = source.GetName();
            snapshot.transform = source.GetTransform();

            if (const auto spriteComponent = source.GetSpriteComponent(); spriteComponent.has_value())
            {
                snapshot.spriteComponent = spriteComponent->get();
            }

            if (const auto collider2DComponent = source.GetCollider2DComponent(); collider2DComponent.has_value())
            {
                snapshot.collider2DComponent = collider2DComponent->get();
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
            destination.SetName(snapshot.name);
            destination.SetTransform(snapshot.transform);

            if (snapshot.spriteComponent.has_value())
            {
                destination.SetSpriteComponent(*snapshot.spriteComponent);
            }
            else
            {
                destination.RemoveSpriteComponent();
            }

            if (snapshot.collider2DComponent.has_value())
            {
                destination.SetCollider2DComponent(*snapshot.collider2DComponent);
                return;
            }

            destination.RemoveCollider2DComponent();
        }
    }

    SceneEditResult SceneEditingOperations::CreateEntity(Game::Scene& scene)
    {
        auto& entity = scene.CreateEntity();
        entity.SetName(BuildDefaultEntityName(entity.GetId()));

        return SceneEditResult{
            true,
            entity.GetId(),
            false,
            "Create Entity"
        };
    }

    SceneEditResult SceneEditingOperations::CreateUntexturedSprite(Game::Scene& scene, float x, float y)
    {
        auto& entity = scene.CreateEntity();
        entity.SetName("Sprite " + std::to_string(entity.GetId()));
        entity.SetPosition(x, y, 0.0f);
        entity.SetSpriteComponent(Game::SpriteComponent{});

        return SceneEditResult{
            true,
            entity.GetId(),
            true,
            "Create Entity"
        };
    }

    SceneEditResult SceneEditingOperations::DeleteSelectedEntity(
        Game::Scene& scene,
        std::optional<Game::EntityId> selectedEntityId)
    {
        if (false == selectedEntityId.has_value())
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

        if (false == selectedIndex.has_value())
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

        if (false == scene.DestroyEntity(*selectedEntityId))
        {
            return SceneEditResult{};
        }

        return SceneEditResult{
            true,
            nextSelection,
            false,
            "Delete Entity"
        };
    }

    SceneEditResult SceneEditingOperations::DuplicateSelectedEntity(
        Game::Scene& scene,
        std::optional<Game::EntityId> selectedEntityId)
    {
        if (false == selectedEntityId.has_value())
        {
            return SceneEditResult{};
        }

        const auto sourceEntity = scene.FindEntity(*selectedEntityId);
        if (false == sourceEntity.has_value())
        {
            return SceneEditResult{};
        }

        const EntitySnapshot snapshot = CaptureEntitySnapshot(sourceEntity->get());
        auto& duplicateEntity = scene.CreateEntity();
        ApplyEntitySnapshot(snapshot, duplicateEntity);
        duplicateEntity.SetName(BuildDuplicateEntityName(snapshot.name, duplicateEntity.GetId()));

        return SceneEditResult{
            true,
            duplicateEntity.GetId(),
            false,
            "Duplicate Entity"
        };
    }

    bool SceneEditingOperations::MoveEntity(
        Game::Scene& scene,
        Game::EntityId entityId,
        float x,
        float y)
    {
        const auto entity = scene.FindEntity(entityId);
        if (false == entity.has_value())
        {
            return false;
        }

        const Game::Transform& transform = entity->get().GetTransform();
        if (transform.position.x == x && transform.position.y == y)
        {
            return false;
        }

        entity->get().SetPosition(x, y, transform.position.z);
        return true;
    }

    bool SceneEditingOperations::AdjustEntityUniformScale(
        Game::Scene& scene,
        Game::EntityId entityId,
        float delta)
    {
        if (0.0f == delta)
        {
            return false;
        }

        const auto entity = scene.FindEntity(entityId);
        if (false == entity.has_value())
        {
            return false;
        }

        const Game::Transform& transform = entity->get().GetTransform();
        entity->get().SetScale(
            transform.scale.x + delta,
            transform.scale.y + delta,
            transform.scale.z);
        return true;
    }

    bool SceneEditingOperations::AdjustEntityRotationZ(
        Game::Scene& scene,
        Game::EntityId entityId,
        float deltaDegrees)
    {
        if (0.0f == deltaDegrees)
        {
            return false;
        }

        const auto entity = scene.FindEntity(entityId);
        if (false == entity.has_value())
        {
            return false;
        }

        const Game::Transform& transform = entity->get().GetTransform();
        entity->get().SetRotation(
            transform.rotation.x,
            transform.rotation.y,
            transform.rotation.z + deltaDegrees);
        return true;
    }

    bool SceneEditingOperations::RenameEntity(Game::Entity& entity, std::string_view newName)
    {
        std::string normalizedName = TrimEntityName(newName);
        if (true == normalizedName.empty())
        {
            normalizedName = BuildDefaultEntityName(entity.GetId());
        }

        if (entity.GetName() == normalizedName)
        {
            return false;
        }

        entity.SetName(std::move(normalizedName));
        return true;
    }

    bool SceneEditingOperations::AddSpriteComponent(Game::Entity& entity)
    {
        if (entity.HasSpriteComponent())
        {
            return false;
        }

        entity.SetSpriteComponent(Game::SpriteComponent{});
        return true;
    }

    bool SceneEditingOperations::RemoveSpriteComponent(Game::Entity& entity)
    {
        if (false == entity.HasSpriteComponent())
        {
            return false;
        }

        entity.RemoveSpriteComponent();
        return true;
    }

    bool SceneEditingOperations::AddCollider2DComponent(Game::Entity& entity)
    {
        if (entity.HasCollider2DComponent())
        {
            return false;
        }

        entity.SetCollider2DComponent(Game::Collider2DComponent{});
        return true;
    }

    bool SceneEditingOperations::RemoveCollider2DComponent(Game::Entity& entity)
    {
        if (false == entity.HasCollider2DComponent())
        {
            return false;
        }

        entity.RemoveCollider2DComponent();
        return true;
    }
}
