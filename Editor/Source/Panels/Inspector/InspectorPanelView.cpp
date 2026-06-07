#include "Panels/Inspector/InspectorPanelView.h"

#include <algorithm>
#include <CommCtrl.h>
#include <windowsx.h>

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr UINT_PTR InspectorScrollSubclassId = 41;

        template <std::size_t Count>
        void AppendArrayControls(std::vector<HWND>& controls, const std::array<HWND, Count>& values)
        {
            controls.insert(controls.end(), values.begin(), values.end());
        }
    }

    InspectorPanelView::InspectorPanelView(EditorPanelHostContext& hostContext)
        : EditorPanelViewBase(hostContext, EditorPanelId::Inspector, L"Inspector")
    {
    }

    InspectorPanelView::~InspectorPanelView()
    {
        RemoveScrollSubclasses();
    }

    std::vector<HWND> InspectorPanelView::BuildControls() const
    {
        std::vector<HWND> controls{
            m_panel,
            m_summaryLabel,
            m_transformSectionLabel
        };
        AppendArrayControls(controls, m_transformLabels);
        AppendArrayControls(controls, m_transformEditControls);
        controls.insert(
            controls.end(),
            {
                m_spriteComponentSectionLabel,
                m_spriteRefLabel,
                m_spriteRefDropHighlight,
                m_spriteRefEdit,
                m_materialOpenButton,
                m_scriptSectionLabel,
                m_scriptAssetLabel,
                m_scriptAssetEdit,
                m_scriptCreateButton,
                m_scriptAssignButton,
                m_scriptClearButton,
                m_spriteComponentActionButton,
                m_materialSharedNoticeLabel,
                m_materialDetailsSectionLabel
            });
        AppendArrayControls(controls, m_materialDetailLabels);
        AppendArrayControls(controls, m_materialDetailEditControls);
        controls.insert(
            controls.end(),
            {
                m_materialTextureDropHighlight,
                m_materialTextureBrowseButton,
                m_materialTintColorButton,
                m_materialOutlineEnabledCheckBox,
                m_materialOutlineColorButton,
                m_materialRemoveButton,
                m_collider2DComponentSectionLabel,
                m_collider2DSummaryLabel,
                m_collider2DEnabledCheckBox,
                m_collider2DTriggerCheckBox,
                m_collider2DShapeTypeLabel,
                m_collider2DShapeTypeEdit,
                m_collider2DOffsetLabel,
                m_collider2DSizeLabel,
                m_collider2DRotationLabel,
                m_collider2DRotationEdit
            });
        AppendArrayControls(controls, m_collider2DEditControls);
        controls.insert(
            controls.end(),
            {
                m_collider2DEditButton,
                m_collider2DComponentActionButton,
                m_addComponentButton
            });
        return controls;
    }

    std::vector<HWND> InspectorPanelView::BuildVisibleControls() const
    {
        std::vector<HWND> controls = BuildControls();
        controls.erase(std::remove(controls.begin(), controls.end(), m_spriteRefDropHighlight), controls.end());
        controls.erase(std::remove(controls.begin(), controls.end(), m_materialTextureDropHighlight), controls.end());
        return controls;
    }

    bool InspectorPanelView::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL;
        m_panel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"INSPECTOR", panelStyle);
        m_summaryLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Inspector: pending", WS_CHILD | WS_VISIBLE);
        m_transformSectionLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Transform", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW);
        if (nullptr == m_panel || nullptr == m_summaryLabel || nullptr == m_transformSectionLabel)
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
            handle = CreateChildWindow(parentWindow, hInstance, L"Edit", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
            if (nullptr == handle)
            {
                return false;
            }
        }

        m_spriteComponentSectionLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Sprite", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW);
        m_spriteRefLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Sprite", WS_CHILD | WS_VISIBLE);
        m_spriteRefDropHighlight = CreateChildWindow(parentWindow, hInstance, L"Static", L"", WS_CHILD | SS_BLACKFRAME);
        m_spriteRefEdit = CreateChildWindow(parentWindow, hInstance, L"Edit", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY);
        if (nullptr == m_spriteComponentSectionLabel
            || nullptr == m_spriteRefLabel
            || nullptr == m_spriteRefDropHighlight
            || nullptr == m_spriteRefEdit
            || false == ApplySpriteRefEditReadOnlySubclass(m_spriteRefEdit))
        {
            return false;
        }

        m_materialOpenButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_scriptSectionLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Script", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW);
        m_scriptAssetLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Script", WS_CHILD | WS_VISIBLE);
        m_scriptAssetEdit = CreateChildWindow(parentWindow, hInstance, L"Edit", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY);
        m_scriptCreateButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_scriptAssignButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_scriptClearButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_spriteComponentActionButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"Remove Sprite", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_collider2DComponentActionButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"Remove Collider2DComponent", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_addComponentButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"Add Component", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_materialOpenButton
            || nullptr == m_scriptSectionLabel
            || nullptr == m_scriptAssetLabel
            || nullptr == m_scriptAssetEdit
            || nullptr == m_scriptCreateButton
            || nullptr == m_scriptAssignButton
            || nullptr == m_scriptClearButton
            || nullptr == m_spriteComponentActionButton
            || nullptr == m_collider2DComponentActionButton
            || nullptr == m_addComponentButton)
        {
            return false;
        }

        m_materialSharedNoticeLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Shared Material editing. Changes affect sprites using this material.",
            WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP);
        m_materialDetailsSectionLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Material",
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW);
        if (nullptr == m_materialSharedNoticeLabel || nullptr == m_materialDetailsSectionLabel)
        {
            return false;
        }

        const std::array<const wchar_t*, 5> materialDetailLabels{
            L"Texture",
            L"Tint",
            L"OutlineEnabled",
            L"OutlineThickness",
            L"OutlineColor"
        };
        for (std::size_t index = 0; index < m_materialDetailLabels.size(); ++index)
        {
            m_materialDetailLabels[index] = CreateChildWindow(parentWindow, hInstance, L"Static", materialDetailLabels[index], WS_CHILD | WS_VISIBLE);
            m_materialDetailEditControls[index] = CreateChildWindow(
                parentWindow,
                hInstance,
                L"Edit",
                L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
            if (nullptr == m_materialDetailLabels[index] || nullptr == m_materialDetailEditControls[index])
            {
                return false;
            }
        }

        m_materialTextureDropHighlight = CreateChildWindow(parentWindow, hInstance, L"Static", L"", WS_CHILD | SS_BLACKFRAME);
        m_materialTextureBrowseButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_materialTintColorButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_materialOutlineEnabledCheckBox = CreateChildWindow(parentWindow, hInstance, L"Button", L"", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
        m_materialOutlineColorButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_materialRemoveButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"Remove Material", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_materialTextureDropHighlight
            || nullptr == m_materialTextureBrowseButton
            || nullptr == m_materialTintColorButton
            || nullptr == m_materialOutlineEnabledCheckBox
            || nullptr == m_materialOutlineColorButton
            || nullptr == m_materialRemoveButton)
        {
            return false;
        }

        m_collider2DComponentSectionLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Collider2D", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW);
        m_collider2DSummaryLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Collider2D: no entity selected", WS_CHILD | WS_VISIBLE);
        m_collider2DEnabledCheckBox = CreateChildWindow(parentWindow, hInstance, L"Button", L"", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
        m_collider2DTriggerCheckBox = CreateChildWindow(parentWindow, hInstance, L"Button", L"", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
        m_collider2DShapeTypeLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Shape", WS_CHILD | WS_VISIBLE);
        m_collider2DShapeTypeEdit = CreateChildWindow(parentWindow, hInstance, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST);
        m_collider2DOffsetLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Offset", WS_CHILD | WS_VISIBLE);
        m_collider2DSizeLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Size", WS_CHILD | WS_VISIBLE);
        m_collider2DRotationLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Rotation", WS_CHILD | WS_VISIBLE);
        m_collider2DRotationEdit = CreateChildWindow(parentWindow, hInstance, L"Edit", L"0.000", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY);
        if (nullptr == m_collider2DComponentSectionLabel
            || nullptr == m_collider2DSummaryLabel
            || nullptr == m_collider2DEnabledCheckBox
            || nullptr == m_collider2DTriggerCheckBox
            || nullptr == m_collider2DShapeTypeLabel
            || nullptr == m_collider2DShapeTypeEdit
            || nullptr == m_collider2DOffsetLabel
            || nullptr == m_collider2DSizeLabel
            || nullptr == m_collider2DRotationLabel
            || nullptr == m_collider2DRotationEdit)
        {
            return false;
        }

        for (HWND& handle : m_collider2DEditControls)
        {
            handle = CreateChildWindow(parentWindow, hInstance, L"Edit", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
            if (nullptr == handle)
            {
                return false;
            }
        }

        m_collider2DEditButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_collider2DEditButton)
        {
            return false;
        }
        SendMessageW(m_collider2DShapeTypeEdit, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Box"));
        SendMessageW(m_collider2DShapeTypeEdit, CB_SETCURSEL, 0, 0);

        SetRootWindow(m_panel);
        SetControls(BuildControls());
        SetVisibleControls(BuildVisibleControls());
        SetAlwaysHiddenControls({});
        SetHideWhenInvisibleControls({ m_spriteRefDropHighlight, m_materialTextureDropHighlight });
        InstallScrollSubclasses();
        return true;
    }

    void InspectorPanelView::InstallScrollSubclasses()
    {
        for (HWND control : BuildControls())
        {
            if (nullptr != control)
            {
                SetWindowSubclass(
                    control,
                    InspectorScrollSubclassProc,
                    InspectorScrollSubclassId,
                    reinterpret_cast<DWORD_PTR>(this));
            }
        }
    }

    void InspectorPanelView::RemoveScrollSubclasses()
    {
        for (HWND control : BuildControls())
        {
            if (nullptr != control)
            {
                RemoveWindowSubclass(control, InspectorScrollSubclassProc, InspectorScrollSubclassId);
            }
        }
    }

    void InspectorPanelView::MoveScrollableChild(
        HWND window,
        const RECT& viewport,
        int x,
        int y,
        int width,
        int height) const
    {
        if (nullptr == window)
        {
            return;
        }

        if (width <= 0 || height <= 0 || y < viewport.top || viewport.bottom < y + height)
        {
            MoveChildWindowNoRedraw(window, x, y, 0, 0);
            return;
        }

        MoveChildWindowNoRedraw(window, x, y, width, height);
    }

    void InspectorPanelView::UpdateScrollInfo(int viewportHeight)
    {
        const int maxScrollOffset = (std::max)(0, m_contentHeight - viewportHeight);
        m_scrollOffsetY = (std::min)((std::max)(0, m_scrollOffsetY), maxScrollOffset);

        if (nullptr == m_panel)
        {
            return;
        }

        SCROLLINFO scrollInfo{};
        scrollInfo.cbSize = sizeof(scrollInfo);
        scrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        scrollInfo.nMin = 0;
        scrollInfo.nMax = (std::max)(0, m_contentHeight - 1);
        scrollInfo.nPage = static_cast<UINT>((std::max)(0, viewportHeight));
        scrollInfo.nPos = m_scrollOffsetY;
        SetScrollInfo(m_panel, SB_VERT, &scrollInfo, TRUE);
        ShowScrollBar(m_panel, SB_VERT, 0 < maxScrollOffset);
    }

    bool InspectorPanelView::SetScrollOffsetY(int scrollOffsetY)
    {
        const int viewportHeight = m_lastLayoutBounds.bottom - m_lastLayoutBounds.top;
        const int maxScrollOffset = (std::max)(0, m_contentHeight - viewportHeight);
        const int clampedOffset = (std::min)((std::max)(0, scrollOffsetY), maxScrollOffset);
        if (clampedOffset == m_scrollOffsetY)
        {
            return false;
        }

        m_scrollOffsetY = clampedOffset;
        Layout(m_lastLayoutBounds);
        InvalidateRect(m_panel, nullptr, TRUE);
        return true;
    }

    bool InspectorPanelView::ScrollBy(int deltaY)
    {
        return SetScrollOffsetY(m_scrollOffsetY + deltaY);
    }

    LRESULT InspectorPanelView::HandleScrollMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        (void)lParam;

        if (WM_MOUSEWHEEL == message)
        {
            const int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            const int scrollDelta = -MulDiv(wheelDelta, ScaleMetric(48), WHEEL_DELTA);
            (void)ScrollBy(scrollDelta);
            return 0;
        }

        if (WM_VSCROLL == message && window == m_panel)
        {
            SCROLLINFO scrollInfo{};
            scrollInfo.cbSize = sizeof(scrollInfo);
            scrollInfo.fMask = SIF_ALL;
            GetScrollInfo(m_panel, SB_VERT, &scrollInfo);

            int nextOffset = m_scrollOffsetY;
            switch (LOWORD(wParam))
            {
            case SB_LINEUP:
                nextOffset -= ScaleMetric(24);
                break;
            case SB_LINEDOWN:
                nextOffset += ScaleMetric(24);
                break;
            case SB_PAGEUP:
                nextOffset -= static_cast<int>(scrollInfo.nPage);
                break;
            case SB_PAGEDOWN:
                nextOffset += static_cast<int>(scrollInfo.nPage);
                break;
            case SB_TOP:
                nextOffset = 0;
                break;
            case SB_BOTTOM:
                nextOffset = m_contentHeight;
                break;
            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                nextOffset = scrollInfo.nTrackPos;
                break;
            default:
                return 0;
            }

            (void)SetScrollOffsetY(nextOffset);
            return 0;
        }

        return DefSubclassProc(window, message, wParam, lParam);
    }

    void InspectorPanelView::Layout(const RECT& bounds)
    {
        m_lastLayoutBounds = bounds;
        const int outerPadding = ScaleMetric(12);
        const int groupHeaderHeight = 0;
        const int labelHeight = ScaleMetric(24);
        const int rowHeight = ScaleMetric(26);
        const int rowSpacing = ScaleMetric(6);
        const int sectionSpacing = ScaleMetric(12);
        const int labelWidth = ScaleMetric(94);
        const int width = bounds.right - bounds.left;
        const int height = bounds.bottom - bounds.top;
        const int innerX = bounds.left + outerPadding;
        const int innerWidth = (std::max)(0, width - outerPadding * 2 - GetSystemMetrics(SM_CXVSCROLL));
        const int editWidth = (std::max)(0, (innerWidth - labelWidth - ScaleMetric(16)) / 3);
        const int transformTop = bounds.top + groupHeaderHeight + labelHeight + ScaleMetric(8);
        const int spriteSectionTop = transformTop + labelHeight + ScaleMetric(4) + 3 * (rowHeight + rowSpacing) + sectionSpacing;
        const int spriteRefTop = spriteSectionTop + labelHeight + ScaleMetric(4);
        const int spriteActionTop = spriteRefTop + rowHeight + rowSpacing;
        const int scriptSectionTop = spriteActionTop + rowHeight + ScaleMetric(4) + sectionSpacing;
        const int scriptAssetTop = scriptSectionTop + labelHeight + ScaleMetric(4);
        const int scriptButtonTop = scriptAssetTop;
        const int materialSectionTop = scriptAssetTop + rowHeight + rowSpacing + sectionSpacing;
        const int materialNoticeTop = materialSectionTop + labelHeight + ScaleMetric(4);
        const int materialFirstRowTop = materialNoticeTop + rowHeight + rowSpacing;
        const int materialRemoveTop = materialFirstRowTop + 5 * (rowHeight + rowSpacing) + ScaleMetric(4);
        const int colliderSectionTop = materialRemoveTop + rowHeight + ScaleMetric(4) + sectionSpacing;
        const int colliderShapeTop = colliderSectionTop + labelHeight + ScaleMetric(4);
        const int colliderOffsetTop = colliderShapeTop + rowHeight + rowSpacing;
        const int colliderSizeTop = colliderOffsetTop + rowHeight + rowSpacing;
        const int colliderRotationTop = colliderSizeTop + rowHeight + rowSpacing;
        const int colliderActionTop = colliderRotationTop + rowHeight + ScaleMetric(4) + rowSpacing;
        const int addComponentTop = colliderActionTop + rowHeight + ScaleMetric(4) + sectionSpacing;
        const int materialOpenButtonWidth = ScaleMetric(56);
        const int materialOpenButtonGap = ScaleMetric(6);
        const int materialEditWidth = (std::max)(0, innerWidth - labelWidth - materialOpenButtonWidth - materialOpenButtonGap);
        const int materialSmallButtonWidth = ScaleMetric(38);
        const int materialDetailEditWidth = (std::max)(0, innerWidth - labelWidth - materialSmallButtonWidth - materialOpenButtonGap);
        const int colliderEdit2Width = (std::max)(0, (innerWidth - labelWidth - ScaleMetric(8)) / 2);
        m_contentHeight = addComponentTop + rowHeight + ScaleMetric(8) + outerPadding - bounds.top;
        UpdateScrollInfo(height);

        const auto move = [this, &bounds](HWND window, int x, int y, int controlWidth, int controlHeight)
        {
            MoveScrollableChild(window, bounds, x, y - m_scrollOffsetY, controlWidth, controlHeight);
        };

        MoveChildWindowNoRedraw(m_panel, bounds.left, bounds.top, width, height);
        move(m_summaryLabel, innerX, bounds.top + groupHeaderHeight, innerWidth, labelHeight);
        move(m_transformSectionLabel, innerX, transformTop, innerWidth, labelHeight);
        for (int row = 0; row < 3; ++row)
        {
            const int rowTop = transformTop + labelHeight + ScaleMetric(4) + row * (rowHeight + rowSpacing);
            move(m_transformLabels[static_cast<std::size_t>(row)], innerX, rowTop + ScaleMetric(4), labelWidth, rowHeight);
            for (int column = 0; column < 3; ++column)
            {
                const int editIndex = row * 3 + column;
                const int editLeft = innerX + labelWidth + column * (editWidth + ScaleMetric(8));
                move(m_transformEditControls[static_cast<std::size_t>(editIndex)], editLeft, rowTop, editWidth, rowHeight);
            }
        }

        move(m_spriteComponentSectionLabel, innerX, spriteSectionTop, innerWidth, labelHeight);
        move(m_spriteRefLabel, innerX, spriteRefTop + ScaleMetric(4), labelWidth, rowHeight);
        move(m_spriteRefDropHighlight, innerX + labelWidth - ScaleMetric(2), spriteRefTop - ScaleMetric(2), materialEditWidth + ScaleMetric(4), rowHeight + ScaleMetric(4));
        move(m_spriteRefEdit, innerX + labelWidth, spriteRefTop, materialEditWidth, rowHeight);
        move(m_materialOpenButton, innerX + labelWidth + materialEditWidth + materialOpenButtonGap, spriteRefTop, materialOpenButtonWidth, rowHeight);
        move(m_spriteComponentActionButton, innerX, spriteActionTop, innerWidth, rowHeight + ScaleMetric(4));
        move(m_scriptSectionLabel, innerX, scriptSectionTop, innerWidth, labelHeight);
        move(m_scriptAssetLabel, innerX, scriptAssetTop + ScaleMetric(4), labelWidth, rowHeight);
        move(m_scriptAssetEdit, innerX + labelWidth, scriptAssetTop, materialEditWidth, rowHeight);
        move(m_scriptCreateButton, innerX, scriptButtonTop, 0, 0);
        move(m_scriptAssignButton, innerX + labelWidth + materialEditWidth + materialOpenButtonGap, scriptButtonTop, materialOpenButtonWidth, rowHeight);
        move(m_scriptClearButton, innerX, scriptButtonTop, 0, 0);

        move(m_materialDetailsSectionLabel, innerX, materialSectionTop, innerWidth, labelHeight);
        move(m_materialSharedNoticeLabel, innerX, materialNoticeTop, innerWidth, rowHeight);
        for (std::size_t index = 0; index < m_materialDetailLabels.size(); ++index)
        {
            const int rowTop = materialFirstRowTop + static_cast<int>(index) * (rowHeight + rowSpacing);
            move(m_materialDetailLabels[index], innerX, rowTop + ScaleMetric(4), labelWidth, rowHeight);
            if (2 == index)
            {
                move(m_materialDetailEditControls[index], innerX + labelWidth, rowTop, 0, 0);
                move(m_materialOutlineEnabledCheckBox, innerX + labelWidth, rowTop + ScaleMetric(2), ScaleMetric(28), rowHeight);
                continue;
            }

            const bool hasAccessoryButton = 0 == index || 1 == index || 4 == index;
            const int currentEditWidth = hasAccessoryButton
                ? materialDetailEditWidth
                : (std::max)(0, innerWidth - labelWidth);
            move(m_materialDetailEditControls[index], innerX + labelWidth, rowTop, currentEditWidth, rowHeight);
            if (0 == index)
            {
                move(m_materialTextureDropHighlight, innerX + labelWidth - ScaleMetric(2), rowTop - ScaleMetric(2), currentEditWidth + ScaleMetric(4), rowHeight + ScaleMetric(4));
                move(m_materialTextureBrowseButton, innerX + labelWidth + currentEditWidth + materialOpenButtonGap, rowTop, materialSmallButtonWidth, rowHeight);
            }
            else if (1 == index)
            {
                move(m_materialTintColorButton, innerX + labelWidth + currentEditWidth + materialOpenButtonGap, rowTop, materialSmallButtonWidth, rowHeight);
            }
            else if (4 == index)
            {
                move(m_materialOutlineColorButton, innerX + labelWidth + currentEditWidth + materialOpenButtonGap, rowTop, materialSmallButtonWidth, rowHeight);
            }
        }
        move(m_materialRemoveButton, innerX, materialRemoveTop, innerWidth, rowHeight + ScaleMetric(4));

        move(m_collider2DComponentSectionLabel, innerX, colliderSectionTop, innerWidth, labelHeight);
        move(m_collider2DSummaryLabel, innerX, colliderSectionTop, 0, 0);
        move(m_collider2DEnabledCheckBox, innerX, colliderSectionTop, 0, 0);
        move(m_collider2DTriggerCheckBox, innerX, colliderSectionTop, 0, 0);
        move(m_collider2DShapeTypeLabel, innerX, colliderShapeTop + ScaleMetric(4), labelWidth, rowHeight);
        move(m_collider2DShapeTypeEdit, innerX + labelWidth, colliderShapeTop, (std::max)(0, innerWidth - labelWidth), rowHeight);
        move(m_collider2DOffsetLabel, innerX, colliderOffsetTop + ScaleMetric(4), labelWidth, rowHeight);
        move(m_collider2DEditControls[0], innerX + labelWidth, colliderOffsetTop, colliderEdit2Width, rowHeight);
        move(m_collider2DEditControls[1], innerX + labelWidth + colliderEdit2Width + ScaleMetric(8), colliderOffsetTop, colliderEdit2Width, rowHeight);
        move(m_collider2DSizeLabel, innerX, colliderSizeTop + ScaleMetric(4), labelWidth, rowHeight);
        move(m_collider2DEditControls[2], innerX + labelWidth, colliderSizeTop, colliderEdit2Width, rowHeight);
        move(m_collider2DEditControls[3], innerX + labelWidth + colliderEdit2Width + ScaleMetric(8), colliderSizeTop, colliderEdit2Width, rowHeight);
        move(m_collider2DRotationLabel, innerX, colliderRotationTop + ScaleMetric(4), labelWidth, rowHeight);
        move(m_collider2DRotationEdit, innerX + labelWidth, colliderRotationTop, (std::max)(0, innerWidth - labelWidth), rowHeight);
        move(m_collider2DEditButton, innerX, colliderActionTop, 0, 0);
        move(m_collider2DComponentActionButton, innerX, colliderActionTop, innerWidth, rowHeight + ScaleMetric(4));
        move(m_addComponentButton, innerX, addComponentTop, innerWidth, rowHeight + ScaleMetric(8));
    }

    LRESULT CALLBACK InspectorPanelView::InspectorScrollSubclassProc(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam,
        UINT_PTR subclassId,
        DWORD_PTR referenceData)
    {
        (void)subclassId;

        InspectorPanelView* view = reinterpret_cast<InspectorPanelView*>(referenceData);
        if (nullptr != view && (WM_MOUSEWHEEL == message || WM_VSCROLL == message))
        {
            return view->HandleScrollMessage(window, message, wParam, lParam);
        }

        if (WM_NCDESTROY == message)
        {
            RemoveWindowSubclass(window, InspectorScrollSubclassProc, InspectorScrollSubclassId);
        }

        return DefSubclassProc(window, message, wParam, lParam);
    }

    bool InspectorPanelView::IsInputControl(HWND window) const
    {
        if (nullptr == window)
        {
            return false;
        }

        if (std::find(m_transformEditControls.begin(), m_transformEditControls.end(), window) != m_transformEditControls.end())
        {
            return true;
        }

        if (std::find(m_collider2DEditControls.begin(), m_collider2DEditControls.end(), window) != m_collider2DEditControls.end())
        {
            return true;
        }

        return window == m_spriteRefEdit
            || window == m_scriptAssetEdit
            || window == m_collider2DShapeTypeEdit
            || window == m_collider2DRotationEdit
            || window == m_materialDetailEditControls[0]
            || window == m_materialDetailEditControls[1]
            || window == m_materialDetailEditControls[3]
            || window == m_materialDetailEditControls[4];
    }

    bool InspectorPanelView::IsSecondaryLabel(HWND window) const
    {
        if (nullptr == window)
        {
            return false;
        }

        if (std::find(m_transformLabels.begin(), m_transformLabels.end(), window) != m_transformLabels.end())
        {
            return true;
        }

        return window == m_summaryLabel
            || window == m_collider2DSummaryLabel
            || window == m_spriteRefLabel
            || window == m_scriptAssetLabel
            || window == m_materialSharedNoticeLabel
            || window == m_materialDetailLabels[0]
            || window == m_materialDetailLabels[1]
            || window == m_materialDetailLabels[2]
            || window == m_materialDetailLabels[3]
            || window == m_materialDetailLabels[4]
            || window == m_collider2DShapeTypeLabel
            || window == m_collider2DOffsetLabel
            || window == m_collider2DSizeLabel
            || window == m_collider2DRotationLabel;
    }

    bool InspectorPanelView::IsSectionLabel(HWND window) const
    {
        return window == m_transformSectionLabel
            || window == m_spriteComponentSectionLabel
            || window == m_scriptSectionLabel
            || window == m_materialDetailsSectionLabel
            || window == m_collider2DComponentSectionLabel;
    }

    HWND InspectorPanelView::GetSummaryLabel() const { return m_summaryLabel; }
    HWND InspectorPanelView::GetTransformSectionLabel() const { return m_transformSectionLabel; }
    const std::array<HWND, 3>& InspectorPanelView::GetTransformLabels() const { return m_transformLabels; }
    const std::array<HWND, 9>& InspectorPanelView::GetTransformEditControls() const { return m_transformEditControls; }
    HWND InspectorPanelView::GetSpriteComponentSectionLabel() const { return m_spriteComponentSectionLabel; }
    HWND InspectorPanelView::GetSpriteRefLabel() const { return m_spriteRefLabel; }
    HWND InspectorPanelView::GetSpriteRefDropHighlight() const { return m_spriteRefDropHighlight; }
    HWND InspectorPanelView::GetSpriteRefEdit() const { return m_spriteRefEdit; }
    HWND InspectorPanelView::GetScriptSectionLabel() const { return m_scriptSectionLabel; }
    HWND InspectorPanelView::GetScriptAssetLabel() const { return m_scriptAssetLabel; }
    HWND InspectorPanelView::GetScriptAssetEdit() const { return m_scriptAssetEdit; }
    HWND InspectorPanelView::GetScriptCreateButton() const { return m_scriptCreateButton; }
    HWND InspectorPanelView::GetScriptAssignButton() const { return m_scriptAssignButton; }
    HWND InspectorPanelView::GetScriptClearButton() const { return m_scriptClearButton; }
    HWND InspectorPanelView::GetSpriteComponentActionButton() const { return m_spriteComponentActionButton; }
    HWND InspectorPanelView::GetCollider2DComponentSectionLabel() const { return m_collider2DComponentSectionLabel; }
    HWND InspectorPanelView::GetCollider2DSummaryLabel() const { return m_collider2DSummaryLabel; }
    HWND InspectorPanelView::GetCollider2DEnabledCheckBox() const { return m_collider2DEnabledCheckBox; }
    HWND InspectorPanelView::GetCollider2DTriggerCheckBox() const { return m_collider2DTriggerCheckBox; }
    HWND InspectorPanelView::GetCollider2DShapeTypeLabel() const { return m_collider2DShapeTypeLabel; }
    HWND InspectorPanelView::GetCollider2DShapeTypeEdit() const { return m_collider2DShapeTypeEdit; }
    HWND InspectorPanelView::GetCollider2DOffsetLabel() const { return m_collider2DOffsetLabel; }
    HWND InspectorPanelView::GetCollider2DSizeLabel() const { return m_collider2DSizeLabel; }
    HWND InspectorPanelView::GetCollider2DRotationLabel() const { return m_collider2DRotationLabel; }
    HWND InspectorPanelView::GetCollider2DRotationEdit() const { return m_collider2DRotationEdit; }
    const std::array<HWND, 4>& InspectorPanelView::GetCollider2DEditControls() const { return m_collider2DEditControls; }
    HWND InspectorPanelView::GetCollider2DEditButton() const { return m_collider2DEditButton; }
    HWND InspectorPanelView::GetCollider2DComponentActionButton() const { return m_collider2DComponentActionButton; }
    HWND InspectorPanelView::GetAddComponentButton() const { return m_addComponentButton; }
    HWND InspectorPanelView::GetMaterialOpenButton() const { return m_materialOpenButton; }
    HWND InspectorPanelView::GetMaterialSharedNoticeLabel() const { return m_materialSharedNoticeLabel; }
    HWND InspectorPanelView::GetMaterialDetailsSectionLabel() const { return m_materialDetailsSectionLabel; }
    const std::array<HWND, 5>& InspectorPanelView::GetMaterialDetailLabels() const { return m_materialDetailLabels; }
    const std::array<HWND, 5>& InspectorPanelView::GetMaterialDetailEditControls() const { return m_materialDetailEditControls; }
    HWND InspectorPanelView::GetMaterialTextureDropHighlight() const { return m_materialTextureDropHighlight; }
    HWND InspectorPanelView::GetMaterialTextureBrowseButton() const { return m_materialTextureBrowseButton; }
    HWND InspectorPanelView::GetMaterialTintColorButton() const { return m_materialTintColorButton; }
    HWND InspectorPanelView::GetMaterialOutlineEnabledCheckBox() const { return m_materialOutlineEnabledCheckBox; }
    HWND InspectorPanelView::GetMaterialOutlineColorButton() const { return m_materialOutlineColorButton; }
    HWND InspectorPanelView::GetMaterialRemoveButton() const { return m_materialRemoveButton; }
}
