#include "SceneViewInputTracker.h"

#include <algorithm>
#include <string>

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

namespace Xelqoria::Editor
{
    void SceneViewInputTracker::Bind(HWND sceneViewHost)
    {
        m_sceneViewHost = sceneViewHost;
    }

    SceneViewInteractionResult SceneViewInputTracker::UpdateInteraction(
        const Game::Scene* scene,
        const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
        const Graphics::TextureAssetRegistry& textureAssetRegistry,
        const AssetsPanelController& assetsPanelController,
        const EditorCamera2D& camera,
        std::uint32_t sceneViewWidth,
        std::uint32_t sceneViewHeight,
        std::optional<Game::EntityId> currentSelectedEntityId)
    {
        SceneViewInteractionResult result{};

        if (nullptr == m_sceneViewHost || 0 == sceneViewWidth || 0 == sceneViewHeight)
        {
            ClearSceneDragPreview();
            return result;
        }

        POINT screenPoint{};
        GetCursorPos(&screenPoint);

        RECT sceneHostRect{};
        GetWindowRect(m_sceneViewHost, &sceneHostRect);

        const bool isCursorInside = screenPoint.x >= sceneHostRect.left
            && screenPoint.x < sceneHostRect.right
            && screenPoint.y >= sceneHostRect.top
            && screenPoint.y < sceneHostRect.bottom;

        const bool isLeftButtonDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
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

            if (false == assetsPanelController.IsDragActive() && nullptr != scene)
            {
                const auto resolvedSprites = scene->ResolveSceneSprites(spriteAssetRegistry, textureAssetRegistry);
                const auto hitTargets = BuildSceneHitTargets(resolvedSprites, currentSelectedEntityId);
                const auto selectedEntityId = PickTopmostEntityAtWorldPoint(hitTargets, worldPoint.x, worldPoint.y);
                if (selectedEntityId != currentSelectedEntityId)
                {
                    result.selectionChanged = true;
                    result.selectedEntityId = selectedEntityId;
                }
            }
        }

        if (assetsPanelController.IsDragActive() && false == assetsPanelController.GetDraggingSpriteAssetId().IsEmpty())
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

        if (assetsPanelController.WasDragReleasedThisFrame() && false == assetsPanelController.GetDraggingSpriteAssetId().IsEmpty())
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

        m_sceneViewLeftButtonDown = isLeftButtonDown;
        return result;
    }

    const SceneDragPreviewState& SceneViewInputTracker::GetDragPreviewState() const
    {
        return m_dragPreviewState;
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
        std::optional<Game::EntityId> selectedEntityId) const
    {
        std::vector<SceneViewHitTarget> hitTargets;
        hitTargets.reserve(resolvedSprites.size());

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
}
