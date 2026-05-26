#include "InspectorPanelController.h"

#include <string>

#include "EditorStringUtils.h"
#include <Windows.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cwctype>
#include <iterator>
#include <optional>
#include <AssetId.h>
#include <Collider2DComponent.h>
#include "AssetsPanelController.h"
#include "EditorShell.h"
#include <Entity.h>
#include "MaterialPanelController.h"
#include <Scene.h>

namespace Xelqoria::Editor
{
    namespace
    {
        [[nodiscard]] POINT ToWin32Point(Platform::Point point)
        {
            return POINT{ static_cast<LONG>(point.x), static_cast<LONG>(point.y) };
        }

        void SetWindowVisible(HWND window, bool visible)
        {
            ShowWindow(window, visible ? SW_SHOW : SW_HIDE);
        }

        void SetEditFloat(HWND editControl, float value)
        {
            wchar_t valueText[32]{};
            std::swprintf(valueText, std::size(valueText), L"%.3f", value);
            SetWindowTextW(editControl, valueText);
        }

        std::optional<float> ReadEditFloat(HWND editControl)
        {
            wchar_t buffer[64]{};
            GetWindowTextW(editControl, buffer, static_cast<int>(std::size(buffer)));

            wchar_t* end = nullptr;
            const float parsed = std::wcstof(buffer, &end);
            if (end == buffer || *end != L'\0')
            {
                return std::nullopt;
            }

            return parsed;
        }

        [[nodiscard]] std::wstring_view Trim(std::wstring_view value)
        {
            const auto isSpace =
                [](wchar_t character)
                {
                    return 0 != std::iswspace(character);
                };

            while (false == value.empty() && isSpace(value.front()))
            {
                value.remove_prefix(1);
            }

            while (false == value.empty() && isSpace(value.back()))
            {
                value.remove_suffix(1);
            }

            return value;
        }

        [[nodiscard]] std::wstring GetWindowTextString(HWND window)
        {
            std::array<wchar_t, 512> buffer{};
            if (nullptr == window)
            {
                return {};
            }

            GetWindowTextW(window, buffer.data(), static_cast<int>(buffer.size()));
            return std::wstring(buffer.data());
        }

        void SetCollider2DControlsVisible(
            HWND enabledCheckBox,
            HWND triggerCheckBox,
            HWND shapeTypeLabel,
            HWND shapeTypeEdit,
            HWND offsetLabel,
            HWND sizeLabel,
            const std::array<HWND, 4>& editControls,
            HWND rotationLabel,
            HWND rotationEdit,
            HWND editButton,
            bool visible)
        {
            SetWindowVisible(enabledCheckBox, visible);
            SetWindowVisible(triggerCheckBox, visible);
            SetWindowVisible(shapeTypeLabel, visible);
            SetWindowVisible(shapeTypeEdit, visible);
            SetWindowVisible(offsetLabel, visible);
            SetWindowVisible(sizeLabel, visible);
            SetWindowVisible(rotationLabel, visible);
            SetWindowVisible(rotationEdit, visible);
            SetWindowVisible(editButton, visible);
            for (HWND editControl : editControls)
            {
                SetWindowVisible(editControl, visible);
            }
        }

        void SetSpriteAssetControlsVisible(
            HWND spriteRefLabel,
            HWND spriteRefEdit,
            HWND materialOpenButton,
            HWND scriptAssetLabel,
            HWND scriptAssetEdit,
            HWND scriptCreateButton,
            HWND scriptAssignButton,
            HWND scriptClearButton,
            bool visible)
        {
            SetWindowVisible(spriteRefLabel, visible);
            SetWindowVisible(spriteRefEdit, visible);
            SetWindowVisible(materialOpenButton, visible);
            SetWindowVisible(scriptAssetLabel, visible);
            SetWindowVisible(scriptAssetEdit, visible);
            SetWindowVisible(scriptCreateButton, visible);
            SetWindowVisible(scriptAssignButton, visible);
            SetWindowVisible(scriptClearButton, visible);
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
        m_collider2DComponentSectionLabel = shell.GetCollider2DComponentSectionLabel();
        m_collider2DSummaryLabel = shell.GetCollider2DSummaryLabel();
        m_collider2DEnabledCheckBox = shell.GetCollider2DEnabledCheckBox();
        m_collider2DTriggerCheckBox = shell.GetCollider2DTriggerCheckBox();
        m_collider2DShapeTypeLabel = shell.GetCollider2DShapeTypeLabel();
        m_collider2DShapeTypeEdit = shell.GetCollider2DShapeTypeEdit();
        m_collider2DOffsetLabel = shell.GetCollider2DOffsetLabel();
        m_collider2DSizeLabel = shell.GetCollider2DSizeLabel();
        m_collider2DRotationLabel = shell.GetCollider2DRotationLabel();
        m_collider2DRotationEdit = shell.GetCollider2DRotationEdit();
        m_collider2DEditControls = shell.GetCollider2DEditControls();
        m_collider2DEditButton = shell.GetCollider2DEditButton();
        m_collider2DComponentActionButton = shell.GetCollider2DComponentActionButton();
        m_addComponentButton = shell.GetAddComponentButton();
        m_materialOpenButton = shell.GetMaterialOpenButton();
        m_materialSharedNoticeLabel = shell.GetMaterialSharedNoticeLabel();
        m_materialDetailsSectionLabel = shell.GetMaterialDetailsSectionLabel();
        m_materialDetailLabels = shell.GetMaterialDetailLabels();
        m_materialDetailEditControls = shell.GetMaterialDetailEditControls();
        m_materialTextureDropHighlight = shell.GetMaterialTextureDropHighlight();
        m_materialTextureBrowseButton = shell.GetMaterialTextureBrowseButton();
        m_materialTintColorButton = shell.GetMaterialTintColorButton();
        m_materialOutlineEnabledCheckBox = shell.GetMaterialOutlineEnabledCheckBox();
        m_materialOutlineColorButton = shell.GetMaterialOutlineColorButton();
        m_cursor = &cursor;
        SetWindowTextW(m_spriteRefLabel, L"Material");
        EnableWindow(m_materialTextureBrowseButton, FALSE);
        EnableWindow(m_materialTintColorButton, FALSE);
        EnableWindow(m_materialOutlineColorButton, FALSE);
        SetWindowTextW(m_materialTintColorButton, L"");
        SetWindowTextW(m_materialOutlineColorButton, L"");
        SetWindowTextW(m_addComponentButton, L"Add Component");
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
            SetSpriteAssetControlsVisible(
                m_spriteRefLabel,
                m_spriteRefEdit,
                m_materialOpenButton,
                m_scriptAssetLabel,
                m_scriptAssetEdit,
                m_scriptCreateButton,
                m_scriptAssignButton,
                m_scriptClearButton,
                false);
            ShowWindow(m_scriptAssetLabel, SW_HIDE);
            ShowWindow(m_scriptAssetEdit, SW_HIDE);
            ShowWindow(m_scriptCreateButton, SW_HIDE);
            ShowWindow(m_scriptAssignButton, SW_HIDE);
            ShowWindow(m_scriptClearButton, SW_HIDE);
            ShowWindow(m_materialOpenButton, SW_HIDE);
            SetWindowTextW(m_collider2DComponentSectionLabel, L"Collider2DComponent (not attached)");
            SetWindowTextW(m_collider2DSummaryLabel, L"Collider2D: no entity selected");
            SetWindowTextW(m_collider2DComponentActionButton, L"Remove Collider2DComponent");
            EnableWindow(m_collider2DComponentActionButton, FALSE);
            EnableWindow(m_addComponentButton, FALSE);
            SetMaterialDetailsVisible(false);
            SetCollider2DControlsVisible(
                m_collider2DEnabledCheckBox,
                m_collider2DTriggerCheckBox,
                m_collider2DShapeTypeLabel,
                m_collider2DShapeTypeEdit,
                m_collider2DOffsetLabel,
                m_collider2DSizeLabel,
                m_collider2DEditControls,
                m_collider2DRotationLabel,
                m_collider2DRotationEdit,
                m_collider2DEditButton,
                false);
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
            SetSpriteAssetControlsVisible(
                m_spriteRefLabel,
                m_spriteRefEdit,
                m_materialOpenButton,
                m_scriptAssetLabel,
                m_scriptAssetEdit,
                m_scriptCreateButton,
                m_scriptAssignButton,
                m_scriptClearButton,
                false);
            ShowWindow(m_scriptAssetLabel, SW_HIDE);
            ShowWindow(m_scriptAssetEdit, SW_HIDE);
            ShowWindow(m_scriptCreateButton, SW_HIDE);
            ShowWindow(m_scriptAssignButton, SW_HIDE);
            ShowWindow(m_scriptClearButton, SW_HIDE);
            ShowWindow(m_materialOpenButton, SW_HIDE);
            SetWindowTextW(m_collider2DComponentSectionLabel, L"Collider2DComponent (not attached)");
            SetWindowTextW(m_collider2DSummaryLabel, L"Collider2D: selected entity not found");
            SetWindowTextW(m_collider2DComponentActionButton, L"Remove Collider2DComponent");
            EnableWindow(m_collider2DComponentActionButton, FALSE);
            EnableWindow(m_addComponentButton, FALSE);
            SetMaterialDetailsVisible(false);
            SetCollider2DControlsVisible(
                m_collider2DEnabledCheckBox,
                m_collider2DTriggerCheckBox,
                m_collider2DShapeTypeLabel,
                m_collider2DShapeTypeEdit,
                m_collider2DOffsetLabel,
                m_collider2DSizeLabel,
                m_collider2DEditControls,
                m_collider2DRotationLabel,
                m_collider2DRotationEdit,
                m_collider2DEditButton,
                false);
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
        EnableWindow(m_spriteComponentActionButton, actionState.enableActionButton ? TRUE : FALSE);
        SetWindowTextW(m_spriteComponentSectionLabel, actionState.sectionLabel);
        SetWindowTextW(m_spriteComponentActionButton, actionState.buttonLabel);
        SetSpriteAssetControlsVisible(
            m_spriteRefLabel,
            m_spriteRefEdit,
            m_materialOpenButton,
            m_scriptAssetLabel,
            m_scriptAssetEdit,
            m_scriptCreateButton,
            m_scriptAssignButton,
            m_scriptClearButton,
            actionState.showSpriteRefControls);
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

            const Core::AssetId selectedMaterialAssetId = false == materialAssetId.IsEmpty()
                ? materialAssetId
                : (spriteAsset.has_value() ? spriteAsset->materialAssetId : Core::AssetId{});
            const auto materialAsset = materialAssetResolver.ResolveMaterialAsset(selectedMaterialAssetId);
            SetMaterialDetailsVisible(true);
            SetMaterialDetailsEnabled(materialAsset.has_value());
            if (materialAsset.has_value())
            {
                SetWindowTextW(
                    m_materialDetailEditControls[0],
                    ToWideString(materialAsset->textureAssetId.GetValue()).c_str());
                SetWindowTextW(m_materialDetailEditControls[1], MaterialPanelController::FormatColorText(materialAsset->color).c_str());
                SendMessageW(
                    m_materialOutlineEnabledCheckBox,
                    BM_SETCHECK,
                    materialAsset->outlineEnabled ? BST_CHECKED : BST_UNCHECKED,
                    0);
                wchar_t thicknessText[64]{};
                std::swprintf(thicknessText, std::size(thicknessText), L"%.6g", materialAsset->outlineThickness);
                SetWindowTextW(m_materialDetailEditControls[3], thicknessText);
                SetWindowTextW(m_materialDetailEditControls[4], MaterialPanelController::FormatColorText(materialAsset->outlineColor).c_str());
            }
            else
            {
                SetWindowTextW(m_materialDetailEditControls[0], L"");
                SetWindowTextW(m_materialDetailEditControls[1], L"");
                SendMessageW(m_materialOutlineEnabledCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);
                SetWindowTextW(m_materialDetailEditControls[3], L"");
                SetWindowTextW(m_materialDetailEditControls[4], L"");
            }
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
            SetMaterialDetailsVisible(false);
        }

        const auto collider2DComponent = entity->get().GetCollider2DComponent();
        const InspectorCollider2DComponentActionState colliderActionState =
            ComputeCollider2DComponentActionState(collider2DComponent.has_value());
        SetWindowTextW(m_collider2DComponentSectionLabel, colliderActionState.sectionLabel);
        wchar_t colliderSummaryText[128]{};
        std::swprintf(
            colliderSummaryText,
            std::size(colliderSummaryText),
            L"Collider2D: Entity %u",
            static_cast<unsigned>(*selectedEntityId));
        SetWindowTextW(m_collider2DSummaryLabel, colliderSummaryText);
        SetWindowTextW(m_collider2DComponentActionButton, colliderActionState.buttonLabel);
        EnableWindow(m_collider2DComponentActionButton, collider2DComponent.has_value() ? TRUE : FALSE);
        EnableWindow(m_addComponentButton, true == hasSpriteComponent ? TRUE : FALSE);
        SetCollider2DControlsVisible(
            m_collider2DEnabledCheckBox,
            m_collider2DTriggerCheckBox,
            m_collider2DShapeTypeLabel,
            m_collider2DShapeTypeEdit,
            m_collider2DOffsetLabel,
            m_collider2DSizeLabel,
            m_collider2DEditControls,
            m_collider2DRotationLabel,
            m_collider2DRotationEdit,
            m_collider2DEditButton,
            colliderActionState.showColliderControls);
        if (collider2DComponent.has_value())
        {
            const Game::Collider2DComponent& collider = collider2DComponent->get();
            SendMessageW(m_collider2DEnabledCheckBox, BM_SETCHECK, collider.enabled ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(m_collider2DTriggerCheckBox, BM_SETCHECK, collider.isTrigger ? BST_CHECKED : BST_UNCHECKED, 0);
            SetWindowTextW(m_collider2DShapeTypeEdit, L"Box");
            SetEditFloat(m_collider2DEditControls[0], collider.offset.x);
            SetEditFloat(m_collider2DEditControls[1], collider.offset.y);
            SetEditFloat(m_collider2DEditControls[2], collider.size.x);
            SetEditFloat(m_collider2DEditControls[3], collider.size.y);
            SetWindowTextW(m_collider2DRotationEdit, L"0.000");
        }

        std::wstring summaryText = L"Inspector: ";
        summaryText += ToWideString(entity->get().GetName());
        SetWindowTextW(m_inspectorSummaryLabel, summaryText.c_str());
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

        if (true == ConsumeButtonClick(m_addComponentButton, inputSnapshot)
            && false == entity->get().HasCollider2DComponent())
        {
            const bool actionChanged = SceneEditingOperations::AddCollider2DComponent(entity->get());
            result.changed = actionChanged || result.changed;
            if (actionChanged)
            {
                result.operationName = "Add Collider2DComponent";
                result.collider2DComponentAdded = true;
            }
        }

        if (true == ConsumeButtonClick(m_collider2DComponentActionButton, inputSnapshot)
            && true == entity->get().HasCollider2DComponent())
        {
            const bool hadCollider2DComponent = entity->get().HasCollider2DComponent();
            const bool actionChanged = ApplyCollider2DComponentAction(entity->get());
            result.changed = actionChanged || result.changed;
            if (actionChanged)
            {
                result.operationName = hadCollider2DComponent
                    ? "Remove Collider2DComponent"
                    : "Add Collider2DComponent";
                result.collider2DComponentAdded = false == hadCollider2DComponent;
            }
        }

        auto collider2DComponent = entity->get().GetCollider2DComponent();
        if (collider2DComponent.has_value() && false == result.collider2DComponentAdded)
        {
            Game::Collider2DComponent& collider = collider2DComponent->get();
            const bool enabled = SendMessageW(m_collider2DEnabledCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
            const bool isTrigger = SendMessageW(m_collider2DTriggerCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
            if (collider.enabled != enabled)
            {
                collider.enabled = enabled;
                result.changed = true;
                result.operationName = "Edit Collider2DComponent";
            }
            if (collider.isTrigger != isTrigger)
            {
                collider.isTrigger = isTrigger;
                result.changed = true;
                result.operationName = "Edit Collider2DComponent";
            }

            const std::optional<float> offsetX = ReadEditFloat(m_collider2DEditControls[0]);
            const std::optional<float> offsetY = ReadEditFloat(m_collider2DEditControls[1]);
            const std::optional<float> sizeX = ReadEditFloat(m_collider2DEditControls[2]);
            const std::optional<float> sizeY = ReadEditFloat(m_collider2DEditControls[3]);
            if (offsetX.has_value() && collider.offset.x != *offsetX)
            {
                collider.offset.x = *offsetX;
                result.changed = true;
                result.operationName = "Edit Collider2DComponent";
            }
            if (offsetY.has_value() && collider.offset.y != *offsetY)
            {
                collider.offset.y = *offsetY;
                result.changed = true;
                result.operationName = "Edit Collider2DComponent";
            }
            if (sizeX.has_value() && *sizeX > 0.0f && collider.size.x != *sizeX)
            {
                collider.size.x = *sizeX;
                result.changed = true;
                result.operationName = "Edit Collider2DComponent";
            }
            if (sizeY.has_value() && *sizeY > 0.0f && collider.size.y != *sizeY)
            {
                collider.size.y = *sizeY;
                result.changed = true;
                result.operationName = "Edit Collider2DComponent";
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

            const Core::AssetId materialAssetId = false == spriteComponent->get().materialAssetRef.IsEmpty()
                ? spriteComponent->get().materialAssetRef
                : (spriteAsset.has_value() ? spriteAsset->materialAssetId : Core::AssetId{});
            const std::optional<Game::Assets::SpriteMaterialAsset> currentMaterialAsset =
                materialAssetResolver.ResolveMaterialAsset(materialAssetId);
            if (false == materialAssetId.IsEmpty() && true == currentMaterialAsset.has_value())
            {
                Game::Assets::SpriteMaterialAsset editedMaterialAsset = *currentMaterialAsset;
                bool materialChanged = false;

                const std::wstring textureText = GetWindowTextString(m_materialDetailEditControls[0]);
                const std::wstring_view trimmedTextureText = Trim(textureText);
                const Core::AssetId textureAssetId(ToNarrowString(trimmedTextureText));
                if (editedMaterialAsset.textureAssetId != textureAssetId)
                {
                    editedMaterialAsset.textureAssetId = textureAssetId;
                    materialChanged = true;
                }

                std::array<float, 4> tintColor{};
                if (MaterialPanelController::TryParseColorText(GetWindowTextString(m_materialDetailEditControls[1]), tintColor)
                    && editedMaterialAsset.color != tintColor)
                {
                    editedMaterialAsset.color = tintColor;
                    materialChanged = true;
                }

                const bool outlineEnabled =
                    SendMessageW(m_materialOutlineEnabledCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                if (editedMaterialAsset.outlineEnabled != outlineEnabled)
                {
                    editedMaterialAsset.outlineEnabled = outlineEnabled;
                    materialChanged = true;
                }

                const std::optional<float> outlineThickness = ReadEditFloat(m_materialDetailEditControls[3]);
                if (outlineThickness.has_value() && editedMaterialAsset.outlineThickness != *outlineThickness)
                {
                    editedMaterialAsset.outlineThickness = *outlineThickness;
                    materialChanged = true;
                }

                std::array<float, 4> outlineColor{};
                if (MaterialPanelController::TryParseColorText(GetWindowTextString(m_materialDetailEditControls[4]), outlineColor)
                    && editedMaterialAsset.outlineColor != outlineColor)
                {
                    editedMaterialAsset.outlineColor = outlineColor;
                    materialChanged = true;
                }

                if (materialChanged)
                {
                    result.materialAssetChanged = true;
                    result.materialTargetAssetId = materialAssetId;
                    result.materialAsset = editedMaterialAsset;
                }
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

    InspectorApplyResult InspectorPanelController::ApplyMaterialTextureDrop(
        const Game::Scene* scene,
        std::optional<Game::EntityId> selectedEntityId,
        const AssetsPanelController& assetsPanelController,
        const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
        const Game::Assets::IMaterialAssetResolver& materialAssetResolver) const
    {
        InspectorApplyResult result{};
        if (nullptr == scene
            || false == selectedEntityId.has_value()
            || true == assetsPanelController.GetDraggingTextureAssetId().IsEmpty()
            || false == assetsPanelController.WasDragReleasedThisFrame()
            || false == IsMaterialTextureDropTargetHovered(assetsPanelController))
        {
            return result;
        }

        const auto entity = scene->FindEntity(*selectedEntityId);
        if (false == entity.has_value())
        {
            return result;
        }

        const auto spriteComponent = entity->get().GetSpriteComponent();
        if (false == spriteComponent.has_value())
        {
            return result;
        }

        const auto spriteAsset = spriteAssetResolver.ResolveSpriteAsset(spriteComponent->get().spriteAssetRef);
        const Core::AssetId materialAssetId = false == spriteComponent->get().materialAssetRef.IsEmpty()
            ? spriteComponent->get().materialAssetRef
            : (spriteAsset.has_value() ? spriteAsset->materialAssetId : Core::AssetId{});
        const std::optional<Game::Assets::SpriteMaterialAsset> currentMaterialAsset =
            materialAssetResolver.ResolveMaterialAsset(materialAssetId);
        if (materialAssetId.IsEmpty()
            || false == currentMaterialAsset.has_value()
            || currentMaterialAsset->textureAssetId == assetsPanelController.GetDraggingTextureAssetId())
        {
            return result;
        }

        result.materialAssetChanged = true;
        result.materialTargetAssetId = materialAssetId;
        result.materialAsset = *currentMaterialAsset;
        result.materialAsset.textureAssetId = assetsPanelController.GetDraggingTextureAssetId();
        return result;
    }

    void InspectorPanelController::UpdateDropHighlight(const AssetsPanelController& assetsPanelController)
    {
        HWND targetEdit = nullptr;
        if (true == IsMaterialDropTargetHovered(assetsPanelController))
        {
            targetEdit = m_spriteRefEdit;
        }
        else if (true == IsMaterialTextureDropTargetHovered(assetsPanelController))
        {
            targetEdit = m_materialDetailEditControls[0];
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

        ShowWindow(m_spriteRefDropHighlight, shouldShow && targetEdit != m_materialDetailEditControls[0] ? SW_SHOW : SW_HIDE);
        ShowWindow(m_materialTextureDropHighlight, shouldShow && targetEdit == m_materialDetailEditControls[0] ? SW_SHOW : SW_HIDE);
        if (shouldShow)
        {
            RECT targetRect{};
            HWND highlightWindow = targetEdit == m_materialDetailEditControls[0]
                ? m_materialTextureDropHighlight
                : m_spriteRefDropHighlight;
            HWND parentWindow = GetParent(highlightWindow);
            GetWindowRect(targetEdit, &targetRect);
            MapWindowPoints(nullptr, parentWindow, reinterpret_cast<POINT*>(&targetRect), 2);
            InflateRect(&targetRect, 2, 2);
            SetWindowPos(
                highlightWindow,
                targetEdit,
                targetRect.left,
                targetRect.top,
                targetRect.right - targetRect.left,
                targetRect.bottom - targetRect.top,
                SWP_NOACTIVATE);
            InvalidateRect(highlightWindow, nullptr, TRUE);
        }
    }

    void InspectorPanelController::ResetTrackedEntity()
    {
        m_lastInspectorEntityId.reset();
    }

    void InspectorPanelController::SetMaterialDetailsVisible(bool visible) const
    {
        const int showCommand = visible ? SW_SHOW : SW_HIDE;
        ShowWindow(m_materialDetailsSectionLabel, showCommand);
        ShowWindow(m_materialSharedNoticeLabel, showCommand);
        ShowWindow(m_materialTextureBrowseButton, showCommand);
        ShowWindow(m_materialTintColorButton, showCommand);
        ShowWindow(m_materialOutlineEnabledCheckBox, showCommand);
        ShowWindow(m_materialOutlineColorButton, showCommand);
        for (HWND label : m_materialDetailLabels)
        {
            ShowWindow(label, showCommand);
        }
        for (std::size_t index = 0; index < m_materialDetailEditControls.size(); ++index)
        {
            ShowWindow(m_materialDetailEditControls[index], visible && 2 != index ? SW_SHOW : SW_HIDE);
        }
        if (false == visible)
        {
            ShowWindow(m_materialTextureDropHighlight, SW_HIDE);
        }
    }

    void InspectorPanelController::SetMaterialDetailsEnabled(bool enabled) const
    {
        const BOOL enableFlag = enabled ? TRUE : FALSE;
        for (std::size_t index = 0; index < m_materialDetailEditControls.size(); ++index)
        {
            EnableWindow(m_materialDetailEditControls[index], 2 != index ? enableFlag : FALSE);
        }
        EnableWindow(m_materialOutlineEnabledCheckBox, enableFlag);
        EnableWindow(m_materialTextureBrowseButton, FALSE);
        EnableWindow(m_materialTintColorButton, FALSE);
        EnableWindow(m_materialOutlineColorButton, FALSE);
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

    bool InspectorPanelController::IsMaterialTextureDropTargetHovered(
        const AssetsPanelController& assetsPanelController) const
    {
        const bool hasTextureDrag = assetsPanelController.IsDragActive()
            || assetsPanelController.WasDragReleasedThisFrame();
        if (false == hasTextureDrag
            || true == assetsPanelController.GetDraggingTextureAssetId().IsEmpty()
            || nullptr == m_materialDetailEditControls[0]
            || false == IsWindowVisible(m_materialDetailEditControls[0])
            || false == IsWindowEnabled(m_materialDetailEditControls[0]))
        {
            return false;
        }

        if (nullptr == m_cursor)
        {
            return false;
        }

        const POINT cursorPoint = ToWin32Point(m_cursor->GetScreenPosition());

        RECT textureRect{};
        GetWindowRect(m_materialDetailEditControls[0], &textureRect);
        return PtInRect(&textureRect, cursorPoint) != FALSE;
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
