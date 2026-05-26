#pragma once

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

    private:
        [[nodiscard]] static std::vector<HWND> GetMaterialControls(EditorShell& shell);
        [[nodiscard]] static std::vector<HWND> GetMaterialVisibleControls(EditorShell& shell);
    };
}
