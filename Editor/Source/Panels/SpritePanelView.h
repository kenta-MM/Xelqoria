#pragma once

#include <array>

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    class EditorShell;

    /// <summary>
    /// Sprite Panel の HWND 群を管理する View。
    /// </summary>
    class SpritePanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した Sprite control 群へ接続する。
        /// </summary>
        explicit SpritePanelView(EditorShell& shell);

        [[nodiscard]] HWND GetSummaryLabel() const;
        [[nodiscard]] HWND GetDetailsSectionLabel() const;
        [[nodiscard]] const std::array<HWND, 4>& GetDetailLabels() const;
        [[nodiscard]] const std::array<HWND, 4>& GetDetailEditControls() const;

    private:
        [[nodiscard]] static std::vector<HWND> GetSpriteControls(EditorShell& shell);
        [[nodiscard]] static std::vector<HWND> GetSpriteVisibleControls(EditorShell& shell);

        EditorShell& m_shell;
    };
}
