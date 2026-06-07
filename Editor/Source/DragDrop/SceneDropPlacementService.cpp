#include "DragDrop/SceneDropPlacementService.h"

#include <memory>
#include <string>

#include "SceneSerializer.h"
#include "SpriteComponent.h"
#include <Windows.h>
#include <AssetId.h>
#include "Project/EditorSceneDocument.h"
#include "SceneView/SceneViewInteractionTypes.h"
#include <Entity.h>
#include <Scene.h>

namespace Xelqoria::Editor
{
    SceneViewDropResult SceneDropPlacementService::ProcessPendingSceneDrop(
        EditorSceneDocument& document,
        const ScenePendingDropState& pendingDropState) const
    {
        SceneViewDropResult result{};
        Game::Scene* scene = document.GetScene();
        if (false == pendingDropState.hasPendingDrop || nullptr == scene)
        {
            return result;
        }

        if (ScenePendingDropState::Kind::ScriptAsset == pendingDropState.kind)
        {
            if (pendingDropState.scriptAssetId.IsEmpty()
                || true == pendingDropState.scriptAssetPath.empty())
            {
                ::OutputDebugStringA("Editor::SceneDropPlacementService could not assign a script because drop payload Script Asset was empty.\n");
                result.status = SceneDropPlacementStatus::EmptyAssetId;
                return result;
            }

            if (false == pendingDropState.targetEntityId.has_value())
            {
                ::OutputDebugStringA("Editor::SceneDropPlacementService could not assign a script because no Sprite Entity was hit.\n");
                result.status = SceneDropPlacementStatus::MissingTargetEntity;
                return result;
            }

            const auto entity = scene->FindEntity(*pendingDropState.targetEntityId);
            if (false == entity.has_value())
            {
                ::OutputDebugStringA("Editor::SceneDropPlacementService could not assign a script because the target Entity was missing.\n");
                result.status = SceneDropPlacementStatus::MissingTargetEntity;
                return result;
            }

            const auto spriteComponent = entity->get().GetSpriteComponent();
            if (false == spriteComponent.has_value()
                || true == spriteComponent->get().spriteAssetRef.IsEmpty())
            {
                ::OutputDebugStringA("Editor::SceneDropPlacementService could not assign a script because the target Entity has no Sprite Asset reference.\n");
                result.status = SceneDropPlacementStatus::MissingSpriteComponent;
                return result;
            }

            if (false == document.AssignScriptAssetToSpriteAsset(
                    spriteComponent->get().spriteAssetRef,
                    pendingDropState.scriptAssetPath))
            {
                ::OutputDebugStringA("Editor::SceneDropPlacementService failed to assign dropped Script Asset to Sprite Asset.\n");
                result.status = SceneDropPlacementStatus::ScriptAssignFailed;
                return result;
            }

            result.assetChanged = true;
            result.selectionChanged = true;
            result.selectedEntityId = *pendingDropState.targetEntityId;
            result.operationName = "Assign Script Asset";
            result.status = SceneDropPlacementStatus::Success;

            const std::string debugLine =
                "Editor::SceneDropPlacementService assigned Script AssetId '"
                + pendingDropState.scriptAssetId.GetValue()
                + "' to Sprite AssetId '"
                + spriteComponent->get().spriteAssetRef.GetValue()
                + "' on Entity "
                + std::to_string(*pendingDropState.targetEntityId)
                + ".\n";
            ::OutputDebugStringA(debugLine.c_str());
            return result;
        }

        const Core::AssetId droppedAssetId = pendingDropState.spriteAssetId;
        const float dropWorldX = pendingDropState.worldX;
        const float dropWorldY = pendingDropState.worldY;

        if (droppedAssetId.IsEmpty())
        {
            ::OutputDebugStringA("Editor::SceneDropPlacementService could not create an entity because drop payload AssetId was empty.\n");
            result.status = SceneDropPlacementStatus::EmptyAssetId;
            return result;
        }

        const auto spriteAsset = document.GetSpriteAssetRegistry().ResolveSpriteAsset(droppedAssetId);
        if (false == spriteAsset.has_value())
        {
            const std::string debugLine =
                "Editor::SceneDropPlacementService could not resolve dropped Sprite AssetId '" + droppedAssetId.GetValue() + "'.\n";
            ::OutputDebugStringA(debugLine.c_str());
            result.status = SceneDropPlacementStatus::MissingSpriteAsset;
            return result;
        }

        auto& entity = scene->CreateEntity();
        entity.SetPosition(dropWorldX, dropWorldY, 0.0f);
        entity.SetSpriteComponent(Game::SpriteComponent{
            droppedAssetId,
            {
                true,
                0,
                1.0f
            }
        });

        const Game::EntityId createdEntityId = entity.GetId();
        result.sceneChanged = true;
        result.selectionChanged = true;
        result.selectedEntityId = createdEntityId;

        const std::string serializedScene = Game::SceneSerializer::SaveToText(*scene);
        const auto loadResult = Game::SceneSerializer::LoadFromText(serializedScene);
        if (false == loadResult.IsSuccess() || false == loadResult.scene.has_value())
        {
            ::OutputDebugStringA("Editor::SceneDropPlacementService failed to reload dropped scene placement snapshot.\n");
            result.status = SceneDropPlacementStatus::ReloadFailed;
            return result;
        }

        document.ReplaceScene(std::make_unique<Game::Scene>(*loadResult.scene));

        result.shouldPushHistory = true;
        result.operationName = "Create Entity";
        result.status = SceneDropPlacementStatus::Success;

        const std::string debugLine =
            "Editor::SceneDropPlacementService created entity "
            + std::to_string(createdEntityId)
            + " from Sprite AssetId '"
            + droppedAssetId.GetValue()
            + "' at world position ("
            + std::to_string(dropWorldX)
            + ", "
            + std::to_string(dropWorldY)
            + ") and reloaded the scene snapshot.\n";
        ::OutputDebugStringA(debugLine.c_str());
        return result;
    }
}
