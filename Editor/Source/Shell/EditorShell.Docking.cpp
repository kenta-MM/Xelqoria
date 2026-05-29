// EditorShell の Dock/Floating 関連実装。
// このファイルは EditorShell.cpp から include され、既存の内部ヘルパーを共有する。

    void EditorShell::LayoutDockArea(DockAreaId dockAreaId, const RECT& areaRect)
    {
        HWND tabControl = GetDockAreaTabControl(dockAreaId);
        if (nullptr == tabControl)
        {
            return;
        }

        const std::vector<EditorPanelId>& panels = GetDockAreaPanels(dockAreaId);
        const int tabHeight = ScaleMetric(28);
        MoveChildWindowNoRedraw(
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
            ShowPanelControls(panels[index], active);
            if (false == active)
            {
                continue;
            }

            SetPanelParent(panels[index], m_parentWindow);
            GetPanelView(panels[index]).Layout(panelRect);
        }
    }

    void EditorShell::BuildInitialDockTree()
    {
        m_docking.dockNodes.clear();

        DockNode assetsNode{};
        assetsNode.kind = DockNodeKind::Leaf;
        assetsNode.panels = { EditorPanelId::Assets };
        assetsNode.tabControl = m_docking.leftTopDockTab;
        const DockNodeId assetsNodeId = AddDockNode(std::move(assetsNode));

        DockNode hierarchyNode{};
        hierarchyNode.kind = DockNodeKind::Leaf;
        hierarchyNode.panels = { EditorPanelId::Hierarchy };
        hierarchyNode.tabControl = m_docking.leftBottomDockTab;
        const DockNodeId hierarchyNodeId = AddDockNode(std::move(hierarchyNode));

        DockNode sceneViewNode{};
        sceneViewNode.kind = DockNodeKind::Leaf;
        sceneViewNode.panels = { EditorPanelId::SceneView };
        sceneViewNode.tabControl = m_docking.centerDockTab;
        const DockNodeId sceneViewNodeId = AddDockNode(std::move(sceneViewNode));

        DockNode logOutputNode{};
        logOutputNode.kind = DockNodeKind::Leaf;
        logOutputNode.panels = { EditorPanelId::LogOutput };
        logOutputNode.tabControl = CreateAdditionalDockTabControl(m_parentWindow);
        m_docking.logOutputDockTab = logOutputNode.tabControl;
        const DockNodeId logOutputNodeId = AddDockNode(std::move(logOutputNode));

        DockNode inspectorNode{};
        inspectorNode.kind = DockNodeKind::Leaf;
        inspectorNode.panels = { EditorPanelId::Inspector };
        inspectorNode.tabControl = m_docking.rightDockTab;
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
        m_docking.rootDockNodeId = AddDockNode(rootNode);
    }

    void EditorShell::LayoutDockNode(DockNodeId dockNodeId, const RECT& nodeRect)
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_docking.dockNodes.size())
        {
            return;
        }

        DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
        dockNode.rect = nodeRect;
        if (DockNodeKind::Leaf == dockNode.kind)
        {
            LayoutDockLeaf(dockNodeId, nodeRect);
            return;
        }

        const int panelSpacing = ScaleMetric(8);
        dockNode.splitRatio = ClampDockSplitRatio(dockNodeId, dockNode.splitRatio);
        if (DockSplitOrientation::Horizontal == dockNode.splitOrientation)
        {
            const int width = (std::max)(0, static_cast<int>(nodeRect.right - nodeRect.left));
            const int firstWidth = (std::max)(ScaleMetric(80), static_cast<int>((width - panelSpacing) * dockNode.splitRatio));
            const RECT firstRect{ nodeRect.left, nodeRect.top, nodeRect.left + firstWidth, nodeRect.bottom };
            const RECT secondRect{ nodeRect.left + firstWidth + panelSpacing, nodeRect.top, nodeRect.right, nodeRect.bottom };
            LayoutDockNode(dockNode.firstChild, firstRect);
            LayoutDockNode(dockNode.secondChild, secondRect);
            return;
        }

        const int height = (std::max)(0, static_cast<int>(nodeRect.bottom - nodeRect.top));
        const int firstHeight = (std::max)(ScaleMetric(80), static_cast<int>((height - panelSpacing) * dockNode.splitRatio));
        const RECT firstRect{ nodeRect.left, nodeRect.top, nodeRect.right, nodeRect.top + firstHeight };
        const RECT secondRect{ nodeRect.left, nodeRect.top + firstHeight + panelSpacing, nodeRect.right, nodeRect.bottom };
        LayoutDockNode(dockNode.firstChild, firstRect);
        LayoutDockNode(dockNode.secondChild, secondRect);
    }

    void EditorShell::LayoutDockLeaf(DockNodeId dockNodeId, const RECT& areaRect)
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_docking.dockNodes.size())
        {
            return;
        }

        DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
        HWND tabControl = dockNode.tabControl;
        if (nullptr == tabControl)
        {
            return;
        }

        const int tabHeight = ScaleMetric(28);
        MoveChildWindowNoRedraw(
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
            ShowPanelControls(panelId, active);
            if (false == active)
            {
                continue;
            }

            SetPanelParent(panelId, m_parentWindow);
            GetPanelView(panelId).Layout(panelRect);
        }
    }

    EditorShell::DockNodeId EditorShell::AddDockNode(DockNode node)
    {
        const DockNodeId dockNodeId = static_cast<DockNodeId>(m_docking.dockNodes.size());
        m_docking.dockNodes.push_back(std::move(node));
        return dockNodeId;
    }

    EditorShell::DockNodeId EditorShell::EnsureDefaultDockLeaf(EditorPanelId panelId)
    {
        HWND targetTabControl = GetDefaultDockTabControl(panelId);
        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_docking.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Leaf == dockNode.kind && dockNode.tabControl == targetTabControl)
            {
                return dockNodeId;
            }
        }

        if (EditorPanelId::LogOutput == panelId || nullptr == targetTabControl)
        {
            targetTabControl = CreateAdditionalDockTabControl(m_parentWindow);
            if (EditorPanelId::LogOutput == panelId)
            {
                m_docking.logOutputDockTab = targetTabControl;
            }
        }

        DockNode newLeaf{};
        newLeaf.kind = DockNodeKind::Leaf;
        newLeaf.tabControl = targetTabControl;
        const DockNodeId newLeafNodeId = AddDockNode(std::move(newLeaf));
        if (m_docking.rootDockNodeId < 0 || static_cast<std::size_t>(m_docking.rootDockNodeId) >= m_docking.dockNodes.size())
        {
            m_docking.rootDockNodeId = newLeafNodeId;
            return newLeafNodeId;
        }

        const DockNode oldRootNode = m_docking.dockNodes[static_cast<std::size_t>(m_docking.rootDockNodeId)];
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

        m_docking.dockNodes[static_cast<std::size_t>(m_docking.rootDockNodeId)] = splitNode;
        return newLeafNodeId;
    }

    HWND EditorShell::GetDefaultDockTabControl(EditorPanelId panelId)
    {
        switch (panelId)
        {
        case EditorPanelId::Hierarchy:
            return m_docking.leftTopDockTab;
        case EditorPanelId::Assets:
            return m_docking.leftBottomDockTab;
        case EditorPanelId::SceneView:
            return m_docking.centerDockTab;
        case EditorPanelId::Inspector:
            return m_docking.rightDockTab;
        case EditorPanelId::Sprite:
            return m_docking.rightDockTab;
        case EditorPanelId::Material:
            return m_docking.rightDockTab;
        case EditorPanelId::Collider2D:
            return m_docking.rightDockTab;
        case EditorPanelId::LogOutput:
            return m_docking.logOutputDockTab;
        default:
            return m_docking.centerDockTab;
        }
    }


    bool EditorShell::UpdateDockingCore(HWND parentWindow, const Core::InputSnapshot& inputSnapshot)
    {
        if (nullptr == parentWindow)
        {
            return false;
        }

        if (inputSnapshot.WasKeyPressed('R') && inputSnapshot.IsKeyDown(VK_CONTROL))
        {
            ResetDockLayout();
            return true;
        }

        bool changed = false;
        const POINT cursorScreenPoint = ToWin32Point(inputSnapshot.GetCursorScreenPoint());

        if (inputSnapshot.WasKeyPressed('R') && inputSnapshot.IsKeyDown(VK_CONTROL))
        {
            ResetDockLayout();
            changed = true;
        }

        if (inputSnapshot.WasMouseButtonPressed(Core::MouseButton::Left))
        {
            const DockNodeId hitSplitterNodeId = HitTestDockSplitter(cursorScreenPoint);
            if (0 <= hitSplitterNodeId && static_cast<std::size_t>(hitSplitterNodeId) < m_docking.dockNodes.size())
            {
                const DockNode& splitNode = m_docking.dockNodes[static_cast<std::size_t>(hitSplitterNodeId)];
                m_docking.dragKind = DockSplitOrientation::Horizontal == splitNode.splitOrientation
                    ? DockDragKind::HorizontalSplitter
                    : DockDragKind::VerticalSplitter;
                m_docking.dragSplitNodeId = hitSplitterNodeId;
                m_docking.dragStartScreenPoint = cursorScreenPoint;
                m_docking.dragStartSplitRatio = splitNode.splitRatio;
                SetCapture(parentWindow);
            }
            else
            {
                const std::optional<EditorPanelId> hitPanel = HitTestDockTab(cursorScreenPoint);
                if (hitPanel.has_value())
                {
                    m_docking.pendingDockDragPanelId = hitPanel;
                    m_docking.dragStartScreenPoint = cursorScreenPoint;
                    m_docking.pendingDockDragStartTick = GetTickCount64();
                    SetCapture(parentWindow);
                }
            }
        }

        if (inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left))
        {
            if (DockDragKind::None == m_docking.dragKind && m_docking.pendingDockDragPanelId.has_value())
            {
                const int dragThresholdX = (std::max)(ScaleMetric(4), GetSystemMetrics(SM_CXDRAG));
                const int dragThresholdY = (std::max)(ScaleMetric(4), GetSystemMetrics(SM_CYDRAG));
                const bool movedEnough =
                    dragThresholdX <= std::abs(cursorScreenPoint.x - m_docking.dragStartScreenPoint.x)
                    || dragThresholdY <= std::abs(cursorScreenPoint.y - m_docking.dragStartScreenPoint.y);
                const bool heldLongEnough =
                    DockPanelDragDelayMilliseconds <= GetTickCount64() - m_docking.pendingDockDragStartTick;
                if (movedEnough && heldLongEnough)
                {
                    m_docking.dragKind = DockDragKind::Panel;
                    m_docking.dragPanelId = m_docking.pendingDockDragPanelId;
                    m_docking.pendingDockDragPanelId.reset();
                    m_docking.currentGuideTarget = DockGuideTarget{};
                    m_docking.hasDockPreview = false;
                    BeginDockPanelDrag(*m_docking.dragPanelId, parentWindow, cursorScreenPoint);
                    changed = true;
                }
            }

            if (DockDragKind::Panel == m_docking.dragKind && m_docking.dragPanelId.has_value())
            {
                UpdateDockPanelDragWindow(*m_docking.dragPanelId, cursorScreenPoint);
                UpdateDockGuideWindows(parentWindow, cursorScreenPoint);
                m_docking.currentGuideTarget = HitTestDockGuideTarget(parentWindow, cursorScreenPoint);
                m_docking.hasDockPreview = DockGuideTargetKind::None != m_docking.currentGuideTarget.kind
                    && DockGuideTargetKind::Float != m_docking.currentGuideTarget.kind;
                m_docking.dockPreviewRect = m_docking.currentGuideTarget.previewRect;
                UpdateDockPreviewWindow(parentWindow);
            }
            else if (DockDragKind::HorizontalSplitter == m_docking.dragKind || DockDragKind::VerticalSplitter == m_docking.dragKind)
            {
                changed = UpdateDockSplitterDrag(parentWindow, cursorScreenPoint) || changed;
                if (nullptr != m_cursor)
                {
                    m_cursor->SetShape(
                        DockDragKind::HorizontalSplitter == m_docking.dragKind
                            ? Platform::CursorShape::HorizontalResize
                            : Platform::CursorShape::VerticalResize);
                }
            }
        }
        else if (DockDragKind::None == m_docking.dragKind)
        {
            const DockNodeId hitSplitterNodeId = HitTestDockSplitter(cursorScreenPoint);
            if (0 <= hitSplitterNodeId && static_cast<std::size_t>(hitSplitterNodeId) < m_docking.dockNodes.size())
            {
                const DockNode& splitNode = m_docking.dockNodes[static_cast<std::size_t>(hitSplitterNodeId)];
                if (nullptr != m_cursor)
                {
                    m_cursor->SetShape(
                        DockSplitOrientation::Horizontal == splitNode.splitOrientation
                            ? Platform::CursorShape::HorizontalResize
                            : Platform::CursorShape::VerticalResize);
                }
            }
        }

        if (inputSnapshot.WasMouseButtonReleased(Core::MouseButton::Left))
        {
            if (DockDragKind::Panel == m_docking.dragKind && m_docking.dragPanelId.has_value())
            {
                const DockGuideTarget guideTarget = m_docking.currentGuideTarget;
                m_docking.currentGuideTarget = DockGuideTarget{};
                m_docking.hasDockPreview = false;
                HideDockGuideWindows();
                UpdateDockPreviewWindow(parentWindow);
                ApplyDockGuideTarget(*m_docking.dragPanelId, guideTarget, parentWindow);
                changed = true;
            }

            m_docking.dragKind = DockDragKind::None;
            m_docking.dragPanelId.reset();
            m_docking.pendingDockDragPanelId.reset();
            m_docking.pendingDockDragStartTick = 0;
            m_docking.dragSplitNodeId = -1;
            m_docking.hasDockPreview = false;
            m_docking.dragPanelWindowOffset = POINT{};
            ReleaseCapture();
            HideDockGuideWindows();
            UpdateDockPreviewWindow(parentWindow);
        }

        if (changed)
        {
            m_layoutInitialized = false;
        }

        return changed;
    }

    bool EditorShell::HandleDockNotifyCore(LPARAM notifyParameter)
    {
        if (false == m_panelViewsInitialized)
        {
            return false;
        }

        const NMHDR* notifyHeader = reinterpret_cast<const NMHDR*>(notifyParameter);
        if (nullptr == notifyHeader || TCN_SELCHANGE != notifyHeader->code)
        {
            return false;
        }

        for (DockNode& dockNode : m_docking.dockNodes)
        {
            if (DockNodeKind::Leaf != dockNode.kind || notifyHeader->hwndFrom != dockNode.tabControl)
            {
                continue;
            }

            dockNode.activeTabIndex = TabCtrl_GetCurSel(dockNode.tabControl);
            m_layoutInitialized = false;
            return true;
        }

        return false;
    }

    void EditorShell::ResetDockLayoutCore()
    {
        m_docking.leftTopDockPanels = { EditorPanelId::Hierarchy };
        m_docking.leftBottomDockPanels = { EditorPanelId::Assets };
        m_docking.centerDockPanels = { EditorPanelId::SceneView };
        m_docking.rightDockPanels = { EditorPanelId::Inspector };
        m_docking.leftTopActiveTabIndex = 0;
        m_docking.leftBottomActiveTabIndex = 0;
        m_docking.centerActiveTabIndex = 0;
        m_docking.rightActiveTabIndex = 0;
        for (HWND tabControl : m_docking.dynamicDockTabs)
        {
            if (nullptr != tabControl)
            {
                DestroyWindow(tabControl);
            }
        }
        m_docking.dynamicDockTabs.clear();
        m_docking.logOutputDockTab = nullptr;
        m_docking.hasDockPreview = false;
        m_docking.currentGuideTarget = DockGuideTarget{};
        m_docking.pendingDockDragPanelId.reset();
        m_docking.pendingDockDragStartTick = 0;
        BuildInitialDockTree();
        HideDockGuideWindows();
        UpdateDockPreviewWindow(m_parentWindow);
        m_docking.leftPaneWidth = ScaleMetric(260);
        m_docking.rightPaneWidth = ScaleMetric(300);
        m_docking.leftTopHeight = ScaleMetric(280);
        SetPanelParent(EditorPanelId::Hierarchy, m_parentWindow);
        SetPanelParent(EditorPanelId::Assets, m_parentWindow);
        SetPanelParent(EditorPanelId::SceneView, m_parentWindow);
        SetPanelParent(EditorPanelId::Inspector, m_parentWindow);
        SetPanelParent(EditorPanelId::LogOutput, m_parentWindow);
        DestroyFloatingWindow(EditorPanelId::Hierarchy);
        DestroyFloatingWindow(EditorPanelId::Assets);
        DestroyFloatingWindow(EditorPanelId::SceneView);
        DestroyFloatingWindow(EditorPanelId::Inspector);
        DestroyFloatingWindow(EditorPanelId::Sprite);
        DestroyFloatingWindow(EditorPanelId::Material);
        DestroyFloatingWindow(EditorPanelId::Collider2D);
        DestroyFloatingWindow(EditorPanelId::LogOutput);
        SyncDockTabs();
        m_layoutInitialized = false;
    }

    void EditorShell::ShowPanelAtDefaultDockCore(EditorPanelId panelId)
    {
        if (nullptr == m_parentWindow)
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
        SetPanelParent(panelId, m_parentWindow);
        const DockNodeId targetDockNodeId = EnsureDefaultDockLeaf(panelId);
        if (targetDockNodeId < 0 || static_cast<std::size_t>(targetDockNodeId) >= m_docking.dockNodes.size())
        {
            return;
        }

        DockNode& targetDockNode = m_docking.dockNodes[static_cast<std::size_t>(targetDockNodeId)];
        if (targetDockNode.panels.end() == std::find(targetDockNode.panels.begin(), targetDockNode.panels.end(), panelId))
        {
            targetDockNode.panels.push_back(panelId);
        }
        targetDockNode.activeTabIndex = static_cast<int>(targetDockNode.panels.size()) - 1;
        ShowPanelControls(panelId, true);
        SyncDockTabs();
        m_layoutInitialized = false;
    }

    void EditorShell::ActivatePanelCore(EditorPanelId panelId)
    {
        if (nullptr == m_parentWindow)
        {
            return;
        }

        const int floatingGroupIndex = FindFloatingPanelGroupIndex(panelId);
        if (floatingGroupIndex >= 0)
        {
            FloatingPanelGroup& group = m_docking.floatingPanelGroups[static_cast<std::size_t>(floatingGroupIndex)];
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
        if (0 <= dockNodeId && static_cast<std::size_t>(dockNodeId) < m_docking.dockNodes.size())
        {
            DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
            const auto panelIt = std::find(dockNode.panels.begin(), dockNode.panels.end(), panelId);
            if (panelIt != dockNode.panels.end())
            {
                dockNode.activeTabIndex = static_cast<int>(std::distance(dockNode.panels.begin(), panelIt));
                SyncDockTabs();
                m_layoutInitialized = false;
                return;
            }
        }

        ShowPanelAtDefaultDock(panelId);
    }

    bool EditorShell::SaveLayoutCore(const std::filesystem::path& layoutPath) const
    {
        std::error_code errorCode;
        std::filesystem::create_directories(layoutPath.parent_path(), errorCode);
        if (errorCode)
        {
            return false;
        }

        std::wofstream output(layoutPath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return false;
        }

        output << L"XelqoriaEditorLayout 1\n";
        output << L"Root " << m_docking.rootDockNodeId << L'\n';
        output << L"Nodes " << m_docking.dockNodes.size() << L'\n';
        for (std::size_t index = 0; index < m_docking.dockNodes.size(); ++index)
        {
            const DockNode& dockNode = m_docking.dockNodes[index];
            output << L"Node "
                << index << L' '
                << (DockNodeKind::Leaf == dockNode.kind ? L"Leaf" : L"Split") << L' '
                << (DockSplitOrientation::Horizontal == dockNode.splitOrientation ? L"Horizontal" : L"Vertical") << L' '
                << dockNode.splitRatio << L' '
                << dockNode.firstChild << L' '
                << dockNode.secondChild << L' '
                << dockNode.activeTabIndex << L' '
                << GetDockTabLayoutKey(dockNode.tabControl) << L' '
                << dockNode.panels.size();
            for (EditorPanelId panelId : dockNode.panels)
            {
                output << L' ' << GetPanelLayoutName(panelId);
            }
            output << L'\n';
        }

        output << L"Floating " << m_docking.floatingPanelGroups.size() << L'\n';
        for (const FloatingPanelGroup& group : m_docking.floatingPanelGroups)
        {
            RECT windowRect{};
            if (nullptr != group.window)
            {
                GetWindowRect(group.window, &windowRect);
            }

            output << L"FloatingGroup "
                << windowRect.left << L' '
                << windowRect.top << L' '
                << windowRect.right << L' '
                << windowRect.bottom << L' '
                << group.activeTabIndex << L' '
                << group.panels.size();
            for (EditorPanelId panelId : group.panels)
            {
                output << L' ' << GetPanelLayoutName(panelId);
            }
            output << L'\n';
        }

        return output.good();
    }

    bool EditorShell::LoadLayoutCore(const std::filesystem::path& layoutPath)
    {
        if (nullptr == m_parentWindow)
        {
            return false;
        }

        std::wifstream input(layoutPath, std::ios::binary);
        if (false == input.is_open())
        {
            return false;
        }

        std::wstring signature{};
        int version = 0;
        input >> signature >> version;
        if (L"XelqoriaEditorLayout" != signature || 1 != version)
        {
            return false;
        }

        std::wstring section{};
        DockNodeId rootDockNodeId = -1;
        input >> section >> rootDockNodeId;
        if (L"Root" != section)
        {
            return false;
        }

        std::size_t nodeCount = 0;
        input >> section >> nodeCount;
        if (L"Nodes" != section || 0 == nodeCount)
        {
            return false;
        }

        std::vector<SavedDockNode> savedNodes(nodeCount);
        for (std::size_t index = 0; index < nodeCount; ++index)
        {
            std::wstring nodeToken{};
            std::size_t savedIndex = 0;
            std::wstring kind{};
            std::wstring orientation{};
            std::size_t panelCount = 0;
            input >> nodeToken
                >> savedIndex
                >> kind
                >> orientation
                >> savedNodes[index].splitRatio
                >> savedNodes[index].firstChild
                >> savedNodes[index].secondChild
                >> savedNodes[index].activeTabIndex
                >> savedNodes[index].tabKey
                >> panelCount;
            if (L"Node" != nodeToken || savedIndex != index)
            {
                return false;
            }

            savedNodes[index].isLeaf = L"Leaf" == kind;
            savedNodes[index].isHorizontalSplit = L"Horizontal" == orientation;
            for (std::size_t panelIndex = 0; panelIndex < panelCount; ++panelIndex)
            {
                std::wstring panelName{};
                input >> panelName;
                const std::optional<EditorPanelId> panelId = TryParsePanelLayoutName(panelName);
                if (panelId.has_value() && IsDockableEditorPanel(*panelId))
                {
                    savedNodes[index].panels.push_back(*panelId);
                }
            }
        }

        std::size_t floatingGroupCount = 0;
        input >> section >> floatingGroupCount;
        if (L"Floating" != section)
        {
            return false;
        }

        std::vector<SavedFloatingGroup> savedFloatingGroups{};
        savedFloatingGroups.reserve(floatingGroupCount);
        for (std::size_t index = 0; index < floatingGroupCount; ++index)
        {
            std::wstring groupToken{};
            SavedFloatingGroup group{};
            std::size_t panelCount = 0;
            input >> groupToken
                >> group.rect.left
                >> group.rect.top
                >> group.rect.right
                >> group.rect.bottom
                >> group.activeTabIndex
                >> panelCount;
            if (L"FloatingGroup" != groupToken)
            {
                return false;
            }

            for (std::size_t panelIndex = 0; panelIndex < panelCount; ++panelIndex)
            {
                std::wstring panelName{};
                input >> panelName;
                const std::optional<EditorPanelId> panelId = TryParsePanelLayoutName(panelName);
                if (panelId.has_value() && IsDockableEditorPanel(*panelId))
                {
                    group.panels.push_back(*panelId);
                }
            }
            if (false == group.panels.empty())
            {
                savedFloatingGroups.push_back(std::move(group));
            }
        }

        ResetDockLayout();
        for (HWND tabControl : m_docking.dynamicDockTabs)
        {
            if (nullptr != tabControl)
            {
                DestroyWindow(tabControl);
            }
        }
        m_docking.dynamicDockTabs.clear();
        m_docking.logOutputDockTab = nullptr;
        m_docking.dockNodes.clear();
        m_docking.dockNodes.reserve(savedNodes.size());

        for (const SavedDockNode& savedNode : savedNodes)
        {
            DockNode dockNode{};
            dockNode.kind = savedNode.isLeaf ? DockNodeKind::Leaf : DockNodeKind::Split;
            dockNode.splitOrientation = savedNode.isHorizontalSplit
                ? DockSplitOrientation::Horizontal
                : DockSplitOrientation::Vertical;
            dockNode.splitRatio = (std::max)(0.05f, (std::min)(0.95f, savedNode.splitRatio));
            dockNode.firstChild = savedNode.firstChild;
            dockNode.secondChild = savedNode.secondChild;
            dockNode.activeTabIndex = savedNode.activeTabIndex;
            dockNode.panels = savedNode.panels;
            if (DockNodeKind::Leaf == dockNode.kind)
            {
                dockNode.tabControl = CreateDockTabControlForLayoutKey(savedNode.tabKey);
            }
            m_docking.dockNodes.push_back(std::move(dockNode));
        }
        m_docking.rootDockNodeId = rootDockNodeId;

        for (const SavedFloatingGroup& group : savedFloatingGroups)
        {
            for (EditorPanelId panelId : group.panels)
            {
                RemovePanelFromDockTree(panelId, false);
            }

            const int width =
                (std::max)(ScaleMetric(240), static_cast<int>(group.rect.right - group.rect.left));
            const int height =
                (std::max)(ScaleMetric(180), static_cast<int>(group.rect.bottom - group.rect.top));
            FloatPanel(group.panels.front(), POINT{ group.rect.left, group.rect.top }, m_parentWindow);
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
                FloatingPanelGroup& floatingGroup = m_docking.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
                floatingGroup.activeTabIndex =
                    (std::max)(0, (std::min)(group.activeTabIndex, static_cast<int>(floatingGroup.panels.size()) - 1));
                SyncFloatingPanelTabs(floatingWindow);
                LayoutFloatingWindow(floatingWindow);
            }
        }

        RestoreMissingPanelsToDefaultDock();
        SyncDockTabs();
        m_layoutInitialized = false;
        return true;
    }

    RECT EditorShell::GetPanelCaptionRect(EditorPanelId panelId) const
    {
        const HWND panelWindow = GetPanelView(panelId).GetRootWindow();

        RECT captionRect{};
        if (nullptr != panelWindow && IsWindowVisible(panelWindow))
        {
            GetWindowRect(panelWindow, &captionRect);
            captionRect.bottom = captionRect.top + ScaleMetric(28);
        }

        return captionRect;
    }

    std::optional<EditorPanelId> EditorShell::HitTestPanelCaption(POINT cursorScreenPoint) const
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
        CollectReachableDockLeaves(m_docking.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
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
            tabRect.bottom = tabRect.top + ScaleMetric(28);
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

    std::optional<EditorPanelId> EditorShell::HitTestDockTab(POINT cursorScreenPoint) const
    {
        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_docking.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
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

    EditorShell::DockNodeId EditorShell::HitTestDockLeaf(POINT cursorClientPoint) const
    {
        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_docking.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
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

    EditorShell::DockNodeId EditorShell::HitTestDockSplitter(POINT cursorScreenPoint) const
    {
        if (nullptr == m_parentWindow)
        {
            return -1;
        }

        POINT cursorClientPoint = cursorScreenPoint;
        ScreenToClient(m_parentWindow, &cursorClientPoint);

        std::vector<DockNodeId> dockSplitNodeIds{};
        CollectReachableDockSplits(m_docking.rootDockNodeId, dockSplitNodeIds);
        for (DockNodeId dockNodeId : dockSplitNodeIds)
        {
            const DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Split != dockNode.kind
                || dockNode.firstChild < 0
                || dockNode.secondChild < 0
                || static_cast<std::size_t>(dockNode.firstChild) >= m_docking.dockNodes.size()
                || static_cast<std::size_t>(dockNode.secondChild) >= m_docking.dockNodes.size())
            {
                continue;
            }

            const RECT firstRect = m_docking.dockNodes[static_cast<std::size_t>(dockNode.firstChild)].rect;
            const RECT secondRect = m_docking.dockNodes[static_cast<std::size_t>(dockNode.secondChild)].rect;
            RECT splitterRect{};
            if (DockSplitOrientation::Horizontal == dockNode.splitOrientation)
            {
                splitterRect = RECT{ firstRect.right, dockNode.rect.top, secondRect.left, dockNode.rect.bottom };
            }
            else
            {
                splitterRect = RECT{ dockNode.rect.left, firstRect.bottom, dockNode.rect.right, secondRect.top };
            }

            const int minimumHitThickness = ScaleMetric(6);
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

    void EditorShell::CollectReachableDockLeaves(DockNodeId dockNodeId, std::vector<DockNodeId>& dockLeafNodeIds) const
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_docking.dockNodes.size())
        {
            return;
        }

        const DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
        if (DockNodeKind::Leaf == dockNode.kind)
        {
            dockLeafNodeIds.push_back(dockNodeId);
            return;
        }

        CollectReachableDockLeaves(dockNode.firstChild, dockLeafNodeIds);
        CollectReachableDockLeaves(dockNode.secondChild, dockLeafNodeIds);
    }

    void EditorShell::CollectReachableDockSplits(DockNodeId dockNodeId, std::vector<DockNodeId>& dockSplitNodeIds) const
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_docking.dockNodes.size())
        {
            return;
        }

        const DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
        if (DockNodeKind::Leaf == dockNode.kind)
        {
            return;
        }

        CollectReachableDockSplits(dockNode.firstChild, dockSplitNodeIds);
        dockSplitNodeIds.push_back(dockNodeId);
        CollectReachableDockSplits(dockNode.secondChild, dockSplitNodeIds);
    }

    bool EditorShell::UpdateDockSplitterDrag(HWND parentWindow, POINT cursorScreenPoint)
    {
        if (nullptr == parentWindow
            || m_docking.dragSplitNodeId < 0
            || static_cast<std::size_t>(m_docking.dragSplitNodeId) >= m_docking.dockNodes.size())
        {
            return false;
        }

        DockNode& splitNode = m_docking.dockNodes[static_cast<std::size_t>(m_docking.dragSplitNodeId)];
        if (DockNodeKind::Split != splitNode.kind)
        {
            return false;
        }

        const int panelSpacing = ScaleMetric(12);
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
            ? cursorScreenPoint.x - m_docking.dragStartScreenPoint.x
            : cursorScreenPoint.y - m_docking.dragStartScreenPoint.y;
        const float nextRatio = ClampDockSplitRatio(
            m_docking.dragSplitNodeId,
            m_docking.dragStartSplitRatio + static_cast<float>(cursorDelta) / static_cast<float>(availableLength));
        if (splitNode.splitRatio == nextRatio)
        {
            return false;
        }

        splitNode.splitRatio = nextRatio;
        m_layoutInitialized = false;
        return true;
    }

    float EditorShell::ClampDockSplitRatio(DockNodeId dockNodeId, float ratio) const
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_docking.dockNodes.size())
        {
            return ratio;
        }

        const DockNode& splitNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
        const int panelSpacing = ScaleMetric(12);
        const int totalLength = DockSplitOrientation::Horizontal == splitNode.splitOrientation
            ? splitNode.rect.right - splitNode.rect.left
            : splitNode.rect.bottom - splitNode.rect.top;
        const int availableLength = totalLength - panelSpacing;
        if (availableLength <= 0)
        {
            return 0.5f;
        }

        const float minimumRatio = (std::min)(0.45f, static_cast<float>(ScaleMetric(80)) / static_cast<float>(availableLength));
        return (std::max)(minimumRatio, (std::min)(1.0f - minimumRatio, ratio));
    }

    EditorShell::DockNodeId EditorShell::FindPanelDockLeaf(EditorPanelId panelId) const
    {
        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_docking.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
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

    bool EditorShell::IsPanelInDockTree(EditorPanelId panelId) const
    {
        return FindPanelDockLeaf(panelId) >= 0;
    }

    void EditorShell::RemovePanelFromDockTree(EditorPanelId panelId, bool collapseEmptyLeaves)
    {
        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_docking.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Leaf != dockNode.kind)
            {
                continue;
            }

            RemovePanelFromDockNode(dockNode, panelId);
        }

        if (collapseEmptyLeaves && m_docking.rootDockNodeId >= 0)
        {
            (void)CollapseEmptyDockLeaves(m_docking.rootDockNodeId);
        }
    }

    void EditorShell::RemovePanelFromDockNode(DockNode& dockNode, EditorPanelId panelId) const
    {
        dockNode.panels.erase(std::remove(dockNode.panels.begin(), dockNode.panels.end(), panelId), dockNode.panels.end());
        if (dockNode.activeTabIndex >= static_cast<int>(dockNode.panels.size()))
        {
            dockNode.activeTabIndex = (std::max)(0, static_cast<int>(dockNode.panels.size()) - 1);
        }
    }

    bool EditorShell::CollapseEmptyDockLeaves(DockNodeId dockNodeId)
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_docking.dockNodes.size())
        {
            return false;
        }

        DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
        if (DockNodeKind::Leaf == dockNode.kind)
        {
            return dockNode.panels.empty();
        }

        const bool firstEmpty = CollapseEmptyDockLeaves(dockNode.firstChild);
        const bool secondEmpty = CollapseEmptyDockLeaves(dockNode.secondChild);
        if (firstEmpty && false == secondEmpty)
        {
            dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNode.secondChild)];
            return false;
        }

        if (secondEmpty && false == firstEmpty)
        {
            dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNode.firstChild)];
            return false;
        }

        return firstEmpty && secondEmpty;
    }

    EditorShell::DockGuideTarget EditorShell::HitTestDockGuideTarget(HWND parentWindow, POINT cursorScreenPoint) const
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

        for (std::size_t index = 0; index < m_docking.dockGuideWindows.size(); ++index)
        {
            if (false == hitVisibleGuide(m_docking.dockGuideWindows[index]))
            {
                continue;
            }

            const DockGuideTargetKind kind = guideKinds[index];
            const DockNodeId dockNodeId = index < 5 ? m_docking.currentGuideTarget.dockNodeId : m_docking.rootDockNodeId;
            if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_docking.dockNodes.size())
            {
                return DockGuideTarget{};
            }

            const RECT sourceRect = index < 5
                ? m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)].rect
                : m_docking.dockNodes[static_cast<std::size_t>(m_docking.rootDockNodeId)].rect;
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

    void EditorShell::ApplyDockGuideTarget(EditorPanelId panelId, const DockGuideTarget& guideTarget, HWND parentWindow)
    {
        if (DockGuideTargetKind::None == guideTarget.kind || DockGuideTargetKind::Float == guideTarget.kind)
        {
            const POINT cursorScreenPoint = GetCursorScreenPoint(m_cursor);
            const HWND targetFloatingWindow = HitTestFloatingWindow(cursorScreenPoint, GetFloatingWindowRef(panelId));
            if (nullptr != targetFloatingWindow)
            {
                AttachPanelToFloatingWindow(panelId, targetFloatingWindow);
                SyncDockTabs();
                m_layoutInitialized = false;
                return;
            }

            RemovePanelFromDockTree(panelId);
            FloatPanel(panelId, cursorScreenPoint, parentWindow);
            SyncDockTabs();
            m_layoutInitialized = false;
            return;
        }

        if (guideTarget.dockNodeId < 0 || static_cast<std::size_t>(guideTarget.dockNodeId) >= m_docking.dockNodes.size())
        {
            return;
        }

        const DockNodeId sourceLeafNodeId = FindPanelDockLeaf(panelId);
        HWND sourceTabControl = GetDockLeafTabControl(sourceLeafNodeId);
        if (nullptr == sourceTabControl)
        {
            sourceTabControl = m_docking.centerDockTab;
        }

        RemovePanelFromDockTree(panelId, false);
        if (guideTarget.dockNodeId < 0 || static_cast<std::size_t>(guideTarget.dockNodeId) >= m_docking.dockNodes.size())
        {
            return;
        }

        DockNode& targetNode = m_docking.dockNodes[static_cast<std::size_t>(guideTarget.dockNodeId)];
        if (DockGuideTargetKind::Tab == guideTarget.kind && DockNodeKind::Leaf == targetNode.kind)
        {
            DestroyFloatingWindow(panelId);
            SetPanelParent(panelId, parentWindow);
            targetNode.panels.push_back(panelId);
            targetNode.activeTabIndex = static_cast<int>(targetNode.panels.size()) - 1;
            if (m_docking.rootDockNodeId >= 0)
            {
                (void)CollapseEmptyDockLeaves(m_docking.rootDockNodeId);
            }
            SyncDockTabs();
            m_layoutInitialized = false;
            return;
        }

        DestroyFloatingWindow(panelId);
        SetPanelParent(panelId, parentWindow);

        DockNode oldTargetNode = targetNode;
        if (sourceLeafNodeId == guideTarget.dockNodeId)
        {
            RemovePanelFromDockNode(oldTargetNode, panelId);
            if (oldTargetNode.panels.empty())
            {
                targetNode.panels.push_back(panelId);
                targetNode.activeTabIndex = 0;
                SyncDockTabs();
                m_layoutInitialized = false;
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

        m_docking.dockNodes[targetNodeIndex] = splitNode;
        if (m_docking.rootDockNodeId >= 0)
        {
            (void)CollapseEmptyDockLeaves(m_docking.rootDockNodeId);
        }
        SyncDockTabs();
        m_layoutInitialized = false;
    }

    void EditorShell::UpdateDockGuideWindows(HWND parentWindow, POINT cursorScreenPoint)
    {
        if (nullptr == parentWindow || DockDragKind::Panel != m_docking.dragKind)
        {
            HideDockGuideWindows();
            return;
        }

        POINT cursorClientPoint = cursorScreenPoint;
        ScreenToClient(parentWindow, &cursorClientPoint);
        const DockNodeId hitLeafNodeId = HitTestDockLeaf(cursorClientPoint);
        m_docking.currentGuideTarget.dockNodeId = hitLeafNodeId;

        const int guideSize = ScaleMetric(40);
        const int guideGap = ScaleMetric(3);
        if (0 <= hitLeafNodeId && static_cast<std::size_t>(hitLeafNodeId) < m_docking.dockNodes.size())
        {
            const RECT leafRect = m_docking.dockNodes[static_cast<std::size_t>(hitLeafNodeId)].rect;
            const int centerX = (leafRect.left + leafRect.right) / 2;
            const int centerY = (leafRect.top + leafRect.bottom) / 2;
            ShowDockGuideWindow(m_docking.dockGuideWindows[4], RECT{ centerX - guideSize / 2, centerY - guideSize / 2, centerX + guideSize / 2, centerY + guideSize / 2 });
            ShowDockGuideWindow(m_docking.dockGuideWindows[0], RECT{ centerX - guideSize / 2, centerY - guideSize - guideGap - guideSize / 2, centerX + guideSize / 2, centerY - guideGap - guideSize / 2 });
            ShowDockGuideWindow(m_docking.dockGuideWindows[1], RECT{ centerX - guideSize / 2, centerY + guideGap + guideSize / 2, centerX + guideSize / 2, centerY + guideSize + guideGap + guideSize / 2 });
            ShowDockGuideWindow(m_docking.dockGuideWindows[2], RECT{ centerX - guideSize - guideGap - guideSize / 2, centerY - guideSize / 2, centerX - guideGap - guideSize / 2, centerY + guideSize / 2 });
            ShowDockGuideWindow(m_docking.dockGuideWindows[3], RECT{ centerX + guideGap + guideSize / 2, centerY - guideSize / 2, centerX + guideSize + guideGap + guideSize / 2, centerY + guideSize / 2 });
        }
        else
        {
            for (std::size_t index = 0; index < 5; ++index)
            {
                ShowWindow(m_docking.dockGuideWindows[index], SW_HIDE);
            }
        }

        const RECT rootRect = 0 <= m_docking.rootDockNodeId && static_cast<std::size_t>(m_docking.rootDockNodeId) < m_docking.dockNodes.size()
            ? m_docking.dockNodes[static_cast<std::size_t>(m_docking.rootDockNodeId)].rect
            : RECT{};
        const int rootCenterX = (rootRect.left + rootRect.right) / 2;
        const int rootCenterY = (rootRect.top + rootRect.bottom) / 2;
        ShowDockGuideWindow(m_docking.dockGuideWindows[5], RECT{ rootCenterX - guideSize / 2, rootRect.top + guideGap, rootCenterX + guideSize / 2, rootRect.top + guideGap + guideSize });
        ShowDockGuideWindow(m_docking.dockGuideWindows[6], RECT{ rootCenterX - guideSize / 2, rootRect.bottom - guideGap - guideSize, rootCenterX + guideSize / 2, rootRect.bottom - guideGap });
        ShowDockGuideWindow(m_docking.dockGuideWindows[7], RECT{ rootRect.left + guideGap, rootCenterY - guideSize / 2, rootRect.left + guideGap + guideSize, rootCenterY + guideSize / 2 });
        ShowDockGuideWindow(m_docking.dockGuideWindows[8], RECT{ rootRect.right - guideGap - guideSize, rootCenterY - guideSize / 2, rootRect.right - guideGap, rootCenterY + guideSize / 2 });
    }

    void EditorShell::HideDockGuideWindows()
    {
        for (HWND guideWindow : m_docking.dockGuideWindows)
        {
            ShowWindow(guideWindow, SW_HIDE);
        }
    }

    void EditorShell::ShowDockGuideWindow(HWND guideWindow, const RECT& guideRect)
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

    EditorShell::DockAreaId EditorShell::HitTestDockArea(HWND parentWindow, POINT cursorScreenPoint) const
    {
        RECT clientRect{};
        GetClientRect(parentWindow, &clientRect);
        POINT cursorClientPoint = cursorScreenPoint;
        ScreenToClient(parentWindow, &cursorClientPoint);
        if (false == PtInRect(&clientRect, cursorClientPoint))
        {
            return DockAreaId::Floating;
        }

        const int outerPadding = ScaleMetric(12);
        const int panelSpacing = ScaleMetric(12);
        if (cursorClientPoint.x < outerPadding + m_docking.leftPaneWidth)
        {
            return cursorClientPoint.y < outerPadding + m_docking.leftTopHeight + panelSpacing / 2
                ? DockAreaId::LeftTop
                : DockAreaId::LeftBottom;
        }

        if (cursorClientPoint.x > clientRect.right - outerPadding - m_docking.rightPaneWidth)
        {
            return DockAreaId::Right;
        }

        return DockAreaId::Center;
    }

    void EditorShell::MovePanelToDockArea(EditorPanelId panelId, DockAreaId dockAreaId, HWND parentWindow)
    {
        const auto removePanel =
            [panelId](std::vector<EditorPanelId>& panels)
            {
                panels.erase(std::remove(panels.begin(), panels.end(), panelId), panels.end());
            };

        removePanel(m_docking.leftTopDockPanels);
        removePanel(m_docking.leftBottomDockPanels);
        removePanel(m_docking.centerDockPanels);
        removePanel(m_docking.rightDockPanels);

        if (DockAreaId::Floating == dockAreaId)
        {
            const POINT cursorScreenPoint = GetCursorScreenPoint(m_cursor);
            FloatPanel(panelId, cursorScreenPoint, parentWindow);
            SyncDockTabs();
            m_layoutInitialized = false;
            return;
        }

        SetPanelParent(panelId, parentWindow);
        DestroyFloatingWindow(panelId);
        std::vector<EditorPanelId>& panels = GetDockAreaPanels(dockAreaId);
        panels.push_back(panelId);
        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            m_docking.leftTopActiveTabIndex = static_cast<int>(panels.size()) - 1;
            break;
        case DockAreaId::LeftBottom:
            m_docking.leftBottomActiveTabIndex = static_cast<int>(panels.size()) - 1;
            break;
        case DockAreaId::Center:
            m_docking.centerActiveTabIndex = static_cast<int>(panels.size()) - 1;
            break;
        case DockAreaId::Right:
            m_docking.rightActiveTabIndex = static_cast<int>(panels.size()) - 1;
            break;
        default:
            break;
        }

        SyncDockTabs();
        m_layoutInitialized = false;
    }

    void EditorShell::BeginDockPanelDrag(EditorPanelId panelId, HWND parentWindow, POINT cursorScreenPoint)
    {
        if (nullptr == parentWindow)
        {
            return;
        }

        RECT captionRect = GetPanelCaptionRect(panelId);
        if (captionRect.right <= captionRect.left || captionRect.bottom <= captionRect.top)
        {
            captionRect = RECT{
                cursorScreenPoint.x - ScaleMetric(24),
                cursorScreenPoint.y - ScaleMetric(12),
                cursorScreenPoint.x + ScaleMetric(336),
                cursorScreenPoint.y + ScaleMetric(348)
            };
        }

        m_docking.dragPanelWindowOffset = POINT{
            static_cast<LONG>((std::max)(ScaleMetric(8), static_cast<int>(cursorScreenPoint.x - captionRect.left))),
            static_cast<LONG>((std::max)(ScaleMetric(8), static_cast<int>(cursorScreenPoint.y - captionRect.top)))
        };

        RemovePanelFromDockTree(panelId);
        const POINT floatingOrigin{
            cursorScreenPoint.x - m_docking.dragPanelWindowOffset.x,
            cursorScreenPoint.y - m_docking.dragPanelWindowOffset.y
        };
        FloatPanel(panelId, floatingOrigin, parentWindow);
        SyncDockTabs();
        m_layoutInitialized = false;
    }

    void EditorShell::UpdateDockPanelDragWindow(EditorPanelId panelId, POINT cursorScreenPoint)
    {
        HWND floatingWindow = GetFloatingWindowRef(panelId);
        if (nullptr == floatingWindow)
        {
            return;
        }

        SetWindowPos(
            floatingWindow,
            HWND_TOP,
            cursorScreenPoint.x - m_docking.dragPanelWindowOffset.x,
            cursorScreenPoint.y - m_docking.dragPanelWindowOffset.y,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    void EditorShell::FloatPanel(EditorPanelId panelId, POINT screenPoint, HWND parentWindow)
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
            ScaleMetric(360),
            ScaleMetric(360),
            parentWindow,
            nullptr,
            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parentWindow, GWLP_HINSTANCE)),
            &createParams);
        if (nullptr == floatingWindow)
        {
            return;
        }
        GetFloatingWindowRef(panelId) = floatingWindow;

        HWND tabControl = CreateChildWindow(
            floatingWindow,
            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parentWindow, GWLP_HINSTANCE)),
            WC_TABCONTROLW,
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_OWNERDRAWFIXED);
        if (nullptr != tabControl)
        {
            SendMessageW(tabControl, WM_SETFONT, reinterpret_cast<WPARAM>(m_defaultFont), TRUE);
            ConfigureEditorTabControl(tabControl);
        }
        m_docking.floatingPanelGroups.push_back(FloatingPanelGroup{
            floatingWindow,
            tabControl,
            { panelId },
            0
        });

        SetPanelParent(panelId, floatingWindow);
        ShowPanelControls(panelId, true);
        SyncFloatingPanelTabs(floatingWindow);
        LayoutFloatingWindow(floatingWindow);
    }

    void EditorShell::BeginFloatingWindowDockDrag(EditorPanelId panelId)
    {
        if (nullptr == m_parentWindow)
        {
            return;
        }

        if (DockDragKind::Panel == m_docking.dragKind && m_docking.dragPanelId.has_value() && *m_docking.dragPanelId == panelId)
        {
            return;
        }

        m_docking.dragKind = DockDragKind::Panel;
        m_docking.dragPanelId = panelId;
        m_docking.dragStartScreenPoint = GetCursorScreenPoint(m_cursor);
        m_docking.currentGuideTarget = DockGuideTarget{};
        m_docking.hasDockPreview = false;
    }

    void EditorShell::UpdateFloatingWindowDockDrag()
    {
        if (nullptr == m_parentWindow || DockDragKind::Panel != m_docking.dragKind || false == m_docking.dragPanelId.has_value())
        {
            return;
        }

        const POINT cursorScreenPoint = GetCursorScreenPoint(m_cursor);
        UpdateDockGuideWindows(m_parentWindow, cursorScreenPoint);
        m_docking.currentGuideTarget = HitTestDockGuideTarget(m_parentWindow, cursorScreenPoint);
        m_docking.hasDockPreview = DockGuideTargetKind::None != m_docking.currentGuideTarget.kind
            && DockGuideTargetKind::Float != m_docking.currentGuideTarget.kind;
        m_docking.dockPreviewRect = m_docking.currentGuideTarget.previewRect;
        UpdateDockPreviewWindow(m_parentWindow);
    }

    void EditorShell::CompleteFloatingWindowDockDrag(EditorPanelId panelId)
    {
        if (nullptr == m_parentWindow)
        {
            return;
        }

        UpdateFloatingWindowDockDrag();
        const DockGuideTarget guideTarget = m_docking.currentGuideTarget;
        m_docking.currentGuideTarget = DockGuideTarget{};
        m_docking.hasDockPreview = false;
        HideDockGuideWindows();
        UpdateDockPreviewWindow(m_parentWindow);
        m_docking.dragKind = DockDragKind::None;
        m_docking.dragPanelId.reset();

        if (DockGuideTargetKind::None == guideTarget.kind || DockGuideTargetKind::Float == guideTarget.kind)
        {
            const HWND targetFloatingWindow =
                HitTestFloatingWindow(GetCursorScreenPoint(m_cursor), GetFloatingWindowRef(panelId));
            if (nullptr != targetFloatingWindow)
            {
                AttachPanelToFloatingWindow(panelId, targetFloatingWindow);
            }
            return;
        }

        ApplyDockGuideTarget(panelId, guideTarget, m_parentWindow);
        m_layoutInitialized = false;
    }

    void EditorShell::LayoutFloatingPanel(EditorPanelId panelId, HWND floatingWindow)
    {
        (void)panelId;
        if (nullptr == floatingWindow)
        {
            return;
        }

        LayoutFloatingWindow(floatingWindow);
    }

    void EditorShell::LayoutFloatingWindow(HWND floatingWindow)
    {
        const int groupIndex = FindFloatingPanelGroupIndex(floatingWindow);
        if (groupIndex < 0)
        {
            return;
        }

        FloatingPanelGroup& group = m_docking.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
        RECT clientRect{};
        GetClientRect(floatingWindow, &clientRect);
        const int padding = ScaleMetric(8);
        const int tabHeight = ScaleMetric(28);
        const bool showsTabs = nullptr != group.tabControl && group.panels.size() > 1;
        if (nullptr != group.tabControl)
        {
            if (showsTabs)
            {
                MoveChildWindowNoRedraw(
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
            ShowPanelControls(panelId, activePanelId == panelId);
        }

        const RECT panelRect{
            clientRect.left + padding,
            clientRect.top + padding + (showsTabs ? tabHeight + ScaleMetric(4) : 0),
            (std::max)(clientRect.left + padding, clientRect.right - padding),
            (std::max)(clientRect.top + padding, clientRect.bottom - padding)
        };

        GetPanelView(activePanelId).Layout(panelRect);

        SendGroupBoxesToBack();
        RedrawLayout(floatingWindow);
        if (EditorPanelId::SceneView == activePanelId)
        {
            (void)UpdateSceneViewHostSize();
        }
    }

    void EditorShell::AttachPanelToFloatingWindow(EditorPanelId panelId, HWND floatingWindow)
    {
        const HWND targetWindow = floatingWindow;
        DestroyFloatingWindow(panelId);
        const int targetGroupIndex = FindFloatingPanelGroupIndex(floatingWindow);
        if (targetGroupIndex < 0)
        {
            return;
        }

        FloatingPanelGroup& targetGroup = m_docking.floatingPanelGroups[static_cast<std::size_t>(targetGroupIndex)];
        if (targetGroup.panels.end() == std::find(targetGroup.panels.begin(), targetGroup.panels.end(), panelId))
        {
            targetGroup.panels.push_back(panelId);
        }
        targetGroup.activeTabIndex = static_cast<int>(targetGroup.panels.size()) - 1;
        GetFloatingWindowRef(panelId) = targetWindow;
        SetPanelParent(panelId, targetWindow);
        ShowPanelControls(panelId, true);
        SyncFloatingPanelTabs(targetWindow);
        LayoutFloatingWindow(targetWindow);
    }

    HWND EditorShell::HitTestFloatingWindow(POINT cursorScreenPoint, HWND excludedWindow) const
    {
        for (const FloatingPanelGroup& group : m_docking.floatingPanelGroups)
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

    void EditorShell::HandleFloatingWindowClose(EditorPanelId panelId, HWND floatingWindow)
    {
        (void)panelId;
        const int groupIndex = FindFloatingPanelGroupIndex(floatingWindow);
        if (groupIndex < 0)
        {
            return;
        }

        const FloatingPanelGroup group = m_docking.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
        for (EditorPanelId floatingPanelId : group.panels)
        {
            HWND& floatingWindowRef = GetFloatingWindowRef(floatingPanelId);
            if (floatingWindowRef == floatingWindow)
            {
                floatingWindowRef = nullptr;
            }
            SetPanelParent(floatingPanelId, m_parentWindow);
            RemovePanelFromDockTree(floatingPanelId, false);
            ShowPanelControls(floatingPanelId, false);
        }

        m_docking.floatingPanelGroups.erase(m_docking.floatingPanelGroups.begin() + groupIndex);
        SyncDockTabs();
        m_layoutInitialized = false;
        DestroyWindow(floatingWindow);
    }

    void EditorShell::DestroyFloatingWindow(EditorPanelId panelId)
    {
        HWND& floatingWindow = GetFloatingWindowRef(panelId);
        if (nullptr == floatingWindow)
        {
            return;
        }

        const HWND windowToUpdate = floatingWindow;
        SetPanelParent(panelId, m_parentWindow);
        floatingWindow = nullptr;

        const int groupIndex = FindFloatingPanelGroupIndex(windowToUpdate);
        if (groupIndex < 0)
        {
            DestroyWindow(windowToUpdate);
            return;
        }

        FloatingPanelGroup& group = m_docking.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
        group.panels.erase(std::remove(group.panels.begin(), group.panels.end(), panelId), group.panels.end());
        if (group.activeTabIndex >= static_cast<int>(group.panels.size()))
        {
            group.activeTabIndex = (std::max)(0, static_cast<int>(group.panels.size()) - 1);
        }

        if (group.panels.empty())
        {
            m_docking.floatingPanelGroups.erase(m_docking.floatingPanelGroups.begin() + groupIndex);
            DestroyWindow(windowToUpdate);
            return;
        }

        SyncFloatingPanelTabs(windowToUpdate);
        LayoutFloatingWindow(windowToUpdate);
    }

    HWND& EditorShell::GetFloatingWindowRef(EditorPanelId panelId)
    {
        switch (panelId)
        {
        case EditorPanelId::Hierarchy:
            return m_docking.hierarchyFloatingWindow;
        case EditorPanelId::Assets:
            return m_docking.assetsFloatingWindow;
        case EditorPanelId::SceneView:
            return m_docking.sceneViewFloatingWindow;
        case EditorPanelId::Inspector:
            return m_docking.inspectorFloatingWindow;
        case EditorPanelId::Sprite:
            return m_docking.spriteFloatingWindow;
        case EditorPanelId::Material:
            return m_docking.materialFloatingWindow;
        case EditorPanelId::Collider2D:
            return m_docking.collider2DFloatingWindow;
        case EditorPanelId::LogOutput:
            return m_docking.logOutputFloatingWindow;
        default:
            return m_docking.sceneViewFloatingWindow;
        }
    }

    void EditorShell::SyncFloatingPanelTabs(HWND floatingWindow)
    {
        const int groupIndex = FindFloatingPanelGroupIndex(floatingWindow);
        if (groupIndex < 0)
        {
            return;
        }

        FloatingPanelGroup& group = m_docking.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
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

    int EditorShell::FindFloatingPanelGroupIndex(HWND floatingWindow) const
    {
        for (std::size_t index = 0; index < m_docking.floatingPanelGroups.size(); ++index)
        {
            if (m_docking.floatingPanelGroups[index].window == floatingWindow)
            {
                return static_cast<int>(index);
            }
        }

        return -1;
    }

    int EditorShell::FindFloatingPanelGroupIndex(EditorPanelId panelId) const
    {
        for (std::size_t index = 0; index < m_docking.floatingPanelGroups.size(); ++index)
        {
            const std::vector<EditorPanelId>& panels = m_docking.floatingPanelGroups[index].panels;
            if (panels.end() != std::find(panels.begin(), panels.end(), panelId))
            {
                return static_cast<int>(index);
            }
        }

        return -1;
    }

    EditorPanelId EditorShell::GetActiveFloatingPanel(HWND floatingWindow) const
    {
        const int groupIndex = FindFloatingPanelGroupIndex(floatingWindow);
        if (groupIndex < 0)
        {
            return EditorPanelId::SceneView;
        }

        const FloatingPanelGroup& group = m_docking.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
        if (group.panels.empty())
        {
            return EditorPanelId::SceneView;
        }

        const int activeIndex = (std::max)(0, (std::min)(group.activeTabIndex, static_cast<int>(group.panels.size()) - 1));
        return group.panels[static_cast<std::size_t>(activeIndex)];
    }

    void EditorShell::SyncDockTabs()
    {
        for (HWND tabControl : { m_docking.leftTopDockTab, m_docking.leftBottomDockTab, m_docking.centerDockTab, m_docking.rightDockTab })
        {
            if (nullptr != tabControl)
            {
                TabCtrl_DeleteAllItems(tabControl);
                ShowWindow(tabControl, SW_HIDE);
            }
        }
        for (HWND tabControl : m_docking.dynamicDockTabs)
        {
            if (nullptr != tabControl)
            {
                TabCtrl_DeleteAllItems(tabControl);
                ShowWindow(tabControl, SW_HIDE);
            }
        }

        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_docking.rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            DockNode& dockNode = m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)];
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

    HWND EditorShell::CreateAdditionalDockTabControl(HWND parentWindow)
    {
        if (nullptr == parentWindow)
        {
            return nullptr;
        }

        constexpr DWORD tabStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_OWNERDRAWFIXED;
        HWND tabControl = CreateChildWindow(
            parentWindow,
            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parentWindow, GWLP_HINSTANCE)),
            WC_TABCONTROLW,
            L"",
            tabStyle);
        if (nullptr == tabControl)
        {
            return nullptr;
        }

        SendMessageW(tabControl, WM_SETFONT, reinterpret_cast<WPARAM>(m_defaultFont), TRUE);
        ConfigureEditorTabControl(tabControl);
        ShowWindow(tabControl, SW_HIDE);
        m_docking.dynamicDockTabs.push_back(tabControl);
        return tabControl;
    }

    HWND EditorShell::CreateDockTabControlForLayoutKey(const std::wstring& layoutKey)
    {
        if (L"LeftTop" == layoutKey)
        {
            return m_docking.leftTopDockTab;
        }
        if (L"LeftBottom" == layoutKey)
        {
            return m_docking.leftBottomDockTab;
        }
        if (L"Center" == layoutKey)
        {
            return m_docking.centerDockTab;
        }
        if (L"Right" == layoutKey)
        {
            return m_docking.rightDockTab;
        }

        HWND tabControl = CreateAdditionalDockTabControl(m_parentWindow);
        if (L"LogOutput" == layoutKey)
        {
            m_docking.logOutputDockTab = tabControl;
        }
        return tabControl;
    }

    void EditorShell::ConfigureEditorTabControl(HWND tabControl) const
    {
        if (nullptr == tabControl)
        {
            return;
        }

        SendMessageW(tabControl, TCM_SETITEMSIZE, 0, MAKELPARAM(ScaleMetric(96), ScaleMetric(28)));
        SetWindowSubclass(
            tabControl,
            EditorShell::EditorTabControlSubclassProc,
            EditorTabControlSubclassId,
            0);
    }

    bool EditorShell::DrawEditorTabControl(const DRAWITEMSTRUCT& drawItem) const
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
        FillRoundRectWithThemeColor(drawItem.hDC, tabRect, backgroundColor, ScaleMetric(6));

        if (isActive)
        {
            RECT accentRect = tabRect;
            accentRect.right = (std::min)(accentRect.right, accentRect.left + ScaleMetric(4));
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
        textRect.left += ScaleMetric(12);
        textRect.right -= ScaleMetric(8);
        DrawTextW(
            drawItem.hDC,
            tabText,
            -1,
            &textRect,
            DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        return true;
    }


    std::wstring EditorShell::GetDockTabLayoutKey(HWND tabControl) const
    {
        if (tabControl == m_docking.leftTopDockTab)
        {
            return L"LeftTop";
        }
        if (tabControl == m_docking.leftBottomDockTab)
        {
            return L"LeftBottom";
        }
        if (tabControl == m_docking.centerDockTab)
        {
            return L"Center";
        }
        if (tabControl == m_docking.rightDockTab)
        {
            return L"Right";
        }
        if (tabControl == m_docking.logOutputDockTab)
        {
            return L"LogOutput";
        }

        return L"Dynamic";
    }

    void EditorShell::RestoreMissingPanelsToDefaultDock()
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

    void EditorShell::SyncDockAreaTabs(DockAreaId dockAreaId)
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

    HWND EditorShell::GetDockAreaTabControl(DockAreaId dockAreaId) const
    {
        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            return m_docking.leftTopDockTab;
        case DockAreaId::LeftBottom:
            return m_docking.leftBottomDockTab;
        case DockAreaId::Center:
            return m_docking.centerDockTab;
        case DockAreaId::Right:
            return m_docking.rightDockTab;
        default:
            return nullptr;
        }
    }

    HWND EditorShell::GetDockLeafTabControl(DockNodeId dockNodeId) const
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_docking.dockNodes.size())
        {
            return nullptr;
        }

        return m_docking.dockNodes[static_cast<std::size_t>(dockNodeId)].tabControl;
    }

    std::vector<EditorPanelId>& EditorShell::GetDockAreaPanels(DockAreaId dockAreaId)
    {
        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            return m_docking.leftTopDockPanels;
        case DockAreaId::LeftBottom:
            return m_docking.leftBottomDockPanels;
        case DockAreaId::Center:
            return m_docking.centerDockPanels;
        case DockAreaId::Right:
            return m_docking.rightDockPanels;
        default:
            return m_docking.centerDockPanels;
        }
    }

    const std::vector<EditorPanelId>& EditorShell::GetDockAreaPanels(DockAreaId dockAreaId) const
    {
        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            return m_docking.leftTopDockPanels;
        case DockAreaId::LeftBottom:
            return m_docking.leftBottomDockPanels;
        case DockAreaId::Center:
            return m_docking.centerDockPanels;
        case DockAreaId::Right:
            return m_docking.rightDockPanels;
        default:
            return m_docking.centerDockPanels;
        }
    }

    const wchar_t* EditorShell::GetPanelTitle(EditorPanelId panelId)
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
        case EditorPanelId::Sprite:
            return L"Sprite";
        case EditorPanelId::Material:
            return L"Material";
        case EditorPanelId::Collider2D:
            return L"Collider2D";
        case EditorPanelId::LogOutput:
            return L"LogOutput";
        default:
            return L"Panel";
        }
    }

    int EditorShell::ClampActiveTabIndex(DockAreaId dockAreaId) const
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
            activeIndex = m_docking.leftTopActiveTabIndex;
            break;
        case DockAreaId::LeftBottom:
            activeIndex = m_docking.leftBottomActiveTabIndex;
            break;
        case DockAreaId::Center:
            activeIndex = m_docking.centerActiveTabIndex;
            break;
        case DockAreaId::Right:
            activeIndex = m_docking.rightActiveTabIndex;
            break;
        default:
            activeIndex = 0;
            break;
        }

        return (std::max)(0, (std::min)(activeIndex, static_cast<int>(panels.size()) - 1));
    }

    RECT EditorShell::GetDockAreaPreviewRect(HWND parentWindow, DockAreaId dockAreaId) const
    {
        RECT clientRect{};
        GetClientRect(parentWindow, &clientRect);

        const int outerPadding = ScaleMetric(12);
        const int panelSpacing = ScaleMetric(12);
        const int clientWidth = static_cast<int>(clientRect.right - clientRect.left);
        const int clientHeight = static_cast<int>(clientRect.bottom - clientRect.top);
        const int availableColumnWidth = (std::max)(0, clientWidth - (outerPadding * 2) - (panelSpacing * 2));
        const int centerWidth = (std::max)(ScaleMetric(120), availableColumnWidth - m_docking.leftPaneWidth - m_docking.rightPaneWidth);
        const int centerX = outerPadding + m_docking.leftPaneWidth + panelSpacing;
        const int rightX = centerX + centerWidth + panelSpacing;
        const int dockHeight = (std::max)(0, clientHeight - outerPadding * 2);

        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            return RECT{
                outerPadding,
                outerPadding,
                outerPadding + m_docking.leftPaneWidth,
                outerPadding + m_docking.leftTopHeight
            };
        case DockAreaId::LeftBottom:
            return RECT{
                outerPadding,
                outerPadding + m_docking.leftTopHeight + panelSpacing,
                outerPadding + m_docking.leftPaneWidth,
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
                rightX + m_docking.rightPaneWidth,
                outerPadding + dockHeight
            };
        default:
            return RECT{};
        }
    }

    void EditorShell::UpdateDockPreviewWindow(HWND parentWindow)
    {
        if (nullptr == m_docking.dockPreviewWindow || nullptr == parentWindow)
        {
            return;
        }

        if (false == m_docking.hasDockPreview)
        {
            ShowWindow(m_docking.dockPreviewWindow, SW_HIDE);
            return;
        }

        SetWindowPos(
            m_docking.dockPreviewWindow,
            HWND_TOP,
            m_docking.dockPreviewRect.left,
            m_docking.dockPreviewRect.top,
            (std::max)(0, static_cast<int>(m_docking.dockPreviewRect.right - m_docking.dockPreviewRect.left)),
            (std::max)(0, static_cast<int>(m_docking.dockPreviewRect.bottom - m_docking.dockPreviewRect.top)),
            SWP_NOACTIVATE | SWP_SHOWWINDOW);
        InvalidateRect(m_docking.dockPreviewWindow, nullptr, TRUE);
        UpdateWindow(m_docking.dockPreviewWindow);

        for (HWND guideWindow : m_docking.dockGuideWindows)
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


    LRESULT CALLBACK EditorShell::FloatingPanelWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (WM_NCCREATE == message)
        {
            const CREATESTRUCTW* createStruct = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            const FloatingPanelCreateParams* createParams =
                nullptr != createStruct
                    ? static_cast<const FloatingPanelCreateParams*>(createStruct->lpCreateParams)
                    : nullptr;
            if (nullptr != createParams && nullptr != createParams->shell)
            {
                FloatingPanelWindowData* windowData = new FloatingPanelWindowData{
                    createParams->shell,
                    createParams->panelId
                };
                SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(windowData));
            }
        }

        FloatingPanelWindowData* windowData =
            reinterpret_cast<FloatingPanelWindowData*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (nullptr != windowData && nullptr != windowData->shell)
        {
            if (WM_SIZE == message)
            {
                windowData->shell->LayoutFloatingWindow(window);
                return 0;
            }

            if (WM_NOTIFY == message)
            {
                const std::optional<LRESULT> themeResult =
                    windowData->shell->HandleThemeMessage(message, wParam, lParam);
                if (true == themeResult.has_value())
                {
                    return *themeResult;
                }

                NMHDR* notifyHeader = reinterpret_cast<NMHDR*>(lParam);
                const int groupIndex = windowData->shell->FindFloatingPanelGroupIndex(window);
                if (nullptr != notifyHeader
                    && groupIndex >= 0
                    && TCN_SELCHANGE == notifyHeader->code
                    && notifyHeader->hwndFrom == windowData->shell->m_docking.floatingPanelGroups[static_cast<std::size_t>(groupIndex)].tabControl)
                {
                    FloatingPanelGroup& group =
                        windowData->shell->m_docking.floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
                    group.activeTabIndex = TabCtrl_GetCurSel(group.tabControl);
                    windowData->shell->LayoutFloatingWindow(window);
                    return 0;
                }
            }

            if (WM_DRAWITEM == message)
            {
                if (windowData->shell->HandleDrawItem(lParam))
                {
                    return TRUE;
                }
            }

            if (WM_MOVING == message)
            {
                windowData->shell->BeginFloatingWindowDockDrag(windowData->shell->GetActiveFloatingPanel(window));
                windowData->shell->UpdateFloatingWindowDockDrag();
            }

            if (WM_EXITSIZEMOVE == message)
            {
                windowData->shell->CompleteFloatingWindowDockDrag(windowData->shell->GetActiveFloatingPanel(window));
            }

            if (WM_CLOSE == message)
            {
                windowData->shell->HandleFloatingWindowClose(windowData->panelId, window);
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
