#pragma once

#include <array>

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Material Panel の HWND 群を管理する View。
    /// </summary>
    class MaterialPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した Material control 群へ接続する。
        /// </summary>
        explicit MaterialPanelView(EditorPanelHostContext& hostContext);

        bool Initialize(HWND parentWindow, HINSTANCE hInstance) override;
        void Layout(const RECT& bounds) override;

        [[nodiscard]] HWND GetSummaryLabel() const;
        [[nodiscard]] HWND GetSharedNoticeLabel() const;
        [[nodiscard]] HWND GetDetailsSectionLabel() const;
        [[nodiscard]] const std::array<HWND, 5>& GetDetailLabels() const;
        [[nodiscard]] const std::array<HWND, 5>& GetDetailEditControls() const;
        [[nodiscard]] HWND GetTextureDropHighlight() const;

    private:
        [[nodiscard]] std::vector<HWND> BuildControls() const;
        [[nodiscard]] std::vector<HWND> BuildVisibleControls() const;

        HWND m_panel = nullptr;
        HWND m_summaryLabel = nullptr;
        HWND m_sharedNoticeLabel = nullptr;
        HWND m_detailsSectionLabel = nullptr;
        std::array<HWND, 5> m_detailLabels{};
        std::array<HWND, 5> m_detailEditControls{};
        HWND m_textureBrowseButton = nullptr;
        HWND m_tintColorButton = nullptr;
        HWND m_outlineEnabledCheckBox = nullptr;
        HWND m_outlineColorButton = nullptr;
        HWND m_textureDropHighlight = nullptr;
    };
}
