#include "SceneViewInputTracker.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "SceneEditingOperations.h"
#include "SceneViewOverlay.h"
#include <Windows.h>
#include <cstdlib>
#include <vector>
#include <cstdint>
#include <optional>
#include <AssetId.h>
#include "AssetsPanelController.h"
#include "EditorCamera2D.h"
#include "SceneViewInteractionTypes.h"
#include <Assets/SpriteAssetRegistry.h>
#include <Entity.h>
#include <Scene.h>
#include <TextureAssetRegistry.h>
#include <Transform.h>

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr float UntexturedSpriteSizeWorldUnits = 64.0f;

        /// <summary>
        /// 指定編集モードキーの押下から次の編集モードを決定する。
        /// </summary>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        /// <returns>押下された編集モード。未押下の場合は空。</returns>
        std::optional<SceneViewEditMode> ReadPressedEditModeKey(const Core::InputSnapshot& inputSnapshot)
        {
            if (inputSnapshot.WasKeyPressed('V'))
            {
                return SceneViewEditMode::Move;
            }

            if (inputSnapshot.WasKeyPressed('S'))
            {
                return SceneViewEditMode::Scale;
            }

            if (inputSnapshot.WasKeyPressed('R'))
            {
                return SceneViewEditMode::Rotate;
            }

            return std::nullopt;
        }

        /// <summary>
        /// ホイール差分から編集量の符号を取得する。
        /// </summary>
        /// <param name="mouseWheelDelta">マウスホイール差分。</param>
        /// <returns>上回転は 1、下回転は -1、差分なしは 0。</returns>
        float GetMouseWheelDirection(int mouseWheelDelta)
        {
            if (mouseWheelDelta > 0)
            {
                return 1.0f;
            }

            if (mouseWheelDelta < 0)
            {
                return -1.0f;
            }

            return 0.0f;
        }
    }

    void SceneViewInputTracker::Bind(HWND sceneViewHost)
    {
        m_sceneViewHost = sceneViewHost;
    }

    SceneViewInteractionResult SceneViewInputTracker::UpdateInteraction(
        Game::Scene* scene,
        const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
        const Graphics::TextureAssetRegistry& textureAssetRegistry,
        const AssetsPanelController& assetsPanelController,
        const EditorCamera2D& camera,
        std::uint32_t sceneViewWidth,
        std::uint32_t sceneViewHeight,
        std::optional<Game::EntityId> currentSelectedEntityId,
        const Core::InputSnapshot& inputSnapshot)
    {
        SceneViewInteractionResult result{};

        if (true == m_lastObservedSelectedEntityId.has_value()
            && m_lastObservedSelectedEntityId != currentSelectedEntityId)
        {
            m_editMode = SceneViewEditMode::None;
            ClearEntityDrag();
        }

        if (const std::optional<SceneViewEditMode> pressedEditMode = ReadPressedEditModeKey(inputSnapshot);
            pressedEditMode.has_value())
        {
            m_editMode = (m_editMode == *pressedEditMode)
                ? SceneViewEditMode::None
                : *pressedEditMode;
            ClearEntityDrag();
        }

        if (nullptr == m_sceneViewHost || 0 == sceneViewWidth || 0 == sceneViewHeight)
        {
            ClearSceneDragPreview();
            m_lastObservedSelectedEntityId = currentSelectedEntityId;
            return result;
        }

        const POINT screenPoint = inputSnapshot.GetCursorScreenPoint();

        RECT sceneHostRect{};
        GetWindowRect(m_sceneViewHost, &sceneHostRect);

        const bool isCursorInside = screenPoint.x >= sceneHostRect.left
            && screenPoint.x < sceneHostRect.right
            && screenPoint.y >= sceneHostRect.top
            && screenPoint.y < sceneHostRect.bottom;

        const bool isLeftButtonDown = inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left);
        const bool isAssetDragActive = assetsPanelController.IsDragActive();
        const bool isScenePlacementDrag = isAssetDragActive
            && assetsPanelController.CanPlaceDraggingAssetInScene();
        if (isCursorInside && isLeftButtonDown && false == m_sceneViewLeftButtonDown)
        {
            POINT clientPoint = screenPoint;
            ScreenToClient(m_sceneViewHost, &clientPoint);

            const auto worldPoint = camera.TransformScreenToWorld(EditorScreenPoint{
                static_cast<float>(clientPoint.x),
                static_cast<float>(clientPoint.y)
            });
            m_lastSceneClickX = worldPoint.x;
            m_lastSceneClickY = worldPoint.y;
            m_hasSceneClick = true;

            if (false == isAssetDragActive && nullptr != scene)
            {
                const auto resolvedSprites = scene->ResolveSceneSprites(spriteAssetRegistry, textureAssetRegistry);
                const auto renderItems = scene->CollectSpriteRenderItems();
                const auto hitTargets = BuildSceneHitTargets(resolvedSprites, renderItems, currentSelectedEntityId);
                const auto selectedEntityId = PickTopmostEntityAtWorldPoint(hitTargets, worldPoint.x, worldPoint.y);
                if (selectedEntityId != currentSelectedEntityId)
                {
                    result.selectionChanged = true;
                    result.selectedEntityId = selectedEntityId;
                    if (true == currentSelectedEntityId.has_value())
                    {
                        m_editMode = SceneViewEditMode::None;
                    }
                }
                else if (false == selectedEntityId.has_value())
                {
                    m_editMode = SceneViewEditMode::None;
                }

                if (selectedEntityId.has_value())
                {
                    const auto entity = scene->FindEntity(*selectedEntityId);
                    if (true == entity.has_value())
                    {
                        const Game::Transform& transform = entity->get().GetTransform();
                        m_entityDragState.entityId = *selectedEntityId;
                        m_entityDragState.grabOffsetX = worldPoint.x - transform.position.x;
                        m_entityDragState.grabOffsetY = worldPoint.y - transform.position.y;
                        m_entityDragState.initialWorldX = transform.position.x;
                        m_entityDragState.initialWorldY = transform.position.y;
                        m_entityDragState.hasMoved = false;
                    }
                }
                else
                {
                    ClearEntityDrag();
                }
            }
        }

        if (false == isAssetDragActive
            && true == isLeftButtonDown
            && true == m_entityDragState.entityId.has_value()
            && (SceneViewEditMode::None == m_editMode || SceneViewEditMode::Move == m_editMode)
            && nullptr != scene)
        {
            POINT clientPoint = screenPoint;
            ScreenToClient(m_sceneViewHost, &clientPoint);

            const auto worldPoint = camera.TransformScreenToWorld(EditorScreenPoint{
                static_cast<float>(clientPoint.x),
                static_cast<float>(clientPoint.y)
            });
            const float nextWorldX = worldPoint.x - m_entityDragState.grabOffsetX;
            const float nextWorldY = worldPoint.y - m_entityDragState.grabOffsetY;
            if (SceneEditingOperations::MoveEntity(*scene, *m_entityDragState.entityId, nextWorldX, nextWorldY))
            {
                result.sceneChanged = true;
                m_entityDragState.hasMoved = true;
            }
        }

        const float wheelDirection = GetMouseWheelDirection(inputSnapshot.GetMouseWheelDelta());
        if (0.0f != wheelDirection
            && false == isAssetDragActive
            && true == currentSelectedEntityId.has_value()
            && nullptr != scene)
        {
            if (SceneViewEditMode::Scale == m_editMode)
            {
                const bool isControlDown = inputSnapshot.IsKeyDown(VK_CONTROL);
                const float scaleStep = true == isControlDown ? 0.05f : 0.1f;
                if (SceneEditingOperations::AdjustEntityUniformScale(*scene, *currentSelectedEntityId, wheelDirection * scaleStep))
                {
                    result.sceneChanged = true;
                    result.shouldPersistScene = true;
                    result.shouldPushHistory = true;
                }
            }
            else if (SceneViewEditMode::Rotate == m_editMode)
            {
                const bool isControlDown = inputSnapshot.IsKeyDown(VK_CONTROL);
                const float rotateStep = true == isControlDown ? 1.0f : 10.0f;
                if (SceneEditingOperations::AdjustEntityRotationZ(*scene, *currentSelectedEntityId, wheelDirection * rotateStep))
                {
                    result.sceneChanged = true;
                    result.shouldPersistScene = true;
                    result.shouldPushHistory = true;
                }
            }
        }

        if (isScenePlacementDrag && false == assetsPanelController.GetDraggingSpriteAssetId().IsEmpty())
        {
            if (isCursorInside)
            {
                POINT clientPoint = screenPoint;
                ScreenToClient(m_sceneViewHost, &clientPoint);

                const EditorScreenPoint previewScreenPoint{
                    static_cast<float>(clientPoint.x),
                    static_cast<float>(clientPoint.y)
                };
                const EditorWorldPoint previewWorldPoint = camera.TransformScreenToWorld(previewScreenPoint);
                UpdateSceneDragPreview(
                    assetsPanelController.GetDraggingSpriteAssetId(),
                    previewWorldPoint,
                    previewScreenPoint,
                    true,
                    sceneViewWidth,
                    sceneViewHeight,
                    spriteAssetRegistry,
                    textureAssetRegistry);
            }
            else
            {
                ClearSceneDragPreview();
            }
        }
        else
        {
            ClearSceneDragPreview();
        }

        if (assetsPanelController.WasDragReleasedThisFrame()
            && true == assetsPanelController.CanPlaceDraggingAssetInScene()
            && false == assetsPanelController.GetDraggingSpriteAssetId().IsEmpty())
        {
            if (isCursorInside)
            {
                POINT clientPoint = screenPoint;
                ScreenToClient(m_sceneViewHost, &clientPoint);

                const EditorWorldPoint worldPoint = camera.TransformScreenToWorld(EditorScreenPoint{
                    static_cast<float>(clientPoint.x),
                    static_cast<float>(clientPoint.y)
                });

                m_pendingDropState.spriteAssetId = assetsPanelController.GetDraggingSpriteAssetId();
                m_pendingDropState.worldX = worldPoint.x;
                m_pendingDropState.worldY = worldPoint.y;
                m_pendingDropState.hasPendingDrop = true;

                const std::string debugLine =
                    "Editor::SceneViewInputTracker accepted SceneView drop for Sprite AssetId '"
                    + m_pendingDropState.spriteAssetId.GetValue()
                    + "' at world position ("
                    + std::to_string(m_pendingDropState.worldX)
                    + ", "
                    + std::to_string(m_pendingDropState.worldY)
                    + ").\n";
                ::OutputDebugStringA(debugLine.c_str());
            }
            else
            {
                const std::string debugLine =
                    "Editor::SceneViewInputTracker ignored asset drop for Sprite AssetId '"
                    + assetsPanelController.GetDraggingSpriteAssetId().GetValue()
                    + "' because the cursor was outside SceneView.\n";
                ::OutputDebugStringA(debugLine.c_str());
            }

            ClearSceneDragPreview();
        }

        if (false == isLeftButtonDown && true == m_sceneViewLeftButtonDown)
        {
            if (true == m_entityDragState.entityId.has_value() && true == m_entityDragState.hasMoved)
            {
                result.sceneChanged = true;
                result.shouldPersistScene = true;
                result.shouldPushHistory = true;
            }

            ClearEntityDrag();
        }

        m_sceneViewLeftButtonDown = isLeftButtonDown;
        m_lastObservedSelectedEntityId =
            true == result.selectionChanged ? result.selectedEntityId : currentSelectedEntityId;
        return result;
    }

    const SceneDragPreviewState& SceneViewInputTracker::GetDragPreviewState() const
    {
        return m_dragPreviewState;
    }

    SceneViewEditMode SceneViewInputTracker::GetEditMode() const
    {
        return m_editMode;
    }

    const ScenePendingDropState& SceneViewInputTracker::GetPendingDropState() const
    {
        return m_pendingDropState;
    }

    void SceneViewInputTracker::ClearPendingSceneDrop()
    {
        m_pendingDropState = {};
    }

    bool SceneViewInputTracker::HasSceneClick() const
    {
        return m_hasSceneClick;
    }

    float SceneViewInputTracker::GetLastSceneClickX() const
    {
        return m_lastSceneClickX;
    }

    float SceneViewInputTracker::GetLastSceneClickY() const
    {
        return m_lastSceneClickY;
    }

    std::vector<SceneViewHitTarget> SceneViewInputTracker::BuildSceneHitTargets(
        const std::vector<Game::ResolvedSceneSprite>& resolvedSprites,
        const std::vector<Game::SceneSpriteRenderItem>& renderItems,
        std::optional<Game::EntityId> selectedEntityId) const
    {
        std::vector<SceneViewHitTarget> hitTargets;
        hitTargets.reserve(resolvedSprites.size() + renderItems.size());

        std::optional<SceneViewHitTarget> selectedTarget{};
        for (const Game::ResolvedSceneSprite& resolvedSprite : resolvedSprites)
        {
            const auto texture = resolvedSprite.sprite.GetTexture();
            if (false == static_cast<bool>(texture))
            {
                continue;
            }

            const auto& position = resolvedSprite.sprite.GetPosition();
            const auto& scale = resolvedSprite.sprite.GetScale();
            const float width = static_cast<float>(texture->GetWidth()) * std::abs(scale.x);
            const float height = static_cast<float>(texture->GetHeight()) * std::abs(scale.y);
            SceneViewHitTarget target{
                resolvedSprite.entityId,
                position.x,
                position.y,
                (std::max)(width, 1.0f),
                (std::max)(height, 1.0f)
            };

            if (true == selectedEntityId.has_value() && resolvedSprite.entityId == *selectedEntityId)
            {
                selectedTarget = target;
                continue;
            }

            hitTargets.push_back(target);
        }

        for (const Game::SceneSpriteRenderItem& renderItem : renderItems)
        {
            if (nullptr == renderItem.transform
                || nullptr == renderItem.spriteComponent
                || false == renderItem.spriteComponent->spriteAssetRef.IsEmpty())
            {
                continue;
            }

            const float width = UntexturedSpriteSizeWorldUnits * std::abs(renderItem.transform->scale.x);
            const float height = UntexturedSpriteSizeWorldUnits * std::abs(renderItem.transform->scale.y);
            SceneViewHitTarget target{
                renderItem.entityId,
                renderItem.transform->position.x,
                renderItem.transform->position.y,
                (std::max)(width, 1.0f),
                (std::max)(height, 1.0f)
            };

            if (true == selectedEntityId.has_value() && renderItem.entityId == *selectedEntityId)
            {
                selectedTarget = target;
                continue;
            }

            hitTargets.push_back(target);
        }

        if (true == selectedTarget.has_value())
        {
            hitTargets.push_back(*selectedTarget);
        }

        return hitTargets;
    }

    void SceneViewInputTracker::UpdateSceneDragPreview(
        const Core::AssetId& spriteAssetId,
        const EditorWorldPoint& worldPoint,
        const EditorScreenPoint& screenPoint,
        bool isCursorInside,
        std::uint32_t sceneViewWidth,
        std::uint32_t sceneViewHeight,
        const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
        const Graphics::TextureAssetRegistry& textureAssetRegistry)
    {
        m_dragPreviewState.isCursorInside = isCursorInside;
        if (false == isCursorInside || true == spriteAssetId.IsEmpty())
        {
            ClearSceneDragPreview();
            return;
        }

        if (m_dragPreviewState.spriteAssetId.GetValue() != spriteAssetId.GetValue() || !m_dragPreviewState.texture)
        {
            const auto spriteAsset = spriteAssetRegistry.ResolveSpriteAsset(spriteAssetId);
            if (false == spriteAsset.has_value())
            {
                ClearSceneDragPreview();
                return;
            }

            const auto previewTexture = textureAssetRegistry.ResolveTexture(spriteAsset->textureAssetId);
            if (false == static_cast<bool>(previewTexture))
            {
                ClearSceneDragPreview();
                return;
            }

            m_dragPreviewState.spriteAssetId = spriteAssetId;
            m_dragPreviewState.texture = previewTexture;
        }

        m_dragPreviewState.worldX = worldPoint.x;
        m_dragPreviewState.worldY = worldPoint.y;
        m_dragPreviewState.viewX = screenPoint.x - static_cast<float>(sceneViewWidth) * 0.5f;
        m_dragPreviewState.viewY = screenPoint.y - static_cast<float>(sceneViewHeight) * 0.5f;
        m_dragPreviewState.hasPreview = true;
        m_dragPreviewState.isCursorInside = true;
    }

    void SceneViewInputTracker::ClearSceneDragPreview()
    {
        m_dragPreviewState.spriteAssetId = {};
        m_dragPreviewState.texture.reset();
        m_dragPreviewState.worldX = 0.0f;
        m_dragPreviewState.worldY = 0.0f;
        m_dragPreviewState.viewX = 0.0f;
        m_dragPreviewState.viewY = 0.0f;
        m_dragPreviewState.hasPreview = false;
        m_dragPreviewState.isCursorInside = false;
    }

    void SceneViewInputTracker::ClearEntityDrag()
    {
        m_entityDragState = {};
    }
}
