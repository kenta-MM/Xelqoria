#include "SceneDropPlacementService.h"

#include <memory>
#include <string>

#include "SceneSerializer.h"
#include "SpriteComponent.h"
#include <Windows.h>
#include <AssetId.h>
#include "EditorSceneDocument.h"
#include "SceneViewInteractionTypes.h"
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

        if (false == static_cast<bool>(document.GetTextureAssetRegistry().ResolveTexture(spriteAsset->textureAssetId)))
        {
            const std::string debugLine =
                "Editor::SceneDropPlacementService could not resolve Texture AssetId '"
                + spriteAsset->textureAssetId.GetValue()
                + "' for dropped Sprite AssetId '"
                + droppedAssetId.GetValue()
                + "'.\n";
            ::OutputDebugStringA(debugLine.c_str());
            result.status = SceneDropPlacementStatus::MissingTexture;
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
