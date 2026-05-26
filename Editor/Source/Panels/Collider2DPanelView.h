#pragma once

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    class EditorShell;

    /// <summary>
    /// Collider2D Panel の HWND 群を管理する View。
    /// </summary>
    class Collider2DPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した Collider2D control 群へ接続する。
        /// </summary>
        explicit Collider2DPanelView(EditorShell& shell);

    private:
        [[nodiscard]] static std::vector<HWND> GetCollider2DControls(EditorShell& shell);
    };
}
