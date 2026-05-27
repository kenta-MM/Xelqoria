#include "Panels/EditorPanelHostContext.h"

#include <algorithm>
#include <array>
#include <CommCtrl.h>
#include <cwchar>
#include <UxTheme.h>

#include "EditorTheme.h"

#pragma comment(lib, "uxtheme.lib")

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr UINT_PTR EditorPanelTabControlSubclassId = 101;
        constexpr UINT_PTR EditorPanelRowControlSubclassId = 102;
        constexpr UINT_PTR EditorPanelHeaderControlSubclassId = 103;
        constexpr UINT_PTR SpriteRefEditSubclassId = 104;

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

        void DrawRectBorder(HDC deviceContext, const RECT& rect, EditorColor color)
        {
            HPEN pen = CreatePen(PS_SOLID, 1, ToColorRef(color));
            HGDIOBJ previousPen = SelectObject(deviceContext, pen);
            HGDIOBJ previousBrush = SelectObject(deviceContext, GetStockObject(NULL_BRUSH));
            Rectangle(deviceContext, rect.left, rect.top, rect.right, rect.bottom);
            SelectObject(deviceContext, previousBrush);
            SelectObject(deviceContext, previousPen);
            DeleteObject(pen);
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

        void DrawAssetsHeaderControl(HWND headerWindow, HDC deviceContext)
        {
            if (nullptr == headerWindow || nullptr == deviceContext)
            {
                return;
            }

            RECT clientRect{};
            GetClientRect(headerWindow, &clientRect);
            FillRectWithThemeColor(deviceContext, clientRect, EditorThemes::XelqoriaDark.panelHeaderBackground);

            const int itemCount = Header_GetItemCount(headerWindow);
            for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex)
            {
                RECT itemRect{};
                if (FALSE == Header_GetItemRect(headerWindow, itemIndex, &itemRect))
                {
                    continue;
                }

                FillRectWithThemeColor(deviceContext, itemRect, EditorThemes::XelqoriaDark.panelHeaderBackground);
                DrawRectBorder(deviceContext, itemRect, EditorThemes::XelqoriaDark.panelBorder);

                wchar_t text[128]{};
                HDITEMW item{};
                item.mask = HDI_TEXT | HDI_FORMAT;
                item.pszText = text;
                item.cchTextMax = static_cast<int>(std::size(text));
                Header_GetItem(headerWindow, itemIndex, &item);

                RECT textRect = itemRect;
                textRect.left += 8;
                textRect.right -= 8;

                UINT textFormat = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS;
                if (0 != (item.fmt & HDF_CENTER))
                {
                    textFormat |= DT_CENTER;
                }
                else if (0 != (item.fmt & HDF_RIGHT))
                {
                    textFormat |= DT_RIGHT;
                }
                else
                {
                    textFormat |= DT_LEFT;
                }

                SetBkMode(deviceContext, TRANSPARENT);
                SetTextColor(deviceContext, ToColorRef(EditorThemes::XelqoriaDark.textPrimary));
                DrawTextW(deviceContext, text, -1, &textRect, textFormat);
            }
        }

        LRESULT CALLBACK EditorPanelTabControlSubclassProc(
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
                RemoveWindowSubclass(window, EditorPanelTabControlSubclassProc, EditorPanelTabControlSubclassId);
            }

            return DefSubclassProc(window, message, wParam, lParam);
        }

        LRESULT CALLBACK EditorPanelRowControlSubclassProc(
            HWND window,
            UINT message,
            WPARAM wParam,
            LPARAM lParam,
            UINT_PTR subclassId,
            DWORD_PTR referenceData)
        {
            (void)subclassId;
            (void)referenceData;

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
                RemoveWindowSubclass(window, EditorPanelRowControlSubclassProc, EditorPanelRowControlSubclassId);
            }

            return DefSubclassProc(window, message, wParam, lParam);
        }

        LRESULT CALLBACK EditorPanelHeaderControlSubclassProc(
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
                DrawAssetsHeaderControl(window, reinterpret_cast<HDC>(wParam));
                return 1;
            }

            if (WM_PAINT == message)
            {
                PAINTSTRUCT paintStruct{};
                HDC deviceContext = BeginPaint(window, &paintStruct);
                DrawAssetsHeaderControl(window, deviceContext);
                EndPaint(window, &paintStruct);
                return 0;
            }

            if (WM_MOUSEMOVE == message || WM_LBUTTONDOWN == message || WM_LBUTTONUP == message)
            {
                InvalidateRect(window, nullptr, FALSE);
            }
            else if (WM_NCDESTROY == message)
            {
                RemoveWindowSubclass(window, EditorPanelHeaderControlSubclassProc, EditorPanelHeaderControlSubclassId);
            }

            return DefSubclassProc(window, message, wParam, lParam);
        }

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
    }

    void EditorPanelHostContext::Bind(
        UINT& currentDpi,
        HFONT& defaultFont,
        std::vector<EditorPanelLayoutMove>& pendingLayoutMoves)
    {
        m_currentDpi = &currentDpi;
        m_defaultFont = &defaultFont;
        m_pendingLayoutMoves = &pendingLayoutMoves;
    }

    HWND EditorPanelHostContext::CreateChildWindow(
        HWND parentWindow,
        HINSTANCE hInstance,
        const wchar_t* className,
        const wchar_t* text,
        DWORD style,
        DWORD exStyle) const
    {
        if (nullptr != className && 0 == wcscmp(className, L"Button"))
        {
            style |= BS_OWNERDRAW;
        }

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

        if (handle != nullptr && nullptr != m_defaultFont && nullptr != *m_defaultFont)
        {
            SendMessageW(handle, WM_SETFONT, reinterpret_cast<WPARAM>(*m_defaultFont), TRUE);
        }

        return handle;
    }

    void EditorPanelHostContext::MoveChildWindowNoRedraw(HWND window, int x, int y, int width, int height) const
    {
        if (nullptr == window || nullptr == m_pendingLayoutMoves)
        {
            return;
        }

        m_pendingLayoutMoves->push_back(EditorPanelLayoutMove{
            window,
            x,
            y,
            width,
            height
        });
    }

    int EditorPanelHostContext::ScaleMetric(int value) const
    {
        const UINT dpi = nullptr != m_currentDpi ? *m_currentDpi : 96u;
        return MulDiv(value, static_cast<int>(dpi), 96);
    }

    void ApplyEditorDarkExplorerTheme(HWND window)
    {
        if (nullptr == window)
        {
            return;
        }

        SetWindowTheme(window, L"DarkMode_Explorer", nullptr);
        SetWindowPos(
            window,
            nullptr,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_UPDATENOW);
    }

    void ConfigureEditorTabControl(HWND tabControl, int itemWidth, int itemHeight)
    {
        if (nullptr == tabControl)
        {
            return;
        }

        SendMessageW(tabControl, TCM_SETITEMSIZE, 0, MAKELPARAM(itemWidth, itemHeight));
        SetWindowSubclass(tabControl, EditorPanelTabControlSubclassProc, EditorPanelTabControlSubclassId, 0);
    }

    void ConfigureEditorHierarchyListBox(HWND listBox, int rowHeight)
    {
        if (nullptr == listBox)
        {
            return;
        }

        SendMessageW(listBox, LB_SETITEMHEIGHT, 0, static_cast<LPARAM>(rowHeight));
        SetWindowSubclass(listBox, EditorPanelRowControlSubclassProc, EditorPanelRowControlSubclassId, 0);
    }

    void ConfigureEditorAssetsListView(HWND listView)
    {
        if (nullptr == listView)
        {
            return;
        }

        ListView_SetExtendedListViewStyle(listView, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        ListView_SetBkColor(listView, ToColorRef(EditorThemes::XelqoriaDark.panelBackground));
        ListView_SetTextBkColor(listView, ToColorRef(EditorThemes::XelqoriaDark.panelBackground));
        ListView_SetTextColor(listView, ToColorRef(EditorThemes::XelqoriaDark.textPrimary));
        ApplyEditorDarkExplorerTheme(listView);
        ConfigureEditorAssetsListHeaderTheme(listView);
    }

    void ConfigureEditorAssetsListHeaderTheme(HWND listView)
    {
        if (nullptr == listView)
        {
            return;
        }

        HWND assetsHeader = ListView_GetHeader(listView);
        if (nullptr == assetsHeader)
        {
            return;
        }

        ApplyEditorDarkExplorerTheme(assetsHeader);
        SetWindowSubclass(assetsHeader, EditorPanelHeaderControlSubclassProc, EditorPanelHeaderControlSubclassId, 0);
        RedrawWindow(assetsHeader, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
    }

    bool ApplySpriteRefEditReadOnlySubclass(HWND editControl)
    {
        return FALSE != SetWindowSubclass(editControl, SpriteRefEditSubclassProc, SpriteRefEditSubclassId, 0);
    }
}
