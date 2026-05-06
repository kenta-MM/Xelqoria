#include "InspectorPanelController.h"

#include <string>

#include "EditorStringUtils.h"
#include <Windows.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <AssetId.h>
#include "AssetsPanelController.h"
#include "EditorShell.h"
#include <Entity.h>
#include <Scene.h>

namespace Xelqoria::Editor
{
    void InspectorPanelController::Bind(const EditorShell& shell)
    {
        m_inspectorSummaryLabel = shell.GetInspectorSummaryLabel();
        m_transformSectionLabel = shell.GetTransformSectionLabel();
        m_transformEditControls = shell.GetTransformEditControls();
        m_spriteComponentSectionLabel = shell.GetSpriteComponentSectionLabel();
        m_spriteRefLabel = shell.GetSpriteRefLabel();
        m_spriteRefDropHighlight = shell.GetSpriteRefDropHighlight();
        m_spriteRefEdit = shell.GetSpriteRefEdit();
        m_spriteComponentActionButton = shell.GetSpriteComponentActionButton();
    }

    void InspectorPanelController::Refresh(
        const Game::Scene* scene,
        std::optional<Game::EntityId> selectedEntityId,
        bool canAddSpriteComponent)
    {
        if (false == selectedEntityId.has_value() || nullptr == scene)
        {
            SetWindowTextW(m_inspectorSummaryLabel, L"Inspector: no entity selected");
            SetWindowTextW(m_spriteRefEdit, L"");
            m_lastSpriteRefAssetId = {};
            m_lastSpriteRefDisplayText.clear();
            return;
        }

        const auto entity = scene->FindEntity(*selectedEntityId);
        if (false == entity.has_value())
        {
            SetWindowTextW(m_inspectorSummaryLabel, L"Inspector: selected entity not found");
            SetWindowTextW(m_spriteRefEdit, L"");
            m_lastSpriteRefAssetId = {};
            m_lastSpriteRefDisplayText.clear();
            return;
        }

        const auto& transform = entity->get().GetTransform();
        const float values[9] = {
            transform.position.x,
            transform.position.y,
            transform.position.z,
            transform.rotation.x,
            transform.rotation.y,
            transform.rotation.z,
            transform.scale.x,
            transform.scale.y,
            transform.scale.z
        };

        for (std::size_t index = 0; index < m_transformEditControls.size(); ++index)
        {
            wchar_t valueText[32]{};
            std::swprintf(valueText, std::size(valueText), L"%.3f", values[index]);
            SetWindowTextW(m_transformEditControls[index], valueText);
        }

        const auto spriteComponent = entity->get().GetSpriteComponent();
        const bool hasSpriteComponent = spriteComponent.has_value();
        const InspectorSpriteComponentActionState actionState =
            ComputeSpriteComponentActionState(hasSpriteComponent, canAddSpriteComponent);
        ShowWindow(m_spriteRefLabel, actionState.showSpriteRefControls ? SW_SHOW : SW_HIDE);
        ShowWindow(m_spriteRefEdit, actionState.showSpriteRefControls ? SW_SHOW : SW_HIDE);
        EnableWindow(m_spriteComponentActionButton, actionState.enableActionButton ? TRUE : FALSE);
        SetWindowTextW(m_spriteComponentSectionLabel, actionState.sectionLabel);
        SetWindowTextW(m_spriteComponentActionButton, actionState.buttonLabel);
        if (true == hasSpriteComponent)
        {
            m_lastSpriteRefAssetId = spriteComponent->get().spriteAssetRef;
            m_lastSpriteRefDisplayText = FormatTextureDisplayText(m_lastSpriteRefAssetId);
            SetWindowTextW(m_spriteRefEdit, m_lastSpriteRefDisplayText.c_str());
        }
        else
        {
            SetWindowTextW(m_spriteRefEdit, L"");
            m_lastSpriteRefAssetId = {};
            m_lastSpriteRefDisplayText.clear();
        }

        wchar_t summaryText[128]{};
        std::swprintf(
            summaryText,
            std::size(summaryText),
            L"Inspector: Entity %u",
            static_cast<unsigned>(*selectedEntityId));
        SetWindowTextW(m_inspectorSummaryLabel, summaryText);
        m_lastInspectorEntityId = selectedEntityId;
    }

    InspectorApplyResult InspectorPanelController::ApplyEdits(
        Game::Scene* scene,
        std::optional<Game::EntityId> selectedEntityId,
        bool canAddSpriteComponent,
        const Core::InputSnapshot& inputSnapshot)
    {
        InspectorApplyResult result{};
        if (false == selectedEntityId.has_value() || nullptr == scene)
        {
            return result;
        }

        if (m_lastInspectorEntityId != selectedEntityId)
        {
            Refresh(scene, selectedEntityId, canAddSpriteComponent);
        }

        const auto entity = scene->FindEntity(*selectedEntityId);
        if (false == entity.has_value())
        {
            return result;
        }

        float* transformValues[9] = {
            &entity->get().GetTransform().position.x,
            &entity->get().GetTransform().position.y,
            &entity->get().GetTransform().position.z,
            &entity->get().GetTransform().rotation.x,
            &entity->get().GetTransform().rotation.y,
            &entity->get().GetTransform().rotation.z,
            &entity->get().GetTransform().scale.x,
            &entity->get().GetTransform().scale.y,
            &entity->get().GetTransform().scale.z
        };

        for (std::size_t index = 0; index < m_transformEditControls.size(); ++index)
        {
            wchar_t buffer[64]{};
            GetWindowTextW(m_transformEditControls[index], buffer, static_cast<int>(std::size(buffer)));

            wchar_t* end = nullptr;
            const float parsed = std::wcstof(buffer, &end);
            if (end != buffer && *end == L'\0')
            {
                if (*transformValues[index] != parsed)
                {
                    *transformValues[index] = parsed;
                    result.changed = true;
                }
            }
        }

        if (true == ConsumeButtonClick(m_spriteComponentActionButton, inputSnapshot))
        {
            result.changed = ApplySpriteComponentAction(entity->get(), canAddSpriteComponent) || result.changed;
            if (false == entity->get().HasSpriteComponent())
            {
                SetWindowTextW(m_spriteRefEdit, L"");
            }
        }

        if (result.changed)
        {
            Refresh(scene, selectedEntityId, canAddSpriteComponent);
        }

        return result;
    }

    InspectorApplyResult InspectorPanelController::ApplyTextureDrop(
        Game::Scene* scene,
        std::optional<Game::EntityId> selectedEntityId,
        const AssetsPanelController& assetsPanelController)
    {
        InspectorApplyResult result{};
        if (nullptr == scene
            || false == selectedEntityId.has_value()
            || true == assetsPanelController.GetDraggingSpriteAssetId().IsEmpty()
            || false == assetsPanelController.WasDragReleasedThisFrame()
            || false == IsTextureDropTargetHovered(assetsPanelController))
        {
            return result;
        }

        const auto entity = scene->FindEntity(*selectedEntityId);
        if (false == entity.has_value())
        {
            return result;
        }

        auto spriteComponent = entity->get().GetSpriteComponent();
        if (false == spriteComponent.has_value())
        {
            return result;
        }

        const Core::AssetId droppedSpriteAssetId = assetsPanelController.GetDraggingSpriteAssetId();
        if (spriteComponent->get().spriteAssetRef == droppedSpriteAssetId)
        {
            return result;
        }

        spriteComponent->get().spriteAssetRef = droppedSpriteAssetId;
        result.changed = true;
        Refresh(scene, selectedEntityId, true);
        return result;
    }

    void InspectorPanelController::UpdateTextureDropHighlight(const AssetsPanelController& assetsPanelController)
    {
        const bool shouldShow = IsTextureDropTargetHovered(assetsPanelController);
        if (shouldShow == m_isTextureDropHighlightVisible)
        {
            return;
        }

        m_isTextureDropHighlightVisible = shouldShow;
        if (nullptr == m_spriteRefDropHighlight)
        {
            return;
        }

        ShowWindow(m_spriteRefDropHighlight, shouldShow ? SW_SHOW : SW_HIDE);
        if (shouldShow)
        {
            SetWindowPos(
                m_spriteRefDropHighlight,
                m_spriteRefEdit,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            InvalidateRect(m_spriteRefDropHighlight, nullptr, TRUE);
        }
    }

    void InspectorPanelController::ResetTrackedEntity()
    {
        m_lastInspectorEntityId.reset();
    }

    bool InspectorPanelController::ConsumeButtonClick(
        HWND buttonHandle,
        const Core::InputSnapshot& inputSnapshot)
    {
        if (nullptr == buttonHandle || false == IsWindowVisible(buttonHandle) || false == IsWindowEnabled(buttonHandle))
        {
            return false;
        }

        const POINT cursorScreenPoint = inputSnapshot.GetCursorScreenPoint();

        RECT buttonRect{};
        GetWindowRect(buttonHandle, &buttonRect);
        const bool isCursorInsideButton = PtInRect(&buttonRect, cursorScreenPoint) != FALSE;
        const bool isLeftMouseButtonDown = inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left);

        if (isLeftMouseButtonDown && false == m_wasLeftMouseButtonDown && true == isCursorInsideButton)
        {
            m_pressedButtonHandle = buttonHandle;
        }

        bool clicked = false;
        if (false == isLeftMouseButtonDown && true == m_wasLeftMouseButtonDown)
        {
            clicked = m_pressedButtonHandle == buttonHandle && true == isCursorInsideButton;
            m_pressedButtonHandle = nullptr;
        }

        m_wasLeftMouseButtonDown = isLeftMouseButtonDown;
        return clicked;
    }

    bool InspectorPanelController::IsTextureDropTargetHovered(const AssetsPanelController& assetsPanelController) const
    {
        const bool hasTextureDrag = assetsPanelController.IsDragActive()
            || assetsPanelController.WasDragReleasedThisFrame();
        if (false == hasTextureDrag
            || true == assetsPanelController.GetDraggingSpriteAssetId().IsEmpty()
            || nullptr == m_spriteRefEdit
            || false == IsWindowVisible(m_spriteRefEdit))
        {
            return false;
        }

        POINT cursorPoint{};
        GetCursorPos(&cursorPoint);

        RECT textureRect{};
        GetWindowRect(m_spriteRefEdit, &textureRect);
        return PtInRect(&textureRect, cursorPoint) != FALSE;
    }
}
