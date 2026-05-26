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
            [&shell]() { return std::vector<HWND>{ shell.m_spriteRefDropHighlight, shell.m_materialTextureDropHighlight }; })
    {
    }
}
