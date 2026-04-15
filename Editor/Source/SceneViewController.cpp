#include "SceneViewController.h"

#include <algorithm>
#include <string>

#include "EditorStringUtils.h"
#include "SceneSerializer.h"
#include "SpriteComponent.h"
#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <vector>
#include <AssetId.h>
#include "AssetsPanelController.h"
#include "EditorCamera2D.h"
#include "EditorSceneDocument.h"
#include "EditorShell.h"
#include "SceneViewOverlay.h"
#include <Assets/SpriteAssetRegistry.h>
#include <Entity.h>
#include <Scene.h>
#include <TextureAssetRegistry.h>

namespace Xelqoria::Editor
{
    void SceneViewController::Bind(const EditorShell& shell)
    {
        m_sceneViewHost = shell.GetSceneViewHost();
        m_sceneViewPlanLabel = shell.GetSceneViewPlanLabel();
        m_sceneViewSizeLabel = shell.GetSceneViewSizeLabel();
    }

    void SceneViewController::OnViewportChanged(std::uint32_t width, std::uint32_t height)
    {
        m_sceneViewWidth = width;
        m_sceneViewHeight = height;
        m_sceneViewCamera.SetViewport(m_sceneViewWidth, m_sceneViewHeight);
    }

    SceneViewInteractionResult SceneViewController::UpdateInteraction(
        const Game::Scene* scene,
        const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
        const Graphics::TextureAssetRegistry& textureAssetRegistry,
        const AssetsPanelController& assetsPanelController,
        std::optional<Game::EntityId> currentSelectedEntityId)
    {
        SceneViewInteractionResult result{};

        if (nullptr == m_sceneViewHost || 0 == m_sceneViewWidth || 0 == m_sceneViewHeight)
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

            const auto worldPoint = m_sceneViewCamera.TransformScreenToWorld(EditorScreenPoint{
                static_cast<float>(clientPoint.x),
                static_cast<float>(clientPoint.y)
            });
            m_lastSceneClickX = worldPoint.x;
            m_lastSceneClickY = worldPoint.y;
            m_hasSceneClick = true;

            if (false == assetsPanelController.IsDragActive() && nullptr != scene)
            {
                const auto hitTargets = BuildSceneHitTargets(*scene, spriteAssetRegistry, textureAssetRegistry, currentSelectedEntityId);
                const auto selectedEntityId = PickTopmostEntityAtWorldPoint(hitTargets, worldPoint.x, worldPoint.y);
                if (selectedEntityId != currentSelectedEntityId)
                {
                    result.selectionChanged = true;
                    result.selectedEntityId = selectedEntityId;
                    currentSelectedEntityId = selectedEntityId;
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
                const EditorWorldPoint previewWorldPoint = m_sceneViewCamera.TransformScreenToWorld(previewScreenPoint);
                UpdateSceneDragPreview(
                    assetsPanelController.GetDraggingSpriteAssetId(),
                    previewWorldPoint,
                    previewScreenPoint,
                    true,
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

                const EditorWorldPoint worldPoint = m_sceneViewCamera.TransformScreenToWorld(EditorScreenPoint{
                    static_cast<float>(clientPoint.x),
                    static_cast<float>(clientPoint.y)
                });

                m_pendingDroppedSpriteAssetId = assetsPanelController.GetDraggingSpriteAssetId();
                m_pendingDropWorldX = worldPoint.x;
                m_pendingDropWorldY = worldPoint.y;
                m_hasPendingSceneDrop = true;

                const std::string debugLine =
                    "Editor::SceneViewController accepted SceneView drop for Sprite AssetId '"
                    + m_pendingDroppedSpriteAssetId.GetValue()
                    + "' at world position ("
                    + std::to_string(m_pendingDropWorldX)
                    + ", "
                    + std::to_string(m_pendingDropWorldY)
                    + ").\n";
                ::OutputDebugStringA(debugLine.c_str());
            }
            else
            {
                const std::string debugLine =
                    "Editor::SceneViewController ignored asset drop for Sprite AssetId '"
                    + assetsPanelController.GetDraggingSpriteAssetId().GetValue()
                    + "' because the cursor was outside SceneView.\n";
                ::OutputDebugStringA(debugLine.c_str());
            }

            ClearSceneDragPreview();
        }

        m_sceneViewLeftButtonDown = isLeftButtonDown;

        wchar_t statusText[160]{};
        if (m_hasPendingSceneDrop && false == m_pendingDroppedSpriteAssetId.IsEmpty())
        {
            const std::wstring assetId = ToWideString(m_pendingDroppedSpriteAssetId.GetValue());
            std::swprintf(
                statusText,
                std::size(statusText),
                L"SceneView size: %u x %u / drop: %ls @ (%.1f, %.1f)",
                m_sceneViewWidth,
                m_sceneViewHeight,
                assetId.c_str(),
                m_pendingDropWorldX,
                m_pendingDropWorldY);
        }
        else if (m_dragPreviewState.hasPreview
            && m_dragPreviewState.isCursorInside
            && false == m_dragPreviewState.spriteAssetId.IsEmpty())
        {
            const std::wstring previewAssetId = ToWideString(m_dragPreviewState.spriteAssetId.GetValue());
            std::swprintf(
                statusText,
                std::size(statusText),
                L"SceneView size: %u x %u / drag preview: %ls @ (%.1f, %.1f)",
                m_sceneViewWidth,
                m_sceneViewHeight,
                previewAssetId.c_str(),
                m_dragPreviewState.worldX,
                m_dragPreviewState.worldY);
        }
        else if (currentSelectedEntityId.has_value() && nullptr != scene)
        {
            const auto entity = scene->FindEntity(*currentSelectedEntityId);
            if (entity.has_value())
            {
                std::swprintf(
                    statusText,
                    std::size(statusText),
                    L"SceneView size: %u x %u / selected: Entity %u @ (%.1f, %.1f)",
                    m_sceneViewWidth,
                    m_sceneViewHeight,
                    static_cast<unsigned>(*currentSelectedEntityId),
                    entity->get().GetTransform().position.x,
                    entity->get().GetTransform().position.y);
            }
        }
        else if (m_hasSceneClick)
        {
            std::swprintf(
                statusText,
                std::size(statusText),
                L"SceneView size: %u x %u / click: (%.1f, %.1f)",
                m_sceneViewWidth,
                m_sceneViewHeight,
                m_lastSceneClickX,
                m_lastSceneClickY);
        }
        else
        {
            std::swprintf(
                statusText,
                std::size(statusText),
                L"SceneView size: %u x %u / click: waiting",
                m_sceneViewWidth,
                m_sceneViewHeight);
        }
        SetWindowTextW(m_sceneViewSizeLabel, statusText);
        SetWindowTextW(
            m_sceneViewPlanLabel,
            m_hasPendingSceneDrop
                ? L"SceneView はドロップを受理済みです。次段で Entity 生成へ入力を引き渡します。"
                : (m_dragPreviewState.hasPreview && m_dragPreviewState.isCursorInside)
                    ? L"SceneView 上でドラッグ配置プレビューを表示中です。ドロップ位置と表示サイズを確認できます。"
                    : L"SceneView にはグリッド、原点、選択フィードバックを重ねて表示しています。");

        return result;
    }

    SceneViewDropResult SceneViewController::ProcessPendingSceneDrop(EditorSceneDocument& document)
    {
        SceneViewDropResult result{};
        Game::Scene* scene = document.GetScene();
        if (false == m_hasPendingSceneDrop || nullptr == scene)
        {
            return result;
        }

        const Core::AssetId droppedAssetId = m_pendingDroppedSpriteAssetId;
        const float dropWorldX = m_pendingDropWorldX;
        const float dropWorldY = m_pendingDropWorldY;

        m_hasPendingSceneDrop = false;
        m_pendingDroppedSpriteAssetId = {};

        if (true == droppedAssetId.IsEmpty())
        {
            ::OutputDebugStringA("Editor::SceneViewController could not create an entity because drop payload AssetId was empty.\n");
            SetWindowTextW(m_sceneViewPlanLabel, L"SceneView のドロップ入力に AssetId が含まれていないため配置を中止しました。");
            return result;
        }

        const auto spriteAsset = document.GetSpriteAssetRegistry().ResolveSpriteAsset(droppedAssetId);
        if (false == spriteAsset.has_value())
        {
            const std::string debugLine =
                "Editor::SceneViewController could not resolve dropped Sprite AssetId '" + droppedAssetId.GetValue() + "'.\n";
            ::OutputDebugStringA(debugLine.c_str());
            SetWindowTextW(m_sceneViewPlanLabel, L"ドロップされた Sprite AssetId を解決できないため配置を中止しました。");
            return result;
        }

        if (false == static_cast<bool>(document.GetTextureAssetRegistry().ResolveTexture(spriteAsset->textureAssetId)))
        {
            const std::string debugLine =
                "Editor::SceneViewController could not resolve Texture AssetId '"
                + spriteAsset->textureAssetId.GetValue()
                + "' for dropped Sprite AssetId '"
                + droppedAssetId.GetValue()
                + "'.\n";
            ::OutputDebugStringA(debugLine.c_str());
            SetWindowTextW(m_sceneViewPlanLabel, L"ドロップされた Sprite の Texture を解決できないため配置を中止しました。");
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
            ::OutputDebugStringA("Editor::SceneViewController failed to reload dropped scene placement snapshot.\n");
            SetWindowTextW(m_sceneViewPlanLabel, L"Entity は生成されましたが、保存/再読込の確認に失敗しました。");
            return result;
        }

        document.ReplaceScene(std::make_unique<Game::Scene>(*loadResult.scene));

        if (false == document.Save())
        {
            SetWindowTextW(m_sceneViewPlanLabel, L"Scene は再読込できましたが、保存ファイルへの書き出しに失敗しました。");
            return result;
        }

        result.shouldPushHistory = true;

        wchar_t statusText[160]{};
        std::swprintf(
            statusText,
            std::size(statusText),
            L"SceneView size: %u x %u / reloaded Entity %u",
            m_sceneViewWidth,
            m_sceneViewHeight,
            static_cast<unsigned>(createdEntityId));
        SetWindowTextW(m_sceneViewSizeLabel, statusText);
        SetWindowTextW(m_sceneViewPlanLabel, L"SceneView ドロップで生成した Entity を保存テキストへ反映し、再読込後も選択を維持しました。");

        const std::string debugLine =
            "Editor::SceneViewController created entity "
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

    void SceneViewController::RefreshSelectionStatus(const Game::Scene* scene, std::optional<Game::EntityId> selectedEntityId)
    {
        if (false == selectedEntityId.has_value() || nullptr == scene)
        {
            return;
        }

        const auto entity = scene->FindEntity(*selectedEntityId);
        if (false == entity.has_value())
        {
            return;
        }

        wchar_t statusText[160]{};
        std::swprintf(
            statusText,
            std::size(statusText),
            L"SceneView size: %u x %u / selected: Entity %u @ (%.1f, %.1f)",
            m_sceneViewWidth,
            m_sceneViewHeight,
            static_cast<unsigned>(*selectedEntityId),
            entity->get().GetTransform().position.x,
            entity->get().GetTransform().position.y);
        SetWindowTextW(m_sceneViewSizeLabel, statusText);
    }

    const EditorCamera2D& SceneViewController::GetCamera() const
    {
        return m_sceneViewCamera;
    }

    const SceneDragPreviewState& SceneViewController::GetDragPreviewState() const
    {
        return m_dragPreviewState;
    }

    std::vector<SceneViewHitTarget> SceneViewController::BuildSceneHitTargets(
        const Game::Scene& scene,
        const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
        const Graphics::TextureAssetRegistry& textureAssetRegistry,
        std::optional<Game::EntityId> selectedEntityId) const
    {
        std::vector<SceneViewHitTarget> hitTargets;
        const auto renderItems = scene.CollectSpriteRenderItems();
        hitTargets.reserve(renderItems.size());

        std::optional<SceneViewHitTarget> selectedTarget{};
        for (const Game::SceneSpriteRenderItem& renderItem : renderItems)
        {
            if (nullptr == renderItem.transform || nullptr == renderItem.spriteComponent)
            {
                continue;
            }

            const auto spriteAsset = spriteAssetRegistry.ResolveSpriteAsset(renderItem.spriteComponent->spriteAssetRef);
            if (false == spriteAsset.has_value())
            {
                continue;
            }

            const auto texture = textureAssetRegistry.ResolveTexture(spriteAsset->textureAssetId);
            if (false == static_cast<bool>(texture))
            {
                continue;
            }

            const float width = static_cast<float>(texture->GetWidth()) * std::abs(renderItem.transform->scale.x);
            const float height = static_cast<float>(texture->GetHeight()) * std::abs(renderItem.transform->scale.y);
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

    void SceneViewController::UpdateSceneDragPreview(
        const Core::AssetId& spriteAssetId,
        const EditorWorldPoint& worldPoint,
        const EditorScreenPoint& screenPoint,
        bool isCursorInside,
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
        m_dragPreviewState.viewX = screenPoint.x - static_cast<float>(m_sceneViewWidth) * 0.5f;
        m_dragPreviewState.viewY = screenPoint.y - static_cast<float>(m_sceneViewHeight) * 0.5f;
        m_dragPreviewState.hasPreview = true;
        m_dragPreviewState.isCursorInside = true;
    }

    void SceneViewController::ClearSceneDragPreview()
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
