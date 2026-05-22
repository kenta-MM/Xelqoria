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
    namespace
    {
        [[nodiscard]] POINT ToWin32Point(Platform::Point point)
        {
            return POINT{ static_cast<LONG>(point.x), static_cast<LONG>(point.y) };
        }
    }

    void InspectorPanelController::Bind(const EditorShell& shell, Platform::ICursor& cursor)
    {
        m_inspectorSummaryLabel = shell.GetInspectorSummaryLabel();
        m_transformSectionLabel = shell.GetTransformSectionLabel();
        m_transformEditControls = shell.GetTransformEditControls();
        m_spriteComponentSectionLabel = shell.GetSpriteComponentSectionLabel();
        m_spriteRefLabel = shell.GetSpriteRefLabel();
        m_spriteRefDropHighlight = shell.GetSpriteRefDropHighlight();
        m_spriteRefEdit = shell.GetSpriteRefEdit();
        m_scriptAssetLabel = shell.GetScriptAssetLabel();
        m_scriptAssetEdit = shell.GetScriptAssetEdit();
        m_scriptCreateButton = shell.GetScriptCreateButton();
        m_scriptAssignButton = shell.GetScriptAssignButton();
        m_scriptClearButton = shell.GetScriptClearButton();
        m_spriteComponentActionButton = shell.GetSpriteComponentActionButton();
        m_materialOpenButton = shell.GetMaterialOpenButton();
        m_cursor = &cursor;
        SetWindowTextW(m_spriteRefLabel, L"Material");
    }

    void InspectorPanelController::Refresh(
        const Game::Scene* scene,
        std::optional<Game::EntityId> selectedEntityId,
        bool canAddSpriteComponent,
        const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
        const Game::Assets::IMaterialAssetResolver& materialAssetResolver,
        const std::filesystem::path& selectedAssetPath)
    {
        (void)materialAssetResolver;

        if (false == selectedEntityId.has_value() || nullptr == scene)
        {
            if (false == selectedAssetPath.empty())
            {
                const std::wstring assetName = selectedAssetPath.filename().wstring();
                std::wstring summaryText = L"Inspector: Asset ";
                summaryText += assetName.empty() ? selectedAssetPath.wstring() : assetName;
                SetWindowTextW(m_inspectorSummaryLabel, summaryText.c_str());
            }
            else
            {
                SetWindowTextW(m_inspectorSummaryLabel, L"Inspector: no entity selected");
            }
            SetWindowTextW(m_spriteRefEdit, L"");
            SetWindowTextW(m_scriptAssetEdit, L"");
            ShowWindow(m_scriptAssetLabel, SW_HIDE);
            ShowWindow(m_scriptAssetEdit, SW_HIDE);
            ShowWindow(m_scriptCreateButton, SW_HIDE);
            ShowWindow(m_scriptAssignButton, SW_HIDE);
            ShowWindow(m_scriptClearButton, SW_HIDE);
            ShowWindow(m_materialOpenButton, SW_HIDE);
            m_lastSpriteRefAssetId = {};
            m_lastSpriteRefDisplayText.clear();
            m_lastScriptAssetId = {};
            m_lastScriptDisplayText.clear();
            return;
        }

        const auto entity = scene->FindEntity(*selectedEntityId);
        if (false == entity.has_value())
        {
            SetWindowTextW(m_inspectorSummaryLabel, L"Inspector: selected entity not found");
            SetWindowTextW(m_spriteRefEdit, L"");
            SetWindowTextW(m_scriptAssetEdit, L"");
            ShowWindow(m_scriptAssetLabel, SW_HIDE);
            ShowWindow(m_scriptAssetEdit, SW_HIDE);
            ShowWindow(m_scriptCreateButton, SW_HIDE);
            ShowWindow(m_scriptAssignButton, SW_HIDE);
            ShowWindow(m_scriptClearButton, SW_HIDE);
            ShowWindow(m_materialOpenButton, SW_HIDE);
            m_lastSpriteRefAssetId = {};
            m_lastSpriteRefDisplayText.clear();
            m_lastScriptAssetId = {};
            m_lastScriptDisplayText.clear();
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
        ShowWindow(m_materialOpenButton, actionState.showSpriteRefControls ? SW_SHOW : SW_HIDE);
        EnableWindow(m_spriteComponentActionButton, actionState.enableActionButton ? TRUE : FALSE);
        SetWindowTextW(m_spriteComponentSectionLabel, actionState.sectionLabel);
        SetWindowTextW(m_spriteComponentActionButton, actionState.buttonLabel);
        if (true == hasSpriteComponent)
        {
            m_lastSpriteRefAssetId = spriteComponent->get().spriteAssetRef;
            const Core::AssetId materialAssetId = spriteComponent->get().materialAssetRef;
            const auto spriteAsset = spriteAssetResolver.ResolveSpriteAsset(m_lastSpriteRefAssetId);
            const Core::AssetId displayMaterialAssetId = false == materialAssetId.IsEmpty()
                ? materialAssetId
                : Core::AssetId{};
            m_lastSpriteRefDisplayText = FormatMaterialDisplayText(displayMaterialAssetId);
            SetWindowTextW(m_spriteRefEdit, m_lastSpriteRefDisplayText.c_str());
            EnableWindow(m_materialOpenButton, false == materialAssetId.IsEmpty() ? TRUE : FALSE);

            m_lastScriptAssetId = spriteAsset.has_value() ? spriteAsset->scriptAssetId : Core::AssetId{};
            m_lastScriptDisplayText = FormatScriptDisplayText(m_lastScriptAssetId);
            const InspectorScriptActionState scriptActionState = ComputeScriptActionState(
                true,
                m_lastSpriteRefAssetId,
                spriteAsset.has_value(),
                m_lastScriptAssetId);
            ShowWindow(m_scriptAssetLabel, scriptActionState.showScriptControls ? SW_SHOW : SW_HIDE);
            ShowWindow(m_scriptAssetEdit, scriptActionState.showScriptControls ? SW_SHOW : SW_HIDE);
            ShowWindow(m_scriptCreateButton, scriptActionState.showScriptControls ? SW_SHOW : SW_HIDE);
            ShowWindow(m_scriptAssignButton, scriptActionState.showScriptControls ? SW_SHOW : SW_HIDE);
            ShowWindow(m_scriptClearButton, scriptActionState.showScriptControls ? SW_SHOW : SW_HIDE);
            EnableWindow(m_scriptCreateButton, scriptActionState.enableCreateButton ? TRUE : FALSE);
            EnableWindow(m_scriptAssignButton, scriptActionState.enableAssignButton ? TRUE : FALSE);
            EnableWindow(m_scriptClearButton, scriptActionState.enableClearButton ? TRUE : FALSE);
            SetWindowTextW(m_scriptAssetEdit, m_lastScriptDisplayText.c_str());
        }
        else
        {
            SetWindowTextW(m_spriteRefEdit, L"");
            SetWindowTextW(m_scriptAssetEdit, L"");
            ShowWindow(m_scriptAssetLabel, SW_HIDE);
            ShowWindow(m_scriptAssetEdit, SW_HIDE);
            ShowWindow(m_scriptCreateButton, SW_HIDE);
            ShowWindow(m_scriptAssignButton, SW_HIDE);
            ShowWindow(m_scriptClearButton, SW_HIDE);
            ShowWindow(m_materialOpenButton, SW_HIDE);
            m_lastSpriteRefAssetId = {};
            m_lastSpriteRefDisplayText.clear();
            m_lastScriptAssetId = {};
            m_lastScriptDisplayText.clear();
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
        const Core::InputSnapshot& inputSnapshot,
        const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
        const Game::Assets::IMaterialAssetResolver& materialAssetResolver)
    {
        InspectorApplyResult result{};
        if (false == selectedEntityId.has_value() || nullptr == scene)
        {
            return result;
        }

        if (m_lastInspectorEntityId != selectedEntityId)
        {
            Refresh(scene, selectedEntityId, canAddSpriteComponent, spriteAssetResolver, materialAssetResolver);
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
                    if (index < 3)
                    {
                        result.operationName = "Move Entity";
                    }
                    else if (index < 6)
                    {
                        result.operationName = "Rotate Entity";
                    }
                    else
                    {
                        result.operationName = "Scale Entity";
                    }
                }
            }
        }

        if (true == ConsumeButtonClick(m_spriteComponentActionButton, inputSnapshot))
        {
            const bool hadSpriteComponent = entity->get().HasSpriteComponent();
            const bool actionChanged = ApplySpriteComponentAction(entity->get(), canAddSpriteComponent);
            result.changed = actionChanged || result.changed;
            if (actionChanged)
            {
                result.operationName = hadSpriteComponent ? "Remove SpriteComponent" : "Add SpriteComponent";
            }
            if (false == entity->get().HasSpriteComponent())
            {
                SetWindowTextW(m_spriteRefEdit, L"");
            }
        }

        auto spriteComponent = entity->get().GetSpriteComponent();
        if (true == spriteComponent.has_value())
        {
            const auto spriteAsset = spriteAssetResolver.ResolveSpriteAsset(spriteComponent->get().spriteAssetRef);
            const Core::AssetId scriptAssetId = spriteAsset.has_value() ? spriteAsset->scriptAssetId : Core::AssetId{};
            const InspectorScriptActionState scriptActionState = ComputeScriptActionState(
                true,
                spriteComponent->get().spriteAssetRef,
                spriteAsset.has_value(),
                scriptAssetId);

            if (scriptActionState.enableCreateButton && true == ConsumeButtonClick(m_scriptCreateButton, inputSnapshot))
            {
                result.scriptAction = InspectorScriptAction::Create;
                result.scriptTargetSpriteAssetId = spriteComponent->get().spriteAssetRef;
            }
            else if (scriptActionState.enableAssignButton && true == ConsumeButtonClick(m_scriptAssignButton, inputSnapshot))
            {
                result.scriptAction = InspectorScriptAction::Assign;
                result.scriptTargetSpriteAssetId = spriteComponent->get().spriteAssetRef;
            }
            else if (scriptActionState.enableClearButton && true == ConsumeButtonClick(m_scriptClearButton, inputSnapshot))
            {
                result.scriptAction = InspectorScriptAction::Clear;
                result.scriptTargetSpriteAssetId = spriteComponent->get().spriteAssetRef;
            }

            if (false == spriteComponent->get().materialAssetRef.IsEmpty()
                && true == ConsumeButtonClick(m_materialOpenButton, inputSnapshot))
            {
                result.openMaterialRequested = true;
                result.openMaterialAssetId = spriteComponent->get().materialAssetRef;
            }
        }

        if (result.changed)
        {
            Refresh(scene, selectedEntityId, canAddSpriteComponent, spriteAssetResolver, materialAssetResolver);
        }

        FinishButtonClickTracking(inputSnapshot);
        return result;
    }

    InspectorApplyResult InspectorPanelController::ApplyMaterialDrop(
        Game::Scene* scene,
        std::optional<Game::EntityId> selectedEntityId,
        const AssetsPanelController& assetsPanelController,
        const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
        const Game::Assets::IMaterialAssetResolver& materialAssetResolver)
    {
        InspectorApplyResult result{};
        if (nullptr == scene
            || false == selectedEntityId.has_value()
            || true == assetsPanelController.GetDraggingMaterialAssetId().IsEmpty()
            || false == assetsPanelController.WasDragReleasedThisFrame()
            || false == IsMaterialDropTargetHovered(assetsPanelController))
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

        const Core::AssetId droppedMaterialAssetId = assetsPanelController.GetDraggingMaterialAssetId();
        if (spriteComponent->get().materialAssetRef == droppedMaterialAssetId)
        {
            return result;
        }

        spriteComponent->get().materialAssetRef = droppedMaterialAssetId;
        result.changed = true;
        result.operationName = "Change Sprite Material";
        Refresh(scene, selectedEntityId, true, spriteAssetResolver, materialAssetResolver);
        return result;
    }

    InspectorApplyResult InspectorPanelController::ApplyScriptDrop(
        const Game::Scene* scene,
        std::optional<Game::EntityId> selectedEntityId,
        const AssetsPanelController& assetsPanelController) const
    {
        InspectorApplyResult result{};
        if (nullptr == scene
            || false == selectedEntityId.has_value()
            || true == assetsPanelController.GetDraggingScriptAssetId().IsEmpty()
            || true == assetsPanelController.GetDraggingScriptAssetPath().empty()
            || false == assetsPanelController.WasDragReleasedThisFrame()
            || false == IsScriptDropTargetHovered(assetsPanelController))
        {
            return result;
        }

        const auto entity = scene->FindEntity(*selectedEntityId);
        if (false == entity.has_value())
        {
            return result;
        }

        const auto spriteComponent = entity->get().GetSpriteComponent();
        if (false == spriteComponent.has_value()
            || true == spriteComponent->get().spriteAssetRef.IsEmpty())
        {
            return result;
        }

        result.scriptAction = InspectorScriptAction::AssignDropped;
        result.scriptTargetSpriteAssetId = spriteComponent->get().spriteAssetRef;
        result.droppedScriptAssetPath = assetsPanelController.GetDraggingScriptAssetPath();
        return result;
    }

    void InspectorPanelController::UpdateDropHighlight(const AssetsPanelController& assetsPanelController)
    {
        HWND targetEdit = nullptr;
        if (true == IsMaterialDropTargetHovered(assetsPanelController))
        {
            targetEdit = m_spriteRefEdit;
        }
        else if (true == IsScriptDropTargetHovered(assetsPanelController))
        {
            targetEdit = m_scriptAssetEdit;
        }

        const bool shouldShow = nullptr != targetEdit;
        if (shouldShow == m_isDropHighlightVisible && targetEdit == m_dropHighlightTargetEdit)
        {
            return;
        }

        m_isDropHighlightVisible = shouldShow;
        m_dropHighlightTargetEdit = targetEdit;
        if (nullptr == m_spriteRefDropHighlight)
        {
            return;
        }

        ShowWindow(m_spriteRefDropHighlight, shouldShow ? SW_SHOW : SW_HIDE);
        if (shouldShow)
        {
            RECT targetRect{};
            HWND parentWindow = GetParent(m_spriteRefDropHighlight);
            GetWindowRect(targetEdit, &targetRect);
            MapWindowPoints(nullptr, parentWindow, reinterpret_cast<POINT*>(&targetRect), 2);
            InflateRect(&targetRect, 2, 2);
            SetWindowPos(
                m_spriteRefDropHighlight,
                targetEdit,
                targetRect.left,
                targetRect.top,
                targetRect.right - targetRect.left,
                targetRect.bottom - targetRect.top,
                SWP_NOACTIVATE);
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

        const POINT cursorScreenPoint = ToWin32Point(inputSnapshot.GetCursorScreenPoint());

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
        }

        return clicked;
    }

    void InspectorPanelController::FinishButtonClickTracking(const Core::InputSnapshot& inputSnapshot)
    {
        const bool isLeftMouseButtonDown = inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left);
        if (false == isLeftMouseButtonDown && true == m_wasLeftMouseButtonDown)
        {
            m_pressedButtonHandle = nullptr;
        }

        m_wasLeftMouseButtonDown = isLeftMouseButtonDown;
    }

    bool InspectorPanelController::IsMaterialDropTargetHovered(const AssetsPanelController& assetsPanelController) const
    {
        const bool hasMaterialDrag = assetsPanelController.IsDragActive()
            || assetsPanelController.WasDragReleasedThisFrame();
        if (false == hasMaterialDrag
            || true == assetsPanelController.GetDraggingMaterialAssetId().IsEmpty()
            || nullptr == m_spriteRefEdit
            || FALSE == IsWindowVisible(m_spriteRefEdit))
        {
            return false;
        }

        if (nullptr == m_cursor)
        {
            return false;
        }

        const POINT cursorPoint = ToWin32Point(m_cursor->GetScreenPosition());

        RECT materialRect{};
        GetWindowRect(m_spriteRefEdit, &materialRect);
        return PtInRect(&materialRect, cursorPoint) != FALSE;
    }

    bool InspectorPanelController::IsScriptDropTargetHovered(const AssetsPanelController& assetsPanelController) const
    {
        const bool hasScriptDrag = assetsPanelController.IsDragActive()
            || assetsPanelController.WasDragReleasedThisFrame();
        if (false == hasScriptDrag
            || true == assetsPanelController.GetDraggingScriptAssetId().IsEmpty()
            || nullptr == m_scriptAssetEdit
            || nullptr == m_scriptAssignButton
            || false == IsWindowVisible(m_scriptAssetEdit)
            || false == IsWindowEnabled(m_scriptAssetEdit)
            || false == IsWindowEnabled(m_scriptAssignButton))
        {
            return false;
        }

        if (nullptr == m_cursor)
        {
            return false;
        }

        const POINT cursorPoint = ToWin32Point(m_cursor->GetScreenPosition());

        RECT scriptRect{};
        GetWindowRect(m_scriptAssetEdit, &scriptRect);
        return PtInRect(&scriptRect, cursorPoint) != FALSE;
    }
}
