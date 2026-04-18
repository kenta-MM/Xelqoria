#include "EditorShell.h"

#include <algorithm>
#include <Windows.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iterator>

namespace Xelqoria::Editor
{
    bool EditorShell::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        m_defaultFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE | BS_GROUPBOX;
        m_hierarchyPanel = CreateChildWindow(parentWindow, hInstance, L"Button", L"Hierarchy", panelStyle);
        if (nullptr == m_hierarchyPanel)
        {
            return false;
        }

        m_assetsPanel = CreateChildWindow(parentWindow, hInstance, L"Button", L"Assets", panelStyle);
        if (nullptr == m_assetsPanel)
        {
            return false;
        }

        m_inspectorPanel = CreateChildWindow(parentWindow, hInstance, L"Button", L"Inspector", panelStyle);
        if (nullptr == m_inspectorPanel)
        {
            return false;
        }

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
        if (nullptr == m_sceneViewSizeLabel)
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
        if (nullptr == m_assetsListBox)
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
        if (nullptr == m_transformLabels[0])
        {
            return false;
        }

        m_transformLabels[1] = CreateChildWindow(parentWindow, hInstance, L"Static", L"Rotation", WS_CHILD | WS_VISIBLE);
        if (nullptr == m_transformLabels[1])
        {
            return false;
        }

        m_transformLabels[2] = CreateChildWindow(parentWindow, hInstance, L"Static", L"Scale", WS_CHILD | WS_VISIBLE);
        if (nullptr == m_transformLabels[2])
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
        if (nullptr == m_spriteComponentActionButton)
        {
            return false;
        }

        return true;
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

        constexpr int outerPadding = 12;
        constexpr int panelSpacing = 12;
        constexpr int leftPaneWidth = 260;
        constexpr int rightPaneWidth = 300;
        constexpr int groupHeaderHeight = 26;
        constexpr int hierarchyHeight = 280;
        constexpr int labelHeight = 24;

        const int centerX = outerPadding + leftPaneWidth + panelSpacing;
        const int centerWidth =
            (std::max)(320, clientWidth - leftPaneWidth - rightPaneWidth - (outerPadding * 2) - (panelSpacing * 2));
        const int rightX = centerX + centerWidth + panelSpacing;
        const int rightWidth = (std::max)(220, clientWidth - rightX - outerPadding);
        const int hierarchyPanelHeight =
            (std::max)(180, (std::min)(hierarchyHeight, clientHeight - (outerPadding * 2) - panelSpacing - 160));
        const int assetsPanelY = outerPadding + hierarchyPanelHeight + panelSpacing;
        const int assetsPanelHeight = (std::max)(140, clientHeight - assetsPanelY - outerPadding);
        const int scenePanelHeight = clientHeight - (outerPadding * 2);

        MoveWindow(m_hierarchyPanel, outerPadding, outerPadding, leftPaneWidth, hierarchyPanelHeight, TRUE);
        MoveWindow(m_assetsPanel, outerPadding, assetsPanelY, leftPaneWidth, assetsPanelHeight, TRUE);
        MoveWindow(m_sceneViewPanel, centerX, outerPadding, centerWidth, scenePanelHeight, TRUE);
        MoveWindow(m_inspectorPanel, rightX, outerPadding, rightWidth, scenePanelHeight, TRUE);

        const int sideInnerWidth = leftPaneWidth - (outerPadding * 2);
        MoveWindow(
            m_assetsSummaryLabel,
            outerPadding + outerPadding,
            assetsPanelY + groupHeaderHeight,
            sideInnerWidth,
            labelHeight,
            TRUE);
        MoveWindow(
            m_assetsListBox,
            outerPadding + outerPadding,
            assetsPanelY + groupHeaderHeight + labelHeight + 6,
            sideInnerWidth,
            (std::max)(100, assetsPanelHeight - groupHeaderHeight - labelHeight - outerPadding - 12),
            TRUE);
        MoveWindow(
            m_hierarchySummaryLabel,
            outerPadding + outerPadding,
            outerPadding + groupHeaderHeight,
            sideInnerWidth,
            labelHeight,
            TRUE);
        MoveWindow(
            m_hierarchyListBox,
            outerPadding + outerPadding,
            outerPadding + groupHeaderHeight + labelHeight + 6,
            sideInnerWidth,
            (std::max)(100, hierarchyPanelHeight - groupHeaderHeight - labelHeight - outerPadding - 12),
            TRUE);

        const int inspectorInnerX = rightX + outerPadding;
        const int inspectorInnerWidth = rightWidth - (outerPadding * 2);
        const int inspectorLabelWidth = 72;
        const int inspectorEditWidth = (std::max)(60, (inspectorInnerWidth - inspectorLabelWidth - 24) / 3);
        const int inspectorRowHeight = 24;
        const int inspectorRowSpacing = 8;
        const int inspectorSectionSpacing = 12;

        MoveWindow(
            m_inspectorSummaryLabel,
            inspectorInnerX,
            outerPadding + groupHeaderHeight,
            inspectorInnerWidth,
            labelHeight,
            TRUE);

        const int transformSectionTop = outerPadding + groupHeaderHeight + labelHeight + 8;
        MoveWindow(
            m_transformSectionLabel,
            inspectorInnerX,
            transformSectionTop,
            inspectorInnerWidth,
            labelHeight,
            TRUE);

        for (int row = 0; row < 3; ++row)
        {
            const int rowTop =
                transformSectionTop + labelHeight + 4 + row * (inspectorRowHeight + inspectorRowSpacing);
            MoveWindow(
                m_transformLabels[static_cast<std::size_t>(row)],
                inspectorInnerX,
                rowTop + 4,
                inspectorLabelWidth,
                inspectorRowHeight,
                TRUE);

            for (int column = 0; column < 3; ++column)
            {
                const int editIndex = row * 3 + column;
                const int editLeft = inspectorInnerX + inspectorLabelWidth + column * (inspectorEditWidth + 8);
                MoveWindow(
                    m_transformEditControls[static_cast<std::size_t>(editIndex)],
                    editLeft,
                    rowTop,
                    inspectorEditWidth,
                    inspectorRowHeight,
                    TRUE);
            }
        }

        const int spriteSectionTop =
            transformSectionTop + labelHeight + 4 + 3 * (inspectorRowHeight + inspectorRowSpacing) + inspectorSectionSpacing;
        MoveWindow(
            m_spriteComponentSectionLabel,
            inspectorInnerX,
            spriteSectionTop,
            inspectorInnerWidth,
            labelHeight,
            TRUE);

        const int spriteRefTop = spriteSectionTop + labelHeight + 4;
        MoveWindow(
            m_spriteRefLabel,
            inspectorInnerX,
            spriteRefTop + 4,
            inspectorLabelWidth,
            inspectorRowHeight,
            TRUE);
        MoveWindow(
            m_spriteRefEdit,
            inspectorInnerX + inspectorLabelWidth,
            spriteRefTop,
            inspectorInnerWidth - inspectorLabelWidth,
            inspectorRowHeight,
            TRUE);
        MoveWindow(
            m_spriteComponentActionButton,
            inspectorInnerX,
            spriteRefTop + inspectorRowHeight + inspectorRowSpacing,
            inspectorInnerWidth,
            inspectorRowHeight + 4,
            TRUE);

        const int sceneInnerWidth = (std::max)(120, centerWidth - (outerPadding * 2));
        const int sceneHostHeight = (std::max)(160, scenePanelHeight - groupHeaderHeight - labelHeight - (outerPadding * 3));

        MoveWindow(
            m_sceneViewPlanLabel,
            centerX + outerPadding,
            outerPadding + groupHeaderHeight,
            sceneInnerWidth,
            labelHeight,
            TRUE);
        MoveWindow(
            m_sceneViewHost,
            centerX + outerPadding,
            outerPadding + groupHeaderHeight + labelHeight + panelSpacing,
            sceneInnerWidth,
            sceneHostHeight,
            TRUE);
        MoveWindow(
            m_sceneViewSizeLabel,
            centerX + outerPadding,
            outerPadding + groupHeaderHeight + labelHeight + panelSpacing + sceneHostHeight + 6,
            sceneInnerWidth,
            labelHeight,
            TRUE);

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
