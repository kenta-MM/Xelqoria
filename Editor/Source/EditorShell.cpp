#include "EditorShell.h"

#include <algorithm>
#include <Windows.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iterator>

namespace Xelqoria::Editor
{
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
        int sceneHostHeight = 0;
    };

    bool EditorShell::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        m_defaultFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

        return InitializeHierarchyPanel(parentWindow, hInstance)
            && InitializeAssetsPanel(parentWindow, hInstance)
            && InitializeInspectorPanel(parentWindow, hInstance)
            && InitializeSceneViewPanel(parentWindow, hInstance);
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

        LayoutMetrics metrics{};
        metrics.centerX = metrics.outerPadding + metrics.leftPaneWidth + metrics.panelSpacing;
        metrics.centerWidth =
            (std::max)(320, clientWidth - metrics.leftPaneWidth - metrics.rightPaneWidth - (metrics.outerPadding * 2) - (metrics.panelSpacing * 2));
        metrics.rightX = metrics.centerX + metrics.centerWidth + metrics.panelSpacing;
        metrics.rightWidth = (std::max)(220, clientWidth - metrics.rightX - metrics.outerPadding);
        metrics.hierarchyPanelHeight =
            (std::max)(180, (std::min)(metrics.hierarchyHeight, clientHeight - (metrics.outerPadding * 2) - metrics.panelSpacing - 160));
        metrics.assetsPanelY = metrics.outerPadding + metrics.hierarchyPanelHeight + metrics.panelSpacing;
        metrics.assetsPanelHeight = (std::max)(140, clientHeight - metrics.assetsPanelY - metrics.outerPadding);
        metrics.scenePanelHeight = clientHeight - (metrics.outerPadding * 2);
        metrics.sideInnerWidth = metrics.leftPaneWidth - (metrics.outerPadding * 2);
        metrics.hierarchyButtonTop = metrics.outerPadding + metrics.groupHeaderHeight + metrics.labelHeight + 6;
        metrics.hierarchyButtonWidth = (std::max)(70, (metrics.sideInnerWidth - 16) / 3);
        metrics.inspectorInnerX = metrics.rightX + metrics.outerPadding;
        metrics.inspectorInnerWidth = metrics.rightWidth - (metrics.outerPadding * 2);
        metrics.inspectorEditWidth = (std::max)(60, (metrics.inspectorInnerWidth - metrics.inspectorLabelWidth - 24) / 3);
        metrics.transformSectionTop = metrics.outerPadding + metrics.groupHeaderHeight + metrics.labelHeight + 8;
        metrics.spriteSectionTop =
            metrics.transformSectionTop + metrics.labelHeight + 4 + 3 * (metrics.inspectorRowHeight + metrics.inspectorRowSpacing) + metrics.inspectorSectionSpacing;
        metrics.spriteRefTop = metrics.spriteSectionTop + metrics.labelHeight + 4;
        metrics.sceneInnerWidth = (std::max)(120, metrics.centerWidth - (metrics.outerPadding * 2));
        metrics.sceneHostHeight = (std::max)(160, metrics.scenePanelHeight - metrics.groupHeaderHeight - metrics.labelHeight - (metrics.outerPadding * 3));

        LayoutHierarchyPanel(metrics);
        LayoutAssetsPanel(metrics);
        LayoutInspectorPanel(metrics);
        LayoutSceneViewPanel(metrics);
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
            L"Sprite assets: pending",
            WS_CHILD | WS_VISIBLE);
        if (nullptr == m_assetsSummaryLabel)
        {
            return false;
        }

        m_assetsListBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER);
        return nullptr != m_assetsListBox;
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

        m_spriteRefLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"SpriteRef", WS_CHILD | WS_VISIBLE);
        if (nullptr == m_spriteRefLabel)
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
        MoveWindow(m_hierarchyPanel, metrics.outerPadding, metrics.outerPadding, metrics.leftPaneWidth, metrics.hierarchyPanelHeight, TRUE);
        MoveWindow(
            m_hierarchySummaryLabel,
            metrics.outerPadding + metrics.outerPadding,
            metrics.outerPadding + metrics.groupHeaderHeight,
            metrics.sideInnerWidth,
            metrics.labelHeight,
            TRUE);
        MoveWindow(
            m_hierarchyCreateButton,
            metrics.outerPadding + metrics.outerPadding,
            metrics.hierarchyButtonTop,
            metrics.hierarchyButtonWidth,
            metrics.buttonHeight,
            TRUE);
        MoveWindow(
            m_hierarchyDuplicateButton,
            metrics.outerPadding + metrics.outerPadding + metrics.hierarchyButtonWidth + 8,
            metrics.hierarchyButtonTop,
            metrics.hierarchyButtonWidth,
            metrics.buttonHeight,
            TRUE);
        MoveWindow(
            m_hierarchyDeleteButton,
            metrics.outerPadding + metrics.outerPadding + (metrics.hierarchyButtonWidth + 8) * 2,
            metrics.hierarchyButtonTop,
            metrics.sideInnerWidth - (metrics.hierarchyButtonWidth + 8) * 2,
            metrics.buttonHeight,
            TRUE);
        MoveWindow(
            m_hierarchyListBox,
            metrics.outerPadding + metrics.outerPadding,
            metrics.hierarchyButtonTop + metrics.buttonHeight + 8,
            metrics.sideInnerWidth,
            (std::max)(72, metrics.hierarchyPanelHeight - metrics.groupHeaderHeight - metrics.labelHeight - metrics.buttonHeight - metrics.outerPadding - metrics.labelHeight - 28),
            TRUE);
        MoveWindow(
            m_hierarchyNameEdit,
            metrics.outerPadding + metrics.outerPadding,
            metrics.outerPadding + metrics.hierarchyPanelHeight - metrics.outerPadding - metrics.labelHeight,
            metrics.sideInnerWidth,
            metrics.labelHeight,
            TRUE);
    }

    void EditorShell::LayoutAssetsPanel(const LayoutMetrics& metrics)
    {
        MoveWindow(m_assetsPanel, metrics.outerPadding, metrics.assetsPanelY, metrics.leftPaneWidth, metrics.assetsPanelHeight, TRUE);
        MoveWindow(
            m_assetsSummaryLabel,
            metrics.outerPadding + metrics.outerPadding,
            metrics.assetsPanelY + metrics.groupHeaderHeight,
            metrics.sideInnerWidth,
            metrics.labelHeight,
            TRUE);
        MoveWindow(
            m_assetsListBox,
            metrics.outerPadding + metrics.outerPadding,
            metrics.assetsPanelY + metrics.groupHeaderHeight + metrics.labelHeight + 6,
            metrics.sideInnerWidth,
            (std::max)(100, metrics.assetsPanelHeight - metrics.groupHeaderHeight - metrics.labelHeight - metrics.outerPadding - 12),
            TRUE);
    }

    void EditorShell::LayoutInspectorPanel(const LayoutMetrics& metrics)
    {
        MoveWindow(m_inspectorPanel, metrics.rightX, metrics.outerPadding, metrics.rightWidth, metrics.scenePanelHeight, TRUE);
        MoveWindow(
            m_inspectorSummaryLabel,
            metrics.inspectorInnerX,
            metrics.outerPadding + metrics.groupHeaderHeight,
            metrics.inspectorInnerWidth,
            metrics.labelHeight,
            TRUE);
        MoveWindow(
            m_transformSectionLabel,
            metrics.inspectorInnerX,
            metrics.transformSectionTop,
            metrics.inspectorInnerWidth,
            metrics.labelHeight,
            TRUE);

        for (int row = 0; row < 3; ++row)
        {
            const int rowTop =
                metrics.transformSectionTop + metrics.labelHeight + 4 + row * (metrics.inspectorRowHeight + metrics.inspectorRowSpacing);
            MoveWindow(
                m_transformLabels[static_cast<std::size_t>(row)],
                metrics.inspectorInnerX,
                rowTop + 4,
                metrics.inspectorLabelWidth,
                metrics.inspectorRowHeight,
                TRUE);

            for (int column = 0; column < 3; ++column)
            {
                const int editIndex = row * 3 + column;
                const int editLeft = metrics.inspectorInnerX + metrics.inspectorLabelWidth + column * (metrics.inspectorEditWidth + 8);
                MoveWindow(
                    m_transformEditControls[static_cast<std::size_t>(editIndex)],
                    editLeft,
                    rowTop,
                    metrics.inspectorEditWidth,
                    metrics.inspectorRowHeight,
                    TRUE);
            }
        }

        MoveWindow(
            m_spriteComponentSectionLabel,
            metrics.inspectorInnerX,
            metrics.spriteSectionTop,
            metrics.inspectorInnerWidth,
            metrics.labelHeight,
            TRUE);
        MoveWindow(
            m_spriteRefLabel,
            metrics.inspectorInnerX,
            metrics.spriteRefTop + 4,
            metrics.inspectorLabelWidth,
            metrics.inspectorRowHeight,
            TRUE);
        MoveWindow(
            m_spriteRefEdit,
            metrics.inspectorInnerX + metrics.inspectorLabelWidth,
            metrics.spriteRefTop,
            metrics.inspectorInnerWidth - metrics.inspectorLabelWidth,
            metrics.inspectorRowHeight,
            TRUE);
        MoveWindow(
            m_spriteComponentActionButton,
            metrics.inspectorInnerX,
            metrics.spriteRefTop + metrics.inspectorRowHeight + metrics.inspectorRowSpacing,
            metrics.inspectorInnerWidth,
            metrics.inspectorRowHeight + 4,
            TRUE);
    }

    void EditorShell::LayoutSceneViewPanel(const LayoutMetrics& metrics)
    {
        MoveWindow(m_sceneViewPanel, metrics.centerX, metrics.outerPadding, metrics.centerWidth, metrics.scenePanelHeight, TRUE);
        MoveWindow(
            m_sceneViewPlanLabel,
            metrics.centerX + metrics.outerPadding,
            metrics.outerPadding + metrics.groupHeaderHeight,
            metrics.sceneInnerWidth,
            metrics.labelHeight,
            TRUE);
        MoveWindow(
            m_sceneViewHost,
            metrics.centerX + metrics.outerPadding,
            metrics.outerPadding + metrics.groupHeaderHeight + metrics.labelHeight + metrics.panelSpacing,
            metrics.sceneInnerWidth,
            metrics.sceneHostHeight,
            TRUE);
        MoveWindow(
            m_sceneViewSizeLabel,
            metrics.centerX + metrics.outerPadding,
            metrics.outerPadding + metrics.groupHeaderHeight + metrics.labelHeight + metrics.panelSpacing + metrics.sceneHostHeight + 6,
            metrics.sceneInnerWidth,
            metrics.labelHeight,
            TRUE);
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

    HWND EditorShell::GetAssetsListBox() const
    {
        return m_assetsListBox;
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
            style,
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
