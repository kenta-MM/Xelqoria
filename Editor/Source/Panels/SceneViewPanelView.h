#pragma once

#include "Panels/EditorPanelViewBase.h"
#include "SceneViewSurface.h"

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

        [[nodiscard]] SceneViewSurface GetSceneViewSurface() const;
        [[nodiscard]] HWND GetSceneViewPlanLabel() const;
        [[nodiscard]] HWND GetSceneViewSizeLabel() const;
        [[nodiscard]] HWND GetProjectSummaryLabel() const;
        [[nodiscard]] HWND GetProjectSceneListBox() const;
        [[nodiscard]] HWND GetProjectSceneDetailLabel() const;

    private:
        EditorShell& m_shell;
    };
}
