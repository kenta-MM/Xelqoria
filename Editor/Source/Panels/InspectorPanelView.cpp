#include "Panels/InspectorPanelView.h"

#include <algorithm>

#include "Shell/EditorShell.h"

namespace Xelqoria::Editor
{
    namespace
    {
        void AppendArrayControls(std::vector<HWND>& controls, const std::array<HWND, 3>& values)
        {
            controls.insert(controls.end(), values.begin(), values.end());
        }

        void AppendArrayControls(std::vector<HWND>& controls, const std::array<HWND, 4>& values)
        {
            controls.insert(controls.end(), values.begin(), values.end());
        }

        void AppendArrayControls(std::vector<HWND>& controls, const std::array<HWND, 5>& values)
        {
            controls.insert(controls.end(), values.begin(), values.end());
        }

        void AppendArrayControls(std::vector<HWND>& controls, const std::array<HWND, 9>& values)
        {
            controls.insert(controls.end(), values.begin(), values.end());
        }

    }

    std::vector<HWND> InspectorPanelView::GetInspectorControls(EditorShell& shell)
    {
        std::vector<HWND> controls{
            shell.m_inspectorPanel,
            shell.m_inspectorSummaryLabel,
            shell.m_transformSectionLabel
        };
        AppendArrayControls(controls, shell.m_transformLabels);
        AppendArrayControls(controls, shell.m_transformEditControls);
        controls.insert(
            controls.end(),
            {
                shell.m_spriteComponentSectionLabel,
                shell.m_spriteRefLabel,
                shell.m_spriteRefDropHighlight,
                shell.m_spriteRefEdit,
                shell.m_materialOpenButton,
                shell.m_scriptAssetLabel,
                shell.m_scriptAssetEdit,
                shell.m_scriptCreateButton,
                shell.m_scriptAssignButton,
                shell.m_scriptClearButton,
                shell.m_spriteComponentActionButton,
                shell.m_materialSharedNoticeLabel,
                shell.m_materialDetailsSectionLabel
            });
        AppendArrayControls(controls, shell.m_materialDetailLabels);
        AppendArrayControls(controls, shell.m_materialDetailEditControls);
        controls.insert(
            controls.end(),
            {
                shell.m_materialTextureDropHighlight,
                shell.m_materialTextureBrowseButton,
                shell.m_materialTintColorButton,
                shell.m_materialOutlineEnabledCheckBox,
                shell.m_materialOutlineColorButton,
                shell.m_collider2DComponentSectionLabel,
                shell.m_collider2DSummaryLabel,
                shell.m_collider2DEnabledCheckBox,
                shell.m_collider2DTriggerCheckBox,
                shell.m_collider2DShapeTypeLabel,
                shell.m_collider2DShapeTypeEdit,
                shell.m_collider2DOffsetLabel,
                shell.m_collider2DSizeLabel,
                shell.m_collider2DRotationLabel,
                shell.m_collider2DRotationEdit
            });
        AppendArrayControls(controls, shell.m_collider2DEditControls);
        controls.insert(
            controls.end(),
            {
                shell.m_collider2DEditButton,
                shell.m_collider2DComponentActionButton,
                shell.m_addComponentButton
            });
        return controls;
    }

    std::vector<HWND> InspectorPanelView::GetInspectorVisibleControls(EditorShell& shell)
    {
        std::vector<HWND> controls = GetInspectorControls(shell);
        controls.erase(
            std::remove(controls.begin(), controls.end(), shell.m_spriteRefDropHighlight),
            controls.end());
        controls.erase(
            std::remove(controls.begin(), controls.end(), shell.m_materialTextureDropHighlight),
            controls.end());
        return controls;
    }

    InspectorPanelView::InspectorPanelView(EditorShell& shell)
        : EditorPanelViewBase(
            EditorPanelId::Inspector,
            L"Inspector",
            [&shell]() { return GetInspectorControls(shell); },
            [&shell]() { return GetInspectorVisibleControls(shell); },
            []() { return std::vector<HWND>{}; },
            [&shell]() { return std::vector<HWND>{ shell.m_spriteRefDropHighlight, shell.m_materialTextureDropHighlight }; }),
          m_shell(shell)
    {
    }

    HWND InspectorPanelView::GetSummaryLabel() const { return m_shell.m_inspectorSummaryLabel; }
    HWND InspectorPanelView::GetTransformSectionLabel() const { return m_shell.m_transformSectionLabel; }
    const std::array<HWND, 9>& InspectorPanelView::GetTransformEditControls() const { return m_shell.m_transformEditControls; }
    HWND InspectorPanelView::GetSpriteComponentSectionLabel() const { return m_shell.m_spriteComponentSectionLabel; }
    HWND InspectorPanelView::GetSpriteRefLabel() const { return m_shell.m_spriteRefLabel; }
    HWND InspectorPanelView::GetSpriteRefDropHighlight() const { return m_shell.m_spriteRefDropHighlight; }
    HWND InspectorPanelView::GetSpriteRefEdit() const { return m_shell.m_spriteRefEdit; }
    HWND InspectorPanelView::GetScriptAssetLabel() const { return m_shell.m_scriptAssetLabel; }
    HWND InspectorPanelView::GetScriptAssetEdit() const { return m_shell.m_scriptAssetEdit; }
    HWND InspectorPanelView::GetScriptCreateButton() const { return m_shell.m_scriptCreateButton; }
    HWND InspectorPanelView::GetScriptAssignButton() const { return m_shell.m_scriptAssignButton; }
    HWND InspectorPanelView::GetScriptClearButton() const { return m_shell.m_scriptClearButton; }
    HWND InspectorPanelView::GetSpriteComponentActionButton() const { return m_shell.m_spriteComponentActionButton; }
    HWND InspectorPanelView::GetCollider2DComponentSectionLabel() const { return m_shell.m_collider2DComponentSectionLabel; }
    HWND InspectorPanelView::GetCollider2DSummaryLabel() const { return m_shell.m_collider2DSummaryLabel; }
    HWND InspectorPanelView::GetCollider2DEnabledCheckBox() const { return m_shell.m_collider2DEnabledCheckBox; }
    HWND InspectorPanelView::GetCollider2DTriggerCheckBox() const { return m_shell.m_collider2DTriggerCheckBox; }
    HWND InspectorPanelView::GetCollider2DShapeTypeLabel() const { return m_shell.m_collider2DShapeTypeLabel; }
    HWND InspectorPanelView::GetCollider2DShapeTypeEdit() const { return m_shell.m_collider2DShapeTypeEdit; }
    HWND InspectorPanelView::GetCollider2DOffsetLabel() const { return m_shell.m_collider2DOffsetLabel; }
    HWND InspectorPanelView::GetCollider2DSizeLabel() const { return m_shell.m_collider2DSizeLabel; }
    HWND InspectorPanelView::GetCollider2DRotationLabel() const { return m_shell.m_collider2DRotationLabel; }
    HWND InspectorPanelView::GetCollider2DRotationEdit() const { return m_shell.m_collider2DRotationEdit; }
    const std::array<HWND, 4>& InspectorPanelView::GetCollider2DEditControls() const { return m_shell.m_collider2DEditControls; }
    HWND InspectorPanelView::GetCollider2DEditButton() const { return m_shell.m_collider2DEditButton; }
    HWND InspectorPanelView::GetCollider2DComponentActionButton() const { return m_shell.m_collider2DComponentActionButton; }
    HWND InspectorPanelView::GetAddComponentButton() const { return m_shell.m_addComponentButton; }
    HWND InspectorPanelView::GetMaterialOpenButton() const { return m_shell.m_materialOpenButton; }
    HWND InspectorPanelView::GetMaterialSharedNoticeLabel() const { return m_shell.m_materialSharedNoticeLabel; }
    HWND InspectorPanelView::GetMaterialDetailsSectionLabel() const { return m_shell.m_materialDetailsSectionLabel; }
    const std::array<HWND, 5>& InspectorPanelView::GetMaterialDetailLabels() const { return m_shell.m_materialDetailLabels; }
    const std::array<HWND, 5>& InspectorPanelView::GetMaterialDetailEditControls() const { return m_shell.m_materialDetailEditControls; }
    HWND InspectorPanelView::GetMaterialTextureDropHighlight() const { return m_shell.m_materialTextureDropHighlight; }
    HWND InspectorPanelView::GetMaterialTextureBrowseButton() const { return m_shell.m_materialTextureBrowseButton; }
    HWND InspectorPanelView::GetMaterialTintColorButton() const { return m_shell.m_materialTintColorButton; }
    HWND InspectorPanelView::GetMaterialOutlineEnabledCheckBox() const { return m_shell.m_materialOutlineEnabledCheckBox; }
    HWND InspectorPanelView::GetMaterialOutlineColorButton() const { return m_shell.m_materialOutlineColorButton; }
}
