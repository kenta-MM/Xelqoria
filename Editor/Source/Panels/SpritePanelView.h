#pragma once

#include <array>

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Sprite Panel の HWND 群を管理する View。
    /// </summary>
    class SpritePanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した Sprite control 群へ接続する。
        /// </summary>
        explicit SpritePanelView(EditorPanelHostContext& hostContext);

        bool Initialize(HWND parentWindow, HINSTANCE hInstance) override;
        void Layout(const RECT& bounds) override;

        [[nodiscard]] HWND GetSummaryLabel() const;
        [[nodiscard]] HWND GetDetailsSectionLabel() const;
        [[nodiscard]] const std::array<HWND, 4>& GetDetailLabels() const;
        [[nodiscard]] const std::array<HWND, 4>& GetDetailEditControls() const;

    private:
        [[nodiscard]] std::vector<HWND> BuildControls() const;
        [[nodiscard]] std::vector<HWND> BuildVisibleControls() const;

        HWND m_panel = nullptr;
        HWND m_summaryLabel = nullptr;
        HWND m_detailsSectionLabel = nullptr;
        std::array<HWND, 4> m_detailLabels{};
        std::array<HWND, 4> m_detailEditControls{};
    };
}
