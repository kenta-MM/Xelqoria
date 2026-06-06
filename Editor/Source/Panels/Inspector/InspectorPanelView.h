#pragma once

#include <array>

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Inspector Panel の HWND 群を管理する View。
    /// </summary>
    class InspectorPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// Inspector control 群を生成・管理する View を作成する。
        /// </summary>
        explicit InspectorPanelView(EditorPanelHostContext& hostContext);

        bool Initialize(HWND parentWindow, HINSTANCE hInstance) override;
        void Layout(const RECT& bounds) override;

        [[nodiscard]] HWND GetSummaryLabel() const;
        [[nodiscard]] const std::array<HWND, 3>& GetTransformLabels() const;
        [[nodiscard]] HWND GetTransformSectionLabel() const;
        [[nodiscard]] const std::array<HWND, 9>& GetTransformEditControls() const;
        [[nodiscard]] HWND GetSpriteComponentSectionLabel() const;
        [[nodiscard]] HWND GetSpriteRefLabel() const;
        [[nodiscard]] HWND GetSpriteRefDropHighlight() const;
        [[nodiscard]] HWND GetSpriteRefEdit() const;
        [[nodiscard]] HWND GetScriptSectionLabel() const;
        [[nodiscard]] HWND GetScriptAssetLabel() const;
        [[nodiscard]] HWND GetScriptAssetEdit() const;
        [[nodiscard]] HWND GetScriptCreateButton() const;
        [[nodiscard]] HWND GetScriptAssignButton() const;
        [[nodiscard]] HWND GetScriptClearButton() const;
        [[nodiscard]] HWND GetSpriteComponentActionButton() const;
        [[nodiscard]] HWND GetCollider2DComponentSectionLabel() const;
        [[nodiscard]] HWND GetCollider2DSummaryLabel() const;
        [[nodiscard]] HWND GetCollider2DEnabledCheckBox() const;
        [[nodiscard]] HWND GetCollider2DTriggerCheckBox() const;
        [[nodiscard]] HWND GetCollider2DShapeTypeLabel() const;
        [[nodiscard]] HWND GetCollider2DShapeTypeEdit() const;
        [[nodiscard]] HWND GetCollider2DOffsetLabel() const;
        [[nodiscard]] HWND GetCollider2DSizeLabel() const;
        [[nodiscard]] HWND GetCollider2DRotationLabel() const;
        [[nodiscard]] HWND GetCollider2DRotationEdit() const;
        [[nodiscard]] const std::array<HWND, 4>& GetCollider2DEditControls() const;
        [[nodiscard]] HWND GetCollider2DEditButton() const;
        [[nodiscard]] HWND GetCollider2DComponentActionButton() const;
        [[nodiscard]] HWND GetAddComponentButton() const;
        [[nodiscard]] HWND GetMaterialOpenButton() const;
        [[nodiscard]] HWND GetMaterialSharedNoticeLabel() const;
        [[nodiscard]] HWND GetMaterialDetailsSectionLabel() const;
        [[nodiscard]] const std::array<HWND, 5>& GetMaterialDetailLabels() const;
        [[nodiscard]] const std::array<HWND, 5>& GetMaterialDetailEditControls() const;
        [[nodiscard]] HWND GetMaterialTextureDropHighlight() const;
        [[nodiscard]] HWND GetMaterialTextureBrowseButton() const;
        [[nodiscard]] HWND GetMaterialTintColorButton() const;
        [[nodiscard]] HWND GetMaterialOutlineEnabledCheckBox() const;
        [[nodiscard]] HWND GetMaterialOutlineColorButton() const;
        [[nodiscard]] HWND GetMaterialRemoveButton() const;
        [[nodiscard]] bool IsInputControl(HWND window) const;
        [[nodiscard]] bool IsSecondaryLabel(HWND window) const;
        [[nodiscard]] bool IsSectionLabel(HWND window) const;

    private:
        [[nodiscard]] std::vector<HWND> BuildControls() const;
        [[nodiscard]] std::vector<HWND> BuildVisibleControls() const;

        HWND m_panel = nullptr;
        HWND m_summaryLabel = nullptr;
        HWND m_transformSectionLabel = nullptr;
        std::array<HWND, 3> m_transformLabels{};
        std::array<HWND, 9> m_transformEditControls{};
        HWND m_spriteComponentSectionLabel = nullptr;
        HWND m_spriteRefLabel = nullptr;
        HWND m_spriteRefDropHighlight = nullptr;
        HWND m_spriteRefEdit = nullptr;
        HWND m_materialOpenButton = nullptr;
        HWND m_scriptSectionLabel = nullptr;
        HWND m_scriptAssetLabel = nullptr;
        HWND m_scriptAssetEdit = nullptr;
        HWND m_scriptCreateButton = nullptr;
        HWND m_scriptAssignButton = nullptr;
        HWND m_scriptClearButton = nullptr;
        HWND m_spriteComponentActionButton = nullptr;
        HWND m_collider2DComponentActionButton = nullptr;
        HWND m_addComponentButton = nullptr;
        HWND m_collider2DComponentSectionLabel = nullptr;
        HWND m_collider2DSummaryLabel = nullptr;
        HWND m_collider2DEnabledCheckBox = nullptr;
        HWND m_collider2DTriggerCheckBox = nullptr;
        HWND m_collider2DShapeTypeLabel = nullptr;
        HWND m_collider2DShapeTypeEdit = nullptr;
        HWND m_collider2DOffsetLabel = nullptr;
        HWND m_collider2DSizeLabel = nullptr;
        HWND m_collider2DRotationLabel = nullptr;
        HWND m_collider2DRotationEdit = nullptr;
        std::array<HWND, 4> m_collider2DEditControls{};
        HWND m_collider2DEditButton = nullptr;
        HWND m_materialSharedNoticeLabel = nullptr;
        HWND m_materialDetailsSectionLabel = nullptr;
        std::array<HWND, 5> m_materialDetailLabels{};
        std::array<HWND, 5> m_materialDetailEditControls{};
        HWND m_materialTextureDropHighlight = nullptr;
        HWND m_materialTextureBrowseButton = nullptr;
        HWND m_materialTintColorButton = nullptr;
        HWND m_materialOutlineEnabledCheckBox = nullptr;
        HWND m_materialOutlineColorButton = nullptr;
        HWND m_materialRemoveButton = nullptr;
    };
}
