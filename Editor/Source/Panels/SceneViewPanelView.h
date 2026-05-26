#pragma once

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    class EditorShell;

    /// <summary>
    /// SceneView Panel の HWND 群を管理する View。
    /// </summary>
    class SceneViewPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した SceneView control 群へ接続する。
        /// </summary>
        explicit SceneViewPanelView(EditorShell& shell);
    };
}
