#pragma once

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

    private:
        [[nodiscard]] static std::vector<HWND> GetInspectorControls(EditorShell& shell);
        [[nodiscard]] static std::vector<HWND> GetInspectorVisibleControls(EditorShell& shell);
    };
}
