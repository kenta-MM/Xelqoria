#include "EditorShell.h"

#include <algorithm>
#include <Windows.h>
#include <array>
#include <CommCtrl.h>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <iterator>

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr UINT_PTR SpriteRefEditSubclassId = 1;

        /// <summary>
        /// Texture 欄の直接テキスト編集だけを抑止する。
        /// </summary>
        /// <param name="window">対象 Edit HWND。</param>
        /// <param name="message">Win32 メッセージ。</param>
        /// <param name="wParam">メッセージ WPARAM。</param>
        /// <param name="lParam">メッセージ LPARAM。</param>
        /// <param name="subclassId">サブクラス ID。</param>
        /// <param name="referenceData">未使用。</param>
        /// <returns>メッセージ処理結果。</returns>
        LRESULT CALLBACK SpriteRefEditSubclassProc(
            HWND window,
            UINT message,
            WPARAM wParam,
            LPARAM lParam,
            UINT_PTR subclassId,
            DWORD_PTR referenceData)
        {
            (void)subclassId;
            (void)referenceData;

            if (WM_NCDESTROY == message)
            {
                RemoveWindowSubclass(window, SpriteRefEditSubclassProc, SpriteRefEditSubclassId);
                return DefSubclassProc(window, message, wParam, lParam);
            }

            if (WM_CHAR == message
                || WM_UNICHAR == message
                || WM_IME_CHAR == message
                || WM_PASTE == message
                || WM_CUT == message
                || WM_CLEAR == message
                || EM_UNDO == message)
            {
                return 0;
            }

            if (WM_KEYDOWN == message)
            {
                const bool isControlDown = 0 != (GetKeyState(VK_CONTROL) & 0x8000);
                const bool isEditingShortcut = isControlDown
                    && (wParam == 'V' || wParam == 'X' || wParam == 'Z' || wParam == 'Y');
                if (VK_BACK == wParam || VK_DELETE == wParam || isEditingShortcut)
                {
                    return 0;
                }
            }

            return DefSubclassProc(window, message, wParam, lParam);
        }

        UINT GetWindowDpi(HWND window)
        {
            HMODULE user32 = GetModuleHandleW(L"user32.dll");
            if (nullptr != user32)
            {
                using GetDpiForWindowFunction = UINT(WINAPI*)(HWND);
                auto getDpiForWindow =
                    reinterpret_cast<GetDpiForWindowFunction>(GetProcAddress(user32, "GetDpiForWindow"));
                if (nullptr != getDpiForWindow)
                {
                    const UINT dpi = getDpiForWindow(window);
                    if (0 != dpi)
                    {
                        return dpi;
                    }
                }
            }

            HDC screenDc = GetDC(nullptr);
            if (nullptr == screenDc)
            {
                return 96;
            }

            const int dpi = GetDeviceCaps(screenDc, LOGPIXELSX);
            ReleaseDC(nullptr, screenDc);
            return dpi > 0 ? static_cast<UINT>(dpi) : 96u;
        }
    }

    struct EditorShell::LayoutMetrics
    {
        int outerPadding = 12;
        int panelSpacing = 12;
        int leftPaneWidth = 260;
        int rightPaneWidth = 300;
        int groupHeaderHeight = 26;
        int hierarchyHeight = 280;
        int labelHeight = 24;
        int buttonHeight = 28;
        int hierarchyButtonGap = 8;
        int centerX = 0;
        int centerWidth = 0;
        int rightX = 0;
        int rightWidth = 0;
        int hierarchyPanelHeight = 0;
        int assetsPanelY = 0;
        int assetsPanelHeight = 0;
        int scenePanelHeight = 0;
        int sideInnerWidth = 0;
        int hierarchyButtonTop = 0;
        int hierarchyButtonWidth = 0;
        int inspectorInnerX = 0;
        int inspectorInnerWidth = 0;
        int inspectorLabelWidth = 72;
        int inspectorEditWidth = 0;
        int inspectorRowHeight = 24;
        int inspectorRowSpacing = 8;
        int inspectorSectionSpacing = 12;
        int transformSectionTop = 0;
        int spriteSectionTop = 0;
        int spriteRefTop = 0;
        int sceneInnerWidth = 0;
        int projectSceneListTop = 0;
        int projectSceneListHeight = 0;
        int sceneHostHeight = 0;
    };

    bool EditorShell::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        INITCOMMONCONTROLSEX commonControls{};
        commonControls.dwSize = sizeof(commonControls);
        commonControls.dwICC = ICC_LISTVIEW_CLASSES;
        if (FALSE == InitCommonControlsEx(&commonControls))
        {
            return false;
        }

        (void)RefreshDpiResources(parentWindow);

        return InitializeHierarchyPanel(parentWindow, hInstance)
            && InitializeAssetsPanel(parentWindow, hInstance)
            && InitializeInspectorPanel(parentWindow, hInstance)
            && InitializeSceneViewPanel(parentWindow, hInstance);
    }

    EditorShell::~EditorShell()
    {
        if (m_ownsDefaultFont && nullptr != m_defaultFont)
        {
            HFONT stockFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            const std::array<HWND, 37> controls{
                m_hierarchyPanel,
                m_assetsPanel,
                m_inspectorPanel,
                m_sceneViewPanel,
                m_sceneViewPlanLabel,
                m_projectSummaryLabel,
                m_projectSceneListBox,
                m_projectSceneDetailLabel,
                m_sceneViewHost,
                m_sceneViewSizeLabel,
                m_assetsListView,
                m_assetsSummaryLabel,
                m_hierarchySummaryLabel,
                m_hierarchyListBox,
                m_hierarchyNameEdit,
                m_hierarchyCreateButton,
                m_hierarchyDuplicateButton,
                m_hierarchyDeleteButton,
                m_inspectorSummaryLabel,
                m_transformSectionLabel,
                m_transformLabels[0],
                m_transformLabels[1],
                m_transformLabels[2],
                m_transformEditControls[0],
                m_transformEditControls[1],
                m_transformEditControls[2],
                m_transformEditControls[3],
                m_transformEditControls[4],
                m_transformEditControls[5],
                m_transformEditControls[6],
                m_transformEditControls[7],
                m_transformEditControls[8],
                m_spriteComponentSectionLabel,
                m_spriteRefLabel,
                m_spriteRefDropHighlight,
                m_spriteRefEdit,
                m_spriteComponentActionButton
            };
            for (HWND control : controls)
            {
                if (nullptr != control)
                {
                    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(stockFont), FALSE);
                }
            }

            DeleteObject(m_defaultFont);
            m_defaultFont = nullptr;
            m_ownsDefaultFont = false;
        }
    }

    bool EditorShell::UpdateLayout(HWND parentWindow)
    {
        RECT clientRect{};
        GetClientRect(parentWindow, &clientRect);

        // 親ウィンドウのleftとtopは必ず0なのでrightとbottomがそのまま幅と高さになる
        const int clientWidth = clientRect.right - clientRect.left;
        const int clientHeight = clientRect.bottom - clientRect.top;
        if (clientWidth <= 0 || clientHeight <= 0)
        {
            return false;
        }

        const bool dpiChanged = RefreshDpiResources(parentWindow);
        if (m_layoutInitialized
            && false == dpiChanged
            && m_lastLayoutClientWidth == clientWidth
            && m_lastLayoutClientHeight == clientHeight
            && m_lastLayoutDpi == m_currentDpi)
        {
            return UpdateSceneViewHostSize();
        }

        LayoutMetrics metrics{};
        metrics.outerPadding = ScaleMetric(12);
        metrics.panelSpacing = ScaleMetric(12);
        metrics.leftPaneWidth = ScaleMetric(260);
        metrics.rightPaneWidth = ScaleMetric(300);
        metrics.groupHeaderHeight = ScaleMetric(26);
        metrics.hierarchyHeight = ScaleMetric(280);
        metrics.labelHeight = ScaleMetric(24);
        metrics.buttonHeight = ScaleMetric(28);
        metrics.hierarchyButtonGap = ScaleMetric(8);
        metrics.inspectorLabelWidth = ScaleMetric(72);
        metrics.inspectorRowHeight = ScaleMetric(24);
        metrics.inspectorRowSpacing = ScaleMetric(8);
        metrics.inspectorSectionSpacing = ScaleMetric(12);

        const int availableColumnWidth =
            (std::max)(0, clientWidth - (metrics.outerPadding * 2) - (metrics.panelSpacing * 2));
        const int preferredLeftPaneWidth = metrics.leftPaneWidth;
        const int preferredRightPaneWidth = metrics.rightPaneWidth;
        const int minimumCenterWidth = ScaleMetric(120);
        if (availableColumnWidth >= preferredLeftPaneWidth + preferredRightPaneWidth + minimumCenterWidth)
        {
            metrics.leftPaneWidth = preferredLeftPaneWidth;
            metrics.rightPaneWidth = preferredRightPaneWidth;
            metrics.centerWidth = availableColumnWidth - metrics.leftPaneWidth - metrics.rightPaneWidth;
        }
        else
        {
            const int sidePaneWidth = availableColumnWidth / 4;
            metrics.leftPaneWidth = sidePaneWidth;
            metrics.rightPaneWidth = sidePaneWidth;
            metrics.centerWidth = availableColumnWidth - metrics.leftPaneWidth - metrics.rightPaneWidth;
        }

        metrics.centerX = metrics.outerPadding + metrics.leftPaneWidth + metrics.panelSpacing;
        metrics.rightX = metrics.centerX + metrics.centerWidth + metrics.panelSpacing;
        metrics.rightWidth = metrics.rightPaneWidth;

        const int availableSidePanelHeight =
            (std::max)(0, clientHeight - (metrics.outerPadding * 2) - metrics.panelSpacing);
        const int minimumHierarchyPanelHeight = ScaleMetric(120);
        const int minimumAssetsPanelHeight = ScaleMetric(100);
        if (availableSidePanelHeight >= minimumHierarchyPanelHeight + minimumAssetsPanelHeight)
        {
            metrics.hierarchyPanelHeight =
                (std::min)(metrics.hierarchyHeight, availableSidePanelHeight - minimumAssetsPanelHeight);
            metrics.hierarchyPanelHeight = (std::max)(minimumHierarchyPanelHeight, metrics.hierarchyPanelHeight);
            metrics.assetsPanelHeight = availableSidePanelHeight - metrics.hierarchyPanelHeight;
        }
        else
        {
            metrics.hierarchyPanelHeight = availableSidePanelHeight / 2;
            metrics.assetsPanelHeight = availableSidePanelHeight - metrics.hierarchyPanelHeight;
        }

        metrics.assetsPanelY = metrics.outerPadding + metrics.hierarchyPanelHeight + metrics.panelSpacing;
        metrics.scenePanelHeight = (std::max)(0, clientHeight - (metrics.outerPadding * 2));
        metrics.sideInnerWidth = (std::max)(0, metrics.leftPaneWidth - (metrics.outerPadding * 2));
        metrics.hierarchyButtonTop = metrics.outerPadding + metrics.groupHeaderHeight + metrics.labelHeight + ScaleMetric(6);
        if (metrics.sideInnerWidth < metrics.hierarchyButtonGap * 2)
        {
            metrics.hierarchyButtonGap = 0;
        }
        metrics.hierarchyButtonWidth =
            (std::max)(0, (metrics.sideInnerWidth - (metrics.hierarchyButtonGap * 2)) / 3);
        metrics.inspectorInnerX = metrics.rightX + metrics.outerPadding;
        metrics.inspectorInnerWidth = (std::max)(0, metrics.rightWidth - (metrics.outerPadding * 2));
        metrics.inspectorEditWidth =
            (std::max)(0, (metrics.inspectorInnerWidth - metrics.inspectorLabelWidth - ScaleMetric(24)) / 3);
        metrics.transformSectionTop = metrics.outerPadding + metrics.groupHeaderHeight + metrics.labelHeight + ScaleMetric(8);
        metrics.spriteSectionTop =
            metrics.transformSectionTop + metrics.labelHeight + ScaleMetric(4) + 3 * (metrics.inspectorRowHeight + metrics.inspectorRowSpacing) + metrics.inspectorSectionSpacing;
        metrics.spriteRefTop = metrics.spriteSectionTop + metrics.labelHeight + ScaleMetric(4);
        metrics.sceneInnerWidth = (std::max)(0, metrics.centerWidth - (metrics.outerPadding * 2));
        metrics.projectSceneListTop = metrics.outerPadding + metrics.groupHeaderHeight + (metrics.labelHeight * 2) + ScaleMetric(8);
        metrics.projectSceneListHeight = ScaleMetric(96);
        metrics.sceneHostHeight = (std::max)(
            0,
            metrics.scenePanelHeight
                - metrics.groupHeaderHeight
                - (metrics.labelHeight * 4)
                - metrics.projectSceneListHeight
                - (metrics.outerPadding * 3)
                - metrics.panelSpacing);

        LayoutHierarchyPanel(metrics);
        LayoutAssetsPanel(metrics);
        LayoutInspectorPanel(metrics);
        LayoutSceneViewPanel(metrics);
        SendGroupBoxesToBack();
        RedrawLayout(parentWindow);
        m_layoutInitialized = true;
        m_lastLayoutClientWidth = clientWidth;
        m_lastLayoutClientHeight = clientHeight;
        m_lastLayoutDpi = m_currentDpi;
        return UpdateSceneViewHostSize();
    }

    bool EditorShell::InitializeHierarchyPanel(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE | BS_GROUPBOX;
        m_hierarchyPanel = CreateChildWindow(parentWindow, hInstance, L"Button", L"Hierarchy", panelStyle);
        if (nullptr == m_hierarchyPanel)
        {
            return false;
        }

        m_hierarchySummaryLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Entities: pending",
            WS_CHILD | WS_VISIBLE);
        if (nullptr == m_hierarchySummaryLabel)
        {
            return false;
        }

        m_hierarchyListBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER);
        if (nullptr == m_hierarchyListBox)
        {
            return false;
        }

        m_hierarchyNameEdit = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Edit",
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
        if (nullptr == m_hierarchyNameEdit)
        {
            return false;
        }

        m_hierarchyCreateButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"New",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_hierarchyCreateButton)
        {
            return false;
        }

        m_hierarchyDuplicateButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Duplicate",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_hierarchyDuplicateButton)
        {
            return false;
        }

        m_hierarchyDeleteButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Delete",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        return nullptr != m_hierarchyDeleteButton;
    }

    bool EditorShell::InitializeAssetsPanel(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE | BS_GROUPBOX;
        m_assetsPanel = CreateChildWindow(parentWindow, hInstance, L"Button", L"Assets", panelStyle);
        if (nullptr == m_assetsPanel)
        {
            return false;
        }

        m_assetsSummaryLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Assets: pending",
            WS_CHILD | WS_VISIBLE);
        if (nullptr == m_assetsSummaryLabel)
        {
            return false;
        }

        m_assetsListView = CreateChildWindow(
            parentWindow,
            hInstance,
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS);
        if (nullptr != m_assetsListView)
        {
            ListView_SetExtendedListViewStyle(
                m_assetsListView,
                LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        }

        return nullptr != m_assetsListView;
    }

    bool EditorShell::InitializeInspectorPanel(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE | BS_GROUPBOX;
        m_inspectorPanel = CreateChildWindow(parentWindow, hInstance, L"Button", L"Inspector", panelStyle);
        if (nullptr == m_inspectorPanel)
        {
            return false;
        }

        m_inspectorSummaryLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Inspector: pending",
            WS_CHILD | WS_VISIBLE);
        if (nullptr == m_inspectorSummaryLabel)
        {
            return false;
        }

        m_transformSectionLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Transform",
            WS_CHILD | WS_VISIBLE);
        if (nullptr == m_transformSectionLabel)
        {
            return false;
        }

        m_transformLabels[0] = CreateChildWindow(parentWindow, hInstance, L"Static", L"Position", WS_CHILD | WS_VISIBLE);
        m_transformLabels[1] = CreateChildWindow(parentWindow, hInstance, L"Static", L"Rotation", WS_CHILD | WS_VISIBLE);
        m_transformLabels[2] = CreateChildWindow(parentWindow, hInstance, L"Static", L"Scale", WS_CHILD | WS_VISIBLE);
        if (nullptr == m_transformLabels[0] || nullptr == m_transformLabels[1] || nullptr == m_transformLabels[2])
        {
            return false;
        }

        for (HWND& handle : m_transformEditControls)
        {
            handle = CreateChildWindow(
                parentWindow,
                hInstance,
                L"Edit",
                L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
            if (nullptr == handle)
            {
                return false;
            }
        }

        m_spriteComponentSectionLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"SpriteComponent",
            WS_CHILD | WS_VISIBLE);
        if (nullptr == m_spriteComponentSectionLabel)
        {
            return false;
        }

        m_spriteRefLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Texture", WS_CHILD | WS_VISIBLE);
        if (nullptr == m_spriteRefLabel)
        {
            return false;
        }

        m_spriteRefDropHighlight = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"",
            WS_CHILD | SS_BLACKFRAME);
        if (nullptr == m_spriteRefDropHighlight)
        {
            return false;
        }

        m_spriteRefEdit = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Edit",
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
        if (nullptr == m_spriteRefEdit)
        {
            return false;
        }
        if (FALSE == SetWindowSubclass(m_spriteRefEdit, SpriteRefEditSubclassProc, SpriteRefEditSubclassId, 0))
        {
            return false;
        }

        m_spriteComponentActionButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Add SpriteComponent",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        return nullptr != m_spriteComponentActionButton;
    }

    bool EditorShell::InitializeSceneViewPanel(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE | BS_GROUPBOX;
        m_sceneViewPanel = CreateChildWindow(parentWindow, hInstance, L"Button", L"SceneView", panelStyle);
        if (nullptr == m_sceneViewPanel)
        {
            return false;
        }

        m_sceneViewPlanLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Runtime 描画は SceneView 専用 child HWND に埋め込みます。",
            WS_CHILD | WS_VISIBLE);
        if (nullptr == m_sceneViewPlanLabel)
        {
            return false;
        }

        m_projectSummaryLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Project: pending",
            WS_CHILD | WS_VISIBLE);
        if (nullptr == m_projectSummaryLabel)
        {
            return false;
        }

        m_projectSceneListBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER);
        if (nullptr == m_projectSceneListBox)
        {
            return false;
        }

        m_projectSceneDetailLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Scene: pending",
            WS_CHILD | WS_VISIBLE);
        if (nullptr == m_projectSceneDetailLabel)
        {
            return false;
        }

        m_sceneViewHost = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"",
            WS_CHILD | WS_VISIBLE | SS_NOTIFY,
            WS_EX_CLIENTEDGE);
        if (nullptr == m_sceneViewHost)
        {
            return false;
        }

        m_sceneViewSizeLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"SceneView size: pending",
            WS_CHILD | WS_VISIBLE);
        return nullptr != m_sceneViewSizeLabel;
    }

    void EditorShell::LayoutHierarchyPanel(const LayoutMetrics& metrics)
    {
        MoveChildWindowNoRedraw(m_hierarchyPanel, metrics.outerPadding, metrics.outerPadding, metrics.leftPaneWidth, metrics.hierarchyPanelHeight);
        MoveChildWindowNoRedraw(
            m_hierarchySummaryLabel,
            metrics.outerPadding + metrics.outerPadding,
            metrics.outerPadding + metrics.groupHeaderHeight,
            metrics.sideInnerWidth,
            metrics.labelHeight);
        MoveChildWindowNoRedraw(
            m_hierarchyCreateButton,
            metrics.outerPadding + metrics.outerPadding,
            metrics.hierarchyButtonTop,
            metrics.hierarchyButtonWidth,
            metrics.buttonHeight);
        MoveChildWindowNoRedraw(
            m_hierarchyDuplicateButton,
            metrics.outerPadding + metrics.outerPadding + metrics.hierarchyButtonWidth + metrics.hierarchyButtonGap,
            metrics.hierarchyButtonTop,
            metrics.hierarchyButtonWidth,
            metrics.buttonHeight);
        MoveChildWindowNoRedraw(
            m_hierarchyDeleteButton,
            metrics.outerPadding + metrics.outerPadding + (metrics.hierarchyButtonWidth + metrics.hierarchyButtonGap) * 2,
            metrics.hierarchyButtonTop,
            (std::max)(0, metrics.sideInnerWidth - (metrics.hierarchyButtonWidth + metrics.hierarchyButtonGap) * 2),
            metrics.buttonHeight);

        const int hierarchyListTop = metrics.hierarchyButtonTop + metrics.buttonHeight + ScaleMetric(8);
        const int hierarchyNameTop =
            metrics.outerPadding + metrics.hierarchyPanelHeight - metrics.outerPadding - metrics.labelHeight;
        MoveChildWindowNoRedraw(
            m_hierarchyListBox,
            metrics.outerPadding + metrics.outerPadding,
            hierarchyListTop,
            metrics.sideInnerWidth,
            (std::max)(0, hierarchyNameTop - hierarchyListTop - ScaleMetric(8)));
        MoveChildWindowNoRedraw(
            m_hierarchyNameEdit,
            metrics.outerPadding + metrics.outerPadding,
            hierarchyNameTop,
            metrics.sideInnerWidth,
            metrics.labelHeight);
    }

    void EditorShell::LayoutAssetsPanel(const LayoutMetrics& metrics)
    {
        MoveChildWindowNoRedraw(m_assetsPanel, metrics.outerPadding, metrics.assetsPanelY, metrics.leftPaneWidth, metrics.assetsPanelHeight);
        MoveChildWindowNoRedraw(
            m_assetsSummaryLabel,
            metrics.outerPadding + metrics.outerPadding,
            metrics.assetsPanelY + metrics.groupHeaderHeight,
            metrics.sideInnerWidth,
            metrics.labelHeight);
        MoveChildWindowNoRedraw(
            m_assetsListView,
            metrics.outerPadding + metrics.outerPadding,
            metrics.assetsPanelY + metrics.groupHeaderHeight + metrics.labelHeight + ScaleMetric(6),
            metrics.sideInnerWidth,
            (std::max)(0, metrics.assetsPanelHeight - metrics.groupHeaderHeight - metrics.labelHeight - metrics.outerPadding - ScaleMetric(12)));
    }

    void EditorShell::LayoutInspectorPanel(const LayoutMetrics& metrics)
    {
        MoveChildWindowNoRedraw(m_inspectorPanel, metrics.rightX, metrics.outerPadding, metrics.rightWidth, metrics.scenePanelHeight);
        MoveChildWindowNoRedraw(
            m_inspectorSummaryLabel,
            metrics.inspectorInnerX,
            metrics.outerPadding + metrics.groupHeaderHeight,
            metrics.inspectorInnerWidth,
            metrics.labelHeight);
        MoveChildWindowNoRedraw(
            m_transformSectionLabel,
            metrics.inspectorInnerX,
            metrics.transformSectionTop,
            metrics.inspectorInnerWidth,
            metrics.labelHeight);

        for (int row = 0; row < 3; ++row)
        {
            const int rowTop =
                metrics.transformSectionTop + metrics.labelHeight + ScaleMetric(4) + row * (metrics.inspectorRowHeight + metrics.inspectorRowSpacing);
            MoveChildWindowNoRedraw(
                m_transformLabels[static_cast<std::size_t>(row)],
                metrics.inspectorInnerX,
                rowTop + ScaleMetric(4),
                metrics.inspectorLabelWidth,
                metrics.inspectorRowHeight);

            for (int column = 0; column < 3; ++column)
            {
                const int editIndex = row * 3 + column;
                const int editLeft = metrics.inspectorInnerX + metrics.inspectorLabelWidth + column * (metrics.inspectorEditWidth + ScaleMetric(8));
                MoveChildWindowNoRedraw(
                    m_transformEditControls[static_cast<std::size_t>(editIndex)],
                    editLeft,
                    rowTop,
                    metrics.inspectorEditWidth,
                    metrics.inspectorRowHeight);
            }
        }

        MoveChildWindowNoRedraw(
            m_spriteComponentSectionLabel,
            metrics.inspectorInnerX,
            metrics.spriteSectionTop,
            metrics.inspectorInnerWidth,
            metrics.labelHeight);
        MoveChildWindowNoRedraw(
            m_spriteRefLabel,
            metrics.inspectorInnerX,
            metrics.spriteRefTop + ScaleMetric(4),
            metrics.inspectorLabelWidth,
            metrics.inspectorRowHeight);
        MoveChildWindowNoRedraw(
            m_spriteRefDropHighlight,
            metrics.inspectorInnerX + metrics.inspectorLabelWidth - ScaleMetric(2),
            metrics.spriteRefTop - ScaleMetric(2),
            (std::max)(0, metrics.inspectorInnerWidth - metrics.inspectorLabelWidth + ScaleMetric(4)),
            metrics.inspectorRowHeight + ScaleMetric(4));
        MoveChildWindowNoRedraw(
            m_spriteRefEdit,
            metrics.inspectorInnerX + metrics.inspectorLabelWidth,
            metrics.spriteRefTop,
            (std::max)(0, metrics.inspectorInnerWidth - metrics.inspectorLabelWidth),
            metrics.inspectorRowHeight);
        MoveChildWindowNoRedraw(
            m_spriteComponentActionButton,
            metrics.inspectorInnerX,
            metrics.spriteRefTop + metrics.inspectorRowHeight + metrics.inspectorRowSpacing,
            metrics.inspectorInnerWidth,
            metrics.inspectorRowHeight + ScaleMetric(4));
    }

    void EditorShell::LayoutSceneViewPanel(const LayoutMetrics& metrics)
    {
        MoveChildWindowNoRedraw(m_sceneViewPanel, metrics.centerX, metrics.outerPadding, metrics.centerWidth, metrics.scenePanelHeight);
        MoveChildWindowNoRedraw(
            m_sceneViewPlanLabel,
            metrics.centerX + metrics.outerPadding,
            metrics.outerPadding + metrics.groupHeaderHeight,
            metrics.sceneInnerWidth,
            metrics.labelHeight);
        MoveChildWindowNoRedraw(
            m_projectSummaryLabel,
            metrics.centerX + metrics.outerPadding,
            metrics.outerPadding + metrics.groupHeaderHeight + metrics.labelHeight + ScaleMetric(4),
            metrics.sceneInnerWidth,
            metrics.labelHeight);
        MoveChildWindowNoRedraw(
            m_projectSceneListBox,
            metrics.centerX + metrics.outerPadding,
            metrics.projectSceneListTop,
            metrics.sceneInnerWidth,
            metrics.projectSceneListHeight);
        MoveChildWindowNoRedraw(
            m_projectSceneDetailLabel,
            metrics.centerX + metrics.outerPadding,
            metrics.projectSceneListTop + metrics.projectSceneListHeight + ScaleMetric(6),
            metrics.sceneInnerWidth,
            metrics.labelHeight);
        MoveChildWindowNoRedraw(
            m_sceneViewHost,
            metrics.centerX + metrics.outerPadding,
            metrics.projectSceneListTop + metrics.projectSceneListHeight + metrics.labelHeight + metrics.panelSpacing,
            metrics.sceneInnerWidth,
            metrics.sceneHostHeight);
        MoveChildWindowNoRedraw(
            m_sceneViewSizeLabel,
            metrics.centerX + metrics.outerPadding,
            metrics.projectSceneListTop + metrics.projectSceneListHeight + metrics.labelHeight + metrics.panelSpacing + metrics.sceneHostHeight + ScaleMetric(6),
            metrics.sceneInnerWidth,
            metrics.labelHeight);
    }

    void EditorShell::SendGroupBoxesToBack()
    {
        const std::array<HWND, 4> groupBoxes{
            m_hierarchyPanel,
            m_assetsPanel,
            m_inspectorPanel,
            m_sceneViewPanel
        };

        for (HWND groupBox : groupBoxes)
        {
            if (nullptr != groupBox)
            {
                SetWindowPos(
                    groupBox,
                    HWND_BOTTOM,
                    0,
                    0,
                    0,
                    0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }
    }

    void EditorShell::MoveChildWindowNoRedraw(HWND window, int x, int y, int width, int height) const
    {
        if (nullptr == window)
        {
            return;
        }

        SetWindowPos(
            window,
            nullptr,
            x,
            y,
            (std::max)(0, width),
            (std::max)(0, height),
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    }

    void EditorShell::RedrawLayout(HWND parentWindow) const
    {
        if (nullptr != parentWindow)
        {
            RedrawWindow(
                parentWindow,
                nullptr,
                nullptr,
                RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME);
        }

        const std::array<HWND, 37> controls{
            m_hierarchyPanel,
            m_assetsPanel,
            m_inspectorPanel,
            m_sceneViewPanel,
            m_sceneViewPlanLabel,
            m_projectSummaryLabel,
            m_projectSceneListBox,
            m_projectSceneDetailLabel,
            m_sceneViewHost,
            m_sceneViewSizeLabel,
            m_assetsListView,
            m_assetsSummaryLabel,
            m_hierarchySummaryLabel,
            m_hierarchyListBox,
            m_hierarchyNameEdit,
            m_hierarchyCreateButton,
            m_hierarchyDuplicateButton,
            m_hierarchyDeleteButton,
            m_inspectorSummaryLabel,
            m_transformSectionLabel,
            m_transformLabels[0],
            m_transformLabels[1],
            m_transformLabels[2],
            m_transformEditControls[0],
            m_transformEditControls[1],
            m_transformEditControls[2],
            m_transformEditControls[3],
            m_transformEditControls[4],
            m_transformEditControls[5],
            m_transformEditControls[6],
            m_transformEditControls[7],
            m_transformEditControls[8],
            m_spriteComponentSectionLabel,
            m_spriteRefLabel,
            m_spriteRefDropHighlight,
            m_spriteRefEdit,
            m_spriteComponentActionButton
        };

        for (HWND control : controls)
        {
            if (nullptr != control)
            {
                RedrawWindow(
                    control,
                    nullptr,
                    nullptr,
                    RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW | RDW_UPDATENOW | RDW_FRAME);
            }
        }
    }

    bool EditorShell::UpdateSceneViewHostSize()
    {
        RECT sceneHostRect{};
        GetClientRect(m_sceneViewHost, &sceneHostRect);
        const auto newWidth = static_cast<std::uint32_t>(sceneHostRect.right - sceneHostRect.left);
        const auto newHeight = static_cast<std::uint32_t>(sceneHostRect.bottom - sceneHostRect.top);
        const bool sizeChanged = newWidth != m_sceneViewWidth || newHeight != m_sceneViewHeight;

        if (sizeChanged)
        {
            m_sceneViewWidth = newWidth;
            m_sceneViewHeight = newHeight;

            wchar_t sizeText[128]{};
            std::swprintf(
                sizeText,
                std::size(sizeText),
                L"SceneView size: %u x %u / host: child HWND",
                m_sceneViewWidth,
                m_sceneViewHeight);
            SetWindowTextW(m_sceneViewSizeLabel, sizeText);
        }

        return sizeChanged;
    }

    bool EditorShell::RefreshDpiResources(HWND parentWindow)
    {
        UINT dpi = GetWindowDpi(parentWindow);
        if (0 == dpi)
        {
            dpi = 96;
        }

        if (nullptr != m_defaultFont && m_currentDpi == dpi)
        {
            return false;
        }

        HFONT previousFont = m_defaultFont;
        const bool ownedPreviousFont = m_ownsDefaultFont;
        m_currentDpi = dpi;

        LOGFONTW fontDesc{};
        fontDesc.lfHeight = -MulDiv(9, static_cast<int>(m_currentDpi), 72);
        fontDesc.lfWeight = FW_NORMAL;
        wcscpy_s(fontDesc.lfFaceName, L"Segoe UI");
        m_defaultFont = CreateFontIndirectW(&fontDesc);
        m_ownsDefaultFont = nullptr != m_defaultFont;
        if (nullptr == m_defaultFont)
        {
            m_defaultFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            m_ownsDefaultFont = false;
        }

        const std::array<HWND, 37> controls{
            m_hierarchyPanel,
            m_assetsPanel,
            m_inspectorPanel,
            m_sceneViewPanel,
            m_sceneViewPlanLabel,
            m_projectSummaryLabel,
            m_projectSceneListBox,
            m_projectSceneDetailLabel,
            m_sceneViewHost,
            m_sceneViewSizeLabel,
            m_assetsListView,
            m_assetsSummaryLabel,
            m_hierarchySummaryLabel,
            m_hierarchyListBox,
            m_hierarchyNameEdit,
            m_hierarchyCreateButton,
            m_hierarchyDuplicateButton,
            m_hierarchyDeleteButton,
            m_inspectorSummaryLabel,
            m_transformSectionLabel,
            m_transformLabels[0],
            m_transformLabels[1],
            m_transformLabels[2],
            m_transformEditControls[0],
            m_transformEditControls[1],
            m_transformEditControls[2],
            m_transformEditControls[3],
            m_transformEditControls[4],
            m_transformEditControls[5],
            m_transformEditControls[6],
            m_transformEditControls[7],
            m_transformEditControls[8],
            m_spriteComponentSectionLabel,
            m_spriteRefLabel,
            m_spriteRefDropHighlight,
            m_spriteRefEdit,
            m_spriteComponentActionButton
        };

        for (HWND control : controls)
        {
            if (nullptr != control)
            {
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(m_defaultFont), TRUE);
            }
        }

        if (ownedPreviousFont && nullptr != previousFont)
        {
            DeleteObject(previousFont);
        }

        return true;
    }

    int EditorShell::ScaleMetric(int value) const
    {
        return MulDiv(value, static_cast<int>(m_currentDpi), 96);
    }

    HWND EditorShell::GetHierarchyListBox() const
    {
        return m_hierarchyListBox;
    }

    HWND EditorShell::GetHierarchySummaryLabel() const
    {
        return m_hierarchySummaryLabel;
    }

    HWND EditorShell::GetHierarchyNameEdit() const
    {
        return m_hierarchyNameEdit;
    }

    HWND EditorShell::GetHierarchyCreateButton() const
    {
        return m_hierarchyCreateButton;
    }

    HWND EditorShell::GetHierarchyDuplicateButton() const
    {
        return m_hierarchyDuplicateButton;
    }

    HWND EditorShell::GetHierarchyDeleteButton() const
    {
        return m_hierarchyDeleteButton;
    }

    HWND EditorShell::GetAssetsListView() const
    {
        return m_assetsListView;
    }

    HWND EditorShell::GetAssetsSummaryLabel() const
    {
        return m_assetsSummaryLabel;
    }

    HWND EditorShell::GetInspectorSummaryLabel() const
    {
        return m_inspectorSummaryLabel;
    }

    HWND EditorShell::GetTransformSectionLabel() const
    {
        return m_transformSectionLabel;
    }

    const std::array<HWND, 3>& EditorShell::GetTransformLabels() const
    {
        return m_transformLabels;
    }

    const std::array<HWND, 9>& EditorShell::GetTransformEditControls() const
    {
        return m_transformEditControls;
    }

    HWND EditorShell::GetSpriteRefLabel() const
    {
        return m_spriteRefLabel;
    }

    HWND EditorShell::GetSpriteComponentSectionLabel() const
    {
        return m_spriteComponentSectionLabel;
    }

    HWND EditorShell::GetSpriteRefEdit() const
    {
        return m_spriteRefEdit;
    }

    HWND EditorShell::GetSpriteRefDropHighlight() const
    {
        return m_spriteRefDropHighlight;
    }

    HWND EditorShell::GetSpriteComponentActionButton() const
    {
        return m_spriteComponentActionButton;
    }

    HWND EditorShell::GetSceneViewSizeLabel() const
    {
        return m_sceneViewSizeLabel;
    }

    HWND EditorShell::GetSceneViewPlanLabel() const
    {
        return m_sceneViewPlanLabel;
    }

    HWND EditorShell::GetProjectSummaryLabel() const
    {
        return m_projectSummaryLabel;
    }

    HWND EditorShell::GetProjectSceneListBox() const
    {
        return m_projectSceneListBox;
    }

    HWND EditorShell::GetProjectSceneDetailLabel() const
    {
        return m_projectSceneDetailLabel;
    }

    HWND EditorShell::GetSceneViewHost() const
    {
        return m_sceneViewHost;
    }

    std::uint32_t EditorShell::GetSceneViewWidth() const
    {
        return m_sceneViewWidth;
    }

    std::uint32_t EditorShell::GetSceneViewHeight() const
    {
        return m_sceneViewHeight;
    }

    HWND EditorShell::CreateChildWindow(
        HWND parentWindow,
        HINSTANCE hInstance,
        const wchar_t* className,
        const wchar_t* text,
        DWORD style,
        DWORD exStyle) const
    {
        HWND handle = CreateWindowExW(
            exStyle,
            className,
            text,
            style | WS_CLIPSIBLINGS,
            0,
            0,
            0,
            0,
            parentWindow,
            nullptr,
            hInstance,
            nullptr);

        if (handle != nullptr && m_defaultFont != nullptr)
        {
            SendMessageW(handle, WM_SETFONT, reinterpret_cast<WPARAM>(m_defaultFont), TRUE);
        }

        return handle;
    }
}
