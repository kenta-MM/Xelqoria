#pragma once

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    class EditorShell;

    /// <summary>
    /// Hierarchy Panel の HWND 群を管理する View。
    /// </summary>
    class HierarchyPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した Hierarchy control 群へ接続する。
        /// </summary>
        explicit HierarchyPanelView(EditorShell& shell);
    };
}
