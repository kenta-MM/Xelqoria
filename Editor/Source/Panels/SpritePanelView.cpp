#include "Panels/SpritePanelView.h"

#include "Shell/EditorShell.h"

namespace Xelqoria::Editor
{
    std::vector<HWND> SpritePanelView::GetSpriteControls(EditorShell& shell)
    {
        std::vector<HWND> controls{
            shell.m_spritePanel,
            shell.m_spriteSummaryLabel,
            shell.m_spriteDetailsSectionLabel
        };
        controls.insert(controls.end(), shell.m_spriteDetailLabels.begin(), shell.m_spriteDetailLabels.end());
        controls.insert(controls.end(), shell.m_spriteDetailEditControls.begin(), shell.m_spriteDetailEditControls.end());
        return controls;
    }

    std::vector<HWND> SpritePanelView::GetSpriteVisibleControls(EditorShell& shell)
    {
        std::vector<HWND> controls{
            shell.m_spritePanel,
            shell.m_spriteDetailsSectionLabel
        };
        controls.insert(controls.end(), shell.m_spriteDetailLabels.begin(), shell.m_spriteDetailLabels.end());
        controls.insert(controls.end(), shell.m_spriteDetailEditControls.begin(), shell.m_spriteDetailEditControls.end());
        return controls;
    }

    SpritePanelView::SpritePanelView(EditorShell& shell)
        : EditorPanelViewBase(
            EditorPanelId::Sprite,
            L"Sprite",
            [&shell]() { return GetSpriteControls(shell); },
            [&shell]() { return GetSpriteVisibleControls(shell); },
            [&shell]() { return std::vector<HWND>{ shell.m_spriteSummaryLabel }; },
            []() { return std::vector<HWND>{}; }),
          m_shell(shell)
    {
    }

    HWND SpritePanelView::GetSummaryLabel() const
    {
        return m_shell.m_spriteSummaryLabel;
    }

    HWND SpritePanelView::GetDetailsSectionLabel() const
    {
        return m_shell.m_spriteDetailsSectionLabel;
    }

    const std::array<HWND, 4>& SpritePanelView::GetDetailLabels() const
    {
        return m_shell.m_spriteDetailLabels;
    }

    const std::array<HWND, 4>& SpritePanelView::GetDetailEditControls() const
    {
        return m_shell.m_spriteDetailEditControls;
    }
}
