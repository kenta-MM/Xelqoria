#pragma once

#include <Windows.h>
#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "InputSystem.h"
#include "Shell/EditorPanelId.h"

namespace Xelqoria::Editor
{
    struct EditorDockingLayoutSnapshot;
    class EditorDockingLayoutSerializer;
    class EditorShell;

    /// <summary>
    /// Editor の Dock/Floating 状態管理と操作を担当する。
    /// </summary>
    class EditorDockingController
    {
    public:
        explicit EditorDockingController(EditorShell& shell);
        ~EditorDockingController();

        [[nodiscard]] bool Initialize(HWND parentWindow, HINSTANCE hInstance);
        void Shutdown();

        void Layout(const RECT& rootRect);
        void HideInactivePanelControls();
        void HideDockGuideWindows();
        void CollectControls(std::vector<HWND>& controls) const;

        [[nodiscard]] bool Update(HWND parentWindow, const Core::InputSnapshot& inputSnapshot);
        [[nodiscard]] bool HandleNotify(LPARAM notifyParameter);
        void ResetLayout();
        void ShowPanelAtDefaultDock(EditorPanelId panelId);
        void ActivatePanel(EditorPanelId panelId);

        [[nodiscard]] bool SaveLayout(const std::filesystem::path& layoutPath) const;
        [[nodiscard]] bool LoadLayout(const std::filesystem::path& layoutPath);

        /// <summary>
        /// 現在の Dock/Floating 状態を保存用 Snapshot として作成する。
        /// </summary>
        /// <returns>保存形式に依存しない Dock/Floating レイアウト Snapshot。</returns>
        [[nodiscard]] EditorDockingLayoutSnapshot CreateLayoutSnapshot() const;

        /// <summary>
        /// 保存用 Snapshot を現在の Dock/Floating UI 状態へ反映する。
        /// </summary>
        /// <param name="snapshot">復元する Dock/Floating レイアウト Snapshot。</param>
        /// <returns>復元できた場合は true。</returns>
        [[nodiscard]] bool ApplyLayoutSnapshot(const EditorDockingLayoutSnapshot& snapshot);

        [[nodiscard]] bool DrawEditorTabControl(const DRAWITEMSTRUCT& drawItem) const;

        static LRESULT CALLBACK FloatingPanelWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    private:
        enum class DockAreaId
        {
            LeftTop,
            LeftBottom,
            Center,
            Right,
            Floating
        };

        enum class DockNodeKind
        {
            Leaf,
            Split
        };

        enum class DockSplitOrientation
        {
            Horizontal,
            Vertical
        };

        enum class DockGuideTargetKind
        {
            None,
            Tab,
            SplitLeft,
            SplitRight,
            SplitTop,
            SplitBottom,
            Float
        };

        enum class DockDragKind
        {
            None,
            Panel,
            VerticalSplitter,
            HorizontalSplitter
        };

        using DockNodeId = int;

        struct DockNode
        {
            DockNodeKind kind = DockNodeKind::Leaf;
            DockSplitOrientation splitOrientation = DockSplitOrientation::Horizontal;
            float splitRatio = 0.5f;
            DockNodeId firstChild = -1;
            DockNodeId secondChild = -1;
            std::vector<EditorPanelId> panels{};
            int activeTabIndex = 0;
            RECT rect{};
            HWND tabControl = nullptr;
        };

        struct DockGuideTarget
        {
            DockGuideTargetKind kind = DockGuideTargetKind::None;
            DockNodeId dockNodeId = -1;
            RECT previewRect{};
        };

        struct FloatingPanelCreateParams
        {
            EditorDockingController* controller = nullptr;
            EditorPanelId panelId = EditorPanelId::SceneView;
        };

        struct FloatingPanelWindowData
        {
            EditorDockingController* controller = nullptr;
            EditorPanelId panelId = EditorPanelId::SceneView;
        };

        struct FloatingPanelGroup
        {
            HWND window = nullptr;
            HWND tabControl = nullptr;
            std::vector<EditorPanelId> panels{};
            int activeTabIndex = 0;
        };

        struct DockingState
        {
            HWND leftTopDockTab = nullptr;
            HWND leftBottomDockTab = nullptr;
            HWND centerDockTab = nullptr;
            HWND rightDockTab = nullptr;
            HWND logOutputDockTab = nullptr;
            std::vector<HWND> dynamicDockTabs{};
            HWND dockPreviewWindow = nullptr;
            std::array<HWND, 9> dockGuideWindows{};
            HWND hierarchyFloatingWindow = nullptr;
            HWND assetsFloatingWindow = nullptr;
            HWND sceneViewFloatingWindow = nullptr;
            HWND inspectorFloatingWindow = nullptr;
            HWND logOutputFloatingWindow = nullptr;
            std::vector<FloatingPanelGroup> floatingPanelGroups{};
            std::vector<EditorPanelId> leftTopDockPanels{};
            std::vector<EditorPanelId> leftBottomDockPanels{};
            std::vector<EditorPanelId> centerDockPanels{};
            std::vector<EditorPanelId> rightDockPanels{};
            std::vector<DockNode> dockNodes{};
            DockNodeId rootDockNodeId = -1;
            DockGuideTarget currentGuideTarget{};
            int leftTopActiveTabIndex = 0;
            int leftBottomActiveTabIndex = 0;
            int centerActiveTabIndex = 0;
            int rightActiveTabIndex = 0;
            int leftPaneWidth = 260;
            int rightPaneWidth = 300;
            int leftTopHeight = 280;
            DockDragKind dragKind = DockDragKind::None;
            std::optional<EditorPanelId> dragPanelId{};
            std::optional<EditorPanelId> pendingDockDragPanelId{};
            bool hasDockPreview = false;
            RECT dockPreviewRect{};
            POINT dragStartScreenPoint{};
            POINT dragPanelWindowOffset{};
            ULONGLONG pendingDockDragStartTick = 0;
            DockNodeId dragSplitNodeId = -1;
            float dragStartSplitRatio = 0.5f;
            int dragStartLeftPaneWidth = 0;
            int dragStartRightPaneWidth = 0;
            int dragStartLeftTopHeight = 0;
        };

        void LayoutDockArea(DockAreaId dockAreaId, const RECT& areaRect);
        void BuildInitialDockTree();
        void LayoutDockNode(DockNodeId dockNodeId, const RECT& nodeRect);
        void LayoutDockLeaf(DockNodeId dockNodeId, const RECT& areaRect);
        [[nodiscard]] DockNodeId AddDockNode(DockNode node);
        [[nodiscard]] DockNodeId EnsureDefaultDockLeaf(EditorPanelId panelId);
        [[nodiscard]] HWND GetDefaultDockTabControl(EditorPanelId panelId);
        [[nodiscard]] DockNodeId HitTestDockLeaf(POINT cursorClientPoint) const;
        [[nodiscard]] DockNodeId HitTestDockSplitter(POINT cursorScreenPoint) const;
        void CollectReachableDockLeaves(DockNodeId dockNodeId, std::vector<DockNodeId>& dockLeafNodeIds) const;
        void CollectReachableDockSplits(DockNodeId dockNodeId, std::vector<DockNodeId>& dockSplitNodeIds) const;
        [[nodiscard]] bool UpdateDockSplitterDrag(HWND parentWindow, POINT cursorScreenPoint);
        [[nodiscard]] float ClampDockSplitRatio(DockNodeId dockNodeId, float ratio) const;
        [[nodiscard]] DockNodeId FindPanelDockLeaf(EditorPanelId panelId) const;
        [[nodiscard]] bool IsPanelInDockTree(EditorPanelId panelId) const;
        void RemovePanelFromDockTree(EditorPanelId panelId, bool collapseEmptyLeaves = true);
        void RemovePanelFromDockNode(DockNode& dockNode, EditorPanelId panelId) const;
        [[nodiscard]] bool CollapseEmptyDockLeaves(DockNodeId dockNodeId);
        void ApplyDockGuideTarget(EditorPanelId panelId, const DockGuideTarget& guideTarget, HWND parentWindow);
        [[nodiscard]] DockGuideTarget HitTestDockGuideTarget(HWND parentWindow, POINT cursorScreenPoint) const;
        void UpdateDockGuideWindows(HWND parentWindow, POINT cursorScreenPoint);
        void ShowDockGuideWindow(HWND guideWindow, const RECT& guideRect);
        [[nodiscard]] RECT GetPanelCaptionRect(EditorPanelId panelId) const;
        [[nodiscard]] std::optional<EditorPanelId> HitTestPanelCaption(POINT cursorScreenPoint) const;
        [[nodiscard]] std::optional<EditorPanelId> HitTestDockTab(POINT cursorScreenPoint) const;
        [[nodiscard]] DockAreaId HitTestDockArea(HWND parentWindow, POINT cursorScreenPoint) const;
        void MovePanelToDockArea(EditorPanelId panelId, DockAreaId dockAreaId, HWND parentWindow);
        void BeginDockPanelDrag(EditorPanelId panelId, HWND parentWindow, POINT cursorScreenPoint);
        void UpdateDockPanelDragWindow(EditorPanelId panelId, POINT cursorScreenPoint);
        void FloatPanel(EditorPanelId panelId, POINT screenPoint, HWND parentWindow);
        void LayoutFloatingPanel(EditorPanelId panelId, HWND floatingWindow);
        void LayoutFloatingWindow(HWND floatingWindow);
        void AttachPanelToFloatingWindow(EditorPanelId panelId, HWND floatingWindow);
        [[nodiscard]] HWND HitTestFloatingWindow(POINT cursorScreenPoint, HWND excludedWindow) const;
        void BeginFloatingWindowDockDrag(EditorPanelId panelId);
        void UpdateFloatingWindowDockDrag();
        void CompleteFloatingWindowDockDrag(EditorPanelId panelId);
        void HandleFloatingWindowClose(EditorPanelId panelId, HWND floatingWindow);
        void DestroyFloatingWindow(EditorPanelId panelId);
        [[nodiscard]] HWND& GetFloatingWindowRef(EditorPanelId panelId);
        void SyncFloatingPanelTabs(HWND floatingWindow);
        [[nodiscard]] int FindFloatingPanelGroupIndex(HWND floatingWindow) const;
        [[nodiscard]] int FindFloatingPanelGroupIndex(EditorPanelId panelId) const;
        [[nodiscard]] EditorPanelId GetActiveFloatingPanel(HWND floatingWindow) const;
        void SyncDockTabs();
        [[nodiscard]] HWND CreateAdditionalDockTabControl(HWND parentWindow);
        [[nodiscard]] HWND CreateDockTabControlForLayoutKey(const std::wstring& layoutKey);
        void ConfigureEditorTabControl(HWND tabControl) const;
        [[nodiscard]] std::wstring GetDockTabLayoutKey(HWND tabControl) const;
        void RestoreMissingPanelsToDefaultDock();
        void SyncDockAreaTabs(DockAreaId dockAreaId);
        [[nodiscard]] HWND GetDockAreaTabControl(DockAreaId dockAreaId) const;
        [[nodiscard]] HWND GetDockLeafTabControl(DockNodeId dockNodeId) const;
        [[nodiscard]] std::vector<EditorPanelId>& GetDockAreaPanels(DockAreaId dockAreaId);
        [[nodiscard]] const std::vector<EditorPanelId>& GetDockAreaPanels(DockAreaId dockAreaId) const;
        [[nodiscard]] static const wchar_t* GetPanelTitle(EditorPanelId panelId);
        [[nodiscard]] int ClampActiveTabIndex(DockAreaId dockAreaId) const;
        [[nodiscard]] RECT GetDockAreaPreviewRect(HWND parentWindow, DockAreaId dockAreaId) const;
        void UpdateDockPreviewWindow(HWND parentWindow);

        EditorShell& m_shell;
        DockingState m_state{};
        std::unique_ptr<EditorDockingLayoutSerializer> m_layoutSerializer{};
    };
}
