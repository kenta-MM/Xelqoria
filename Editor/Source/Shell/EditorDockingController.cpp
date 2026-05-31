#include "Shell/EditorDockingController.h"

#include <algorithm>
#include <array>
#include <CommCtrl.h>
#include <cstdlib>
#include <cwchar>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <Windows.h>

#include "EditorTheme.h"
#include "ICursor.h"
#include "Panels/IEditorPanelView.h"
#include "Shell/EditorDockingLayoutSnapshot.h"
#include "Shell/EditorDockingLayoutSerializer.h"
#include "Shell/EditorShell.h"

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr UINT_PTR EditorTabControlSubclassId = 2;
        constexpr const wchar_t* DockPreviewWindowClassName = L"XelqoriaDockPreviewWindow";
        constexpr const wchar_t* DockGuideWindowClassName = L"XelqoriaDockGuideWindow";
        constexpr const wchar_t* FloatingPanelWindowClassName = L"XelqoriaFloatingPanelWindow";
        constexpr ULONGLONG DockPanelDragDelayMilliseconds = 200;

        [[nodiscard]] BYTE ToColorByte(float value)
        {
            const float clampedValue = (std::max)(0.0f, (std::min)(1.0f, value));
            return static_cast<BYTE>((clampedValue * 255.0f) + 0.5f);
        }

        [[nodiscard]] COLORREF ToColorRef(EditorColor color)
        {
            return RGB(ToColorByte(color.red), ToColorByte(color.green), ToColorByte(color.blue));
        }

        void FillRectWithThemeColor(HDC deviceContext, const RECT& rect, EditorColor color)
        {
            HBRUSH brush = CreateSolidBrush(ToColorRef(color));
            FillRect(deviceContext, &rect, brush);
            DeleteObject(brush);
        }

        void FillRoundRectWithThemeColor(HDC deviceContext, const RECT& rect, EditorColor color, int radius)
        {
            HBRUSH brush = CreateSolidBrush(ToColorRef(color));
            HGDIOBJ previousBrush = SelectObject(deviceContext, brush);
            HGDIOBJ previousPen = SelectObject(deviceContext, GetStockObject(NULL_PEN));
            RoundRect(deviceContext, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
            SelectObject(deviceContext, previousPen);
            SelectObject(deviceContext, previousBrush);
            DeleteObject(brush);
        }

        void FillTabControlBackground(HWND tabControl, HDC deviceContext)
        {
            if (nullptr == tabControl || nullptr == deviceContext)
            {
                return;
            }

            RECT clientRect{};
            GetClientRect(tabControl, &clientRect);
            HRGN backgroundRegion = CreateRectRgn(clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
            if (nullptr == backgroundRegion)
            {
                FillRectWithThemeColor(deviceContext, clientRect, EditorColor::FromRgb8(0x13, 0x0F, 0x2A));
                return;
            }

            const int itemCount = TabCtrl_GetItemCount(tabControl);
            for (int index = 0; index < itemCount; ++index)
            {
                RECT itemRect{};
                if (FALSE == TabCtrl_GetItemRect(tabControl, index, &itemRect))
                {
                    continue;
                }

                InflateRect(&itemRect, 1, 1);
                HRGN itemRegion = CreateRectRgn(itemRect.left, itemRect.top, itemRect.right, itemRect.bottom);
                if (nullptr != itemRegion)
                {
                    CombineRgn(backgroundRegion, backgroundRegion, itemRegion, RGN_DIFF);
                    DeleteObject(itemRegion);
                }
            }

            HBRUSH backgroundBrush = CreateSolidBrush(ToColorRef(EditorColor::FromRgb8(0x13, 0x0F, 0x2A)));
            FillRgn(deviceContext, backgroundRegion, backgroundBrush);
            DeleteObject(backgroundBrush);
            DeleteObject(backgroundRegion);
        }

        [[nodiscard]] constexpr std::array<EditorPanelId, 5> GetAllEditorPanels()
        {
            return {
                EditorPanelId::Hierarchy,
                EditorPanelId::Assets,
                EditorPanelId::SceneView,
                EditorPanelId::Inspector,
                EditorPanelId::LogOutput
            };
        }

        [[nodiscard]] constexpr bool IsDockableEditorPanel(EditorPanelId panelId)
        {
            switch (panelId)
            {
            case EditorPanelId::Hierarchy:
            case EditorPanelId::Assets:
            case EditorPanelId::SceneView:
            case EditorPanelId::Inspector:
            case EditorPanelId::LogOutput:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]] POINT ToWin32Point(Platform::Point point)
        {
            return POINT{ static_cast<LONG>(point.x), static_cast<LONG>(point.y) };
        }

        [[nodiscard]] POINT GetCursorScreenPoint(const Platform::ICursor* cursor)
        {
            if (nullptr == cursor)
            {
                return {};
            }

            return ToWin32Point(cursor->GetScreenPosition());
        }

        [[nodiscard]] int GetHoveredTabIndex(HWND tabControl)
        {
            POINT cursorPoint{};
            if (FALSE == GetCursorPos(&cursorPoint))
            {
                return -1;
            }

            ScreenToClient(tabControl, &cursorPoint);
            TCHITTESTINFO hitTestInfo{};
            hitTestInfo.pt = cursorPoint;
            return TabCtrl_HitTest(tabControl, &hitTestInfo);
        }
    }

    EditorDockingController::EditorDockingController(EditorShell& shell)
        : m_shell(shell)
        , m_layoutSerializer(std::make_unique<EditorDockingLayoutSerializer>(*this))
    {
    }

    EditorDockingController::~EditorDockingController() = default;
    bool EditorDockingController::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD tabStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_OWNERDRAWFIXED;
        m_state.leftTopDockTab = m_shell.CreateChildWindow(parentWindow, hInstance, WC_TABCONTROLW, L"", tabStyle);
        m_state.leftBottomDockTab = m_shell.CreateChildWindow(parentWindow, hInstance, WC_TABCONTROLW, L"", tabStyle);
        m_state.centerDockTab = m_shell.CreateChildWindow(parentWindow, hInstance, WC_TABCONTROLW, L"", tabStyle);
        m_state.rightDockTab = m_shell.CreateChildWindow(parentWindow, hInstance, WC_TABCONTROLW, L"", tabStyle);
        ConfigureEditorTabControl(m_state.leftTopDockTab);
        ConfigureEditorTabControl(m_state.leftBottomDockTab);
        ConfigureEditorTabControl(m_state.centerDockTab);
        ConfigureEditorTabControl(m_state.rightDockTab);

        m_state.dockPreviewWindow = m_shell.CreateChildWindow(
            parentWindow,
            hInstance,
            DockPreviewWindowClassName,
            L"",
            WS_CHILD | WS_CLIPSIBLINGS);

        const std::array<const wchar_t*, 9> guideTexts{
            L"top",
            L"bottom",
            L"left",
            L"right",
            L"tab",
            L"top",
            L"bottom",
            L"left",
            L"right"
        };
        for (std::size_t index = 0; index < m_state.dockGuideWindows.size(); ++index)
        {
            m_state.dockGuideWindows[index] = m_shell.CreateChildWindow(
                parentWindow,
                hInstance,
                DockGuideWindowClassName,
                guideTexts[index],
                WS_CHILD | WS_CLIPSIBLINGS);
            if (nullptr == m_state.dockGuideWindows[index])
            {
                return false;
            }
            ShowWindow(m_state.dockGuideWindows[index], SW_HIDE);
        }

        if (nullptr == m_state.leftTopDockTab
            || nullptr == m_state.leftBottomDockTab
            || nullptr == m_state.centerDockTab
            || nullptr == m_state.rightDockTab
            || nullptr == m_state.dockPreviewWindow)
        {
            return false;
        }

        ShowWindow(m_state.dockPreviewWindow, SW_HIDE);
        return true;
    }

    void EditorDockingController::Shutdown()
    {
        DestroyFloatingWindow(EditorPanelId::Hierarchy);
        DestroyFloatingWindow(EditorPanelId::Assets);
        DestroyFloatingWindow(EditorPanelId::SceneView);
        DestroyFloatingWindow(EditorPanelId::Inspector);
        DestroyFloatingWindow(EditorPanelId::LogOutput);

        for (HWND tabControl : m_state.dynamicDockTabs)
        {
            if (nullptr != tabControl)
            {
                DestroyWindow(tabControl);
            }
        }
        m_state.dynamicDockTabs.clear();
    }

    void EditorDockingController::Layout(const RECT& rootRect)
    {
        LayoutDockNode(m_state.rootDockNodeId, rootRect);
    }

    bool EditorDockingController::SaveLayout(const std::filesystem::path& layoutPath) const
    {
        return m_layoutSerializer->Save(layoutPath);
    }

    bool EditorDockingController::LoadLayout(const std::filesystem::path& layoutPath)
    {
        return m_layoutSerializer->Load(layoutPath);
    }

    EditorDockingLayoutSnapshot EditorDockingController::CreateLayoutSnapshot() const
    {
        EditorDockingLayoutSnapshot snapshot{};
        snapshot.rootDockNodeId = m_state.rootDockNodeId;
        snapshot.dockNodes.reserve(m_state.dockNodes.size());
        for (const DockNode& dockNode : m_state.dockNodes)
        {
            EditorDockingLayoutNodeSnapshot nodeSnapshot{};
            nodeSnapshot.isLeaf = DockNodeKind::Leaf == dockNode.kind;
            nodeSnapshot.isHorizontalSplit = DockSplitOrientation::Horizontal == dockNode.splitOrientation;
            nodeSnapshot.splitRatio = dockNode.splitRatio;
            nodeSnapshot.firstChild = dockNode.firstChild;
            nodeSnapshot.secondChild = dockNode.secondChild;
            nodeSnapshot.activeTabIndex = dockNode.activeTabIndex;
            nodeSnapshot.tabKey = GetDockTabLayoutKey(dockNode.tabControl);
            nodeSnapshot.panels = dockNode.panels;
            snapshot.dockNodes.push_back(std::move(nodeSnapshot));
        }

        snapshot.floatingGroups.reserve(m_state.floatingPanelGroups.size());
        for (const FloatingPanelGroup& group : m_state.floatingPanelGroups)
        {
            EditorFloatingPanelGroupSnapshot groupSnapshot{};
            if (nullptr != group.window)
            {
                GetWindowRect(group.window, &groupSnapshot.rect);
            }
            groupSnapshot.activeTabIndex = group.activeTabIndex;
            groupSnapshot.panels = group.panels;
            snapshot.floatingGroups.push_back(std::move(groupSnapshot));
        }

        return snapshot;
    }

    bool EditorDockingController::ApplyLayoutSnapshot(const EditorDockingLayoutSnapshot& snapshot)
    {
        if (nullptr == m_shell.m_parentWindow || snapshot.dockNodes.empty())
        {
            return false;
        }

        ResetLayout();
        for (HWND tabControl : m_state.dynamicDockTabs)
        {
            if (nullptr != tabControl)
            {
                DestroyWindow(tabControl);
            }
        }
        m_state.dynamicDockTabs.clear();
        m_state.logOutputDockTab = nullptr;
        m_state.dockNodes.clear();
        m_state.dockNodes.reserve(snapshot.dockNodes.size());

        for (const EditorDockingLayoutNodeSnapshot& nodeSnapshot : snapshot.dockNodes)
        {
            DockNode dockNode{};
            dockNode.kind = nodeSnapshot.isLeaf ? DockNodeKind::Leaf : DockNodeKind::Split;
            dockNode.splitOrientation = nodeSnapshot.isHorizontalSplit
                ? DockSplitOrientation::Horizontal
                : DockSplitOrientation::Vertical;
            dockNode.splitRatio = (std::max)(0.05f, (std::min)(0.95f, nodeSnapshot.splitRatio));
            dockNode.firstChild = nodeSnapshot.firstChild;
            dockNode.secondChild = nodeSnapshot.secondChild;
            dockNode.activeTabIndex = nodeSnapshot.activeTabIndex;
            dockNode.panels = nodeSnapshot.panels;
            if (DockNodeKind::Leaf == dockNode.kind)
            {
                dockNode.tabControl = CreateDockTabControlForLayoutKey(nodeSnapshot.tabKey);
            }
            m_state.dockNodes.push_back(std::move(dockNode));
        }
        m_state.rootDockNodeId = snapshot.rootDockNodeId;

        for (const EditorFloatingPanelGroupSnapshot& group : snapshot.floatingGroups)
        {
            if (group.panels.empty())
            {
                continue;
            }

            for (EditorPanelId panelId : group.panels)
            {
                RemovePanelFromDockTree(panelId, false);
            }

            const int width =
                (std::max)(m_shell.ScaleMetric(240), static_cast<int>(group.rect.right - group.rect.left));
            const int height =
                (std::max)(m_shell.ScaleMetric(180), static_cast<int>(group.rect.bottom - group.rect.top));
            FloatPanel(group.panels.front(), POINT{ group.rect.left, group.rect.top }, m_shell.m_parentWindow);
            HWND floatingWindow = GetFloatingWindowRef(group.panels.front());
            if (nullptr == floatingWindow)
            {
                continue;
            }

            SetWindowPos(
                floatingWindow,
                nullptr,
                group.rect.left,
                group.rect.top,
                width,
                height,
                SWP_NOZORDER | SWP_NOACTIVATE);
            for (std::size_t panelIndex = 1; panelIndex < group.panels.size(); ++panelIndex)
            {
                AttachPanelToFloatingWindow(group.panels[panelIndex], floatingWindow);
            }

            const int groupIndex = FindFloatingPanelGroupIndex(floatingWindow);
            if (groupIndex >= 0)
            {
                FloatingPanelGroup& floatingGroup = m_state.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
                floatingGroup.activeTabIndex =
                    (std::max)(0, (std::min)(group.activeTabIndex, static_cast<int>(floatingGroup.panels.size()) - 1));
                SyncFloatingPanelTabs(floatingWindow);
                LayoutFloatingWindow(floatingWindow);
            }
        }

        RestoreMissingPanelsToDefaultDock();
        SyncDockTabs();
        m_shell.m_layoutInitialized = false;
        return true;
    }

    void EditorDockingController::HideInactivePanelControls()
    {
        std::vector<EditorPanelId> activePanels{};
        const auto markActivePanel =
            [&activePanels](EditorPanelId panelId)
            {
                const auto activePanel = std::find(activePanels.begin(), activePanels.end(), panelId);
                if (activePanels.end() == activePanel)
                {
                    activePanels.push_back(panelId);
                }
            };

        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_state.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_state.dockNodes.size())
            {
                continue;
            }

            const DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Leaf != dockNode.kind || dockNode.panels.empty())
            {
                continue;
            }

            const int activeIndex =
                (std::max)(0, (std::min)(dockNode.activeTabIndex, static_cast<int>(dockNode.panels.size()) - 1));
            markActivePanel(dockNode.panels[static_cast<std::size_t>(activeIndex)]);
        }

        for (const FloatingPanelGroup& group : m_state.floatingPanelGroups)
        {
            if (nullptr == group.window || false == IsWindowVisible(group.window) || group.panels.empty())
            {
                continue;
            }

            const int activeIndex =
                (std::max)(0, (std::min)(group.activeTabIndex, static_cast<int>(group.panels.size()) - 1));
            markActivePanel(group.panels[static_cast<std::size_t>(activeIndex)]);
        }

        for (EditorPanelId panelId : GetAllEditorPanels())
        {
            const auto activePanel = std::find(activePanels.begin(), activePanels.end(), panelId);
            if (activePanels.end() == activePanel)
            {
                m_shell.ShowPanelControls(panelId, false);
            }
        }
    }

    void EditorDockingController::LayoutDockArea(DockAreaId dockAreaId, const RECT& areaRect)
    {
        HWND tabControl = GetDockAreaTabControl(dockAreaId);
        if (nullptr == tabControl)
        {
            return;
        }

        const std::vector<EditorPanelId>& panels = GetDockAreaPanels(dockAreaId);
        const int tabHeight = m_shell.ScaleMetric(28);
        m_shell.MoveChildWindowNoRedraw(
            tabControl,
            areaRect.left,
            areaRect.top,
            areaRect.right - areaRect.left,
            tabHeight);
        ShowWindow(tabControl, panels.empty() ? SW_HIDE : SW_SHOW);
        if (panels.empty())
        {
            return;
        }

        const RECT panelRect{
            areaRect.left,
            areaRect.top + tabHeight,
            areaRect.right,
            areaRect.bottom
        };

        const int activeTabIndex = ClampActiveTabIndex(dockAreaId);
        for (std::size_t index = 0; index < panels.size(); ++index)
        {
            const bool active = static_cast<int>(index) == activeTabIndex;
            m_shell.ShowPanelControls(panels[index], active);
            if (false == active)
            {
                continue;
            }

            m_shell.SetPanelParent(panels[index], m_shell.m_parentWindow);
            m_shell.GetPanelView(panels[index]).Layout(panelRect);
        }
    }

    void EditorDockingController::BuildInitialDockTree()
    {
        m_state.dockNodes.clear();

        DockNode assetsNode{};
        assetsNode.kind = DockNodeKind::Leaf;
        assetsNode.panels = { EditorPanelId::Assets };
        assetsNode.tabControl = m_state.leftTopDockTab;
        const DockNodeId assetsNodeId = AddDockNode(std::move(assetsNode));

        DockNode hierarchyNode{};
        hierarchyNode.kind = DockNodeKind::Leaf;
        hierarchyNode.panels = { EditorPanelId::Hierarchy };
        hierarchyNode.tabControl = m_state.leftBottomDockTab;
        const DockNodeId hierarchyNodeId = AddDockNode(std::move(hierarchyNode));

        DockNode sceneViewNode{};
        sceneViewNode.kind = DockNodeKind::Leaf;
        sceneViewNode.panels = { EditorPanelId::SceneView };
        sceneViewNode.tabControl = m_state.centerDockTab;
        const DockNodeId sceneViewNodeId = AddDockNode(std::move(sceneViewNode));

        DockNode logOutputNode{};
        logOutputNode.kind = DockNodeKind::Leaf;
        logOutputNode.panels = { EditorPanelId::LogOutput };
        logOutputNode.tabControl = CreateAdditionalDockTabControl(m_shell.m_parentWindow);
        m_state.logOutputDockTab = logOutputNode.tabControl;
        const DockNodeId logOutputNodeId = AddDockNode(std::move(logOutputNode));

        DockNode inspectorNode{};
        inspectorNode.kind = DockNodeKind::Leaf;
        inspectorNode.panels = { EditorPanelId::Inspector };
        inspectorNode.tabControl = m_state.rightDockTab;
        const DockNodeId inspectorNodeId = AddDockNode(std::move(inspectorNode));

        DockNode leftColumnNode{};
        leftColumnNode.kind = DockNodeKind::Split;
        leftColumnNode.splitOrientation = DockSplitOrientation::Vertical;
        leftColumnNode.splitRatio = 0.49f;
        leftColumnNode.firstChild = assetsNodeId;
        leftColumnNode.secondChild = hierarchyNodeId;
        const DockNodeId leftColumnNodeId = AddDockNode(leftColumnNode);

        DockNode centerColumnNode{};
        centerColumnNode.kind = DockNodeKind::Split;
        centerColumnNode.splitOrientation = DockSplitOrientation::Vertical;
        centerColumnNode.splitRatio = 0.74f;
        centerColumnNode.firstChild = sceneViewNodeId;
        centerColumnNode.secondChild = logOutputNodeId;
        const DockNodeId centerColumnNodeId = AddDockNode(centerColumnNode);

        DockNode centerRightNode{};
        centerRightNode.kind = DockNodeKind::Split;
        centerRightNode.splitOrientation = DockSplitOrientation::Horizontal;
        centerRightNode.splitRatio = 0.68f;
        centerRightNode.firstChild = centerColumnNodeId;
        centerRightNode.secondChild = inspectorNodeId;
        const DockNodeId centerRightNodeId = AddDockNode(centerRightNode);

        DockNode rootNode{};
        rootNode.kind = DockNodeKind::Split;
        rootNode.splitOrientation = DockSplitOrientation::Horizontal;
        rootNode.splitRatio = 0.245f;
        rootNode.firstChild = leftColumnNodeId;
        rootNode.secondChild = centerRightNodeId;
        m_state.rootDockNodeId = AddDockNode(rootNode);
    }

    void EditorDockingController::LayoutDockNode(DockNodeId dockNodeId, const RECT& nodeRect)
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_state.dockNodes.size())
        {
            return;
        }

        DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
        dockNode.rect = nodeRect;
        if (DockNodeKind::Leaf == dockNode.kind)
        {
            LayoutDockLeaf(dockNodeId, nodeRect);
            return;
        }

        const int panelSpacing = m_shell.ScaleMetric(8);
        dockNode.splitRatio = ClampDockSplitRatio(dockNodeId, dockNode.splitRatio);
        if (DockSplitOrientation::Horizontal == dockNode.splitOrientation)
        {
            const int width = (std::max)(0, static_cast<int>(nodeRect.right - nodeRect.left));
            const int firstWidth = (std::max)(m_shell.ScaleMetric(80), static_cast<int>((width - panelSpacing) * dockNode.splitRatio));
            const RECT firstRect{ nodeRect.left, nodeRect.top, nodeRect.left + firstWidth, nodeRect.bottom };
            const RECT secondRect{ nodeRect.left + firstWidth + panelSpacing, nodeRect.top, nodeRect.right, nodeRect.bottom };
            LayoutDockNode(dockNode.firstChild, firstRect);
            LayoutDockNode(dockNode.secondChild, secondRect);
            return;
        }

        const int height = (std::max)(0, static_cast<int>(nodeRect.bottom - nodeRect.top));
        const int firstHeight = (std::max)(m_shell.ScaleMetric(80), static_cast<int>((height - panelSpacing) * dockNode.splitRatio));
        const RECT firstRect{ nodeRect.left, nodeRect.top, nodeRect.right, nodeRect.top + firstHeight };
        const RECT secondRect{ nodeRect.left, nodeRect.top + firstHeight + panelSpacing, nodeRect.right, nodeRect.bottom };
        LayoutDockNode(dockNode.firstChild, firstRect);
        LayoutDockNode(dockNode.secondChild, secondRect);
    }

    void EditorDockingController::LayoutDockLeaf(DockNodeId dockNodeId, const RECT& areaRect)
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_state.dockNodes.size())
        {
            return;
        }

        DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
        HWND tabControl = dockNode.tabControl;
        if (nullptr == tabControl)
        {
            return;
        }

        const int tabHeight = m_shell.ScaleMetric(28);
        m_shell.MoveChildWindowNoRedraw(
            tabControl,
            areaRect.left,
            areaRect.top,
            areaRect.right - areaRect.left,
            tabHeight);
        ShowWindow(tabControl, dockNode.panels.empty() ? SW_HIDE : SW_SHOW);
        if (dockNode.panels.empty())
        {
            return;
        }

        const RECT panelRect{
            areaRect.left,
            areaRect.top + tabHeight,
            areaRect.right,
            areaRect.bottom
        };
        dockNode.activeTabIndex = (std::max)(0, (std::min)(dockNode.activeTabIndex, static_cast<int>(dockNode.panels.size()) - 1));

        for (std::size_t index = 0; index < dockNode.panels.size(); ++index)
        {
            const EditorPanelId panelId = dockNode.panels[index];
            const bool active = static_cast<int>(index) == dockNode.activeTabIndex;
            m_shell.ShowPanelControls(panelId, active);
            if (false == active)
            {
                continue;
            }

            m_shell.SetPanelParent(panelId, m_shell.m_parentWindow);
            m_shell.GetPanelView(panelId).Layout(panelRect);
        }
    }

    EditorDockingController::DockNodeId EditorDockingController::AddDockNode(DockNode node)
    {
        const DockNodeId dockNodeId = static_cast<DockNodeId>(m_state.dockNodes.size());
        m_state.dockNodes.push_back(std::move(node));
        return dockNodeId;
    }

    EditorDockingController::DockNodeId EditorDockingController::EnsureDefaultDockLeaf(EditorPanelId panelId)
    {
        HWND targetTabControl = GetDefaultDockTabControl(panelId);
        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_state.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Leaf == dockNode.kind && dockNode.tabControl == targetTabControl)
            {
                return dockNodeId;
            }
        }

        if (EditorPanelId::LogOutput == panelId || nullptr == targetTabControl)
        {
            targetTabControl = CreateAdditionalDockTabControl(m_shell.m_parentWindow);
            if (EditorPanelId::LogOutput == panelId)
            {
                m_state.logOutputDockTab = targetTabControl;
            }
        }

        DockNode newLeaf{};
        newLeaf.kind = DockNodeKind::Leaf;
        newLeaf.tabControl = targetTabControl;
        const DockNodeId newLeafNodeId = AddDockNode(std::move(newLeaf));
        if (m_state.rootDockNodeId < 0 || static_cast<std::size_t>(m_state.rootDockNodeId) >= m_state.dockNodes.size())
        {
            m_state.rootDockNodeId = newLeafNodeId;
            return newLeafNodeId;
        }

        const DockNode oldRootNode = m_state.dockNodes[static_cast<std::size_t>(m_state.rootDockNodeId)];
        const DockNodeId oldRootNodeId = AddDockNode(oldRootNode);
        DockNode splitNode{};
        splitNode.kind = DockNodeKind::Split;
        splitNode.splitOrientation =
            EditorPanelId::LogOutput == panelId || EditorPanelId::SceneView == panelId
                ? DockSplitOrientation::Vertical
                : DockSplitOrientation::Horizontal;
        splitNode.splitRatio = 0.5f;

        if (EditorPanelId::Hierarchy == panelId || EditorPanelId::Assets == panelId || EditorPanelId::SceneView == panelId)
        {
            splitNode.firstChild = newLeafNodeId;
            splitNode.secondChild = oldRootNodeId;
            splitNode.splitRatio = EditorPanelId::SceneView == panelId ? 0.70f : 0.18f;
        }
        else
        {
            splitNode.firstChild = oldRootNodeId;
            splitNode.secondChild = newLeafNodeId;
            splitNode.splitRatio = EditorPanelId::LogOutput == panelId ? 0.75f : 0.84f;
        }

        m_state.dockNodes[static_cast<std::size_t>(m_state.rootDockNodeId)] = splitNode;
        return newLeafNodeId;
    }

    HWND EditorDockingController::GetDefaultDockTabControl(EditorPanelId panelId)
    {
        switch (panelId)
        {
        case EditorPanelId::Hierarchy:
            return m_state.leftTopDockTab;
        case EditorPanelId::Assets:
            return m_state.leftBottomDockTab;
        case EditorPanelId::SceneView:
            return m_state.centerDockTab;
        case EditorPanelId::Inspector:
            return m_state.rightDockTab;
        case EditorPanelId::LogOutput:
            return m_state.logOutputDockTab;
        default:
            return m_state.centerDockTab;
        }
    }


    bool EditorDockingController::Update(HWND parentWindow, const Core::InputSnapshot& inputSnapshot)
    {
        if (nullptr == parentWindow)
        {
            return false;
        }

        if (inputSnapshot.WasKeyPressed('R') && inputSnapshot.IsKeyDown(VK_CONTROL))
        {
            ResetLayout();
            return true;
        }

        bool changed = false;
        const POINT cursorScreenPoint = ToWin32Point(inputSnapshot.GetCursorScreenPoint());

        if (inputSnapshot.WasKeyPressed('R') && inputSnapshot.IsKeyDown(VK_CONTROL))
        {
            ResetLayout();
            changed = true;
        }

        if (inputSnapshot.WasMouseButtonPressed(Core::MouseButton::Left))
        {
            const DockNodeId hitSplitterNodeId = HitTestDockSplitter(cursorScreenPoint);
            if (0 <= hitSplitterNodeId && static_cast<std::size_t>(hitSplitterNodeId) < m_state.dockNodes.size())
            {
                const DockNode& splitNode = m_state.dockNodes[static_cast<std::size_t>(hitSplitterNodeId)];
                m_state.dragKind = DockSplitOrientation::Horizontal == splitNode.splitOrientation
                    ? DockDragKind::HorizontalSplitter
                    : DockDragKind::VerticalSplitter;
                m_state.dragSplitNodeId = hitSplitterNodeId;
                m_state.dragStartScreenPoint = cursorScreenPoint;
                m_state.dragStartSplitRatio = splitNode.splitRatio;
                SetCapture(parentWindow);
            }
            else
            {
                const std::optional<EditorPanelId> hitPanel = HitTestDockTab(cursorScreenPoint);
                if (hitPanel.has_value())
                {
                    m_state.pendingDockDragPanelId = hitPanel;
                    m_state.dragStartScreenPoint = cursorScreenPoint;
                    m_state.pendingDockDragStartTick = GetTickCount64();
                    SetCapture(parentWindow);
                }
            }
        }

        if (inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left))
        {
            if (DockDragKind::None == m_state.dragKind && m_state.pendingDockDragPanelId.has_value())
            {
                const int dragThresholdX = (std::max)(m_shell.ScaleMetric(4), GetSystemMetrics(SM_CXDRAG));
                const int dragThresholdY = (std::max)(m_shell.ScaleMetric(4), GetSystemMetrics(SM_CYDRAG));
                const bool movedEnough =
                    dragThresholdX <= std::abs(cursorScreenPoint.x - m_state.dragStartScreenPoint.x)
                    || dragThresholdY <= std::abs(cursorScreenPoint.y - m_state.dragStartScreenPoint.y);
                const bool heldLongEnough =
                    DockPanelDragDelayMilliseconds <= GetTickCount64() - m_state.pendingDockDragStartTick;
                if (movedEnough && heldLongEnough)
                {
                    m_state.dragKind = DockDragKind::Panel;
                    m_state.dragPanelId = m_state.pendingDockDragPanelId;
                    m_state.pendingDockDragPanelId.reset();
                    m_state.currentGuideTarget = DockGuideTarget{};
                    m_state.hasDockPreview = false;
                    BeginDockPanelDrag(*m_state.dragPanelId, parentWindow, cursorScreenPoint);
                    changed = true;
                }
            }

            if (DockDragKind::Panel == m_state.dragKind && m_state.dragPanelId.has_value())
            {
                UpdateDockPanelDragWindow(*m_state.dragPanelId, cursorScreenPoint);
                UpdateDockGuideWindows(parentWindow, cursorScreenPoint);
                m_state.currentGuideTarget = HitTestDockGuideTarget(parentWindow, cursorScreenPoint);
                m_state.hasDockPreview = DockGuideTargetKind::None != m_state.currentGuideTarget.kind
                    && DockGuideTargetKind::Float != m_state.currentGuideTarget.kind;
                m_state.dockPreviewRect = m_state.currentGuideTarget.previewRect;
                UpdateDockPreviewWindow(parentWindow);
            }
            else if (DockDragKind::HorizontalSplitter == m_state.dragKind || DockDragKind::VerticalSplitter == m_state.dragKind)
            {
                changed = UpdateDockSplitterDrag(parentWindow, cursorScreenPoint) || changed;
                if (nullptr != m_shell.m_cursor)
                {
                    m_shell.m_cursor->SetShape(
                        DockDragKind::HorizontalSplitter == m_state.dragKind
                            ? Platform::CursorShape::HorizontalResize
                            : Platform::CursorShape::VerticalResize);
                }
            }
        }
        else if (DockDragKind::None == m_state.dragKind)
        {
            const DockNodeId hitSplitterNodeId = HitTestDockSplitter(cursorScreenPoint);
            if (0 <= hitSplitterNodeId && static_cast<std::size_t>(hitSplitterNodeId) < m_state.dockNodes.size())
            {
                const DockNode& splitNode = m_state.dockNodes[static_cast<std::size_t>(hitSplitterNodeId)];
                if (nullptr != m_shell.m_cursor)
                {
                    m_shell.m_cursor->SetShape(
                        DockSplitOrientation::Horizontal == splitNode.splitOrientation
                            ? Platform::CursorShape::HorizontalResize
                            : Platform::CursorShape::VerticalResize);
                }
            }
        }

        if (inputSnapshot.WasMouseButtonReleased(Core::MouseButton::Left))
        {
            if (DockDragKind::Panel == m_state.dragKind && m_state.dragPanelId.has_value())
            {
                const DockGuideTarget guideTarget = m_state.currentGuideTarget;
                m_state.currentGuideTarget = DockGuideTarget{};
                m_state.hasDockPreview = false;
                HideDockGuideWindows();
                UpdateDockPreviewWindow(parentWindow);
                ApplyDockGuideTarget(*m_state.dragPanelId, guideTarget, parentWindow);
                changed = true;
            }

            m_state.dragKind = DockDragKind::None;
            m_state.dragPanelId.reset();
            m_state.pendingDockDragPanelId.reset();
            m_state.pendingDockDragStartTick = 0;
            m_state.dragSplitNodeId = -1;
            m_state.hasDockPreview = false;
            m_state.dragPanelWindowOffset = POINT{};
            ReleaseCapture();
            HideDockGuideWindows();
            UpdateDockPreviewWindow(parentWindow);
        }

        if (changed)
        {
            m_shell.m_layoutInitialized = false;
        }

        return changed;
    }

    bool EditorDockingController::HandleNotify(LPARAM notifyParameter)
    {
        if (false == m_shell.m_panelViewsInitialized)
        {
            return false;
        }

        const NMHDR* notifyHeader = reinterpret_cast<const NMHDR*>(notifyParameter);
        if (nullptr == notifyHeader || TCN_SELCHANGE != notifyHeader->code)
        {
            return false;
        }

        for (DockNode& dockNode : m_state.dockNodes)
        {
            if (DockNodeKind::Leaf != dockNode.kind || notifyHeader->hwndFrom != dockNode.tabControl)
            {
                continue;
            }

            dockNode.activeTabIndex = TabCtrl_GetCurSel(dockNode.tabControl);
            m_shell.m_layoutInitialized = false;
            return true;
        }

        return false;
    }

    void EditorDockingController::ResetLayout()
    {
        m_state.leftTopDockPanels = { EditorPanelId::Hierarchy };
        m_state.leftBottomDockPanels = { EditorPanelId::Assets };
        m_state.centerDockPanels = { EditorPanelId::SceneView };
        m_state.rightDockPanels = { EditorPanelId::Inspector };
        m_state.leftTopActiveTabIndex = 0;
        m_state.leftBottomActiveTabIndex = 0;
        m_state.centerActiveTabIndex = 0;
        m_state.rightActiveTabIndex = 0;
        for (HWND tabControl : m_state.dynamicDockTabs)
        {
            if (nullptr != tabControl)
            {
                DestroyWindow(tabControl);
            }
        }
        m_state.dynamicDockTabs.clear();
        m_state.logOutputDockTab = nullptr;
        m_state.hasDockPreview = false;
        m_state.currentGuideTarget = DockGuideTarget{};
        m_state.pendingDockDragPanelId.reset();
        m_state.pendingDockDragStartTick = 0;
        BuildInitialDockTree();
        HideDockGuideWindows();
        UpdateDockPreviewWindow(m_shell.m_parentWindow);
        m_state.leftPaneWidth = m_shell.ScaleMetric(260);
        m_state.rightPaneWidth = m_shell.ScaleMetric(300);
        m_state.leftTopHeight = m_shell.ScaleMetric(280);
        m_shell.SetPanelParent(EditorPanelId::Hierarchy, m_shell.m_parentWindow);
        m_shell.SetPanelParent(EditorPanelId::Assets, m_shell.m_parentWindow);
        m_shell.SetPanelParent(EditorPanelId::SceneView, m_shell.m_parentWindow);
        m_shell.SetPanelParent(EditorPanelId::Inspector, m_shell.m_parentWindow);
        m_shell.SetPanelParent(EditorPanelId::LogOutput, m_shell.m_parentWindow);
        DestroyFloatingWindow(EditorPanelId::Hierarchy);
        DestroyFloatingWindow(EditorPanelId::Assets);
        DestroyFloatingWindow(EditorPanelId::SceneView);
        DestroyFloatingWindow(EditorPanelId::Inspector);
        DestroyFloatingWindow(EditorPanelId::LogOutput);
        SyncDockTabs();
        m_shell.m_layoutInitialized = false;
    }

    void EditorDockingController::ShowPanelAtDefaultDock(EditorPanelId panelId)
    {
        if (nullptr == m_shell.m_parentWindow)
        {
            return;
        }

        if (false == IsDockableEditorPanel(panelId))
        {
            RemovePanelFromDockTree(panelId);
            DestroyFloatingWindow(panelId);
            return;
        }

        RemovePanelFromDockTree(panelId);
        DestroyFloatingWindow(panelId);
        m_shell.SetPanelParent(panelId, m_shell.m_parentWindow);
        const DockNodeId targetDockNodeId = EnsureDefaultDockLeaf(panelId);
        if (targetDockNodeId < 0 || static_cast<std::size_t>(targetDockNodeId) >= m_state.dockNodes.size())
        {
            return;
        }

        DockNode& targetDockNode = m_state.dockNodes[static_cast<std::size_t>(targetDockNodeId)];
        if (targetDockNode.panels.end() == std::find(targetDockNode.panels.begin(), targetDockNode.panels.end(), panelId))
        {
            targetDockNode.panels.push_back(panelId);
        }
        targetDockNode.activeTabIndex = static_cast<int>(targetDockNode.panels.size()) - 1;
        m_shell.ShowPanelControls(panelId, true);
        SyncDockTabs();
        m_shell.m_layoutInitialized = false;
    }

    void EditorDockingController::ActivatePanel(EditorPanelId panelId)
    {
        if (nullptr == m_shell.m_parentWindow)
        {
            return;
        }

        const int floatingGroupIndex = FindFloatingPanelGroupIndex(panelId);
        if (floatingGroupIndex >= 0)
        {
            FloatingPanelGroup& group = m_state.floatingPanelGroups[static_cast<std::size_t>(floatingGroupIndex)];
            const auto panelIt = std::find(group.panels.begin(), group.panels.end(), panelId);
            if (panelIt != group.panels.end())
            {
                group.activeTabIndex = static_cast<int>(std::distance(group.panels.begin(), panelIt));
                if (nullptr != group.window)
                {
                    SyncFloatingPanelTabs(group.window);
                    LayoutFloatingWindow(group.window);
                    ShowWindow(group.window, SW_SHOW);
                    SetForegroundWindow(group.window);
                }
                return;
            }
        }

        const DockNodeId dockNodeId = FindPanelDockLeaf(panelId);
        if (0 <= dockNodeId && static_cast<std::size_t>(dockNodeId) < m_state.dockNodes.size())
        {
            DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
            const auto panelIt = std::find(dockNode.panels.begin(), dockNode.panels.end(), panelId);
            if (panelIt != dockNode.panels.end())
            {
                dockNode.activeTabIndex = static_cast<int>(std::distance(dockNode.panels.begin(), panelIt));
                SyncDockTabs();
                m_shell.m_layoutInitialized = false;
                return;
            }
        }

        ShowPanelAtDefaultDock(panelId);
    }

    RECT EditorDockingController::GetPanelCaptionRect(EditorPanelId panelId) const
    {
        const HWND panelWindow = m_shell.GetPanelView(panelId).GetRootWindow();

        RECT captionRect{};
        if (nullptr != panelWindow && IsWindowVisible(panelWindow))
        {
            GetWindowRect(panelWindow, &captionRect);
            captionRect.bottom = captionRect.top + m_shell.ScaleMetric(28);
        }

        return captionRect;
    }

    std::optional<EditorPanelId> EditorDockingController::HitTestPanelCaption(POINT cursorScreenPoint) const
    {
        for (EditorPanelId panelId : GetAllEditorPanels())
        {
            const RECT captionRect = GetPanelCaptionRect(panelId);
            if (PtInRect(&captionRect, cursorScreenPoint))
            {
                return panelId;
            }
        }

        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_state.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Leaf != dockNode.kind)
            {
                continue;
            }

            HWND tabControl = dockNode.tabControl;
            if (nullptr == tabControl || false == IsWindowVisible(tabControl))
            {
                continue;
            }

            RECT tabRect{};
            GetWindowRect(tabControl, &tabRect);
            tabRect.bottom = tabRect.top + m_shell.ScaleMetric(28);
            if (false == PtInRect(&tabRect, cursorScreenPoint))
            {
                continue;
            }

            const int activeIndex = (std::max)(0, (std::min)(dockNode.activeTabIndex, static_cast<int>(dockNode.panels.size()) - 1));
            if (0 <= activeIndex && activeIndex < static_cast<int>(dockNode.panels.size()))
            {
                return dockNode.panels[static_cast<std::size_t>(activeIndex)];
            }
        }

        return std::nullopt;
    }

    std::optional<EditorPanelId> EditorDockingController::HitTestDockTab(POINT cursorScreenPoint) const
    {
        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_state.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Leaf != dockNode.kind || dockNode.panels.empty())
            {
                continue;
            }

            HWND tabControl = dockNode.tabControl;
            if (nullptr == tabControl || false == IsWindowVisible(tabControl))
            {
                continue;
            }

            POINT cursorClientPoint = cursorScreenPoint;
            ScreenToClient(tabControl, &cursorClientPoint);

            TCHITTESTINFO hitTestInfo{};
            hitTestInfo.pt = cursorClientPoint;
            const int tabIndex = TabCtrl_HitTest(tabControl, &hitTestInfo);
            if (tabIndex < 0 || tabIndex >= static_cast<int>(dockNode.panels.size()))
            {
                continue;
            }

            return dockNode.panels[static_cast<std::size_t>(tabIndex)];
        }

        return std::nullopt;
    }

    EditorDockingController::DockNodeId EditorDockingController::HitTestDockLeaf(POINT cursorClientPoint) const
    {
        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_state.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Leaf != dockNode.kind || dockNode.panels.empty())
            {
                continue;
            }

            if (PtInRect(&dockNode.rect, cursorClientPoint))
            {
                return dockNodeId;
            }
        }

        return -1;
    }

    EditorDockingController::DockNodeId EditorDockingController::HitTestDockSplitter(POINT cursorScreenPoint) const
    {
        if (nullptr == m_shell.m_parentWindow)
        {
            return -1;
        }

        POINT cursorClientPoint = cursorScreenPoint;
        ScreenToClient(m_shell.m_parentWindow, &cursorClientPoint);

        std::vector<DockNodeId> dockSplitNodeIds{};
        CollectReachableDockSplits(m_state.rootDockNodeId, dockSplitNodeIds);
        for (DockNodeId dockNodeId : dockSplitNodeIds)
        {
            const DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Split != dockNode.kind
                || dockNode.firstChild < 0
                || dockNode.secondChild < 0
                || static_cast<std::size_t>(dockNode.firstChild) >= m_state.dockNodes.size()
                || static_cast<std::size_t>(dockNode.secondChild) >= m_state.dockNodes.size())
            {
                continue;
            }

            const RECT firstRect = m_state.dockNodes[static_cast<std::size_t>(dockNode.firstChild)].rect;
            const RECT secondRect = m_state.dockNodes[static_cast<std::size_t>(dockNode.secondChild)].rect;
            RECT splitterRect{};
            if (DockSplitOrientation::Horizontal == dockNode.splitOrientation)
            {
                splitterRect = RECT{ firstRect.right, dockNode.rect.top, secondRect.left, dockNode.rect.bottom };
            }
            else
            {
                splitterRect = RECT{ dockNode.rect.left, firstRect.bottom, dockNode.rect.right, secondRect.top };
            }

            const int minimumHitThickness = m_shell.ScaleMetric(6);
            if ((splitterRect.right - splitterRect.left) < minimumHitThickness)
            {
                const int centerX = (splitterRect.left + splitterRect.right) / 2;
                splitterRect.left = centerX - minimumHitThickness / 2;
                splitterRect.right = splitterRect.left + minimumHitThickness;
            }
            if ((splitterRect.bottom - splitterRect.top) < minimumHitThickness)
            {
                const int centerY = (splitterRect.top + splitterRect.bottom) / 2;
                splitterRect.top = centerY - minimumHitThickness / 2;
                splitterRect.bottom = splitterRect.top + minimumHitThickness;
            }

            if (PtInRect(&splitterRect, cursorClientPoint))
            {
                return dockNodeId;
            }
        }

        return -1;
    }

    void EditorDockingController::CollectReachableDockLeaves(DockNodeId dockNodeId, std::vector<DockNodeId>& dockLeafNodeIds) const
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_state.dockNodes.size())
        {
            return;
        }

        const DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
        if (DockNodeKind::Leaf == dockNode.kind)
        {
            dockLeafNodeIds.push_back(dockNodeId);
            return;
        }

        CollectReachableDockLeaves(dockNode.firstChild, dockLeafNodeIds);
        CollectReachableDockLeaves(dockNode.secondChild, dockLeafNodeIds);
    }

    void EditorDockingController::CollectReachableDockSplits(DockNodeId dockNodeId, std::vector<DockNodeId>& dockSplitNodeIds) const
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_state.dockNodes.size())
        {
            return;
        }

        const DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
        if (DockNodeKind::Leaf == dockNode.kind)
        {
            return;
        }

        CollectReachableDockSplits(dockNode.firstChild, dockSplitNodeIds);
        dockSplitNodeIds.push_back(dockNodeId);
        CollectReachableDockSplits(dockNode.secondChild, dockSplitNodeIds);
    }

    bool EditorDockingController::UpdateDockSplitterDrag(HWND parentWindow, POINT cursorScreenPoint)
    {
        if (nullptr == parentWindow
            || m_state.dragSplitNodeId < 0
            || static_cast<std::size_t>(m_state.dragSplitNodeId) >= m_state.dockNodes.size())
        {
            return false;
        }

        DockNode& splitNode = m_state.dockNodes[static_cast<std::size_t>(m_state.dragSplitNodeId)];
        if (DockNodeKind::Split != splitNode.kind)
        {
            return false;
        }

        const int panelSpacing = m_shell.ScaleMetric(12);
        const int width = (std::max)(0, static_cast<int>(splitNode.rect.right - splitNode.rect.left));
        const int height = (std::max)(0, static_cast<int>(splitNode.rect.bottom - splitNode.rect.top));
        const int availableLength = DockSplitOrientation::Horizontal == splitNode.splitOrientation
            ? width - panelSpacing
            : height - panelSpacing;
        if (availableLength <= 0)
        {
            return false;
        }

        const int cursorDelta = DockSplitOrientation::Horizontal == splitNode.splitOrientation
            ? cursorScreenPoint.x - m_state.dragStartScreenPoint.x
            : cursorScreenPoint.y - m_state.dragStartScreenPoint.y;
        const float nextRatio = ClampDockSplitRatio(
            m_state.dragSplitNodeId,
            m_state.dragStartSplitRatio + static_cast<float>(cursorDelta) / static_cast<float>(availableLength));
        if (splitNode.splitRatio == nextRatio)
        {
            return false;
        }

        splitNode.splitRatio = nextRatio;
        m_shell.m_layoutInitialized = false;
        return true;
    }

    float EditorDockingController::ClampDockSplitRatio(DockNodeId dockNodeId, float ratio) const
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_state.dockNodes.size())
        {
            return ratio;
        }

        const DockNode& splitNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
        const int panelSpacing = m_shell.ScaleMetric(12);
        const int totalLength = DockSplitOrientation::Horizontal == splitNode.splitOrientation
            ? splitNode.rect.right - splitNode.rect.left
            : splitNode.rect.bottom - splitNode.rect.top;
        const int availableLength = totalLength - panelSpacing;
        if (availableLength <= 0)
        {
            return 0.5f;
        }

        const float minimumRatio = (std::min)(0.45f, static_cast<float>(m_shell.ScaleMetric(80)) / static_cast<float>(availableLength));
        return (std::max)(minimumRatio, (std::min)(1.0f - minimumRatio, ratio));
    }

    EditorDockingController::DockNodeId EditorDockingController::FindPanelDockLeaf(EditorPanelId panelId) const
    {
        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_state.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Leaf != dockNode.kind)
            {
                continue;
            }

            if (dockNode.panels.end() != std::find(dockNode.panels.begin(), dockNode.panels.end(), panelId))
            {
                return dockNodeId;
            }
        }

        return -1;
    }

    bool EditorDockingController::IsPanelInDockTree(EditorPanelId panelId) const
    {
        return FindPanelDockLeaf(panelId) >= 0;
    }

    void EditorDockingController::RemovePanelFromDockTree(EditorPanelId panelId, bool collapseEmptyLeaves)
    {
        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_state.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Leaf != dockNode.kind)
            {
                continue;
            }

            RemovePanelFromDockNode(dockNode, panelId);
        }

        if (collapseEmptyLeaves && m_state.rootDockNodeId >= 0)
        {
            (void)CollapseEmptyDockLeaves(m_state.rootDockNodeId);
        }
    }

    void EditorDockingController::RemovePanelFromDockNode(DockNode& dockNode, EditorPanelId panelId) const
    {
        dockNode.panels.erase(std::remove(dockNode.panels.begin(), dockNode.panels.end(), panelId), dockNode.panels.end());
        if (dockNode.activeTabIndex >= static_cast<int>(dockNode.panels.size()))
        {
            dockNode.activeTabIndex = (std::max)(0, static_cast<int>(dockNode.panels.size()) - 1);
        }
    }

    bool EditorDockingController::CollapseEmptyDockLeaves(DockNodeId dockNodeId)
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_state.dockNodes.size())
        {
            return false;
        }

        DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
        if (DockNodeKind::Leaf == dockNode.kind)
        {
            return dockNode.panels.empty();
        }

        const bool firstEmpty = CollapseEmptyDockLeaves(dockNode.firstChild);
        const bool secondEmpty = CollapseEmptyDockLeaves(dockNode.secondChild);
        if (firstEmpty && false == secondEmpty)
        {
            dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNode.secondChild)];
            return false;
        }

        if (secondEmpty && false == firstEmpty)
        {
            dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNode.firstChild)];
            return false;
        }

        return firstEmpty && secondEmpty;
    }

    EditorDockingController::DockGuideTarget EditorDockingController::HitTestDockGuideTarget(HWND parentWindow, POINT cursorScreenPoint) const
    {
        (void)parentWindow;

        const auto hitVisibleGuide =
            [cursorScreenPoint](HWND guideWindow)
            {
                RECT guideRect{};
                if (nullptr == guideWindow || false == IsWindowVisible(guideWindow))
                {
                    return false;
                }

                GetWindowRect(guideWindow, &guideRect);
                return TRUE == PtInRect(&guideRect, cursorScreenPoint);
            };

        const std::array<DockGuideTargetKind, 9> guideKinds{
            DockGuideTargetKind::SplitTop,
            DockGuideTargetKind::SplitBottom,
            DockGuideTargetKind::SplitLeft,
            DockGuideTargetKind::SplitRight,
            DockGuideTargetKind::Tab,
            DockGuideTargetKind::SplitTop,
            DockGuideTargetKind::SplitBottom,
            DockGuideTargetKind::SplitLeft,
            DockGuideTargetKind::SplitRight
        };

        for (std::size_t index = 0; index < m_state.dockGuideWindows.size(); ++index)
        {
            if (false == hitVisibleGuide(m_state.dockGuideWindows[index]))
            {
                continue;
            }

            const DockGuideTargetKind kind = guideKinds[index];
            const DockNodeId dockNodeId = index < 5 ? m_state.currentGuideTarget.dockNodeId : m_state.rootDockNodeId;
            if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_state.dockNodes.size())
            {
                return DockGuideTarget{};
            }

            const RECT sourceRect = index < 5
                ? m_state.dockNodes[static_cast<std::size_t>(dockNodeId)].rect
                : m_state.dockNodes[static_cast<std::size_t>(m_state.rootDockNodeId)].rect;
            RECT previewRect = sourceRect;
            const int width = sourceRect.right - sourceRect.left;
            const int height = sourceRect.bottom - sourceRect.top;
            const float splitRatio = index < 5 ? 0.35f : 0.25f;
            if (DockGuideTargetKind::Tab == kind)
            {
                previewRect = sourceRect;
            }
            else if (DockGuideTargetKind::SplitLeft == kind)
            {
                previewRect.right = sourceRect.left + static_cast<int>(width * splitRatio);
            }
            else if (DockGuideTargetKind::SplitRight == kind)
            {
                previewRect.left = sourceRect.right - static_cast<int>(width * splitRatio);
            }
            else if (DockGuideTargetKind::SplitTop == kind)
            {
                previewRect.bottom = sourceRect.top + static_cast<int>(height * splitRatio);
            }
            else if (DockGuideTargetKind::SplitBottom == kind)
            {
                previewRect.top = sourceRect.bottom - static_cast<int>(height * splitRatio);
            }

            return DockGuideTarget{ kind, dockNodeId, previewRect };
        }

        return DockGuideTarget{};
    }

    void EditorDockingController::ApplyDockGuideTarget(EditorPanelId panelId, const DockGuideTarget& guideTarget, HWND parentWindow)
    {
        if (DockGuideTargetKind::None == guideTarget.kind || DockGuideTargetKind::Float == guideTarget.kind)
        {
            const POINT cursorScreenPoint = GetCursorScreenPoint(m_shell.m_cursor);
            const HWND targetFloatingWindow = HitTestFloatingWindow(cursorScreenPoint, GetFloatingWindowRef(panelId));
            if (nullptr != targetFloatingWindow)
            {
                AttachPanelToFloatingWindow(panelId, targetFloatingWindow);
                SyncDockTabs();
                m_shell.m_layoutInitialized = false;
                return;
            }

            RemovePanelFromDockTree(panelId);
            FloatPanel(panelId, cursorScreenPoint, parentWindow);
            SyncDockTabs();
            m_shell.m_layoutInitialized = false;
            return;
        }

        if (guideTarget.dockNodeId < 0 || static_cast<std::size_t>(guideTarget.dockNodeId) >= m_state.dockNodes.size())
        {
            return;
        }

        const DockNodeId sourceLeafNodeId = FindPanelDockLeaf(panelId);
        HWND sourceTabControl = GetDockLeafTabControl(sourceLeafNodeId);
        if (nullptr == sourceTabControl)
        {
            sourceTabControl = m_state.centerDockTab;
        }

        RemovePanelFromDockTree(panelId, false);
        if (guideTarget.dockNodeId < 0 || static_cast<std::size_t>(guideTarget.dockNodeId) >= m_state.dockNodes.size())
        {
            return;
        }

        DockNode& targetNode = m_state.dockNodes[static_cast<std::size_t>(guideTarget.dockNodeId)];
        if (DockGuideTargetKind::Tab == guideTarget.kind && DockNodeKind::Leaf == targetNode.kind)
        {
            DestroyFloatingWindow(panelId);
            m_shell.SetPanelParent(panelId, parentWindow);
            targetNode.panels.push_back(panelId);
            targetNode.activeTabIndex = static_cast<int>(targetNode.panels.size()) - 1;
            if (m_state.rootDockNodeId >= 0)
            {
                (void)CollapseEmptyDockLeaves(m_state.rootDockNodeId);
            }
            SyncDockTabs();
            m_shell.m_layoutInitialized = false;
            return;
        }

        DestroyFloatingWindow(panelId);
        m_shell.SetPanelParent(panelId, parentWindow);

        DockNode oldTargetNode = targetNode;
        if (sourceLeafNodeId == guideTarget.dockNodeId)
        {
            RemovePanelFromDockNode(oldTargetNode, panelId);
            if (oldTargetNode.panels.empty())
            {
                targetNode.panels.push_back(panelId);
                targetNode.activeTabIndex = 0;
                SyncDockTabs();
                m_shell.m_layoutInitialized = false;
                return;
            }
        }

        DockNode newLeaf{};
        newLeaf.kind = DockNodeKind::Leaf;
        newLeaf.panels = { panelId };
        newLeaf.tabControl = CreateAdditionalDockTabControl(parentWindow);
        if (nullptr == newLeaf.tabControl)
        {
            newLeaf.tabControl = sourceTabControl;
        }

        const std::size_t targetNodeIndex = static_cast<std::size_t>(guideTarget.dockNodeId);
        const DockNodeId newLeafNodeId = AddDockNode(std::move(newLeaf));
        DockNode splitNode{};
        splitNode.kind = DockNodeKind::Split;
        splitNode.splitOrientation =
            (DockGuideTargetKind::SplitLeft == guideTarget.kind || DockGuideTargetKind::SplitRight == guideTarget.kind)
                ? DockSplitOrientation::Horizontal
                : DockSplitOrientation::Vertical;
        splitNode.splitRatio = 0.35f;

        const DockNodeId oldTargetNodeId = AddDockNode(oldTargetNode);
        if (DockGuideTargetKind::SplitLeft == guideTarget.kind || DockGuideTargetKind::SplitTop == guideTarget.kind)
        {
            splitNode.firstChild = newLeafNodeId;
            splitNode.secondChild = oldTargetNodeId;
        }
        else
        {
            splitNode.firstChild = oldTargetNodeId;
            splitNode.secondChild = newLeafNodeId;
            splitNode.splitRatio = 0.65f;
        }

        m_state.dockNodes[targetNodeIndex] = splitNode;
        if (m_state.rootDockNodeId >= 0)
        {
            (void)CollapseEmptyDockLeaves(m_state.rootDockNodeId);
        }
        SyncDockTabs();
        m_shell.m_layoutInitialized = false;
    }

    void EditorDockingController::UpdateDockGuideWindows(HWND parentWindow, POINT cursorScreenPoint)
    {
        if (nullptr == parentWindow || DockDragKind::Panel != m_state.dragKind)
        {
            HideDockGuideWindows();
            return;
        }

        POINT cursorClientPoint = cursorScreenPoint;
        ScreenToClient(parentWindow, &cursorClientPoint);
        const DockNodeId hitLeafNodeId = HitTestDockLeaf(cursorClientPoint);
        m_state.currentGuideTarget.dockNodeId = hitLeafNodeId;

        const int guideSize = m_shell.ScaleMetric(40);
        const int guideGap = m_shell.ScaleMetric(3);
        if (0 <= hitLeafNodeId && static_cast<std::size_t>(hitLeafNodeId) < m_state.dockNodes.size())
        {
            const RECT leafRect = m_state.dockNodes[static_cast<std::size_t>(hitLeafNodeId)].rect;
            const int centerX = (leafRect.left + leafRect.right) / 2;
            const int centerY = (leafRect.top + leafRect.bottom) / 2;
            ShowDockGuideWindow(m_state.dockGuideWindows[4], RECT{ centerX - guideSize / 2, centerY - guideSize / 2, centerX + guideSize / 2, centerY + guideSize / 2 });
            ShowDockGuideWindow(m_state.dockGuideWindows[0], RECT{ centerX - guideSize / 2, centerY - guideSize - guideGap - guideSize / 2, centerX + guideSize / 2, centerY - guideGap - guideSize / 2 });
            ShowDockGuideWindow(m_state.dockGuideWindows[1], RECT{ centerX - guideSize / 2, centerY + guideGap + guideSize / 2, centerX + guideSize / 2, centerY + guideSize + guideGap + guideSize / 2 });
            ShowDockGuideWindow(m_state.dockGuideWindows[2], RECT{ centerX - guideSize - guideGap - guideSize / 2, centerY - guideSize / 2, centerX - guideGap - guideSize / 2, centerY + guideSize / 2 });
            ShowDockGuideWindow(m_state.dockGuideWindows[3], RECT{ centerX + guideGap + guideSize / 2, centerY - guideSize / 2, centerX + guideSize + guideGap + guideSize / 2, centerY + guideSize / 2 });
        }
        else
        {
            for (std::size_t index = 0; index < 5; ++index)
            {
                ShowWindow(m_state.dockGuideWindows[index], SW_HIDE);
            }
        }

        const RECT rootRect = 0 <= m_state.rootDockNodeId && static_cast<std::size_t>(m_state.rootDockNodeId) < m_state.dockNodes.size()
            ? m_state.dockNodes[static_cast<std::size_t>(m_state.rootDockNodeId)].rect
            : RECT{};
        const int rootCenterX = (rootRect.left + rootRect.right) / 2;
        const int rootCenterY = (rootRect.top + rootRect.bottom) / 2;
        ShowDockGuideWindow(m_state.dockGuideWindows[5], RECT{ rootCenterX - guideSize / 2, rootRect.top + guideGap, rootCenterX + guideSize / 2, rootRect.top + guideGap + guideSize });
        ShowDockGuideWindow(m_state.dockGuideWindows[6], RECT{ rootCenterX - guideSize / 2, rootRect.bottom - guideGap - guideSize, rootCenterX + guideSize / 2, rootRect.bottom - guideGap });
        ShowDockGuideWindow(m_state.dockGuideWindows[7], RECT{ rootRect.left + guideGap, rootCenterY - guideSize / 2, rootRect.left + guideGap + guideSize, rootCenterY + guideSize / 2 });
        ShowDockGuideWindow(m_state.dockGuideWindows[8], RECT{ rootRect.right - guideGap - guideSize, rootCenterY - guideSize / 2, rootRect.right - guideGap, rootCenterY + guideSize / 2 });
    }

    void EditorDockingController::HideDockGuideWindows()
    {
        for (HWND guideWindow : m_state.dockGuideWindows)
        {
            ShowWindow(guideWindow, SW_HIDE);
        }
    }

    void EditorDockingController::CollectControls(std::vector<HWND>& controls) const
    {
        controls.insert(
            controls.end(),
            {
                m_state.leftTopDockTab,
                m_state.leftBottomDockTab,
                m_state.centerDockTab,
                m_state.rightDockTab,
                m_state.dockPreviewWindow,
                m_state.dockGuideWindows[0],
                m_state.dockGuideWindows[1],
                m_state.dockGuideWindows[2],
                m_state.dockGuideWindows[3],
                m_state.dockGuideWindows[4],
                m_state.dockGuideWindows[5],
                m_state.dockGuideWindows[6],
                m_state.dockGuideWindows[7],
                m_state.dockGuideWindows[8]
            });
        controls.insert(controls.end(), m_state.dynamicDockTabs.begin(), m_state.dynamicDockTabs.end());
    }

    void EditorDockingController::ShowDockGuideWindow(HWND guideWindow, const RECT& guideRect)
    {
        if (nullptr == guideWindow)
        {
            return;
        }

        SetWindowPos(
            guideWindow,
            HWND_TOP,
            guideRect.left,
            guideRect.top,
            (std::max)(0, static_cast<int>(guideRect.right - guideRect.left)),
            (std::max)(0, static_cast<int>(guideRect.bottom - guideRect.top)),
            SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    EditorDockingController::DockAreaId EditorDockingController::HitTestDockArea(HWND parentWindow, POINT cursorScreenPoint) const
    {
        RECT clientRect{};
        GetClientRect(parentWindow, &clientRect);
        POINT cursorClientPoint = cursorScreenPoint;
        ScreenToClient(parentWindow, &cursorClientPoint);
        if (false == PtInRect(&clientRect, cursorClientPoint))
        {
            return DockAreaId::Floating;
        }

        const int outerPadding = m_shell.ScaleMetric(12);
        const int panelSpacing = m_shell.ScaleMetric(12);
        if (cursorClientPoint.x < outerPadding + m_state.leftPaneWidth)
        {
            return cursorClientPoint.y < outerPadding + m_state.leftTopHeight + panelSpacing / 2
                ? DockAreaId::LeftTop
                : DockAreaId::LeftBottom;
        }

        if (cursorClientPoint.x > clientRect.right - outerPadding - m_state.rightPaneWidth)
        {
            return DockAreaId::Right;
        }

        return DockAreaId::Center;
    }

    void EditorDockingController::MovePanelToDockArea(EditorPanelId panelId, DockAreaId dockAreaId, HWND parentWindow)
    {
        const auto removePanel =
            [panelId](std::vector<EditorPanelId>& panels)
            {
                panels.erase(std::remove(panels.begin(), panels.end(), panelId), panels.end());
            };

        removePanel(m_state.leftTopDockPanels);
        removePanel(m_state.leftBottomDockPanels);
        removePanel(m_state.centerDockPanels);
        removePanel(m_state.rightDockPanels);

        if (DockAreaId::Floating == dockAreaId)
        {
            const POINT cursorScreenPoint = GetCursorScreenPoint(m_shell.m_cursor);
            FloatPanel(panelId, cursorScreenPoint, parentWindow);
            SyncDockTabs();
            m_shell.m_layoutInitialized = false;
            return;
        }

        m_shell.SetPanelParent(panelId, parentWindow);
        DestroyFloatingWindow(panelId);
        std::vector<EditorPanelId>& panels = GetDockAreaPanels(dockAreaId);
        panels.push_back(panelId);
        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            m_state.leftTopActiveTabIndex = static_cast<int>(panels.size()) - 1;
            break;
        case DockAreaId::LeftBottom:
            m_state.leftBottomActiveTabIndex = static_cast<int>(panels.size()) - 1;
            break;
        case DockAreaId::Center:
            m_state.centerActiveTabIndex = static_cast<int>(panels.size()) - 1;
            break;
        case DockAreaId::Right:
            m_state.rightActiveTabIndex = static_cast<int>(panels.size()) - 1;
            break;
        default:
            break;
        }

        SyncDockTabs();
        m_shell.m_layoutInitialized = false;
    }

    void EditorDockingController::BeginDockPanelDrag(EditorPanelId panelId, HWND parentWindow, POINT cursorScreenPoint)
    {
        if (nullptr == parentWindow)
        {
            return;
        }

        RECT captionRect = GetPanelCaptionRect(panelId);
        if (captionRect.right <= captionRect.left || captionRect.bottom <= captionRect.top)
        {
            captionRect = RECT{
                cursorScreenPoint.x - m_shell.ScaleMetric(24),
                cursorScreenPoint.y - m_shell.ScaleMetric(12),
                cursorScreenPoint.x + m_shell.ScaleMetric(336),
                cursorScreenPoint.y + m_shell.ScaleMetric(348)
            };
        }

        m_state.dragPanelWindowOffset = POINT{
            static_cast<LONG>((std::max)(m_shell.ScaleMetric(8), static_cast<int>(cursorScreenPoint.x - captionRect.left))),
            static_cast<LONG>((std::max)(m_shell.ScaleMetric(8), static_cast<int>(cursorScreenPoint.y - captionRect.top)))
        };

        RemovePanelFromDockTree(panelId);
        const POINT floatingOrigin{
            cursorScreenPoint.x - m_state.dragPanelWindowOffset.x,
            cursorScreenPoint.y - m_state.dragPanelWindowOffset.y
        };
        FloatPanel(panelId, floatingOrigin, parentWindow);
        SyncDockTabs();
        m_shell.m_layoutInitialized = false;
    }

    void EditorDockingController::UpdateDockPanelDragWindow(EditorPanelId panelId, POINT cursorScreenPoint)
    {
        HWND floatingWindow = GetFloatingWindowRef(panelId);
        if (nullptr == floatingWindow)
        {
            return;
        }

        SetWindowPos(
            floatingWindow,
            HWND_TOP,
            cursorScreenPoint.x - m_state.dragPanelWindowOffset.x,
            cursorScreenPoint.y - m_state.dragPanelWindowOffset.y,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    void EditorDockingController::FloatPanel(EditorPanelId panelId, POINT screenPoint, HWND parentWindow)
    {
        DestroyFloatingWindow(panelId);
        FloatingPanelCreateParams createParams{
            this,
            panelId
        };
        HWND floatingWindow = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            FloatingPanelWindowClassName,
            GetPanelTitle(panelId),
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            screenPoint.x,
            screenPoint.y,
            m_shell.ScaleMetric(360),
            m_shell.ScaleMetric(360),
            parentWindow,
            nullptr,
            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parentWindow, GWLP_HINSTANCE)),
            &createParams);
        if (nullptr == floatingWindow)
        {
            return;
        }
        GetFloatingWindowRef(panelId) = floatingWindow;

        HWND tabControl = m_shell.CreateChildWindow(
            floatingWindow,
            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parentWindow, GWLP_HINSTANCE)),
            WC_TABCONTROLW,
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_OWNERDRAWFIXED);
        if (nullptr != tabControl)
        {
            SendMessageW(tabControl, WM_SETFONT, reinterpret_cast<WPARAM>(m_shell.m_defaultFont), TRUE);
            ConfigureEditorTabControl(tabControl);
        }
        m_state.floatingPanelGroups.push_back(FloatingPanelGroup{
            floatingWindow,
            tabControl,
            { panelId },
            0
        });

        m_shell.SetPanelParent(panelId, floatingWindow);
        m_shell.ShowPanelControls(panelId, true);
        SyncFloatingPanelTabs(floatingWindow);
        LayoutFloatingWindow(floatingWindow);
    }

    void EditorDockingController::BeginFloatingWindowDockDrag(EditorPanelId panelId)
    {
        if (nullptr == m_shell.m_parentWindow)
        {
            return;
        }

        if (DockDragKind::Panel == m_state.dragKind && m_state.dragPanelId.has_value() && *m_state.dragPanelId == panelId)
        {
            return;
        }

        m_state.dragKind = DockDragKind::Panel;
        m_state.dragPanelId = panelId;
        m_state.dragStartScreenPoint = GetCursorScreenPoint(m_shell.m_cursor);
        m_state.currentGuideTarget = DockGuideTarget{};
        m_state.hasDockPreview = false;
    }

    void EditorDockingController::UpdateFloatingWindowDockDrag()
    {
        if (nullptr == m_shell.m_parentWindow || DockDragKind::Panel != m_state.dragKind || false == m_state.dragPanelId.has_value())
        {
            return;
        }

        const POINT cursorScreenPoint = GetCursorScreenPoint(m_shell.m_cursor);
        UpdateDockGuideWindows(m_shell.m_parentWindow, cursorScreenPoint);
        m_state.currentGuideTarget = HitTestDockGuideTarget(m_shell.m_parentWindow, cursorScreenPoint);
        m_state.hasDockPreview = DockGuideTargetKind::None != m_state.currentGuideTarget.kind
            && DockGuideTargetKind::Float != m_state.currentGuideTarget.kind;
        m_state.dockPreviewRect = m_state.currentGuideTarget.previewRect;
        UpdateDockPreviewWindow(m_shell.m_parentWindow);
    }

    void EditorDockingController::CompleteFloatingWindowDockDrag(EditorPanelId panelId)
    {
        if (nullptr == m_shell.m_parentWindow)
        {
            return;
        }

        UpdateFloatingWindowDockDrag();
        const DockGuideTarget guideTarget = m_state.currentGuideTarget;
        m_state.currentGuideTarget = DockGuideTarget{};
        m_state.hasDockPreview = false;
        HideDockGuideWindows();
        UpdateDockPreviewWindow(m_shell.m_parentWindow);
        m_state.dragKind = DockDragKind::None;
        m_state.dragPanelId.reset();

        if (DockGuideTargetKind::None == guideTarget.kind || DockGuideTargetKind::Float == guideTarget.kind)
        {
            const HWND targetFloatingWindow =
                HitTestFloatingWindow(GetCursorScreenPoint(m_shell.m_cursor), GetFloatingWindowRef(panelId));
            if (nullptr != targetFloatingWindow)
            {
                AttachPanelToFloatingWindow(panelId, targetFloatingWindow);
            }
            return;
        }

        ApplyDockGuideTarget(panelId, guideTarget, m_shell.m_parentWindow);
        m_shell.m_layoutInitialized = false;
    }

    void EditorDockingController::LayoutFloatingPanel(EditorPanelId panelId, HWND floatingWindow)
    {
        (void)panelId;
        if (nullptr == floatingWindow)
        {
            return;
        }

        LayoutFloatingWindow(floatingWindow);
    }

    void EditorDockingController::LayoutFloatingWindow(HWND floatingWindow)
    {
        const int groupIndex = FindFloatingPanelGroupIndex(floatingWindow);
        if (groupIndex < 0)
        {
            return;
        }

        FloatingPanelGroup& group = m_state.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
        RECT clientRect{};
        GetClientRect(floatingWindow, &clientRect);
        const int padding = m_shell.ScaleMetric(8);
        const int tabHeight = m_shell.ScaleMetric(28);
        const bool showsTabs = nullptr != group.tabControl && group.panels.size() > 1;
        if (nullptr != group.tabControl)
        {
            if (showsTabs)
            {
                m_shell.MoveChildWindowNoRedraw(
                    group.tabControl,
                    clientRect.left + padding,
                    clientRect.top + padding,
                    (std::max)(0, static_cast<int>(clientRect.right - clientRect.left) - padding * 2),
                    tabHeight);
                ShowWindow(group.tabControl, SW_SHOW);
            }
            else
            {
                ShowWindow(group.tabControl, SW_HIDE);
            }
        }

        group.activeTabIndex = (std::max)(0, (std::min)(group.activeTabIndex, static_cast<int>(group.panels.size()) - 1));
        const EditorPanelId activePanelId = group.panels[static_cast<std::size_t>(group.activeTabIndex)];
        for (EditorPanelId panelId : group.panels)
        {
            m_shell.ShowPanelControls(panelId, activePanelId == panelId);
        }

        const RECT panelRect{
            clientRect.left + padding,
            clientRect.top + padding + (showsTabs ? tabHeight + m_shell.ScaleMetric(4) : 0),
            (std::max)(clientRect.left + padding, clientRect.right - padding),
            (std::max)(clientRect.top + padding, clientRect.bottom - padding)
        };

        m_shell.GetPanelView(activePanelId).Layout(panelRect);

        m_shell.SendGroupBoxesToBack();
        m_shell.RedrawLayout(floatingWindow);
        if (EditorPanelId::SceneView == activePanelId)
        {
            (void)m_shell.UpdateSceneViewHostSize();
        }
    }

    void EditorDockingController::AttachPanelToFloatingWindow(EditorPanelId panelId, HWND floatingWindow)
    {
        const HWND targetWindow = floatingWindow;
        DestroyFloatingWindow(panelId);
        const int targetGroupIndex = FindFloatingPanelGroupIndex(floatingWindow);
        if (targetGroupIndex < 0)
        {
            return;
        }

        FloatingPanelGroup& targetGroup = m_state.floatingPanelGroups[static_cast<std::size_t>(targetGroupIndex)];
        if (targetGroup.panels.end() == std::find(targetGroup.panels.begin(), targetGroup.panels.end(), panelId))
        {
            targetGroup.panels.push_back(panelId);
        }
        targetGroup.activeTabIndex = static_cast<int>(targetGroup.panels.size()) - 1;
        GetFloatingWindowRef(panelId) = targetWindow;
        m_shell.SetPanelParent(panelId, targetWindow);
        m_shell.ShowPanelControls(panelId, true);
        SyncFloatingPanelTabs(targetWindow);
        LayoutFloatingWindow(targetWindow);
    }

    HWND EditorDockingController::HitTestFloatingWindow(POINT cursorScreenPoint, HWND excludedWindow) const
    {
        for (const FloatingPanelGroup& group : m_state.floatingPanelGroups)
        {
            if (nullptr == group.window || group.window == excludedWindow || false == IsWindowVisible(group.window))
            {
                continue;
            }

            RECT windowRect{};
            GetWindowRect(group.window, &windowRect);
            if (PtInRect(&windowRect, cursorScreenPoint))
            {
                return group.window;
            }
        }

        return nullptr;
    }

    void EditorDockingController::HandleFloatingWindowClose(EditorPanelId panelId, HWND floatingWindow)
    {
        (void)panelId;
        const int groupIndex = FindFloatingPanelGroupIndex(floatingWindow);
        if (groupIndex < 0)
        {
            return;
        }

        const FloatingPanelGroup group = m_state.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
        for (EditorPanelId floatingPanelId : group.panels)
        {
            HWND& floatingWindowRef = GetFloatingWindowRef(floatingPanelId);
            if (floatingWindowRef == floatingWindow)
            {
                floatingWindowRef = nullptr;
            }
            m_shell.SetPanelParent(floatingPanelId, m_shell.m_parentWindow);
            RemovePanelFromDockTree(floatingPanelId, false);
            m_shell.ShowPanelControls(floatingPanelId, false);
        }

        m_state.floatingPanelGroups.erase(m_state.floatingPanelGroups.begin() + groupIndex);
        SyncDockTabs();
        m_shell.m_layoutInitialized = false;
        DestroyWindow(floatingWindow);
    }

    void EditorDockingController::DestroyFloatingWindow(EditorPanelId panelId)
    {
        HWND& floatingWindow = GetFloatingWindowRef(panelId);
        if (nullptr == floatingWindow)
        {
            return;
        }

        const HWND windowToUpdate = floatingWindow;
        m_shell.SetPanelParent(panelId, m_shell.m_parentWindow);
        floatingWindow = nullptr;

        const int groupIndex = FindFloatingPanelGroupIndex(windowToUpdate);
        if (groupIndex < 0)
        {
            DestroyWindow(windowToUpdate);
            return;
        }

        FloatingPanelGroup& group = m_state.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
        group.panels.erase(std::remove(group.panels.begin(), group.panels.end(), panelId), group.panels.end());
        if (group.activeTabIndex >= static_cast<int>(group.panels.size()))
        {
            group.activeTabIndex = (std::max)(0, static_cast<int>(group.panels.size()) - 1);
        }

        if (group.panels.empty())
        {
            m_state.floatingPanelGroups.erase(m_state.floatingPanelGroups.begin() + groupIndex);
            DestroyWindow(windowToUpdate);
            return;
        }

        SyncFloatingPanelTabs(windowToUpdate);
        LayoutFloatingWindow(windowToUpdate);
    }

    HWND& EditorDockingController::GetFloatingWindowRef(EditorPanelId panelId)
    {
        switch (panelId)
        {
        case EditorPanelId::Hierarchy:
            return m_state.hierarchyFloatingWindow;
        case EditorPanelId::Assets:
            return m_state.assetsFloatingWindow;
        case EditorPanelId::SceneView:
            return m_state.sceneViewFloatingWindow;
        case EditorPanelId::Inspector:
            return m_state.inspectorFloatingWindow;
        case EditorPanelId::LogOutput:
            return m_state.logOutputFloatingWindow;
        default:
            return m_state.sceneViewFloatingWindow;
        }
    }

    void EditorDockingController::SyncFloatingPanelTabs(HWND floatingWindow)
    {
        const int groupIndex = FindFloatingPanelGroupIndex(floatingWindow);
        if (groupIndex < 0)
        {
            return;
        }

        FloatingPanelGroup& group = m_state.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
        if (nullptr == group.tabControl)
        {
            return;
        }

        TabCtrl_DeleteAllItems(group.tabControl);
        for (std::size_t index = 0; index < group.panels.size(); ++index)
        {
            TCITEMW item{};
            item.mask = TCIF_TEXT;
            item.pszText = const_cast<LPWSTR>(GetPanelTitle(group.panels[index]));
            TabCtrl_InsertItem(group.tabControl, static_cast<int>(index), &item);
        }
        group.activeTabIndex = (std::max)(0, (std::min)(group.activeTabIndex, static_cast<int>(group.panels.size()) - 1));
        TabCtrl_SetCurSel(group.tabControl, group.activeTabIndex);
    }

    int EditorDockingController::FindFloatingPanelGroupIndex(HWND floatingWindow) const
    {
        for (std::size_t index = 0; index < m_state.floatingPanelGroups.size(); ++index)
        {
            if (m_state.floatingPanelGroups[index].window == floatingWindow)
            {
                return static_cast<int>(index);
            }
        }

        return -1;
    }

    int EditorDockingController::FindFloatingPanelGroupIndex(EditorPanelId panelId) const
    {
        for (std::size_t index = 0; index < m_state.floatingPanelGroups.size(); ++index)
        {
            const std::vector<EditorPanelId>& panels = m_state.floatingPanelGroups[index].panels;
            if (panels.end() != std::find(panels.begin(), panels.end(), panelId))
            {
                return static_cast<int>(index);
            }
        }

        return -1;
    }

    EditorPanelId EditorDockingController::GetActiveFloatingPanel(HWND floatingWindow) const
    {
        const int groupIndex = FindFloatingPanelGroupIndex(floatingWindow);
        if (groupIndex < 0)
        {
            return EditorPanelId::SceneView;
        }

        const FloatingPanelGroup& group = m_state.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
        if (group.panels.empty())
        {
            return EditorPanelId::SceneView;
        }

        const int activeIndex = (std::max)(0, (std::min)(group.activeTabIndex, static_cast<int>(group.panels.size()) - 1));
        return group.panels[static_cast<std::size_t>(activeIndex)];
    }

    void EditorDockingController::SyncDockTabs()
    {
        for (HWND tabControl : { m_state.leftTopDockTab, m_state.leftBottomDockTab, m_state.centerDockTab, m_state.rightDockTab })
        {
            if (nullptr != tabControl)
            {
                TabCtrl_DeleteAllItems(tabControl);
                ShowWindow(tabControl, SW_HIDE);
            }
        }
        for (HWND tabControl : m_state.dynamicDockTabs)
        {
            if (nullptr != tabControl)
            {
                TabCtrl_DeleteAllItems(tabControl);
                ShowWindow(tabControl, SW_HIDE);
            }
        }

        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_state.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            DockNode& dockNode = m_state.dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Leaf != dockNode.kind || nullptr == dockNode.tabControl)
            {
                continue;
            }

            TabCtrl_DeleteAllItems(dockNode.tabControl);
            for (std::size_t index = 0; index < dockNode.panels.size(); ++index)
            {
                TCITEMW item{};
                item.mask = TCIF_TEXT;
                item.pszText = const_cast<LPWSTR>(GetPanelTitle(dockNode.panels[index]));
                TabCtrl_InsertItem(dockNode.tabControl, static_cast<int>(index), &item);
                if (EditorPanelId::SceneView == dockNode.panels[index])
                {
                    TCITEMW gameItem{};
                    gameItem.mask = TCIF_TEXT;
                    gameItem.pszText = const_cast<LPWSTR>(L"Game");
                    TabCtrl_InsertItem(dockNode.tabControl, static_cast<int>(index + 1), &gameItem);
                }
            }

            dockNode.activeTabIndex = (std::max)(0, (std::min)(dockNode.activeTabIndex, static_cast<int>(dockNode.panels.size()) - 1));
            TabCtrl_SetCurSel(dockNode.tabControl, dockNode.activeTabIndex);
        }
    }

    HWND EditorDockingController::CreateAdditionalDockTabControl(HWND parentWindow)
    {
        if (nullptr == parentWindow)
        {
            return nullptr;
        }

        constexpr DWORD tabStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_OWNERDRAWFIXED;
        HWND tabControl = m_shell.CreateChildWindow(
            parentWindow,
            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parentWindow, GWLP_HINSTANCE)),
            WC_TABCONTROLW,
            L"",
            tabStyle);
        if (nullptr == tabControl)
        {
            return nullptr;
        }

        SendMessageW(tabControl, WM_SETFONT, reinterpret_cast<WPARAM>(m_shell.m_defaultFont), TRUE);
        ConfigureEditorTabControl(tabControl);
        ShowWindow(tabControl, SW_HIDE);
        m_state.dynamicDockTabs.push_back(tabControl);
        return tabControl;
    }

    HWND EditorDockingController::CreateDockTabControlForLayoutKey(const std::wstring& layoutKey)
    {
        if (L"LeftTop" == layoutKey)
        {
            return m_state.leftTopDockTab;
        }
        if (L"LeftBottom" == layoutKey)
        {
            return m_state.leftBottomDockTab;
        }
        if (L"Center" == layoutKey)
        {
            return m_state.centerDockTab;
        }
        if (L"Right" == layoutKey)
        {
            return m_state.rightDockTab;
        }

        HWND tabControl = CreateAdditionalDockTabControl(m_shell.m_parentWindow);
        if (L"LogOutput" == layoutKey)
        {
            m_state.logOutputDockTab = tabControl;
        }
        return tabControl;
    }

    void EditorDockingController::ConfigureEditorTabControl(HWND tabControl) const
    {
        if (nullptr == tabControl)
        {
            return;
        }

        SendMessageW(tabControl, TCM_SETITEMSIZE, 0, MAKELPARAM(m_shell.ScaleMetric(96), m_shell.ScaleMetric(28)));
        SetWindowSubclass(
            tabControl,
            EditorShell::EditorTabControlSubclassProc,
            EditorTabControlSubclassId,
            0);
    }

    bool EditorDockingController::DrawEditorTabControl(const DRAWITEMSTRUCT& drawItem) const
    {
        if (ODT_TAB != drawItem.CtlType || nullptr == drawItem.hwndItem || nullptr == drawItem.hDC)
        {
            return false;
        }

        const int tabIndex = static_cast<int>(drawItem.itemID);
        if (tabIndex < 0)
        {
            return true;
        }

        const int activeTabIndex = TabCtrl_GetCurSel(drawItem.hwndItem);
        const int hoveredTabIndex = GetHoveredTabIndex(drawItem.hwndItem);
        const bool isActive = tabIndex == activeTabIndex;
        const bool isHovered = tabIndex == hoveredTabIndex;

        EditorColor backgroundColor = EditorColor::FromRgb8(0x13, 0x0F, 0x2A);
        EditorColor textColor = EditorColor::FromRgb8(0xB8, 0x8C, 0xFF);
        if (isHovered)
        {
            backgroundColor = EditorColor::FromRgb8(0x1D, 0x16, 0x42);
            textColor = EditorColor::FromRgb8(0xD6, 0xAE, 0xFF);
        }
        if (isActive)
        {
            backgroundColor = EditorThemes::XelqoriaDark.selection;
            textColor = EditorColor::FromRgb8(0xF0, 0xB7, 0xFF);
        }

        RECT tabRect = drawItem.rcItem;
        InflateRect(&tabRect, -2, -2);
        FillRoundRectWithThemeColor(drawItem.hDC, tabRect, backgroundColor, m_shell.ScaleMetric(6));

        if (isActive)
        {
            RECT accentRect = tabRect;
            accentRect.right = (std::min)(accentRect.right, accentRect.left + m_shell.ScaleMetric(4));
            FillRectWithThemeColor(drawItem.hDC, accentRect, EditorThemes::XelqoriaDark.accent);
        }

        wchar_t tabText[128]{};
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = tabText;
        item.cchTextMax = static_cast<int>(std::size(tabText));
        TabCtrl_GetItem(drawItem.hwndItem, tabIndex, &item);

        SetBkMode(drawItem.hDC, TRANSPARENT);
        SetTextColor(drawItem.hDC, ToColorRef(textColor));

        RECT textRect = tabRect;
        textRect.left += m_shell.ScaleMetric(12);
        textRect.right -= m_shell.ScaleMetric(8);
        DrawTextW(
            drawItem.hDC,
            tabText,
            -1,
            &textRect,
            DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        return true;
    }


    std::wstring EditorDockingController::GetDockTabLayoutKey(HWND tabControl) const
    {
        if (tabControl == m_state.leftTopDockTab)
        {
            return L"LeftTop";
        }
        if (tabControl == m_state.leftBottomDockTab)
        {
            return L"LeftBottom";
        }
        if (tabControl == m_state.centerDockTab)
        {
            return L"Center";
        }
        if (tabControl == m_state.rightDockTab)
        {
            return L"Right";
        }
        if (tabControl == m_state.logOutputDockTab)
        {
            return L"LogOutput";
        }

        return L"Dynamic";
    }

    void EditorDockingController::RestoreMissingPanelsToDefaultDock()
    {
        for (EditorPanelId panelId : GetAllEditorPanels())
        {
            if (IsPanelInDockTree(panelId) || FindFloatingPanelGroupIndex(panelId) >= 0)
            {
                continue;
            }

            ShowPanelAtDefaultDock(panelId);
        }
    }

    void EditorDockingController::SyncDockAreaTabs(DockAreaId dockAreaId)
    {
        HWND tabControl = GetDockAreaTabControl(dockAreaId);
        if (nullptr == tabControl)
        {
            return;
        }

        TabCtrl_DeleteAllItems(tabControl);
        const std::vector<EditorPanelId>& panels = GetDockAreaPanels(dockAreaId);
        for (std::size_t index = 0; index < panels.size(); ++index)
        {
            TCITEMW item{};
            item.mask = TCIF_TEXT;
            item.pszText = const_cast<LPWSTR>(GetPanelTitle(panels[index]));
            TabCtrl_InsertItem(tabControl, static_cast<int>(index), &item);
        }

        TabCtrl_SetCurSel(tabControl, ClampActiveTabIndex(dockAreaId));
    }

    HWND EditorDockingController::GetDockAreaTabControl(DockAreaId dockAreaId) const
    {
        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            return m_state.leftTopDockTab;
        case DockAreaId::LeftBottom:
            return m_state.leftBottomDockTab;
        case DockAreaId::Center:
            return m_state.centerDockTab;
        case DockAreaId::Right:
            return m_state.rightDockTab;
        default:
            return nullptr;
        }
    }

    HWND EditorDockingController::GetDockLeafTabControl(DockNodeId dockNodeId) const
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_state.dockNodes.size())
        {
            return nullptr;
        }

        return m_state.dockNodes[static_cast<std::size_t>(dockNodeId)].tabControl;
    }

    std::vector<EditorPanelId>& EditorDockingController::GetDockAreaPanels(DockAreaId dockAreaId)
    {
        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            return m_state.leftTopDockPanels;
        case DockAreaId::LeftBottom:
            return m_state.leftBottomDockPanels;
        case DockAreaId::Center:
            return m_state.centerDockPanels;
        case DockAreaId::Right:
            return m_state.rightDockPanels;
        default:
            return m_state.centerDockPanels;
        }
    }

    const std::vector<EditorPanelId>& EditorDockingController::GetDockAreaPanels(DockAreaId dockAreaId) const
    {
        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            return m_state.leftTopDockPanels;
        case DockAreaId::LeftBottom:
            return m_state.leftBottomDockPanels;
        case DockAreaId::Center:
            return m_state.centerDockPanels;
        case DockAreaId::Right:
            return m_state.rightDockPanels;
        default:
            return m_state.centerDockPanels;
        }
    }

    const wchar_t* EditorDockingController::GetPanelTitle(EditorPanelId panelId)
    {
        switch (panelId)
        {
        case EditorPanelId::Hierarchy:
            return L"Hierarchy";
        case EditorPanelId::Assets:
            return L"Assets";
        case EditorPanelId::SceneView:
            return L"Scene";
        case EditorPanelId::Inspector:
            return L"Inspector";
        case EditorPanelId::LogOutput:
            return L"LogOutput";
        default:
            return L"Panel";
        }
    }

    int EditorDockingController::ClampActiveTabIndex(DockAreaId dockAreaId) const
    {
        const std::vector<EditorPanelId>& panels = GetDockAreaPanels(dockAreaId);
        if (panels.empty())
        {
            return -1;
        }

        int activeIndex = 0;
        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            activeIndex = m_state.leftTopActiveTabIndex;
            break;
        case DockAreaId::LeftBottom:
            activeIndex = m_state.leftBottomActiveTabIndex;
            break;
        case DockAreaId::Center:
            activeIndex = m_state.centerActiveTabIndex;
            break;
        case DockAreaId::Right:
            activeIndex = m_state.rightActiveTabIndex;
            break;
        default:
            activeIndex = 0;
            break;
        }

        return (std::max)(0, (std::min)(activeIndex, static_cast<int>(panels.size()) - 1));
    }

    RECT EditorDockingController::GetDockAreaPreviewRect(HWND parentWindow, DockAreaId dockAreaId) const
    {
        RECT clientRect{};
        GetClientRect(parentWindow, &clientRect);

        const int outerPadding = m_shell.ScaleMetric(12);
        const int panelSpacing = m_shell.ScaleMetric(12);
        const int clientWidth = static_cast<int>(clientRect.right - clientRect.left);
        const int clientHeight = static_cast<int>(clientRect.bottom - clientRect.top);
        const int availableColumnWidth = (std::max)(0, clientWidth - (outerPadding * 2) - (panelSpacing * 2));
        const int centerWidth = (std::max)(m_shell.ScaleMetric(120), availableColumnWidth - m_state.leftPaneWidth - m_state.rightPaneWidth);
        const int centerX = outerPadding + m_state.leftPaneWidth + panelSpacing;
        const int rightX = centerX + centerWidth + panelSpacing;
        const int dockHeight = (std::max)(0, clientHeight - outerPadding * 2);

        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            return RECT{
                outerPadding,
                outerPadding,
                outerPadding + m_state.leftPaneWidth,
                outerPadding + m_state.leftTopHeight
            };
        case DockAreaId::LeftBottom:
            return RECT{
                outerPadding,
                outerPadding + m_state.leftTopHeight + panelSpacing,
                outerPadding + m_state.leftPaneWidth,
                outerPadding + dockHeight
            };
        case DockAreaId::Center:
            return RECT{
                centerX,
                outerPadding,
                centerX + centerWidth,
                outerPadding + dockHeight
            };
        case DockAreaId::Right:
            return RECT{
                rightX,
                outerPadding,
                rightX + m_state.rightPaneWidth,
                outerPadding + dockHeight
            };
        default:
            return RECT{};
        }
    }

    void EditorDockingController::UpdateDockPreviewWindow(HWND parentWindow)
    {
        if (nullptr == m_state.dockPreviewWindow || nullptr == parentWindow)
        {
            return;
        }

        if (false == m_state.hasDockPreview)
        {
            ShowWindow(m_state.dockPreviewWindow, SW_HIDE);
            return;
        }

        SetWindowPos(
            m_state.dockPreviewWindow,
            HWND_TOP,
            m_state.dockPreviewRect.left,
            m_state.dockPreviewRect.top,
            (std::max)(0, static_cast<int>(m_state.dockPreviewRect.right - m_state.dockPreviewRect.left)),
            (std::max)(0, static_cast<int>(m_state.dockPreviewRect.bottom - m_state.dockPreviewRect.top)),
            SWP_NOACTIVATE | SWP_SHOWWINDOW);
        InvalidateRect(m_state.dockPreviewWindow, nullptr, TRUE);
        UpdateWindow(m_state.dockPreviewWindow);

        for (HWND guideWindow : m_state.dockGuideWindows)
        {
            if (nullptr != guideWindow && IsWindowVisible(guideWindow))
            {
                SetWindowPos(
                    guideWindow,
                    HWND_TOP,
                    0,
                    0,
                    0,
                    0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }
    }


    LRESULT CALLBACK EditorDockingController::FloatingPanelWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (WM_NCCREATE == message)
        {
            const CREATESTRUCTW* createStruct = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            const FloatingPanelCreateParams* createParams =
                nullptr != createStruct
                    ? static_cast<const FloatingPanelCreateParams*>(createStruct->lpCreateParams)
                    : nullptr;
            if (nullptr != createParams && nullptr != createParams->controller)
            {
                FloatingPanelWindowData* windowData = new FloatingPanelWindowData{
                    createParams->controller,
                    createParams->panelId
                };
                SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(windowData));
            }
        }

        FloatingPanelWindowData* windowData =
            reinterpret_cast<FloatingPanelWindowData*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (nullptr != windowData && nullptr != windowData->controller)
        {
            if (WM_SIZE == message)
            {
                windowData->controller->LayoutFloatingWindow(window);
                return 0;
            }

            if (WM_NOTIFY == message)
            {
                const std::optional<LRESULT> themeResult =
                    windowData->controller->m_shell.HandleThemeMessage(message, wParam, lParam);
                if (true == themeResult.has_value())
                {
                    return *themeResult;
                }

                NMHDR* notifyHeader = reinterpret_cast<NMHDR*>(lParam);
                const int groupIndex = windowData->controller->FindFloatingPanelGroupIndex(window);
                if (nullptr != notifyHeader
                    && groupIndex >= 0
                    && TCN_SELCHANGE == notifyHeader->code
                    && notifyHeader->hwndFrom == windowData->controller->m_state.floatingPanelGroups[static_cast<std::size_t>(groupIndex)].tabControl)
                {
                    FloatingPanelGroup& group =
                        windowData->controller->m_state.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
                    group.activeTabIndex = TabCtrl_GetCurSel(group.tabControl);
                    windowData->controller->LayoutFloatingWindow(window);
                    return 0;
                }
            }

            if (WM_DRAWITEM == message)
            {
                if (windowData->controller->m_shell.HandleDrawItem(lParam))
                {
                    return TRUE;
                }
            }

            if (WM_MOVING == message)
            {
                windowData->controller->BeginFloatingWindowDockDrag(windowData->controller->GetActiveFloatingPanel(window));
                windowData->controller->UpdateFloatingWindowDockDrag();
            }

            if (WM_EXITSIZEMOVE == message)
            {
                windowData->controller->CompleteFloatingWindowDockDrag(windowData->controller->GetActiveFloatingPanel(window));
            }

            if (WM_CLOSE == message)
            {
                windowData->controller->HandleFloatingWindowClose(windowData->panelId, window);
                return 0;
            }
        }

        if (WM_NCDESTROY == message)
        {
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            delete windowData;
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

    LRESULT CALLBACK EditorShell::EditorTabControlSubclassProc(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam,
        UINT_PTR subclassId,
        DWORD_PTR referenceData)
    {
        (void)subclassId;
        (void)referenceData;

        if (WM_ERASEBKGND == message)
        {
            FillTabControlBackground(window, reinterpret_cast<HDC>(wParam));
            return 1;
        }

        if (WM_PAINT == message)
        {
            const LRESULT result = DefSubclassProc(window, message, wParam, lParam);
            HDC deviceContext = GetDC(window);
            if (nullptr != deviceContext)
            {
                FillTabControlBackground(window, deviceContext);
                ReleaseDC(window, deviceContext);
            }
            return result;
        }

        if (WM_MOUSEMOVE == message)
        {
            TRACKMOUSEEVENT trackMouseEvent{};
            trackMouseEvent.cbSize = sizeof(trackMouseEvent);
            trackMouseEvent.dwFlags = TME_LEAVE;
            trackMouseEvent.hwndTrack = window;
            TrackMouseEvent(&trackMouseEvent);
            InvalidateRect(window, nullptr, FALSE);
        }
        else if (WM_MOUSELEAVE == message)
        {
            InvalidateRect(window, nullptr, FALSE);
        }
        else if (WM_NCDESTROY == message)
        {
            RemoveWindowSubclass(window, EditorShell::EditorTabControlSubclassProc, EditorTabControlSubclassId);
        }

        return DefSubclassProc(window, message, wParam, lParam);
    }

}
