#pragma once

#include "Panels/EditorPanelViewBase.h"
#include "SceneViewSurface.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// SceneView Panel の HWND 群を管理する View。
    /// </summary>
    class SceneViewPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// EditorShell が生成した SceneView control 群へ接続する。
        /// </summary>
        explicit SceneViewPanelView(EditorPanelHostContext& hostContext);

        bool Initialize(HWND parentWindow, HINSTANCE hInstance) override;
        void Layout(const RECT& bounds) override;

        [[nodiscard]] SceneViewSurface GetSceneViewSurface() const;
        [[nodiscard]] HWND GetSceneViewHost() const;
        [[nodiscard]] HWND GetSceneViewPlanLabel() const;
        [[nodiscard]] HWND GetSceneViewSizeLabel() const;
        [[nodiscard]] HWND GetProjectSummaryLabel() const;
        [[nodiscard]] HWND GetProjectSceneListBox() const;
        [[nodiscard]] HWND GetProjectSceneDetailLabel() const;
        [[nodiscard]] HWND GetBuildAndPlayButton() const;
        [[nodiscard]] HWND GetPauseResumePlayButton() const;
        [[nodiscard]] HWND GetEndPlayButton() const;

    private:
        HWND m_panel = nullptr;
        HWND m_sceneViewPlanLabel = nullptr;
        HWND m_projectSummaryLabel = nullptr;
        HWND m_projectSceneListBox = nullptr;
        HWND m_projectSceneDetailLabel = nullptr;
        HWND m_sceneViewHost = nullptr;
        HWND m_sceneViewSizeLabel = nullptr;
        HWND m_buildAndPlayButton = nullptr;
        HWND m_pauseResumePlayButton = nullptr;
        HWND m_endPlayButton = nullptr;
        std::uint32_t m_sceneViewWidth = 0;
        std::uint32_t m_sceneViewHeight = 0;
    };
}
