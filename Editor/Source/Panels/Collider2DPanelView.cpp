#include "Panels/Collider2DPanelView.h"

#include "Shell/EditorShell.h"

namespace Xelqoria::Editor
{
    std::vector<HWND> Collider2DPanelView::GetCollider2DControls(EditorShell& shell)
    {
        std::vector<HWND> controls{
            shell.m_collider2DPanel,
            shell.m_collider2DSummaryLabel,
            shell.m_collider2DComponentSectionLabel,
            shell.m_collider2DEnabledCheckBox,
            shell.m_collider2DTriggerCheckBox,
            shell.m_collider2DShapeTypeLabel,
            shell.m_collider2DShapeTypeEdit,
            shell.m_collider2DOffsetLabel,
            shell.m_collider2DSizeLabel
        };
        controls.insert(controls.end(), shell.m_collider2DEditControls.begin(), shell.m_collider2DEditControls.end());
        return controls;
    }

    Collider2DPanelView::Collider2DPanelView(EditorShell& shell)
        : EditorPanelViewBase(
            EditorPanelId::Collider2D,
            L"Collider2D",
            [&shell]() { return GetCollider2DControls(shell); },
            [&shell]() { return GetCollider2DControls(shell); },
            []() { return std::vector<HWND>{}; },
            []() { return std::vector<HWND>{}; })
    {
    }
}
