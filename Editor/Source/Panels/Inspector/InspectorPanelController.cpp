#include "Panels/Inspector/InspectorPanelController.h"

#include <string>

#include "Utils/EditorStringUtils.h"
#include <Windows.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <AssetId.h>
#include <Collider2DComponent.h>
#include "Panels/Assets/AssetsPanelController.h"
#include "Panels/Inspector/InspectorPanelView.h"
#include <Entity.h>
#include "Panels/Inspector/MaterialPanelController.h"
#include <Scene.h>

namespace Xelqoria::Editor
{
    namespace
    {
        [[nodiscard]] POINT ToWin32Point(Platform::Point point)
        {
            return POINT{ static_cast<LONG>(point.x), static_cast<LONG>(point.y) };
        }

        void SetWindowTextIfChanged(HWND window, const wchar_t* text)
        {
            if (nullptr == window)
            {
                return;
            }

            const wchar_t* nextText = nullptr != text ? text : L"";
            const int currentTextLength = GetWindowTextLengthW(window);
            std::wstring currentText(static_cast<std::size_t>((std::max)(0, currentTextLength)) + 1u, L'\0');
            const int copiedLength = GetWindowTextW(
                window,
                currentText.data(),
                static_cast<int>(currentText.size()));
            currentText.resize(static_cast<std::size_t>((std::max)(0, copiedLength)));
            if (currentText == nextText)
            {
                return;
            }

            SetWindowTextW(window, nextText);
        }

        void SetWindowVisible(HWND window, bool visible)
        {
            if (nullptr == window)
            {
                return;
            }

            const bool isVisible = FALSE != IsWindowVisible(window);
            if (isVisible == visible)
            {
                return;
            }

            ShowWindow(window, visible ? SW_SHOW : SW_HIDE);
        }

        void SetWindowEnabled(HWND window, bool enabled)
        {
            if (nullptr == window)
            {
                return;
            }

            const bool isEnabled = FALSE != IsWindowEnabled(window);
            if (isEnabled == enabled)
            {
                return;
            }

            EnableWindow(window, enabled ? TRUE : FALSE);
        }

        void SetButtonCheck(HWND button, bool checked)
        {
            if (nullptr == button)
            {
                return;
            }

            const LRESULT checkState = checked ? BST_CHECKED : BST_UNCHECKED;
            if (SendMessageW(button, BM_GETCHECK, 0, 0) == checkState)
            {
                return;
            }

            SendMessageW(button, BM_SETCHECK, static_cast<WPARAM>(checkState), 0);
        }

        void SetEditFloat(HWND editControl, float value)
        {
            wchar_t valueText[32]{};
            std::swprintf(valueText, std::size(valueText), L"%.3f", value);
            SetWindowTextIfChanged(editControl, valueText);
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
            SetWindowVisible(enabledCheckBox, false);
            SetWindowVisible(triggerCheckBox, false);
            SetWindowVisible(shapeTypeLabel, visible);
            SetWindowVisible(shapeTypeEdit, visible);
            SetWindowVisible(offsetLabel, visible);
            SetWindowVisible(sizeLabel, visible);
            SetWindowVisible(rotationLabel, visible);
            SetWindowVisible(rotationEdit, visible);
            SetWindowVisible(editButton, false);
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

        [[nodiscard]] std::filesystem::path GetInspectorStatePath()
        {
            std::filesystem::path statePath = std::filesystem::temp_directory_path();
            statePath /= L"XelqoriaEditorInspectorState.txt";
            return statePath;
        }

        [[nodiscard]] bool HitTestWindow(HWND window, Platform::Point cursorPoint)
        {
            if (nullptr == window || FALSE == IsWindowVisible(window))
            {
                return false;
            }

            RECT rect{};
            GetWindowRect(window, &rect);
            const POINT point{ static_cast<LONG>(cursorPoint.x), static_cast<LONG>(cursorPoint.y) };
            return PtInRect(&rect, point) != FALSE;
        }
    }

    void InspectorPanelController::Bind(const InspectorPanelView& view, Platform::ICursor& cursor)
    {
        m_inspectorSummaryLabel = view.GetSummaryLabel();
        m_transformSectionLabel = view.GetTransformSectionLabel();
        m_transformEditControls = view.GetTransformEditControls();
        m_spriteComponentSectionLabel = view.GetSpriteComponentSectionLabel();
        m_spriteRefLabel = view.GetSpriteRefLabel();
        m_spriteRefDropHighlight = view.GetSpriteRefDropHighlight();
        m_spriteRefEdit = view.GetSpriteRefEdit();
        m_scriptSectionLabel = view.GetScriptSectionLabel();
        m_scriptAssetLabel = view.GetScriptAssetLabel();
        m_scriptAssetEdit = view.GetScriptAssetEdit();
        m_scriptCreateButton = view.GetScriptCreateButton();
        m_scriptAssignButton = view.GetScriptAssignButton();
        m_scriptClearButton = view.GetScriptClearButton();
        m_spriteComponentActionButton = view.GetSpriteComponentActionButton();
        m_collider2DComponentSectionLabel = view.GetCollider2DComponentSectionLabel();
        m_collider2DSummaryLabel = view.GetCollider2DSummaryLabel();
        m_collider2DEnabledCheckBox = view.GetCollider2DEnabledCheckBox();
        m_collider2DTriggerCheckBox = view.GetCollider2DTriggerCheckBox();
        m_collider2DShapeTypeLabel = view.GetCollider2DShapeTypeLabel();
        m_collider2DShapeTypeEdit = view.GetCollider2DShapeTypeEdit();
        m_collider2DOffsetLabel = view.GetCollider2DOffsetLabel();
        m_collider2DSizeLabel = view.GetCollider2DSizeLabel();
        m_collider2DRotationLabel = view.GetCollider2DRotationLabel();
        m_collider2DRotationEdit = view.GetCollider2DRotationEdit();
        m_collider2DEditControls = view.GetCollider2DEditControls();
        m_collider2DEditButton = view.GetCollider2DEditButton();
        m_collider2DComponentActionButton = view.GetCollider2DComponentActionButton();
        m_addComponentButton = view.GetAddComponentButton();
        m_materialOpenButton = view.GetMaterialOpenButton();
        m_materialSharedNoticeLabel = view.GetMaterialSharedNoticeLabel();
        m_materialDetailsSectionLabel = view.GetMaterialDetailsSectionLabel();
        m_materialDetailLabels = view.GetMaterialDetailLabels();
        m_materialDetailEditControls = view.GetMaterialDetailEditControls();
        m_materialTextureDropHighlight = view.GetMaterialTextureDropHighlight();
        m_materialTextureBrowseButton = view.GetMaterialTextureBrowseButton();
        m_materialTintColorButton = view.GetMaterialTintColorButton();
        m_materialOutlineEnabledCheckBox = view.GetMaterialOutlineEnabledCheckBox();
        m_materialOutlineColorButton = view.GetMaterialOutlineColorButton();
        m_materialRemoveButton = view.GetMaterialRemoveButton();
        m_cursor = &cursor;
        SetWindowTextIfChanged(m_spriteRefLabel, L"Sprite");
        SetWindowTextIfChanged(m_scriptAssignButton, L"...");
        SetWindowTextIfChanged(m_materialRemoveButton, L"Remove Material");
        SetWindowVisible(m_scriptCreateButton, false);
        SetWindowVisible(m_scriptClearButton, false);
        SetWindowEnabled(m_materialOpenButton, FALSE);
        SetWindowEnabled(m_materialTextureBrowseButton, FALSE);
        SetWindowEnabled(m_materialTintColorButton, FALSE);
        SetWindowEnabled(m_materialOutlineColorButton, FALSE);
        SetWindowTextIfChanged(m_materialTintColorButton, L"");
        SetWindowTextIfChanged(m_materialOutlineColorButton, L"");
        SetWindowTextIfChanged(m_addComponentButton, L"Add Component");
        LoadCollapseState();
    }

    void InspectorPanelController::Refresh(
        const Game::Scene* scene,
        std::optional<Game::EntityId> selectedEntityId,
        bool canAddSpriteComponent,
        const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
        const Game::Assets::IMaterialAssetResolver& materialAssetResolver,
        HierarchyPanelController::VisibleItemKind selectedItemKind,
        const std::filesystem::path& selectedAssetPath)
    {
        (void)materialAssetResolver;
        UpdateSectionLabels(selectedItemKind);

        if (false == selectedEntityId.has_value() || nullptr == scene)
        {
            if (false == selectedAssetPath.empty())
            {
                const std::wstring assetName = selectedAssetPath.filename().wstring();
                std::wstring summaryText = L"Inspector: Asset ";
                summaryText += assetName.empty() ? selectedAssetPath.wstring() : assetName;
                SetWindowTextIfChanged(m_inspectorSummaryLabel, summaryText.c_str());
            }
            else
            {
                SetWindowTextIfChanged(m_inspectorSummaryLabel, L"Inspector: no entity selected");
            }
            SetWindowTextIfChanged(m_spriteRefEdit, L"");
            SetWindowTextIfChanged(m_scriptAssetEdit, L"");
            SetWindowVisible(m_scriptSectionLabel, false);
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
            SetWindowVisible(m_scriptAssetLabel, false);
            SetWindowVisible(m_scriptAssetEdit, false);
            SetWindowVisible(m_scriptCreateButton, false);
            SetWindowVisible(m_scriptAssignButton, false);
            SetWindowVisible(m_scriptClearButton, false);
            SetWindowVisible(m_materialOpenButton, false);
            SetWindowTextIfChanged(m_collider2DComponentSectionLabel, L"Collider2D");
            SetWindowTextIfChanged(m_collider2DSummaryLabel, L"Collider2D: no entity selected");
            SetWindowTextIfChanged(m_collider2DComponentActionButton, L"Remove Collider2DComponent");
            SetWindowEnabled(m_collider2DComponentActionButton, FALSE);
            SetWindowEnabled(m_addComponentButton, FALSE);
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
            ApplyCardVisibility(false, false, false);
            return;
        }

        const auto entity = scene->FindEntity(*selectedEntityId);
        if (false == entity.has_value())
        {
            SetWindowTextIfChanged(m_inspectorSummaryLabel, L"Inspector: selected entity not found");
            SetWindowTextIfChanged(m_spriteRefEdit, L"");
            SetWindowTextIfChanged(m_scriptAssetEdit, L"");
            SetWindowVisible(m_scriptSectionLabel, false);
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
            SetWindowVisible(m_scriptAssetLabel, false);
            SetWindowVisible(m_scriptAssetEdit, false);
            SetWindowVisible(m_scriptCreateButton, false);
            SetWindowVisible(m_scriptAssignButton, false);
            SetWindowVisible(m_scriptClearButton, false);
            SetWindowVisible(m_materialOpenButton, false);
            SetWindowTextIfChanged(m_collider2DComponentSectionLabel, L"Collider2D");
            SetWindowTextIfChanged(m_collider2DSummaryLabel, L"Collider2D: selected entity not found");
            SetWindowTextIfChanged(m_collider2DComponentActionButton, L"Remove Collider2DComponent");
            SetWindowEnabled(m_collider2DComponentActionButton, FALSE);
            SetWindowEnabled(m_addComponentButton, FALSE);
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
            ApplyCardVisibility(false, false, false);
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
            SetWindowTextIfChanged(m_transformEditControls[index], valueText);
        }

        const auto spriteComponent = entity->get().GetSpriteComponent();
        const bool hasSpriteComponent = spriteComponent.has_value();
        const InspectorSpriteComponentActionState actionState =
            ComputeSpriteComponentActionState(hasSpriteComponent, canAddSpriteComponent);
        SetWindowEnabled(m_spriteComponentActionButton, actionState.enableActionButton ? TRUE : FALSE);
        SetWindowTextIfChanged(m_spriteComponentSectionLabel, actionState.sectionLabel);
        SetWindowTextIfChanged(m_spriteComponentActionButton, actionState.buttonLabel);
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
            m_lastSpriteRefDisplayText = FormatSpriteDisplayText(m_lastSpriteRefAssetId);
            SetWindowTextIfChanged(m_spriteRefEdit, m_lastSpriteRefDisplayText.c_str());
            SetWindowEnabled(m_materialOpenButton, FALSE);

            m_lastScriptAssetId = spriteAsset.has_value() ? spriteAsset->scriptAssetId : Core::AssetId{};
            m_lastScriptDisplayText = FormatScriptDisplayText(m_lastScriptAssetId);
            const InspectorScriptActionState scriptActionState = ComputeScriptActionState(
                true,
                m_lastSpriteRefAssetId,
                spriteAsset.has_value(),
                m_lastScriptAssetId);
            SetWindowVisible(m_scriptSectionLabel, true);
            SetWindowVisible(m_scriptAssetLabel, scriptActionState.showScriptControls);
            SetWindowVisible(m_scriptAssetEdit, scriptActionState.showScriptControls);
            SetWindowVisible(m_scriptCreateButton, false);
            SetWindowVisible(m_scriptAssignButton, scriptActionState.showScriptControls);
            SetWindowVisible(m_scriptClearButton, false);
            SetWindowEnabled(m_scriptCreateButton, FALSE);
            SetWindowEnabled(m_scriptAssignButton, scriptActionState.enableAssignButton ? TRUE : FALSE);
            SetWindowEnabled(m_scriptClearButton, FALSE);
            SetWindowTextIfChanged(m_scriptAssetEdit, m_lastScriptDisplayText.c_str());

            const Core::AssetId selectedMaterialAssetId = false == materialAssetId.IsEmpty()
                ? materialAssetId
                : (spriteAsset.has_value() ? spriteAsset->materialAssetId : Core::AssetId{});
            const auto materialAsset = materialAssetResolver.ResolveMaterialAsset(selectedMaterialAssetId);
            SetMaterialDetailsVisible(true);
            SetMaterialDetailsEnabled(materialAsset.has_value());
            SetWindowEnabled(m_materialRemoveButton, false == materialAssetId.IsEmpty());
            if (materialAsset.has_value())
            {
                SetWindowTextIfChanged(
                    m_materialDetailEditControls[0],
                    ToWideString(materialAsset->textureAssetId.GetValue()).c_str());
                SetWindowTextIfChanged(m_materialDetailEditControls[1], MaterialPanelController::FormatColorText(materialAsset->color).c_str());
                SetButtonCheck(m_materialOutlineEnabledCheckBox, materialAsset->outlineEnabled);
                wchar_t thicknessText[64]{};
                std::swprintf(thicknessText, std::size(thicknessText), L"%.6g", materialAsset->outlineThickness);
                SetWindowTextIfChanged(m_materialDetailEditControls[3], thicknessText);
                SetWindowTextIfChanged(m_materialDetailEditControls[4], MaterialPanelController::FormatColorText(materialAsset->outlineColor).c_str());
            }
            else
            {
                SetWindowTextIfChanged(m_materialDetailEditControls[0], L"");
                SetWindowTextIfChanged(m_materialDetailEditControls[1], L"");
                SetButtonCheck(m_materialOutlineEnabledCheckBox, false);
                SetWindowTextIfChanged(m_materialDetailEditControls[3], L"");
                SetWindowTextIfChanged(m_materialDetailEditControls[4], L"");
            }
        }
        else
        {
            SetWindowTextIfChanged(m_spriteRefEdit, L"");
            SetWindowTextIfChanged(m_scriptAssetEdit, L"");
            SetWindowVisible(m_scriptAssetLabel, false);
            SetWindowVisible(m_scriptAssetEdit, false);
            SetWindowVisible(m_scriptSectionLabel, false);
            SetWindowVisible(m_scriptCreateButton, false);
            SetWindowVisible(m_scriptAssignButton, false);
            SetWindowVisible(m_scriptClearButton, false);
            SetWindowVisible(m_materialOpenButton, false);
            m_lastSpriteRefAssetId = {};
            m_lastSpriteRefDisplayText.clear();
            m_lastScriptAssetId = {};
            m_lastScriptDisplayText.clear();
            SetMaterialDetailsVisible(false);
        }

        const auto collider2DComponent = entity->get().GetCollider2DComponent();
        const InspectorCollider2DComponentActionState colliderActionState =
            ComputeCollider2DComponentActionState(collider2DComponent.has_value());
        SetWindowTextIfChanged(m_collider2DComponentSectionLabel, colliderActionState.sectionLabel);
        wchar_t colliderSummaryText[128]{};
        std::swprintf(
            colliderSummaryText,
            std::size(colliderSummaryText),
            L"Collider2D: Entity %u",
            static_cast<unsigned>(*selectedEntityId));
        SetWindowTextIfChanged(m_collider2DSummaryLabel, colliderSummaryText);
        SetWindowTextIfChanged(m_collider2DComponentActionButton, colliderActionState.buttonLabel);
        SetWindowEnabled(m_collider2DComponentActionButton, collider2DComponent.has_value() ? TRUE : FALSE);
        SetWindowEnabled(m_addComponentButton, true == hasSpriteComponent ? TRUE : FALSE);
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
            (void)collider.enabled;
            (void)collider.isTrigger;
            SendMessageW(m_collider2DShapeTypeEdit, CB_SETCURSEL, 0, 0);
            SetEditFloat(m_collider2DEditControls[0], collider.offset.x);
            SetEditFloat(m_collider2DEditControls[1], collider.offset.y);
            SetEditFloat(m_collider2DEditControls[2], collider.size.x);
            SetEditFloat(m_collider2DEditControls[3], collider.size.y);
            SetWindowTextIfChanged(m_collider2DRotationEdit, L"0.000");
        }
        ApplyCardVisibility(hasSpriteComponent, hasSpriteComponent, collider2DComponent.has_value());
        UpdateSectionLabels(selectedItemKind);

        std::wstring summaryText = L"Inspector: ";
        summaryText += ToWideString(entity->get().GetName());
        SetWindowTextIfChanged(m_inspectorSummaryLabel, summaryText.c_str());
        m_lastInspectorEntityId = selectedEntityId;
    }

    InspectorApplyResult InspectorPanelController::ApplyEdits(
        Game::Scene* scene,
        std::optional<Game::EntityId> selectedEntityId,
        bool canAddSpriteComponent,
        const Core::InputSnapshot& inputSnapshot,
        const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
        const Game::Assets::IMaterialAssetResolver& materialAssetResolver,
        HierarchyPanelController::VisibleItemKind selectedItemKind)
    {
        InspectorApplyResult result{};
        if (ToggleSectionCollapseFromInput(inputSnapshot))
        {
            Refresh(scene, selectedEntityId, canAddSpriteComponent, spriteAssetResolver, materialAssetResolver, selectedItemKind);
        }
        if (false == selectedEntityId.has_value() || nullptr == scene)
        {
            return result;
        }

        if (m_lastInspectorEntityId != selectedEntityId)
        {
            Refresh(scene, selectedEntityId, canAddSpriteComponent, spriteAssetResolver, materialAssetResolver, selectedItemKind);
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
                SetWindowTextIfChanged(m_spriteRefEdit, L"");
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

            if (scriptActionState.enableAssignButton && true == ConsumeButtonClick(m_scriptAssignButton, inputSnapshot))
            {
                result.scriptAction = InspectorScriptAction::Assign;
                result.scriptTargetSpriteAssetId = spriteComponent->get().spriteAssetRef;
            }

            if (true == ConsumeButtonClick(m_materialRemoveButton, inputSnapshot))
            {
                const bool materialCleared = ClearSpriteMaterialReference(entity->get());
                result.changed = result.changed || materialCleared;
                if (materialCleared)
                {
                    result.operationName = "Remove Material";
                }
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
        Refresh(
            scene,
            selectedEntityId,
            true,
            spriteAssetResolver,
            materialAssetResolver,
            HierarchyPanelController::VisibleItemKind::SpriteComponent);
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
            targetEdit = m_materialDetailsSectionLabel;
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

        SetWindowVisible(m_spriteRefDropHighlight, shouldShow && targetEdit != m_materialDetailEditControls[0]);
        SetWindowVisible(m_materialTextureDropHighlight, shouldShow && targetEdit == m_materialDetailEditControls[0]);
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

    void InspectorPanelController::LoadCollapseState()
    {
        std::ifstream input(GetInspectorStatePath());
        if (false == input.is_open())
        {
            return;
        }

        std::string key{};
        int value = 0;
        while (input >> key >> value)
        {
            const bool collapsed = 0 != value;
            if ("transform" == key)
            {
                m_isTransformCollapsed = collapsed;
            }
            else if ("sprite" == key)
            {
                m_isSpriteComponentCollapsed = collapsed;
            }
            else if ("script" == key)
            {
                m_isScriptCollapsed = collapsed;
            }
            else if ("material" == key)
            {
                m_isMaterialCollapsed = collapsed;
            }
            else if ("collider2d" == key)
            {
                m_isCollider2DCollapsed = collapsed;
            }
        }
    }

    void InspectorPanelController::SaveCollapseState() const
    {
        std::ofstream output(GetInspectorStatePath(), std::ios::trunc);
        if (false == output.is_open())
        {
            return;
        }

        output << "transform " << (m_isTransformCollapsed ? 1 : 0) << '\n';
        output << "sprite " << (m_isSpriteComponentCollapsed ? 1 : 0) << '\n';
        output << "script " << (m_isScriptCollapsed ? 1 : 0) << '\n';
        output << "material " << (m_isMaterialCollapsed ? 1 : 0) << '\n';
        output << "collider2d " << (m_isCollider2DCollapsed ? 1 : 0) << '\n';
    }

    void InspectorPanelController::SetMaterialDetailsVisible(bool visible) const
    {
        SetWindowVisible(m_materialDetailsSectionLabel, visible);
        SetWindowVisible(m_materialSharedNoticeLabel, visible);
        SetWindowVisible(m_materialTextureBrowseButton, visible);
        SetWindowVisible(m_materialTintColorButton, visible);
        SetWindowVisible(m_materialOutlineEnabledCheckBox, visible);
        SetWindowVisible(m_materialOutlineColorButton, visible);
        SetWindowVisible(m_materialRemoveButton, visible);
        for (HWND label : m_materialDetailLabels)
        {
            SetWindowVisible(label, visible);
        }
        for (std::size_t index = 0; index < m_materialDetailEditControls.size(); ++index)
        {
            SetWindowVisible(m_materialDetailEditControls[index], visible && 2 != index);
        }
        if (false == visible)
        {
            SetWindowVisible(m_materialTextureDropHighlight, false);
        }
    }

    void InspectorPanelController::SetMaterialDetailsEnabled(bool enabled) const
    {
        const BOOL enableFlag = enabled ? TRUE : FALSE;
        for (std::size_t index = 0; index < m_materialDetailEditControls.size(); ++index)
        {
            SetWindowEnabled(m_materialDetailEditControls[index], 2 != index ? enableFlag : FALSE);
        }
        SetWindowEnabled(m_materialOutlineEnabledCheckBox, enableFlag);
        SetWindowEnabled(m_materialTextureBrowseButton, FALSE);
        SetWindowEnabled(m_materialTintColorButton, FALSE);
        SetWindowEnabled(m_materialOutlineColorButton, FALSE);
        SetWindowEnabled(m_materialRemoveButton, enabled ? TRUE : FALSE);
    }

    void InspectorPanelController::ApplyCardVisibility(
        bool hasSpriteComponent,
        bool hasMaterialDetails,
        bool hasCollider2DComponent) const
    {
        for (HWND transformEdit : m_transformEditControls)
        {
            SetWindowVisible(transformEdit, false == m_isTransformCollapsed);
        }

        const bool showSprite = hasSpriteComponent && false == m_isSpriteComponentCollapsed;
        SetWindowVisible(m_spriteRefLabel, showSprite);
        SetWindowVisible(m_spriteRefEdit, showSprite);
        SetWindowVisible(m_materialOpenButton, showSprite);
        SetWindowVisible(m_spriteComponentActionButton, false == m_isSpriteComponentCollapsed);

        const bool showScript = hasSpriteComponent && false == m_isScriptCollapsed;
        SetWindowVisible(m_scriptSectionLabel, hasSpriteComponent);
        SetWindowVisible(m_scriptAssetLabel, showScript);
        SetWindowVisible(m_scriptAssetEdit, showScript);
        SetWindowVisible(m_scriptCreateButton, false);
        SetWindowVisible(m_scriptAssignButton, showScript);
        SetWindowVisible(m_scriptClearButton, false);

        const bool showMaterialBody = hasMaterialDetails && false == m_isMaterialCollapsed;
        SetMaterialDetailsVisible(showMaterialBody);
        SetWindowVisible(m_materialDetailsSectionLabel, hasMaterialDetails);

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
            hasCollider2DComponent && false == m_isCollider2DCollapsed);
        SetWindowVisible(m_collider2DComponentActionButton, false == m_isCollider2DCollapsed);
    }

    void InspectorPanelController::UpdateSectionLabels(HierarchyPanelController::VisibleItemKind selectedItemKind)
    {
        const auto makeLabel =
            [](bool collapsed, bool selected, const wchar_t* name)
            {
                std::wstring label = selected ? L"> " : L"";
                label += collapsed ? L"▶ " : L"▼ ";
                label += name;
                return label;
            };

        SetWindowTextIfChanged(
            m_transformSectionLabel,
            makeLabel(m_isTransformCollapsed, HierarchyPanelController::VisibleItemKind::Entity == selectedItemKind, L"Transform").c_str());
        SetWindowTextIfChanged(
            m_spriteComponentSectionLabel,
            makeLabel(m_isSpriteComponentCollapsed, HierarchyPanelController::VisibleItemKind::SpriteComponent == selectedItemKind, L"Sprite").c_str());
        SetWindowTextIfChanged(
            m_scriptSectionLabel,
            makeLabel(m_isScriptCollapsed, false, L"Script").c_str());
        SetWindowTextIfChanged(
            m_materialDetailsSectionLabel,
            makeLabel(m_isMaterialCollapsed, HierarchyPanelController::VisibleItemKind::Material == selectedItemKind, L"Material").c_str());
        SetWindowTextIfChanged(
            m_collider2DComponentSectionLabel,
            makeLabel(m_isCollider2DCollapsed, HierarchyPanelController::VisibleItemKind::Collider2DComponent == selectedItemKind, L"Collider2D").c_str());
    }

    bool InspectorPanelController::ToggleSectionCollapseFromInput(const Core::InputSnapshot& inputSnapshot)
    {
        const bool isLeftMouseButtonDown = inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left);
        if (false == isLeftMouseButtonDown || true == m_wasLeftMouseButtonDown)
        {
            return false;
        }

        if (HitTestWindow(m_transformSectionLabel, inputSnapshot.GetCursorScreenPoint()))
        {
            m_isTransformCollapsed = false == m_isTransformCollapsed;
            SaveCollapseState();
            return true;
        }

        if (HitTestWindow(m_spriteComponentSectionLabel, inputSnapshot.GetCursorScreenPoint()))
        {
            m_isSpriteComponentCollapsed = false == m_isSpriteComponentCollapsed;
            SaveCollapseState();
            return true;
        }

        if (HitTestWindow(m_scriptSectionLabel, inputSnapshot.GetCursorScreenPoint()))
        {
            m_isScriptCollapsed = false == m_isScriptCollapsed;
            SaveCollapseState();
            return true;
        }

        if (HitTestWindow(m_materialDetailsSectionLabel, inputSnapshot.GetCursorScreenPoint()))
        {
            m_isMaterialCollapsed = false == m_isMaterialCollapsed;
            SaveCollapseState();
            return true;
        }

        if (HitTestWindow(m_collider2DComponentSectionLabel, inputSnapshot.GetCursorScreenPoint()))
        {
            m_isCollider2DCollapsed = false == m_isCollider2DCollapsed;
            SaveCollapseState();
            return true;
        }

        return false;
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
            || nullptr == m_materialDetailsSectionLabel
            || FALSE == IsWindowVisible(m_materialDetailsSectionLabel))
        {
            return false;
        }

        if (nullptr == m_cursor)
        {
            return false;
        }

        const POINT cursorPoint = ToWin32Point(m_cursor->GetScreenPosition());

        RECT materialRect{};
        GetWindowRect(m_materialDetailsSectionLabel, &materialRect);
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
