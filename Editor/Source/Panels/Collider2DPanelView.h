#pragma once

#include <array>

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Collider2D Panel の HWND 群を管理する View。
    /// </summary>
    class Collider2DPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した Collider2D control 群へ接続する。
        /// </summary>
        explicit Collider2DPanelView(EditorPanelHostContext& hostContext);

        bool Initialize(HWND parentWindow, HINSTANCE hInstance) override;
        void Layout(const RECT& bounds) override;

    private:
        [[nodiscard]] std::vector<HWND> BuildControls() const;

        HWND m_panel = nullptr;
        HWND m_summaryLabel = nullptr;
        HWND m_componentSectionLabel = nullptr;
        HWND m_enabledCheckBox = nullptr;
        HWND m_triggerCheckBox = nullptr;
        HWND m_shapeTypeLabel = nullptr;
        HWND m_shapeTypeEdit = nullptr;
        HWND m_offsetLabel = nullptr;
        HWND m_sizeLabel = nullptr;
        std::array<HWND, 4> m_editControls{};
    };
}
