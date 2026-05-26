#pragma once

#include <array>

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    class EditorShell;

    /// <summary>
    /// Material Panel の HWND 群を管理する View。
    /// </summary>
    class MaterialPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した Material control 群へ接続する。
        /// </summary>
        explicit MaterialPanelView(EditorShell& shell);

        [[nodiscard]] HWND GetSummaryLabel() const;
        [[nodiscard]] HWND GetSharedNoticeLabel() const;
        [[nodiscard]] HWND GetDetailsSectionLabel() const;
        [[nodiscard]] const std::array<HWND, 5>& GetDetailLabels() const;
        [[nodiscard]] const std::array<HWND, 5>& GetDetailEditControls() const;
        [[nodiscard]] HWND GetTextureDropHighlight() const;

    private:
        [[nodiscard]] static std::vector<HWND> GetMaterialControls(EditorShell& shell);
        [[nodiscard]] static std::vector<HWND> GetMaterialVisibleControls(EditorShell& shell);

        EditorShell& m_shell;
    };
}
