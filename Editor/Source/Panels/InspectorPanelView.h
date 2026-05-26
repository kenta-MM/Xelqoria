#pragma once

#include <array>

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    class EditorShell;

    /// <summary>
    /// Inspector Panel の HWND 群を管理する View。
    /// </summary>
    class InspectorPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した Inspector control 群へ接続する。
        /// </summary>
        explicit InspectorPanelView(EditorShell& shell);

        [[nodiscard]] HWND GetSummaryLabel() const;
        [[nodiscard]] HWND GetTransformSectionLabel() const;
        [[nodiscard]] const std::array<HWND, 9>& GetTransformEditControls() const;
        [[nodiscard]] HWND GetSpriteComponentSectionLabel() const;
        [[nodiscard]] HWND GetSpriteRefLabel() const;
        [[nodiscard]] HWND GetSpriteRefDropHighlight() const;
        [[nodiscard]] HWND GetSpriteRefEdit() const;
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

    private:
        [[nodiscard]] static std::vector<HWND> GetInspectorControls(EditorShell& shell);
        [[nodiscard]] static std::vector<HWND> GetInspectorVisibleControls(EditorShell& shell);

        EditorShell& m_shell;
    };
}
