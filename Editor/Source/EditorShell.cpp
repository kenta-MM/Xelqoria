#include "EditorShell.h"

#include <algorithm>
#include <Windows.h>
#include <array>
#include <CommCtrl.h>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cwchar>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>
#include <UxTheme.h>

#include "EditorTheme.h"

#pragma comment(lib, "uxtheme.lib")

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr UINT_PTR ParentWindowSubclassId = 1;
        constexpr UINT_PTR SpriteRefEditSubclassId = 1;
        constexpr UINT_PTR EditorTabControlSubclassId = 2;
        constexpr UINT_PTR EditorRowControlSubclassId = 3;
        constexpr UINT_PTR EditorHeaderControlSubclassId = 4;
        constexpr int EditorRowHeight = 22;
        constexpr const wchar_t* WorkspaceBackgroundWindowClassName = L"XelqoriaEditorWorkspaceBackground";
        constexpr const wchar_t* EditorChromeWindowClassName = L"XelqoriaEditorChrome";
        constexpr const wchar_t* EditorPanelWindowClassName = L"XelqoriaEditorPanel";
        constexpr const wchar_t* DockPreviewWindowClassName = L"XelqoriaDockPreviewWindow";
        constexpr const wchar_t* DockGuideWindowClassName = L"XelqoriaDockGuideWindow";
        constexpr const wchar_t* FloatingPanelWindowClassName = L"XelqoriaFloatingPanelWindow";
        constexpr ULONGLONG DockPanelDragDelayMilliseconds = 200;

        struct SavedDockNode
        {
            bool isLeaf = true;
            bool isHorizontalSplit = true;
            float splitRatio = 0.5f;
            int firstChild = -1;
            int secondChild = -1;
            int activeTabIndex = 0;
            std::wstring tabKey{};
            std::vector<EditorShell::EditorPanelId> panels{};
        };

        struct SavedFloatingGroup
        {
            RECT rect{};
            int activeTabIndex = 0;
            std::vector<EditorShell::EditorPanelId> panels{};
        };

        [[nodiscard]] BYTE ToColorByte(float value)
        {
            const float clampedValue = (std::max)(0.0f, (std::min)(1.0f, value));
            return static_cast<BYTE>((clampedValue * 255.0f) + 0.5f);
        }

        [[nodiscard]] COLORREF ToColorRef(EditorColor color)
        {
            return RGB(ToColorByte(color.red), ToColorByte(color.green), ToColorByte(color.blue));
        }

        [[nodiscard]] EditorColor ScaleColor(EditorColor color, float scale)
        {
            return EditorColor{
                (std::max)(0.0f, (std::min)(1.0f, color.red * scale)),
                (std::max)(0.0f, (std::min)(1.0f, color.green * scale)),
                (std::max)(0.0f, (std::min)(1.0f, color.blue * scale)),
                color.alpha
            };
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

        void ApplyDarkExplorerTheme(HWND window)
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

        void DrawRoundRectBorder(HDC deviceContext, const RECT& rect, EditorColor color, int radius)
        {
            HPEN pen = CreatePen(PS_SOLID, 1, ToColorRef(color));
            HGDIOBJ previousPen = SelectObject(deviceContext, pen);
            HGDIOBJ previousBrush = SelectObject(deviceContext, GetStockObject(NULL_BRUSH));
            RoundRect(deviceContext, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
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

        [[nodiscard]] int GetHoveredListBoxIndex(HWND listBox)
        {
            POINT cursorPoint{};
            if (FALSE == GetCursorPos(&cursorPoint))
            {
                return -1;
            }

            ScreenToClient(listBox, &cursorPoint);
            RECT clientRect{};
            GetClientRect(listBox, &clientRect);
            if (FALSE == PtInRect(&clientRect, cursorPoint))
            {
                return -1;
            }

            const LRESULT hitResult = SendMessageW(
                listBox,
                LB_ITEMFROMPOINT,
                0,
                MAKELPARAM(cursorPoint.x, cursorPoint.y));
            if (0 != HIWORD(hitResult))
            {
                return -1;
            }

            return LOWORD(hitResult);
        }

        LRESULT CALLBACK WorkspaceBackgroundWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (WM_PAINT == message)
            {
                PAINTSTRUCT paintStruct{};
                HDC deviceContext = BeginPaint(window, &paintStruct);
                RECT clientRect{};
                GetClientRect(window, &clientRect);
                FillRectWithThemeColor(deviceContext, clientRect, EditorThemes::XelqoriaDark.windowBackground);
                EndPaint(window, &paintStruct);
                return 0;
            }

            if (WM_ERASEBKGND == message)
            {
                return 1;
            }

            return DefWindowProcW(window, message, wParam, lParam);
        }

        LRESULT CALLBACK EditorChromeWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (WM_PAINT == message)
            {
                PAINTSTRUCT paintStruct{};
                HDC deviceContext = BeginPaint(window, &paintStruct);
                RECT clientRect{};
                GetClientRect(window, &clientRect);
                FillRectWithThemeColor(deviceContext, clientRect, EditorThemes::XelqoriaDark.windowBackground);

                HPEN borderPen = CreatePen(PS_SOLID, 1, ToColorRef(EditorThemes::XelqoriaDark.panelBorder));
                HGDIOBJ previousPen = SelectObject(deviceContext, borderPen);
                MoveToEx(deviceContext, clientRect.left, clientRect.bottom - 1, nullptr);
                LineTo(deviceContext, clientRect.right, clientRect.bottom - 1);
                SelectObject(deviceContext, previousPen);
                DeleteObject(borderPen);

                wchar_t text[128]{};
                GetWindowTextW(window, text, static_cast<int>(std::size(text)));
                SetBkMode(deviceContext, TRANSPARENT);
                if (L'\0' != text[0])
                {
                    SetTextColor(deviceContext, ToColorRef(EditorThemes::XelqoriaDark.textSecondary));
                    RECT textRect = clientRect;
                    textRect.left += 10;
                    textRect.right -= 12;
                    DrawTextW(deviceContext, text, -1, &textRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

                    RECT versionRect = clientRect;
                    versionRect.right -= 12;
                    SetTextColor(deviceContext, ToColorRef(ScaleColor(EditorThemes::XelqoriaDark.textSecondary, 0.78f)));
                    DrawTextW(deviceContext, L"X E L Q O R I A   E N G I N E   v0.1.0", -1, &versionRect, DT_RIGHT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
                }

                EndPaint(window, &paintStruct);
                return 0;
            }

            if (WM_ERASEBKGND == message)
            {
                return 1;
            }

            return DefWindowProcW(window, message, wParam, lParam);
        }

        LRESULT CALLBACK EditorPanelWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (WM_PAINT == message)
            {
                PAINTSTRUCT paintStruct{};
                HDC deviceContext = BeginPaint(window, &paintStruct);
                RECT clientRect{};
                GetClientRect(window, &clientRect);

                const int panelRadius = EditorThemes::XelqoriaDark.panelCornerRadius;
                FillRoundRectWithThemeColor(
                    deviceContext,
                    clientRect,
                    EditorThemes::XelqoriaDark.panelBackground,
                    panelRadius);
                DrawRoundRectBorder(deviceContext, clientRect, EditorThemes::XelqoriaDark.panelBorder, panelRadius);

                EndPaint(window, &paintStruct);
                return 0;
            }

            if (WM_ERASEBKGND == message)
            {
                return 1;
            }

            return DefWindowProcW(window, message, wParam, lParam);
        }

        [[nodiscard]] constexpr std::array<EditorShell::EditorPanelId, 5> GetAllEditorPanels()
        {
            return {
                EditorShell::EditorPanelId::Hierarchy,
                EditorShell::EditorPanelId::Assets,
                EditorShell::EditorPanelId::SceneView,
                EditorShell::EditorPanelId::Inspector,
                EditorShell::EditorPanelId::LogOutput
            };
        }

        [[nodiscard]] const wchar_t* GetPanelLayoutName(EditorShell::EditorPanelId panelId)
        {
            switch (panelId)
            {
            case EditorShell::EditorPanelId::Hierarchy:
                return L"Hierarchy";
            case EditorShell::EditorPanelId::Assets:
                return L"Assets";
            case EditorShell::EditorPanelId::SceneView:
                return L"SceneView";
            case EditorShell::EditorPanelId::Inspector:
                return L"Inspector";
            case EditorShell::EditorPanelId::Sprite:
                return L"Sprite";
            case EditorShell::EditorPanelId::Material:
                return L"Material";
            case EditorShell::EditorPanelId::Collider2D:
                return L"Collider2D";
            case EditorShell::EditorPanelId::LogOutput:
                return L"LogOutput";
            default:
                return L"SceneView";
            }
        }

        [[nodiscard]] std::optional<EditorShell::EditorPanelId> TryParsePanelLayoutName(const std::wstring& name)
        {
            for (EditorShell::EditorPanelId panelId : GetAllEditorPanels())
            {
                if (name == GetPanelLayoutName(panelId))
                {
                    return panelId;
                }
            }

            return std::nullopt;
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

        /// <summary>
        /// Dock 先プレビューの青い配置範囲を描画する。
        /// </summary>
        /// <param name="window">プレビュー用 child window。</param>
        /// <param name="message">Win32 メッセージ。</param>
        /// <param name="wParam">メッセージ WPARAM。</param>
        /// <param name="lParam">メッセージ LPARAM。</param>
        /// <returns>メッセージ処理結果。</returns>
        LRESULT CALLBACK DockPreviewWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (WM_PAINT == message)
            {
                PAINTSTRUCT paintStruct{};
                HDC deviceContext = BeginPaint(window, &paintStruct);
                RECT clientRect{};
                GetClientRect(window, &clientRect);

                HBRUSH previewBrush = CreateSolidBrush(RGB(43, 83, 98));
                FillRect(deviceContext, &clientRect, previewBrush);
                DeleteObject(previewBrush);

                HPEN previewPen = CreatePen(PS_SOLID, 1, RGB(115, 196, 226));
                HGDIOBJ previousPen = SelectObject(deviceContext, previewPen);
                HGDIOBJ previousBrush = SelectObject(deviceContext, GetStockObject(NULL_BRUSH));
                Rectangle(deviceContext, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
                SelectObject(deviceContext, previousBrush);
                SelectObject(deviceContext, previousPen);
                DeleteObject(previewPen);

                EndPaint(window, &paintStruct);
                return 0;
            }

            if (WM_ERASEBKGND == message)
            {
                return 1;
            }

            return DefWindowProcW(window, message, wParam, lParam);
        }

        /// <summary>
        /// Dock ガイドアイコンに描く配置方向を文字列から判定する。
        /// </summary>
        /// <param name="window">Dock ガイド window。</param>
        /// <returns>配置方向を表す文字列。</returns>
        const wchar_t* GetDockGuideDirection(HWND window)
        {
            static wchar_t direction[16]{};
            GetWindowTextW(window, direction, static_cast<int>(std::size(direction)));
            return direction;
        }

        /// <summary>
        /// Dock ガイドアイコン内の配置イメージを描画する。
        /// </summary>
        /// <param name="deviceContext">描画先 DC。</param>
        /// <param name="iconRect">アイコン描画矩形。</param>
        /// <param name="direction">配置方向。</param>
        void DrawDockGuideGlyph(HDC deviceContext, const RECT& iconRect, const wchar_t* direction)
        {
            HBRUSH panelBrush = CreateSolidBrush(RGB(38, 38, 38));
            HBRUSH targetBrush = CreateSolidBrush(RGB(64, 64, 64));
            HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(174, 174, 174));
            HPEN targetPen = CreatePen(PS_DOT, 1, RGB(208, 208, 208));

            HGDIOBJ previousPen = SelectObject(deviceContext, borderPen);
            HGDIOBJ previousBrush = SelectObject(deviceContext, panelBrush);
            Rectangle(deviceContext, iconRect.left, iconRect.top, iconRect.right, iconRect.bottom);

            RECT targetRect = iconRect;
            InflateRect(&targetRect, -4, -4);
            FillRect(deviceContext, &targetRect, targetBrush);

            const bool isTop = 0 == wcscmp(direction, L"top");
            const bool isBottom = 0 == wcscmp(direction, L"bottom");
            const bool isLeft = 0 == wcscmp(direction, L"left");
            const bool isRight = 0 == wcscmp(direction, L"right");
            const int width = targetRect.right - targetRect.left;
            const int height = targetRect.bottom - targetRect.top;

            RECT splitRect = targetRect;
            if (isTop)
            {
                splitRect.bottom = targetRect.top + (std::max)(2, height / 3);
            }
            else if (isBottom)
            {
                splitRect.top = targetRect.bottom - (std::max)(2, height / 3);
            }
            else if (isLeft)
            {
                splitRect.right = targetRect.left + (std::max)(2, width / 3);
            }
            else if (isRight)
            {
                splitRect.left = targetRect.right - (std::max)(2, width / 3);
            }

            if (isTop || isBottom || isLeft || isRight)
            {
                HBRUSH splitBrush = CreateSolidBrush(RGB(82, 82, 82));
                FillRect(deviceContext, &splitRect, splitBrush);
                DeleteObject(splitBrush);
            }

            SelectObject(deviceContext, targetPen);
            Rectangle(deviceContext, targetRect.left, targetRect.top, targetRect.right, targetRect.bottom);

            SelectObject(deviceContext, previousBrush);
            SelectObject(deviceContext, previousPen);
            DeleteObject(targetPen);
            DeleteObject(borderPen);
            DeleteObject(targetBrush);
            DeleteObject(panelBrush);
        }

        /// <summary>
        /// Visual Studio 風の Dock ガイドアイコンを描画する。
        /// </summary>
        /// <param name="window">Dock ガイド window。</param>
        /// <param name="message">Win32 メッセージ。</param>
        /// <param name="wParam">メッセージ WPARAM。</param>
        /// <param name="lParam">メッセージ LPARAM。</param>
        /// <returns>メッセージ処理結果。</returns>
        LRESULT CALLBACK DockGuideWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (WM_PAINT == message)
            {
                PAINTSTRUCT paintStruct{};
                HDC deviceContext = BeginPaint(window, &paintStruct);
                RECT clientRect{};
                GetClientRect(window, &clientRect);

                HBRUSH backgroundBrush = CreateSolidBrush(RGB(31, 31, 31));
                FillRect(deviceContext, &clientRect, backgroundBrush);
                DeleteObject(backgroundBrush);

                HPEN outlinePen = CreatePen(PS_SOLID, 1, RGB(72, 72, 72));
                HGDIOBJ previousPen = SelectObject(deviceContext, outlinePen);
                HGDIOBJ previousBrush = SelectObject(deviceContext, GetStockObject(NULL_BRUSH));
                Rectangle(deviceContext, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
                SelectObject(deviceContext, previousBrush);
                SelectObject(deviceContext, previousPen);
                DeleteObject(outlinePen);

                RECT iconRect = clientRect;
                InflateRect(&iconRect, -6, -6);
                DrawDockGuideGlyph(deviceContext, iconRect, GetDockGuideDirection(window));

                EndPaint(window, &paintStruct);
                return 0;
            }

            if (WM_ERASEBKGND == message)
            {
                return 1;
            }

            return DefWindowProcW(window, message, wParam, lParam);
        }

        /// <summary>
        /// Dock 先プレビュー用 child window class を登録する。
        /// </summary>
        /// <param name="hInstance">Windows アプリケーションインスタンス。</param>
        /// <returns>登録済みまたは登録成功の場合は true。</returns>
        bool RegisterDockPreviewWindowClass(HINSTANCE hInstance)
        {
            WNDCLASSW existingClass{};
            if (GetClassInfoW(hInstance, DockPreviewWindowClassName, &existingClass))
            {
                return true;
            }

            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = DockPreviewWindowProc;
            windowClass.hInstance = hInstance;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            windowClass.lpszClassName = DockPreviewWindowClassName;
            return 0 != RegisterClassW(&windowClass);
        }

        bool RegisterWorkspaceBackgroundWindowClass(HINSTANCE hInstance)
        {
            WNDCLASSW existingClass{};
            if (GetClassInfoW(hInstance, WorkspaceBackgroundWindowClassName, &existingClass))
            {
                return true;
            }

            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = WorkspaceBackgroundWindowProc;
            windowClass.hInstance = hInstance;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            windowClass.lpszClassName = WorkspaceBackgroundWindowClassName;
            return 0 != RegisterClassW(&windowClass);
        }

        bool RegisterEditorChromeWindowClass(HINSTANCE hInstance)
        {
            WNDCLASSW existingClass{};
            if (GetClassInfoW(hInstance, EditorChromeWindowClassName, &existingClass))
            {
                return true;
            }

            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = EditorChromeWindowProc;
            windowClass.hInstance = hInstance;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            windowClass.lpszClassName = EditorChromeWindowClassName;
            return 0 != RegisterClassW(&windowClass);
        }

        bool RegisterEditorPanelWindowClass(HINSTANCE hInstance)
        {
            WNDCLASSW existingClass{};
            if (GetClassInfoW(hInstance, EditorPanelWindowClassName, &existingClass))
            {
                return true;
            }

            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = EditorPanelWindowProc;
            windowClass.hInstance = hInstance;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            windowClass.lpszClassName = EditorPanelWindowClassName;
            return 0 != RegisterClassW(&windowClass);
        }

        /// <summary>
        /// Dock ガイド用 child window class を登録する。
        /// </summary>
        /// <param name="hInstance">Windows アプリケーションインスタンス。</param>
        /// <returns>登録済みまたは登録成功の場合は true。</returns>
        bool RegisterDockGuideWindowClass(HINSTANCE hInstance)
        {
            WNDCLASSW existingClass{};
            if (GetClassInfoW(hInstance, DockGuideWindowClassName, &existingClass))
            {
                return true;
            }

            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = DockGuideWindowProc;
            windowClass.hInstance = hInstance;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            windowClass.lpszClassName = DockGuideWindowClassName;
            return 0 != RegisterClassW(&windowClass);
        }

        /// <summary>
        /// 独立ビュー用 top-level window class を登録する。
        /// </summary>
        /// <param name="hInstance">Windows アプリケーションインスタンス。</param>
        /// <param name="windowProc">フローティングビューの window procedure。</param>
        /// <returns>登録済みまたは登録成功の場合は true。</returns>
        bool RegisterFloatingPanelWindowClass(HINSTANCE hInstance, WNDPROC windowProc)
        {
            WNDCLASSW existingClass{};
            if (GetClassInfoW(hInstance, FloatingPanelWindowClassName, &existingClass))
            {
                return true;
            }

            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = windowProc;
            windowClass.hInstance = hInstance;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
            windowClass.lpszClassName = FloatingPanelWindowClassName;
            return 0 != RegisterClassW(&windowClass);
        }

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
        int groupHeaderHeight = 0;
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

    bool EditorShell::Initialize(HWND parentWindow, HINSTANCE hInstance, Platform::ICursor& cursor)
    {
        m_cursor = &cursor;
        if (false == RegisterWorkspaceBackgroundWindowClass(hInstance))
        {
            return false;
        }

        if (false == RegisterEditorChromeWindowClass(hInstance))
        {
            return false;
        }

        if (false == RegisterEditorPanelWindowClass(hInstance))
        {
            return false;
        }

        if (false == RegisterDockPreviewWindowClass(hInstance))
        {
            return false;
        }

        if (false == RegisterDockGuideWindowClass(hInstance))
        {
            return false;
        }

        if (false == RegisterFloatingPanelWindowClass(hInstance, EditorShell::FloatingPanelWindowProc))
        {
            return false;
        }

        INITCOMMONCONTROLSEX commonControls{};
        commonControls.dwSize = sizeof(commonControls);
        commonControls.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;
        if (FALSE == InitCommonControlsEx(&commonControls))
        {
            return false;
        }

        (void)RefreshDpiResources(parentWindow);
        m_parentWindow = parentWindow;
        m_windowBackgroundBrush = CreateSolidBrush(ToColorRef(EditorThemes::XelqoriaDark.windowBackground));
        m_panelBackgroundBrush = CreateSolidBrush(ToColorRef(EditorThemes::XelqoriaDark.panelBackground));
        m_inputBackgroundBrush = CreateSolidBrush(ToColorRef(EditorThemes::XelqoriaDark.panelHeaderBackground));
        if (nullptr == m_windowBackgroundBrush || nullptr == m_panelBackgroundBrush || nullptr == m_inputBackgroundBrush)
        {
            return false;
        }

        if (FALSE == SetWindowSubclass(
                parentWindow,
                EditorShell::ParentWindowSubclassProc,
                ParentWindowSubclassId,
                reinterpret_cast<DWORD_PTR>(this)))
        {
            return false;
        }

        m_workspaceBackground = CreateChildWindow(
            parentWindow,
            hInstance,
            WorkspaceBackgroundWindowClassName,
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS);
        if (nullptr == m_workspaceBackground)
        {
            return false;
        }

        m_topBar = CreateChildWindow(
            parentWindow,
            hInstance,
            EditorChromeWindowClassName,
            L"",
            WS_CHILD | WS_CLIPSIBLINGS);
        m_statusBar = CreateChildWindow(
            parentWindow,
            hInstance,
            EditorChromeWindowClassName,
            L"Ready  |  Fixed Layout  |  Xelqoria Dark",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS);
        m_topBarProjectButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"SampleProject  v", WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW);
        m_topBarPlayButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"▶", WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW);
        m_topBarLayoutButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"■", WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW);
        if (nullptr == m_topBar
            || nullptr == m_statusBar
            || nullptr == m_topBarProjectButton
            || nullptr == m_topBarPlayButton
            || nullptr == m_topBarLayoutButton)
        {
            return false;
        }

        constexpr DWORD tabStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_OWNERDRAWFIXED;
        m_leftTopDockTab = CreateChildWindow(parentWindow, hInstance, WC_TABCONTROLW, L"", tabStyle);
        m_leftBottomDockTab = CreateChildWindow(parentWindow, hInstance, WC_TABCONTROLW, L"", tabStyle);
        m_centerDockTab = CreateChildWindow(parentWindow, hInstance, WC_TABCONTROLW, L"", tabStyle);
        m_rightDockTab = CreateChildWindow(parentWindow, hInstance, WC_TABCONTROLW, L"", tabStyle);
        ConfigureEditorTabControl(m_leftTopDockTab);
        ConfigureEditorTabControl(m_leftBottomDockTab);
        ConfigureEditorTabControl(m_centerDockTab);
        ConfigureEditorTabControl(m_rightDockTab);
        m_dockPreviewWindow = CreateChildWindow(
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
        for (std::size_t index = 0; index < m_dockGuideWindows.size(); ++index)
        {
            m_dockGuideWindows[index] = CreateChildWindow(
                parentWindow,
                hInstance,
                DockGuideWindowClassName,
                guideTexts[index],
                WS_CHILD | WS_CLIPSIBLINGS);
            if (nullptr == m_dockGuideWindows[index])
            {
                return false;
            }
            ShowWindow(m_dockGuideWindows[index], SW_HIDE);
        }
        if (nullptr == m_leftTopDockTab
            || nullptr == m_leftBottomDockTab
            || nullptr == m_centerDockTab
            || nullptr == m_rightDockTab
            || nullptr == m_dockPreviewWindow)
        {
            return false;
        }
        ShowWindow(m_dockPreviewWindow, SW_HIDE);
        BuildInitialDockTree();

        const bool initialized = InitializeHierarchyPanel(parentWindow, hInstance)
            && InitializeAssetsPanel(parentWindow, hInstance)
            && InitializeInspectorPanel(parentWindow, hInstance)
            && InitializeMaterialPanel(parentWindow, hInstance)
            && InitializeCollider2DPanel(parentWindow, hInstance)
            && InitializeSceneViewPanel(parentWindow, hInstance)
            && InitializeLogOutputPanel(parentWindow, hInstance);
        if (initialized)
        {
            SyncDockTabs();
        }

        return initialized;
    }

    EditorShell::~EditorShell()
    {
        if (nullptr != m_parentWindow)
        {
            RemoveWindowSubclass(m_parentWindow, EditorShell::ParentWindowSubclassProc, ParentWindowSubclassId);
        }

        DestroyFloatingWindow(EditorPanelId::Hierarchy);
        DestroyFloatingWindow(EditorPanelId::Assets);
        DestroyFloatingWindow(EditorPanelId::SceneView);
        DestroyFloatingWindow(EditorPanelId::Inspector);
        DestroyFloatingWindow(EditorPanelId::Sprite);
        DestroyFloatingWindow(EditorPanelId::Material);
        DestroyFloatingWindow(EditorPanelId::Collider2D);
        DestroyFloatingWindow(EditorPanelId::LogOutput);

        for (HWND tabControl : m_dynamicDockTabs)
        {
            if (nullptr != tabControl)
            {
                DestroyWindow(tabControl);
            }
        }
        m_dynamicDockTabs.clear();
        m_pendingLayoutMoves.clear();

        if (nullptr != m_windowBackgroundBrush)
        {
            DeleteObject(m_windowBackgroundBrush);
            m_windowBackgroundBrush = nullptr;
        }

        if (nullptr != m_panelBackgroundBrush)
        {
            DeleteObject(m_panelBackgroundBrush);
            m_panelBackgroundBrush = nullptr;
        }

        if (nullptr != m_inputBackgroundBrush)
        {
            DeleteObject(m_inputBackgroundBrush);
            m_inputBackgroundBrush = nullptr;
        }

        if (m_ownsDefaultFont && nullptr != m_defaultFont)
        {
            HFONT stockFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            const std::array<HWND, 121> controls = CollectControls();
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

        const int outerPadding = ScaleMetric(8);
        const int statusBarHeight = ScaleMetric(18);
        MoveChildWindowNoRedraw(m_workspaceBackground, 0, 0, clientWidth, clientHeight);
        MoveChildWindowNoRedraw(m_topBar, 0, 0, 0, 0);
        MoveChildWindowNoRedraw(m_topBarProjectButton, 0, 0, 0, 0);
        MoveChildWindowNoRedraw(m_topBarPlayButton, 0, 0, 0, 0);
        MoveChildWindowNoRedraw(m_topBarLayoutButton, 0, 0, 0, 0);
        ShowWindow(m_topBar, SW_HIDE);
        ShowWindow(m_topBarProjectButton, SW_HIDE);
        ShowWindow(m_topBarPlayButton, SW_HIDE);
        ShowWindow(m_topBarLayoutButton, SW_HIDE);
        MoveChildWindowNoRedraw(m_statusBar, 0, clientHeight - statusBarHeight, clientWidth, statusBarHeight);
        const RECT rootRect{
            outerPadding,
            outerPadding,
            clientWidth - outerPadding,
            clientHeight - statusBarHeight - outerPadding
        };
        LayoutDockNode(m_rootDockNodeId, rootRect);
        HideDockGuideWindows();
        SendGroupBoxesToBack();
        RedrawLayout(parentWindow);
        m_layoutInitialized = true;
        m_lastLayoutClientWidth = clientWidth;
        m_lastLayoutClientHeight = clientHeight;
        m_lastLayoutDpi = m_currentDpi;
        return UpdateSceneViewHostSize();
    }

    void EditorShell::HideInactivePanelControls()
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
        CollectReachableDockLeaves(m_rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_dockNodes.size())
            {
                continue;
            }

            const DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Leaf != dockNode.kind || dockNode.panels.empty())
            {
                continue;
            }

            const int activeIndex =
                (std::max)(0, (std::min)(dockNode.activeTabIndex, static_cast<int>(dockNode.panels.size()) - 1));
            markActivePanel(dockNode.panels[static_cast<std::size_t>(activeIndex)]);
        }

        for (const FloatingPanelGroup& group : m_floatingPanelGroups)
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
                ShowPanelControls(panelId, false);
            }
        }
    }

    bool EditorShell::InitializeHierarchyPanel(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE;
        m_hierarchyPanel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"HIERARCHY", panelStyle);
        if (nullptr == m_hierarchyPanel)
        {
            return false;
        }

        m_hierarchySummaryLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Entities: pending",
            WS_CHILD);
        if (nullptr == m_hierarchySummaryLabel)
        {
            return false;
        }

        m_hierarchySearchEdit = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Edit",
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
        if (nullptr == m_hierarchySearchEdit)
        {
            return false;
        }
        SendMessageW(m_hierarchySearchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Search Entity"));

        m_hierarchyListBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_BORDER);
        if (nullptr == m_hierarchyListBox)
        {
            return false;
        }

        SendMessageW(m_hierarchyListBox, LB_SETITEMHEIGHT, 0, static_cast<LPARAM>(ScaleMetric(EditorRowHeight)));
        SetWindowSubclass(m_hierarchyListBox, EditorRowControlSubclassProc, EditorRowControlSubclassId, 0);

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
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE;
        m_assetsPanel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"ASSETS", panelStyle);
        if (nullptr == m_assetsPanel)
        {
            return false;
        }

        m_assetsSummaryLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Assets: pending",
            WS_CHILD);
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
                LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            ListView_SetBkColor(m_assetsListView, ToColorRef(EditorThemes::XelqoriaDark.panelBackground));
            ListView_SetTextBkColor(m_assetsListView, ToColorRef(EditorThemes::XelqoriaDark.panelBackground));
            ListView_SetTextColor(m_assetsListView, ToColorRef(EditorThemes::XelqoriaDark.textPrimary));
            ApplyDarkExplorerTheme(m_assetsListView);
            ConfigureAssetsListHeaderTheme();
        }

        return nullptr != m_assetsListView;
    }

    bool EditorShell::InitializeInspectorPanel(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE;
        m_inspectorPanel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"INSPECTOR", panelStyle);
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
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW);
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
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW);
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

        m_materialOpenButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Open",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_materialOpenButton)
        {
            return false;
        }

        m_scriptAssetLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Script", WS_CHILD | WS_VISIBLE);
        if (nullptr == m_scriptAssetLabel)
        {
            return false;
        }

        m_scriptAssetEdit = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Edit",
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY);
        if (nullptr == m_scriptAssetEdit)
        {
            return false;
        }

        m_scriptCreateButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Create Script",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_scriptCreateButton)
        {
            return false;
        }

        m_scriptAssignButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Assign Script",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_scriptAssignButton)
        {
            return false;
        }

        m_scriptClearButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Clear Script",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_scriptClearButton)
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

        m_collider2DComponentActionButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Remove Collider2DComponent",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_collider2DComponentActionButton)
        {
            return false;
        }

        m_addComponentButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Add Component",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        return nullptr != m_addComponentButton;
    }

    bool EditorShell::InitializeCollider2DPanel(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD;
        m_collider2DPanel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"COLLIDER2D", panelStyle);
        if (nullptr == m_collider2DPanel)
        {
            return false;
        }

        m_collider2DSummaryLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Collider2D: no entity selected",
            WS_CHILD | WS_VISIBLE);
        if (nullptr == m_collider2DSummaryLabel)
        {
            return false;
        }

        m_collider2DComponentSectionLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Collider2DComponent",
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW);
        if (nullptr == m_collider2DComponentSectionLabel)
        {
            return false;
        }

        m_collider2DEnabledCheckBox = CreateChildWindow(parentWindow, hInstance, L"Button", L"Enabled", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
        m_collider2DTriggerCheckBox = CreateChildWindow(parentWindow, hInstance, L"Button", L"Is Trigger", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
        m_collider2DShapeTypeLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Shape", WS_CHILD | WS_VISIBLE);
        m_collider2DShapeTypeEdit = CreateChildWindow(parentWindow, hInstance, L"Edit", L"Box", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY);
        m_collider2DOffsetLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Offset", WS_CHILD | WS_VISIBLE);
        m_collider2DSizeLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Size", WS_CHILD | WS_VISIBLE);
        m_collider2DRotationLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Rotation", WS_CHILD | WS_VISIBLE);
        m_collider2DRotationEdit = CreateChildWindow(parentWindow, hInstance, L"Edit", L"0.000", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY);
        if (nullptr == m_collider2DEnabledCheckBox
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

        m_collider2DEditButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Edit Collider2D",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        return nullptr != m_collider2DEditButton;
    }

    bool EditorShell::InitializeMaterialPanel(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD;
        m_materialPanel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"MATERIAL", panelStyle);
        if (nullptr == m_materialPanel)
        {
            return false;
        }

        m_materialSummaryLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Material: no material selected",
            WS_CHILD);
        if (nullptr == m_materialSummaryLabel)
        {
            return false;
        }

        m_materialSharedNoticeLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Shared Material editing. Changes affect sprites using this material.",
            WS_CHILD | WS_VISIBLE);
        if (nullptr == m_materialSharedNoticeLabel)
        {
            return false;
        }

        m_materialDetailsSectionLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Material Details (from SpriteComponent)",
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW);
        if (nullptr == m_materialDetailsSectionLabel)
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
            m_materialDetailLabels[index] = CreateChildWindow(
                parentWindow,
                hInstance,
                L"Static",
                materialDetailLabels[index],
                WS_CHILD | WS_VISIBLE);
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

        m_materialTextureBrowseButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"...",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_materialTintColorButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_materialOutlineEnabledCheckBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX);
        m_materialOutlineColorButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_materialTextureBrowseButton
            || nullptr == m_materialTintColorButton
            || nullptr == m_materialOutlineEnabledCheckBox
            || nullptr == m_materialOutlineColorButton)
        {
            return false;
        }

        m_materialTextureDropHighlight = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"",
            WS_CHILD | SS_BLACKFRAME);
        return nullptr != m_materialTextureDropHighlight;
    }

    bool EditorShell::InitializeSpritePanel(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE;
        m_spritePanel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"SPRITE", panelStyle);
        if (nullptr == m_spritePanel)
        {
            return false;
        }

        m_spriteSummaryLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Sprite: no sprite selected",
            WS_CHILD);
        if (nullptr == m_spriteSummaryLabel)
        {
            return false;
        }

        m_spriteDetailsSectionLabel = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Static",
            L"Sprite",
            WS_CHILD | WS_VISIBLE);
        if (nullptr == m_spriteDetailsSectionLabel)
        {
            return false;
        }

        const std::array<const wchar_t*, 4> spriteDetailLabels{
            L"Texture",
            L"Material",
            L"Script",
            L"Collider2D"
        };
        for (std::size_t index = 0; index < m_spriteDetailLabels.size(); ++index)
        {
            m_spriteDetailLabels[index] = CreateChildWindow(
                parentWindow,
                hInstance,
                L"Static",
                spriteDetailLabels[index],
                WS_CHILD | WS_VISIBLE);
            m_spriteDetailEditControls[index] = CreateChildWindow(
                parentWindow,
                hInstance,
                L"Edit",
                L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY);
            if (nullptr == m_spriteDetailLabels[index] || nullptr == m_spriteDetailEditControls[index])
            {
                return false;
            }
        }

        return true;
    }

    bool EditorShell::InitializeSceneViewPanel(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE;
        m_sceneViewPanel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"SCENE", panelStyle);
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
        if (nullptr == m_sceneViewSizeLabel)
        {
            return false;
        }

        m_buildAndPlayButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"▶",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_buildAndPlayButton)
        {
            return false;
        }

        m_pauseResumePlayButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Ⅱ",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_pauseResumePlayButton)
        {
            return false;
        }

        m_endPlayButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"■",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        return nullptr != m_endPlayButton;
    }

    bool EditorShell::InitializeLogOutputPanel(HWND parentWindow, HINSTANCE hInstance)
    {
        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE;
        m_logOutputPanel = CreateChildWindow(parentWindow, hInstance, EditorPanelWindowClassName, L"CONSOLE", panelStyle);
        if (nullptr == m_logOutputPanel)
        {
            return false;
        }

        m_logOutputTabControl = CreateChildWindow(
            parentWindow,
            hInstance,
            WC_TABCONTROLW,
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS | TCS_OWNERDRAWFIXED);
        if (nullptr == m_logOutputTabControl)
        {
            return false;
        }
        ConfigureEditorTabControl(m_logOutputTabControl);

        const std::array<const wchar_t*, 5> tabNames{ L"All", L"Info", L"Editor", L"Warn", L"Error" };
        for (std::size_t index = 0; index < tabNames.size(); ++index)
        {
            TCITEMW item{};
            item.mask = TCIF_TEXT;
            item.pszText = const_cast<LPWSTR>(tabNames[index]);
            TabCtrl_InsertItem(m_logOutputTabControl, static_cast<int>(index), &item);
        }
        TabCtrl_SetCurSel(m_logOutputTabControl, 0);

        m_logClearButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Clear",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_logClearButton)
        {
            return false;
        }

        m_logCopyButton = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Button",
            L"Copy",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        if (nullptr == m_logCopyButton)
        {
            return false;
        }

        m_logFilterEdit = CreateChildWindow(
            parentWindow,
            hInstance,
            L"Edit",
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_CENTER);
        if (nullptr == m_logFilterEdit)
        {
            return false;
        }
        SendMessageW(m_logFilterEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"検索フィルタ"));

        m_logListBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | WS_BORDER);
        return nullptr != m_logListBox;
    }

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
            switch (panels[index])
            {
            case EditorPanelId::Hierarchy:
                LayoutHierarchyPanelInRect(panelRect);
                break;
            case EditorPanelId::Assets:
                LayoutAssetsPanelInRect(panelRect);
                break;
            case EditorPanelId::SceneView:
                LayoutSceneViewPanelInRect(panelRect);
                break;
            case EditorPanelId::Inspector:
                LayoutInspectorPanelInRect(panelRect);
                break;
            case EditorPanelId::Sprite:
                LayoutSpritePanelInRect(panelRect);
                break;
            case EditorPanelId::Material:
                LayoutMaterialPanelInRect(panelRect);
                break;
            case EditorPanelId::Collider2D:
                LayoutCollider2DPanelInRect(panelRect);
                break;
            case EditorPanelId::LogOutput:
                LayoutLogOutputPanelInRect(panelRect);
                break;
            default:
                break;
            }
        }
    }

    void EditorShell::BuildInitialDockTree()
    {
        m_dockNodes.clear();

        DockNode assetsNode{};
        assetsNode.kind = DockNodeKind::Leaf;
        assetsNode.panels = { EditorPanelId::Assets };
        assetsNode.tabControl = m_leftTopDockTab;
        const DockNodeId assetsNodeId = AddDockNode(std::move(assetsNode));

        DockNode hierarchyNode{};
        hierarchyNode.kind = DockNodeKind::Leaf;
        hierarchyNode.panels = { EditorPanelId::Hierarchy };
        hierarchyNode.tabControl = m_leftBottomDockTab;
        const DockNodeId hierarchyNodeId = AddDockNode(std::move(hierarchyNode));

        DockNode sceneViewNode{};
        sceneViewNode.kind = DockNodeKind::Leaf;
        sceneViewNode.panels = { EditorPanelId::SceneView };
        sceneViewNode.tabControl = m_centerDockTab;
        const DockNodeId sceneViewNodeId = AddDockNode(std::move(sceneViewNode));

        DockNode logOutputNode{};
        logOutputNode.kind = DockNodeKind::Leaf;
        logOutputNode.panels = { EditorPanelId::LogOutput };
        logOutputNode.tabControl = CreateAdditionalDockTabControl(m_parentWindow);
        m_logOutputDockTab = logOutputNode.tabControl;
        const DockNodeId logOutputNodeId = AddDockNode(std::move(logOutputNode));

        DockNode inspectorNode{};
        inspectorNode.kind = DockNodeKind::Leaf;
        inspectorNode.panels = { EditorPanelId::Inspector };
        inspectorNode.tabControl = m_rightDockTab;
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
        m_rootDockNodeId = AddDockNode(rootNode);
    }

    void EditorShell::LayoutDockNode(DockNodeId dockNodeId, const RECT& nodeRect)
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_dockNodes.size())
        {
            return;
        }

        DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
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
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_dockNodes.size())
        {
            return;
        }

        DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
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
            switch (panelId)
            {
            case EditorPanelId::Hierarchy:
                LayoutHierarchyPanelInRect(panelRect);
                break;
            case EditorPanelId::Assets:
                LayoutAssetsPanelInRect(panelRect);
                break;
            case EditorPanelId::SceneView:
                LayoutSceneViewPanelInRect(panelRect);
                break;
            case EditorPanelId::Inspector:
                LayoutInspectorPanelInRect(panelRect);
                break;
            case EditorPanelId::Sprite:
                LayoutSpritePanelInRect(panelRect);
                break;
            case EditorPanelId::Material:
                LayoutMaterialPanelInRect(panelRect);
                break;
            case EditorPanelId::Collider2D:
                LayoutCollider2DPanelInRect(panelRect);
                break;
            case EditorPanelId::LogOutput:
                LayoutLogOutputPanelInRect(panelRect);
                break;
            default:
                break;
            }
        }
    }

    EditorShell::DockNodeId EditorShell::AddDockNode(DockNode node)
    {
        const DockNodeId dockNodeId = static_cast<DockNodeId>(m_dockNodes.size());
        m_dockNodes.push_back(std::move(node));
        return dockNodeId;
    }

    EditorShell::DockNodeId EditorShell::EnsureDefaultDockLeaf(EditorPanelId panelId)
    {
        HWND targetTabControl = GetDefaultDockTabControl(panelId);
        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
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
                m_logOutputDockTab = targetTabControl;
            }
        }

        DockNode newLeaf{};
        newLeaf.kind = DockNodeKind::Leaf;
        newLeaf.tabControl = targetTabControl;
        const DockNodeId newLeafNodeId = AddDockNode(std::move(newLeaf));
        if (m_rootDockNodeId < 0 || static_cast<std::size_t>(m_rootDockNodeId) >= m_dockNodes.size())
        {
            m_rootDockNodeId = newLeafNodeId;
            return newLeafNodeId;
        }

        const DockNode oldRootNode = m_dockNodes[static_cast<std::size_t>(m_rootDockNodeId)];
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

        m_dockNodes[static_cast<std::size_t>(m_rootDockNodeId)] = splitNode;
        return newLeafNodeId;
    }

    HWND EditorShell::GetDefaultDockTabControl(EditorPanelId panelId)
    {
        switch (panelId)
        {
        case EditorPanelId::Hierarchy:
            return m_leftTopDockTab;
        case EditorPanelId::Assets:
            return m_leftBottomDockTab;
        case EditorPanelId::SceneView:
            return m_centerDockTab;
        case EditorPanelId::Inspector:
            return m_rightDockTab;
        case EditorPanelId::Sprite:
            return m_rightDockTab;
        case EditorPanelId::Material:
            return m_rightDockTab;
        case EditorPanelId::Collider2D:
            return m_rightDockTab;
        case EditorPanelId::LogOutput:
            return m_logOutputDockTab;
        default:
            return m_centerDockTab;
        }
    }

    void EditorShell::LayoutHierarchyPanelInRect(const RECT& panelRect)
    {
        const int outerPadding = ScaleMetric(12);
        const int groupHeaderHeight = 0;
        const int labelHeight = ScaleMetric(24);
        const int buttonHeight = ScaleMetric(28);
        int hierarchyButtonGap = ScaleMetric(8);
        const int width = panelRect.right - panelRect.left;
        const int height = panelRect.bottom - panelRect.top;
        const int innerWidth = (std::max)(0, width - outerPadding * 2);
        if (innerWidth < hierarchyButtonGap * 2)
        {
            hierarchyButtonGap = 0;
        }

        const int searchTop = panelRect.top + groupHeaderHeight + ScaleMetric(6);
        const int buttonTop = searchTop + labelHeight + ScaleMetric(6);
        const int buttonWidth = (std::max)(0, (innerWidth - hierarchyButtonGap * 2) / 3);
        MoveChildWindowNoRedraw(m_hierarchyPanel, panelRect.left, panelRect.top, width, height);
        MoveChildWindowNoRedraw(m_hierarchySummaryLabel, panelRect.left + outerPadding, panelRect.top + groupHeaderHeight, 0, 0);
        MoveChildWindowNoRedraw(m_hierarchySearchEdit, panelRect.left + outerPadding, searchTop, innerWidth, labelHeight);
        MoveChildWindowNoRedraw(m_hierarchyCreateButton, panelRect.left + outerPadding, buttonTop, buttonWidth, buttonHeight);
        MoveChildWindowNoRedraw(
            m_hierarchyDuplicateButton,
            panelRect.left + outerPadding + buttonWidth + hierarchyButtonGap,
            buttonTop,
            buttonWidth,
            buttonHeight);
        MoveChildWindowNoRedraw(
            m_hierarchyDeleteButton,
            panelRect.left + outerPadding + (buttonWidth + hierarchyButtonGap) * 2,
            buttonTop,
            (std::max)(0, innerWidth - (buttonWidth + hierarchyButtonGap) * 2),
            buttonHeight);

        const int hierarchyListTop = buttonTop + buttonHeight + ScaleMetric(8);
        const int hierarchyNameTop = panelRect.top + height - outerPadding - labelHeight;
        MoveChildWindowNoRedraw(m_hierarchyListBox, panelRect.left + outerPadding, hierarchyListTop, innerWidth, (std::max)(0, hierarchyNameTop - hierarchyListTop - ScaleMetric(8)));
        MoveChildWindowNoRedraw(m_hierarchyNameEdit, panelRect.left + outerPadding, hierarchyNameTop, innerWidth, labelHeight);
    }

    void EditorShell::LayoutAssetsPanelInRect(const RECT& panelRect)
    {
        const int outerPadding = ScaleMetric(12);
        const int groupHeaderHeight = 0;
        const int labelHeight = ScaleMetric(24);
        const int width = panelRect.right - panelRect.left;
        const int height = panelRect.bottom - panelRect.top;
        const int innerWidth = (std::max)(0, width - outerPadding * 2);
        MoveChildWindowNoRedraw(m_assetsPanel, panelRect.left, panelRect.top, width, height);
        MoveChildWindowNoRedraw(m_assetsSummaryLabel, panelRect.left + outerPadding, panelRect.top + groupHeaderHeight, 0, 0);
        MoveChildWindowNoRedraw(
            m_assetsListView,
            panelRect.left + outerPadding,
            panelRect.top + groupHeaderHeight + ScaleMetric(6),
            innerWidth,
            (std::max)(0, height - groupHeaderHeight - outerPadding - ScaleMetric(12)));
    }

    void EditorShell::LayoutInspectorPanelInRect(const RECT& panelRect)
    {
        const int outerPadding = ScaleMetric(12);
        const int groupHeaderHeight = 0;
        const int labelHeight = ScaleMetric(24);
        const int rowHeight = ScaleMetric(26);
        const int rowSpacing = ScaleMetric(6);
        const int sectionSpacing = ScaleMetric(12);
        const int labelWidth = ScaleMetric(94);
        const int width = panelRect.right - panelRect.left;
        const int height = panelRect.bottom - panelRect.top;
        const int innerX = panelRect.left + outerPadding;
        const int innerWidth = (std::max)(0, width - outerPadding * 2);
        const int editWidth = (std::max)(0, (innerWidth - labelWidth - ScaleMetric(16)) / 3);
        const int transformTop = panelRect.top + groupHeaderHeight + labelHeight + ScaleMetric(8);
        const int spriteSectionTop = transformTop + labelHeight + ScaleMetric(4) + 3 * (rowHeight + rowSpacing) + sectionSpacing;
        const int spriteRefTop = spriteSectionTop + labelHeight + ScaleMetric(4);
        const int scriptAssetTop = spriteRefTop + rowHeight + rowSpacing;
        const int scriptButtonTop = scriptAssetTop + rowHeight + rowSpacing;
        const int spriteActionTop = scriptButtonTop + rowHeight + ScaleMetric(4) + rowSpacing;
        const int materialSectionTop = spriteActionTop + rowHeight + ScaleMetric(4) + sectionSpacing;
        const int materialNoticeTop = materialSectionTop + labelHeight + ScaleMetric(4);
        const int materialFirstRowTop = materialNoticeTop + rowHeight;
        const int colliderSectionTop = materialFirstRowTop + 5 * (rowHeight + rowSpacing) + sectionSpacing;
        const int colliderCheckTop = colliderSectionTop + labelHeight + ScaleMetric(4);
        const int colliderShapeTop = colliderCheckTop + rowHeight + rowSpacing;
        const int colliderOffsetTop = colliderShapeTop + rowHeight + rowSpacing;
        const int colliderSizeTop = colliderOffsetTop + rowHeight + rowSpacing;
        const int colliderRotationTop = colliderSizeTop + rowHeight + rowSpacing;
        const int colliderEditButtonTop = colliderRotationTop + rowHeight + rowSpacing;
        const int colliderActionTop = colliderEditButtonTop + rowHeight + ScaleMetric(4) + rowSpacing;
        const int addComponentTop = colliderActionTop + rowHeight + ScaleMetric(4) + sectionSpacing;
        const int scriptButtonGap = ScaleMetric(6);
        const int scriptButtonWidth = (std::max)(0, (innerWidth - scriptButtonGap * 2) / 3);
        const int materialOpenButtonWidth = ScaleMetric(56);
        const int materialOpenButtonGap = ScaleMetric(6);
        const int materialEditWidth = (std::max)(0, innerWidth - labelWidth - materialOpenButtonWidth - materialOpenButtonGap);
        const int materialSmallButtonWidth = ScaleMetric(38);
        const int materialDetailEditWidth = (std::max)(0, innerWidth - labelWidth - materialSmallButtonWidth - materialOpenButtonGap);
        const int colliderEdit2Width = (std::max)(0, (innerWidth - labelWidth - ScaleMetric(8)) / 2);

        MoveChildWindowNoRedraw(m_inspectorPanel, panelRect.left, panelRect.top, width, height);
        MoveChildWindowNoRedraw(m_inspectorSummaryLabel, innerX, panelRect.top + groupHeaderHeight, innerWidth, labelHeight);
        MoveChildWindowNoRedraw(m_transformSectionLabel, innerX, transformTop, innerWidth, labelHeight);
        for (int row = 0; row < 3; ++row)
        {
            const int rowTop = transformTop + labelHeight + ScaleMetric(4) + row * (rowHeight + rowSpacing);
            MoveChildWindowNoRedraw(m_transformLabels[static_cast<std::size_t>(row)], innerX, rowTop + ScaleMetric(4), labelWidth, rowHeight);
            for (int column = 0; column < 3; ++column)
            {
                const int editIndex = row * 3 + column;
                const int editLeft = innerX + labelWidth + column * (editWidth + ScaleMetric(8));
                MoveChildWindowNoRedraw(m_transformEditControls[static_cast<std::size_t>(editIndex)], editLeft, rowTop, editWidth, rowHeight);
            }
        }

        MoveChildWindowNoRedraw(m_spriteComponentSectionLabel, innerX, spriteSectionTop, innerWidth, labelHeight);
        MoveChildWindowNoRedraw(m_spriteRefLabel, innerX, spriteRefTop + ScaleMetric(4), labelWidth, rowHeight);
        MoveChildWindowNoRedraw(m_spriteRefDropHighlight, innerX + labelWidth - ScaleMetric(2), spriteRefTop - ScaleMetric(2), materialEditWidth + ScaleMetric(4), rowHeight + ScaleMetric(4));
        MoveChildWindowNoRedraw(m_spriteRefEdit, innerX + labelWidth, spriteRefTop, materialEditWidth, rowHeight);
        MoveChildWindowNoRedraw(m_materialOpenButton, innerX + labelWidth + materialEditWidth + materialOpenButtonGap, spriteRefTop, materialOpenButtonWidth, rowHeight);
        MoveChildWindowNoRedraw(m_scriptAssetLabel, innerX, scriptAssetTop + ScaleMetric(4), labelWidth, rowHeight);
        MoveChildWindowNoRedraw(m_scriptAssetEdit, innerX + labelWidth, scriptAssetTop, (std::max)(0, innerWidth - labelWidth), rowHeight);
        MoveChildWindowNoRedraw(m_scriptCreateButton, innerX, scriptButtonTop, scriptButtonWidth, rowHeight + ScaleMetric(4));
        MoveChildWindowNoRedraw(m_scriptAssignButton, innerX + scriptButtonWidth + scriptButtonGap, scriptButtonTop, scriptButtonWidth, rowHeight + ScaleMetric(4));
        MoveChildWindowNoRedraw(m_scriptClearButton, innerX + (scriptButtonWidth + scriptButtonGap) * 2, scriptButtonTop, (std::max)(0, innerWidth - (scriptButtonWidth + scriptButtonGap) * 2), rowHeight + ScaleMetric(4));
        MoveChildWindowNoRedraw(m_spriteComponentActionButton, innerX, spriteActionTop, innerWidth, rowHeight + ScaleMetric(4));

        MoveChildWindowNoRedraw(m_materialDetailsSectionLabel, innerX, materialSectionTop, innerWidth, labelHeight);
        MoveChildWindowNoRedraw(m_materialSharedNoticeLabel, innerX, materialNoticeTop, innerWidth, rowHeight);
        for (std::size_t index = 0; index < m_materialDetailLabels.size(); ++index)
        {
            const int rowTop = materialFirstRowTop + static_cast<int>(index) * (rowHeight + rowSpacing);
            MoveChildWindowNoRedraw(m_materialDetailLabels[index], innerX, rowTop + ScaleMetric(4), labelWidth, rowHeight);
            if (2 == index)
            {
                MoveChildWindowNoRedraw(m_materialDetailEditControls[index], innerX + labelWidth, rowTop, 0, 0);
                MoveChildWindowNoRedraw(m_materialOutlineEnabledCheckBox, innerX + labelWidth, rowTop + ScaleMetric(2), ScaleMetric(28), rowHeight);
                continue;
            }

            const bool hasAccessoryButton = 0 == index || 1 == index || 4 == index;
            const int currentEditWidth = hasAccessoryButton
                ? materialDetailEditWidth
                : (std::max)(0, innerWidth - labelWidth);
            MoveChildWindowNoRedraw(m_materialDetailEditControls[index], innerX + labelWidth, rowTop, currentEditWidth, rowHeight);
            if (0 == index)
            {
                MoveChildWindowNoRedraw(m_materialTextureDropHighlight, innerX + labelWidth - ScaleMetric(2), rowTop - ScaleMetric(2), currentEditWidth + ScaleMetric(4), rowHeight + ScaleMetric(4));
                MoveChildWindowNoRedraw(m_materialTextureBrowseButton, innerX + labelWidth + currentEditWidth + materialOpenButtonGap, rowTop, materialSmallButtonWidth, rowHeight);
            }
            else if (1 == index)
            {
                MoveChildWindowNoRedraw(m_materialTintColorButton, innerX + labelWidth + currentEditWidth + materialOpenButtonGap, rowTop, materialSmallButtonWidth, rowHeight);
            }
            else if (4 == index)
            {
                MoveChildWindowNoRedraw(m_materialOutlineColorButton, innerX + labelWidth + currentEditWidth + materialOpenButtonGap, rowTop, materialSmallButtonWidth, rowHeight);
            }
        }

        MoveChildWindowNoRedraw(m_collider2DComponentSectionLabel, innerX, colliderSectionTop, innerWidth, labelHeight);
        MoveChildWindowNoRedraw(m_collider2DSummaryLabel, innerX, colliderSectionTop, 0, 0);
        MoveChildWindowNoRedraw(m_collider2DEnabledCheckBox, innerX + labelWidth, colliderCheckTop, (std::max)(0, innerWidth / 2 - labelWidth), rowHeight);
        MoveChildWindowNoRedraw(m_collider2DTriggerCheckBox, innerX + innerWidth / 2, colliderCheckTop, (std::max)(0, innerWidth / 2), rowHeight);
        MoveChildWindowNoRedraw(m_collider2DShapeTypeLabel, innerX, colliderShapeTop + ScaleMetric(4), labelWidth, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DShapeTypeEdit, innerX + labelWidth, colliderShapeTop, (std::max)(0, innerWidth - labelWidth), rowHeight);
        MoveChildWindowNoRedraw(m_collider2DOffsetLabel, innerX, colliderOffsetTop + ScaleMetric(4), labelWidth, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DEditControls[0], innerX + labelWidth, colliderOffsetTop, colliderEdit2Width, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DEditControls[1], innerX + labelWidth + colliderEdit2Width + ScaleMetric(8), colliderOffsetTop, colliderEdit2Width, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DSizeLabel, innerX, colliderSizeTop + ScaleMetric(4), labelWidth, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DEditControls[2], innerX + labelWidth, colliderSizeTop, colliderEdit2Width, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DEditControls[3], innerX + labelWidth + colliderEdit2Width + ScaleMetric(8), colliderSizeTop, colliderEdit2Width, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DRotationLabel, innerX, colliderRotationTop + ScaleMetric(4), labelWidth, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DRotationEdit, innerX + labelWidth, colliderRotationTop, (std::max)(0, innerWidth - labelWidth), rowHeight);
        MoveChildWindowNoRedraw(m_collider2DEditButton, innerX, colliderEditButtonTop, innerWidth, rowHeight + ScaleMetric(4));
        MoveChildWindowNoRedraw(m_collider2DComponentActionButton, innerX, colliderActionTop, innerWidth, rowHeight + ScaleMetric(4));
        MoveChildWindowNoRedraw(m_addComponentButton, innerX, addComponentTop, innerWidth, rowHeight + ScaleMetric(8));
    }

    void EditorShell::LayoutCollider2DPanelInRect(const RECT& panelRect)
    {
        const int outerPadding = ScaleMetric(12);
        const int groupHeaderHeight = 0;
        const int labelHeight = ScaleMetric(24);
        const int rowHeight = ScaleMetric(26);
        const int rowSpacing = ScaleMetric(8);
        const int labelWidth = ScaleMetric(78);
        const int width = panelRect.right - panelRect.left;
        const int height = panelRect.bottom - panelRect.top;
        const int innerX = panelRect.left + outerPadding;
        const int innerWidth = (std::max)(0, width - outerPadding * 2);
        const int editWidth = (std::max)(0, (innerWidth - labelWidth - ScaleMetric(16)) / 3);
        const int sectionTop = panelRect.top + groupHeaderHeight + labelHeight + ScaleMetric(8);
        const int checkTop = sectionTop + labelHeight + ScaleMetric(6);
        const int shapeTop = checkTop + rowHeight + rowSpacing;
        const int offsetTop = shapeTop + rowHeight + rowSpacing;
        const int sizeTop = offsetTop + rowHeight + rowSpacing;

        MoveChildWindowNoRedraw(m_collider2DPanel, panelRect.left, panelRect.top, width, height);
        MoveChildWindowNoRedraw(m_collider2DSummaryLabel, innerX, panelRect.top + groupHeaderHeight, innerWidth, labelHeight);
        MoveChildWindowNoRedraw(m_collider2DComponentSectionLabel, innerX, sectionTop, innerWidth, labelHeight);
        MoveChildWindowNoRedraw(m_collider2DEnabledCheckBox, innerX, checkTop, (std::max)(0, innerWidth / 2), rowHeight);
        MoveChildWindowNoRedraw(m_collider2DTriggerCheckBox, innerX + (std::max)(0, innerWidth / 2), checkTop, (std::max)(0, innerWidth / 2), rowHeight);
        MoveChildWindowNoRedraw(m_collider2DShapeTypeLabel, innerX, shapeTop + ScaleMetric(4), labelWidth, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DShapeTypeEdit, innerX + labelWidth, shapeTop, (std::max)(0, innerWidth - labelWidth), rowHeight);
        MoveChildWindowNoRedraw(m_collider2DOffsetLabel, innerX, offsetTop + ScaleMetric(4), labelWidth, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DEditControls[0], innerX + labelWidth, offsetTop, editWidth, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DEditControls[1], innerX + labelWidth + editWidth + ScaleMetric(8), offsetTop, editWidth, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DSizeLabel, innerX, sizeTop + ScaleMetric(4), labelWidth, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DEditControls[2], innerX + labelWidth, sizeTop, editWidth, rowHeight);
        MoveChildWindowNoRedraw(m_collider2DEditControls[3], innerX + labelWidth + editWidth + ScaleMetric(8), sizeTop, editWidth, rowHeight);
    }

    void EditorShell::LayoutMaterialPanelInRect(const RECT& panelRect)
    {
        const int outerPadding = ScaleMetric(12);
        const int groupHeaderHeight = 0;
        const int labelHeight = ScaleMetric(24);
        const int rowHeight = ScaleMetric(24);
        const int rowSpacing = ScaleMetric(8);
        const int labelWidth = ScaleMetric(116);
        const int width = panelRect.right - panelRect.left;
        const int height = panelRect.bottom - panelRect.top;
        const int innerX = panelRect.left + outerPadding;
        const int innerWidth = (std::max)(0, width - outerPadding * 2);
        const int noticeTop = panelRect.top + groupHeaderHeight + ScaleMetric(6);
        const int sectionTop = noticeTop + labelHeight + ScaleMetric(10);
        const int firstRowTop = sectionTop + labelHeight + ScaleMetric(4);
        const int editWidth = (std::max)(0, innerWidth - labelWidth);

        MoveChildWindowNoRedraw(m_materialPanel, panelRect.left, panelRect.top, width, height);
        MoveChildWindowNoRedraw(m_materialSummaryLabel, innerX, panelRect.top + groupHeaderHeight, 0, 0);
        MoveChildWindowNoRedraw(m_materialSharedNoticeLabel, innerX, noticeTop, innerWidth, labelHeight);
        MoveChildWindowNoRedraw(m_materialDetailsSectionLabel, innerX, sectionTop, innerWidth, labelHeight);

        for (std::size_t index = 0; index < m_materialDetailLabels.size(); ++index)
        {
            const int rowTop = firstRowTop + static_cast<int>(index) * (rowHeight + rowSpacing);
            MoveChildWindowNoRedraw(
                m_materialDetailLabels[index],
                innerX,
                rowTop + ScaleMetric(4),
                labelWidth,
                rowHeight);
            MoveChildWindowNoRedraw(
                m_materialDetailEditControls[index],
                innerX + labelWidth,
                rowTop,
                editWidth,
                rowHeight);
        }

        MoveChildWindowNoRedraw(
            m_materialTextureDropHighlight,
            innerX + labelWidth - ScaleMetric(2),
            firstRowTop - ScaleMetric(2),
            editWidth + ScaleMetric(4),
            rowHeight + ScaleMetric(4));
    }

    void EditorShell::LayoutSpritePanelInRect(const RECT& panelRect)
    {
        const int outerPadding = ScaleMetric(12);
        const int groupHeaderHeight = 0;
        const int labelHeight = ScaleMetric(24);
        const int rowHeight = ScaleMetric(24);
        const int rowSpacing = ScaleMetric(8);
        const int labelWidth = ScaleMetric(116);
        const int width = panelRect.right - panelRect.left;
        const int height = panelRect.bottom - panelRect.top;
        const int innerX = panelRect.left + outerPadding;
        const int innerWidth = (std::max)(0, width - outerPadding * 2);
        const int sectionTop = panelRect.top + groupHeaderHeight + ScaleMetric(6);
        const int firstRowTop = sectionTop + labelHeight + ScaleMetric(8);
        const int editWidth = (std::max)(0, innerWidth - labelWidth);

        MoveChildWindowNoRedraw(m_spritePanel, panelRect.left, panelRect.top, width, height);
        MoveChildWindowNoRedraw(m_spriteSummaryLabel, innerX, panelRect.top + groupHeaderHeight, 0, 0);
        MoveChildWindowNoRedraw(m_spriteDetailsSectionLabel, innerX, sectionTop, innerWidth, labelHeight);

        for (std::size_t index = 0; index < m_spriteDetailLabels.size(); ++index)
        {
            const int rowTop = firstRowTop + static_cast<int>(index) * (rowHeight + rowSpacing);
            MoveChildWindowNoRedraw(
                m_spriteDetailLabels[index],
                innerX,
                rowTop + ScaleMetric(4),
                labelWidth,
                rowHeight);
            MoveChildWindowNoRedraw(
                m_spriteDetailEditControls[index],
                innerX + labelWidth,
                rowTop,
                editWidth,
                rowHeight);
        }
    }

    void EditorShell::LayoutSceneViewPanelInRect(const RECT& panelRect)
    {
        const int borderInset = ScaleMetric(8);
        const int toolbarHeight = ScaleMetric(26);
        const int toolbarGap = ScaleMetric(8);
        const int buttonGap = ScaleMetric(6);
        const int buttonWidth = ScaleMetric(42);
        const int width = panelRect.right - panelRect.left;
        const int height = panelRect.bottom - panelRect.top;
        const int sceneHostWidth = (std::max)(0, width - borderInset * 2);
        const int sceneHostTop = panelRect.top + borderInset + toolbarHeight + toolbarGap;
        const int panelBottom = static_cast<int>(panelRect.bottom);
        const int sceneHostHeight = (std::max)(0, panelBottom - borderInset - sceneHostTop);

        MoveChildWindowNoRedraw(m_sceneViewPanel, panelRect.left, panelRect.top, width, height);
        MoveChildWindowNoRedraw(m_projectSummaryLabel, panelRect.left, panelRect.top, 0, 0);
        MoveChildWindowNoRedraw(m_projectSceneListBox, panelRect.left, panelRect.top, 0, 0);
        MoveChildWindowNoRedraw(m_projectSceneDetailLabel, panelRect.left, panelRect.top, 0, 0);
        MoveChildWindowNoRedraw(m_sceneViewPlanLabel, panelRect.left, panelRect.top, 0, 0);
        MoveChildWindowNoRedraw(m_sceneViewSizeLabel, panelRect.left, panelRect.top, 0, 0);
        MoveChildWindowNoRedraw(
            m_buildAndPlayButton,
            panelRect.left + borderInset,
            panelRect.top + borderInset,
            buttonWidth,
            toolbarHeight);
        MoveChildWindowNoRedraw(
            m_pauseResumePlayButton,
            panelRect.left + borderInset + buttonWidth + buttonGap,
            panelRect.top + borderInset,
            buttonWidth,
            toolbarHeight);
        MoveChildWindowNoRedraw(
            m_endPlayButton,
            panelRect.left + borderInset + (buttonWidth + buttonGap) * 2,
            panelRect.top + borderInset,
            buttonWidth,
            toolbarHeight);
        MoveChildWindowNoRedraw(
            m_sceneViewHost,
            panelRect.left + borderInset,
            sceneHostTop,
            sceneHostWidth,
            sceneHostHeight);
    }

    void EditorShell::LayoutLogOutputPanelInRect(const RECT& panelRect)
    {
        const int outerPadding = ScaleMetric(8);
        const int rowHeight = ScaleMetric(24);
        const int gap = ScaleMetric(6);
        const int panelLeft = static_cast<int>(panelRect.left);
        const int panelTop = static_cast<int>(panelRect.top);
        const int panelRight = static_cast<int>(panelRect.right);
        const int panelBottom = static_cast<int>(panelRect.bottom);
        const int width = panelRight - panelLeft;
        const int height = panelBottom - panelTop;
        const int innerWidth = (std::max)(0, width - outerPadding * 2);
        const int buttonWidth = ScaleMetric(64);
        const int tabWidth = (std::min)(innerWidth - buttonWidth - gap, ScaleMetric(300));
        const int tabLeft = panelLeft + outerPadding;
        const int toolbarTop = panelTop + outerPadding;
        const int filterLeft = panelLeft;
        const int filterRight = panelLeft;
        const int filterWidth = (std::max)(0, filterRight - filterLeft);
        const int listTop = toolbarTop + rowHeight + gap;

        MoveChildWindowNoRedraw(m_logOutputPanel, panelLeft, panelTop, width, height);
        MoveChildWindowNoRedraw(m_logOutputTabControl, tabLeft, toolbarTop, tabWidth, rowHeight);
        MoveChildWindowNoRedraw(m_logClearButton, panelRight - outerPadding - buttonWidth, toolbarTop, buttonWidth, rowHeight);
        MoveChildWindowNoRedraw(m_logCopyButton, panelLeft, panelTop, 0, 0);
        ShowWindow(m_logCopyButton, SW_HIDE);
        MoveChildWindowNoRedraw(m_logFilterEdit, filterLeft, toolbarTop, filterWidth, rowHeight);
        ShowWindow(m_logFilterEdit, SW_HIDE);
        MoveChildWindowNoRedraw(
            m_logListBox,
            panelLeft + outerPadding,
            listTop,
            innerWidth,
            (std::max)(0, panelBottom - outerPadding - listTop));
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
        const int materialOpenButtonWidth = ScaleMetric(56);
        const int materialOpenButtonGap = ScaleMetric(6);
        const int materialEditWidth =
            (std::max)(0, metrics.inspectorInnerWidth - metrics.inspectorLabelWidth - materialOpenButtonWidth - materialOpenButtonGap);
        MoveChildWindowNoRedraw(
            m_spriteRefDropHighlight,
            metrics.inspectorInnerX + metrics.inspectorLabelWidth - ScaleMetric(2),
            metrics.spriteRefTop - ScaleMetric(2),
            materialEditWidth + ScaleMetric(4),
            metrics.inspectorRowHeight + ScaleMetric(4));
        MoveChildWindowNoRedraw(
            m_spriteRefEdit,
            metrics.inspectorInnerX + metrics.inspectorLabelWidth,
            metrics.spriteRefTop,
            materialEditWidth,
            metrics.inspectorRowHeight);
        MoveChildWindowNoRedraw(
            m_materialOpenButton,
            metrics.inspectorInnerX + metrics.inspectorLabelWidth + materialEditWidth + materialOpenButtonGap,
            metrics.spriteRefTop,
            materialOpenButtonWidth,
            metrics.inspectorRowHeight);

        const int scriptAssetTop = metrics.spriteRefTop + metrics.inspectorRowHeight + metrics.inspectorRowSpacing;
        const int scriptButtonTop = scriptAssetTop + metrics.inspectorRowHeight + metrics.inspectorRowSpacing;
        const int scriptButtonGap = ScaleMetric(6);
        const int scriptButtonWidth = (std::max)(0, (metrics.inspectorInnerWidth - scriptButtonGap * 2) / 3);
        MoveChildWindowNoRedraw(
            m_scriptAssetLabel,
            metrics.inspectorInnerX,
            scriptAssetTop + ScaleMetric(4),
            metrics.inspectorLabelWidth,
            metrics.inspectorRowHeight);
        MoveChildWindowNoRedraw(
            m_scriptAssetEdit,
            metrics.inspectorInnerX + metrics.inspectorLabelWidth,
            scriptAssetTop,
            (std::max)(0, metrics.inspectorInnerWidth - metrics.inspectorLabelWidth),
            metrics.inspectorRowHeight);
        MoveChildWindowNoRedraw(
            m_scriptCreateButton,
            metrics.inspectorInnerX,
            scriptButtonTop,
            scriptButtonWidth,
            metrics.inspectorRowHeight + ScaleMetric(4));
        MoveChildWindowNoRedraw(
            m_scriptAssignButton,
            metrics.inspectorInnerX + scriptButtonWidth + scriptButtonGap,
            scriptButtonTop,
            scriptButtonWidth,
            metrics.inspectorRowHeight + ScaleMetric(4));
        MoveChildWindowNoRedraw(
            m_scriptClearButton,
            metrics.inspectorInnerX + (scriptButtonWidth + scriptButtonGap) * 2,
            scriptButtonTop,
            (std::max)(0, metrics.inspectorInnerWidth - (scriptButtonWidth + scriptButtonGap) * 2),
            metrics.inspectorRowHeight + ScaleMetric(4));
        MoveChildWindowNoRedraw(
            m_spriteComponentActionButton,
            metrics.inspectorInnerX,
            scriptButtonTop + metrics.inspectorRowHeight + ScaleMetric(4) + metrics.inspectorRowSpacing,
            metrics.inspectorInnerWidth,
            metrics.inspectorRowHeight + ScaleMetric(4));

        const int colliderActionTop =
            scriptButtonTop + metrics.inspectorRowHeight + ScaleMetric(4) + metrics.inspectorRowSpacing + metrics.inspectorRowHeight + ScaleMetric(4) + ScaleMetric(12);
        MoveChildWindowNoRedraw(m_collider2DComponentActionButton, metrics.inspectorInnerX, colliderActionTop, metrics.inspectorInnerWidth, metrics.inspectorRowHeight + ScaleMetric(4));
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
        const std::array<HWND, 8> groupBoxes{
            m_hierarchyPanel,
            m_assetsPanel,
            m_inspectorPanel,
            m_spritePanel,
            m_materialPanel,
            m_collider2DPanel,
            m_sceneViewPanel,
            m_logOutputPanel
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

        if (nullptr != m_workspaceBackground)
        {
            SetWindowPos(
                m_workspaceBackground,
                HWND_BOTTOM,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    bool EditorShell::UpdateDocking(HWND parentWindow, const Core::InputSnapshot& inputSnapshot)
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
            if (0 <= hitSplitterNodeId && static_cast<std::size_t>(hitSplitterNodeId) < m_dockNodes.size())
            {
                const DockNode& splitNode = m_dockNodes[static_cast<std::size_t>(hitSplitterNodeId)];
                m_dragKind = DockSplitOrientation::Horizontal == splitNode.splitOrientation
                    ? DockDragKind::HorizontalSplitter
                    : DockDragKind::VerticalSplitter;
                m_dragSplitNodeId = hitSplitterNodeId;
                m_dragStartScreenPoint = cursorScreenPoint;
                m_dragStartSplitRatio = splitNode.splitRatio;
                SetCapture(parentWindow);
            }
            else
            {
                const std::optional<EditorPanelId> hitPanel = HitTestDockTab(cursorScreenPoint);
                if (hitPanel.has_value())
                {
                    m_pendingDockDragPanelId = hitPanel;
                    m_dragStartScreenPoint = cursorScreenPoint;
                    m_pendingDockDragStartTick = GetTickCount64();
                    SetCapture(parentWindow);
                }
            }
        }

        if (inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left))
        {
            if (DockDragKind::None == m_dragKind && m_pendingDockDragPanelId.has_value())
            {
                const int dragThresholdX = (std::max)(ScaleMetric(4), GetSystemMetrics(SM_CXDRAG));
                const int dragThresholdY = (std::max)(ScaleMetric(4), GetSystemMetrics(SM_CYDRAG));
                const bool movedEnough =
                    dragThresholdX <= std::abs(cursorScreenPoint.x - m_dragStartScreenPoint.x)
                    || dragThresholdY <= std::abs(cursorScreenPoint.y - m_dragStartScreenPoint.y);
                const bool heldLongEnough =
                    DockPanelDragDelayMilliseconds <= GetTickCount64() - m_pendingDockDragStartTick;
                if (movedEnough && heldLongEnough)
                {
                    m_dragKind = DockDragKind::Panel;
                    m_dragPanelId = m_pendingDockDragPanelId;
                    m_pendingDockDragPanelId.reset();
                    m_currentGuideTarget = DockGuideTarget{};
                    m_hasDockPreview = false;
                    BeginDockPanelDrag(*m_dragPanelId, parentWindow, cursorScreenPoint);
                    changed = true;
                }
            }

            if (DockDragKind::Panel == m_dragKind && m_dragPanelId.has_value())
            {
                UpdateDockPanelDragWindow(*m_dragPanelId, cursorScreenPoint);
                UpdateDockGuideWindows(parentWindow, cursorScreenPoint);
                m_currentGuideTarget = HitTestDockGuideTarget(parentWindow, cursorScreenPoint);
                m_hasDockPreview = DockGuideTargetKind::None != m_currentGuideTarget.kind
                    && DockGuideTargetKind::Float != m_currentGuideTarget.kind;
                m_dockPreviewRect = m_currentGuideTarget.previewRect;
                UpdateDockPreviewWindow(parentWindow);
            }
            else if (DockDragKind::HorizontalSplitter == m_dragKind || DockDragKind::VerticalSplitter == m_dragKind)
            {
                changed = UpdateDockSplitterDrag(parentWindow, cursorScreenPoint) || changed;
                if (nullptr != m_cursor)
                {
                    m_cursor->SetShape(
                        DockDragKind::HorizontalSplitter == m_dragKind
                            ? Platform::CursorShape::HorizontalResize
                            : Platform::CursorShape::VerticalResize);
                }
            }
        }
        else if (DockDragKind::None == m_dragKind)
        {
            const DockNodeId hitSplitterNodeId = HitTestDockSplitter(cursorScreenPoint);
            if (0 <= hitSplitterNodeId && static_cast<std::size_t>(hitSplitterNodeId) < m_dockNodes.size())
            {
                const DockNode& splitNode = m_dockNodes[static_cast<std::size_t>(hitSplitterNodeId)];
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
            if (DockDragKind::Panel == m_dragKind && m_dragPanelId.has_value())
            {
                const DockGuideTarget guideTarget = m_currentGuideTarget;
                m_currentGuideTarget = DockGuideTarget{};
                m_hasDockPreview = false;
                HideDockGuideWindows();
                UpdateDockPreviewWindow(parentWindow);
                ApplyDockGuideTarget(*m_dragPanelId, guideTarget, parentWindow);
                changed = true;
            }

            m_dragKind = DockDragKind::None;
            m_dragPanelId.reset();
            m_pendingDockDragPanelId.reset();
            m_pendingDockDragStartTick = 0;
            m_dragSplitNodeId = -1;
            m_hasDockPreview = false;
            m_dragPanelWindowOffset = POINT{};
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

    bool EditorShell::HandleNotify(LPARAM notifyParameter)
    {
        const NMHDR* notifyHeader = reinterpret_cast<const NMHDR*>(notifyParameter);
        if (nullptr == notifyHeader || TCN_SELCHANGE != notifyHeader->code)
        {
            return false;
        }

        for (DockNode& dockNode : m_dockNodes)
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

    void EditorShell::ResetDockLayout()
    {
        m_leftTopDockPanels = { EditorPanelId::Hierarchy };
        m_leftBottomDockPanels = { EditorPanelId::Assets };
        m_centerDockPanels = { EditorPanelId::SceneView };
        m_rightDockPanels = { EditorPanelId::Inspector, EditorPanelId::Material, EditorPanelId::Collider2D };
        m_leftTopActiveTabIndex = 0;
        m_leftBottomActiveTabIndex = 0;
        m_centerActiveTabIndex = 0;
        m_rightActiveTabIndex = 0;
        for (HWND tabControl : m_dynamicDockTabs)
        {
            if (nullptr != tabControl)
            {
                DestroyWindow(tabControl);
            }
        }
        m_dynamicDockTabs.clear();
        m_logOutputDockTab = nullptr;
        m_hasDockPreview = false;
        m_currentGuideTarget = DockGuideTarget{};
        m_pendingDockDragPanelId.reset();
        m_pendingDockDragStartTick = 0;
        BuildInitialDockTree();
        HideDockGuideWindows();
        UpdateDockPreviewWindow(m_parentWindow);
        m_leftPaneWidth = ScaleMetric(260);
        m_rightPaneWidth = ScaleMetric(300);
        m_leftTopHeight = ScaleMetric(280);
        SetPanelParent(EditorPanelId::Hierarchy, m_parentWindow);
        SetPanelParent(EditorPanelId::Assets, m_parentWindow);
        SetPanelParent(EditorPanelId::SceneView, m_parentWindow);
        SetPanelParent(EditorPanelId::Inspector, m_parentWindow);
        SetPanelParent(EditorPanelId::Sprite, m_parentWindow);
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

    void EditorShell::ShowPanelAtDefaultDock(EditorPanelId panelId)
    {
        if (nullptr == m_parentWindow)
        {
            return;
        }

        RemovePanelFromDockTree(panelId);
        DestroyFloatingWindow(panelId);
        SetPanelParent(panelId, m_parentWindow);
        const DockNodeId targetDockNodeId = EnsureDefaultDockLeaf(panelId);
        if (targetDockNodeId < 0 || static_cast<std::size_t>(targetDockNodeId) >= m_dockNodes.size())
        {
            return;
        }

        DockNode& targetDockNode = m_dockNodes[static_cast<std::size_t>(targetDockNodeId)];
        if (targetDockNode.panels.end() == std::find(targetDockNode.panels.begin(), targetDockNode.panels.end(), panelId))
        {
            targetDockNode.panels.push_back(panelId);
        }
        targetDockNode.activeTabIndex = static_cast<int>(targetDockNode.panels.size()) - 1;
        ShowPanelControls(panelId, true);
        SyncDockTabs();
        m_layoutInitialized = false;
    }

    void EditorShell::ActivatePanel(EditorPanelId panelId)
    {
        if (nullptr == m_parentWindow)
        {
            return;
        }

        const int floatingGroupIndex = FindFloatingPanelGroupIndex(panelId);
        if (floatingGroupIndex >= 0)
        {
            FloatingPanelGroup& group = m_floatingPanelGroups[static_cast<std::size_t>(floatingGroupIndex)];
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
        if (0 <= dockNodeId && static_cast<std::size_t>(dockNodeId) < m_dockNodes.size())
        {
            DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
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

    bool EditorShell::SaveLayout(const std::filesystem::path& layoutPath) const
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
        output << L"Root " << m_rootDockNodeId << L'\n';
        output << L"Nodes " << m_dockNodes.size() << L'\n';
        for (std::size_t index = 0; index < m_dockNodes.size(); ++index)
        {
            const DockNode& dockNode = m_dockNodes[index];
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

        output << L"Floating " << m_floatingPanelGroups.size() << L'\n';
        for (const FloatingPanelGroup& group : m_floatingPanelGroups)
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

    bool EditorShell::LoadLayout(const std::filesystem::path& layoutPath)
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
                if (panelId.has_value())
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
                if (panelId.has_value())
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
        for (HWND tabControl : m_dynamicDockTabs)
        {
            if (nullptr != tabControl)
            {
                DestroyWindow(tabControl);
            }
        }
        m_dynamicDockTabs.clear();
        m_logOutputDockTab = nullptr;
        m_dockNodes.clear();
        m_dockNodes.reserve(savedNodes.size());

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
            m_dockNodes.push_back(std::move(dockNode));
        }
        m_rootDockNodeId = rootDockNodeId;

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
                FloatingPanelGroup& floatingGroup = m_floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
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

    bool EditorShell::HandleDrawItem(LPARAM drawItemParameter) const
    {
        const DRAWITEMSTRUCT* drawItem = reinterpret_cast<const DRAWITEMSTRUCT*>(drawItemParameter);
        if (nullptr == drawItem)
        {
            return false;
        }

        if (DrawEditorTabControl(*drawItem))
        {
            return true;
        }

        if (DrawEditorButton(*drawItem))
        {
            return true;
        }

        if (DrawInspectorSectionLabel(*drawItem))
        {
            return true;
        }

        return DrawHierarchyListBoxItem(*drawItem);
    }

    void EditorShell::ShowPanelControls(EditorPanelId panelId, bool visible) const
    {
        const int showCommand = visible ? SW_SHOW : SW_HIDE;
        switch (panelId)
        {
        case EditorPanelId::Hierarchy:
            for (HWND control : { m_hierarchyPanel, m_hierarchyListBox, m_hierarchyNameEdit, m_hierarchyCreateButton, m_hierarchyDuplicateButton, m_hierarchyDeleteButton })
            {
                ShowWindow(control, showCommand);
            }
            ShowWindow(m_hierarchySummaryLabel, SW_HIDE);
            break;
        case EditorPanelId::Assets:
            for (HWND control : { m_assetsPanel, m_assetsListView })
            {
                ShowWindow(control, showCommand);
            }
            ShowWindow(m_assetsSummaryLabel, SW_HIDE);
            break;
        case EditorPanelId::SceneView:
            for (HWND control : { m_sceneViewPanel, m_sceneViewHost, m_buildAndPlayButton, m_pauseResumePlayButton, m_endPlayButton })
            {
                ShowWindow(control, showCommand);
            }
            ShowWindow(m_sceneViewPlanLabel, SW_HIDE);
            ShowWindow(m_projectSummaryLabel, SW_HIDE);
            ShowWindow(m_projectSceneListBox, SW_HIDE);
            ShowWindow(m_projectSceneDetailLabel, SW_HIDE);
            ShowWindow(m_sceneViewSizeLabel, SW_HIDE);
            break;
        case EditorPanelId::Inspector:
            for (HWND control : { m_inspectorPanel, m_inspectorSummaryLabel, m_transformSectionLabel, m_transformLabels[0], m_transformLabels[1], m_transformLabels[2], m_transformEditControls[0], m_transformEditControls[1], m_transformEditControls[2], m_transformEditControls[3], m_transformEditControls[4], m_transformEditControls[5], m_transformEditControls[6], m_transformEditControls[7], m_transformEditControls[8], m_spriteComponentSectionLabel, m_spriteRefLabel, m_spriteRefEdit, m_materialOpenButton, m_scriptAssetLabel, m_scriptAssetEdit, m_scriptCreateButton, m_scriptAssignButton, m_scriptClearButton, m_spriteComponentActionButton, m_materialSharedNoticeLabel, m_materialDetailsSectionLabel, m_materialDetailLabels[0], m_materialDetailLabels[1], m_materialDetailLabels[2], m_materialDetailLabels[3], m_materialDetailLabels[4], m_materialDetailEditControls[0], m_materialDetailEditControls[1], m_materialDetailEditControls[2], m_materialDetailEditControls[3], m_materialDetailEditControls[4], m_materialTextureBrowseButton, m_materialTintColorButton, m_materialOutlineEnabledCheckBox, m_materialOutlineColorButton, m_collider2DComponentSectionLabel, m_collider2DSummaryLabel, m_collider2DEnabledCheckBox, m_collider2DTriggerCheckBox, m_collider2DShapeTypeLabel, m_collider2DShapeTypeEdit, m_collider2DOffsetLabel, m_collider2DSizeLabel, m_collider2DRotationLabel, m_collider2DRotationEdit, m_collider2DEditControls[0], m_collider2DEditControls[1], m_collider2DEditControls[2], m_collider2DEditControls[3], m_collider2DEditButton, m_collider2DComponentActionButton, m_addComponentButton })
            {
                ShowWindow(control, showCommand);
            }
            if (false == visible)
            {
                ShowWindow(m_spriteRefDropHighlight, SW_HIDE);
                ShowWindow(m_materialTextureDropHighlight, SW_HIDE);
            }
            break;
        case EditorPanelId::Material:
            for (HWND control : { m_materialPanel, m_materialSharedNoticeLabel, m_materialDetailsSectionLabel, m_materialDetailLabels[0], m_materialDetailLabels[1], m_materialDetailLabels[2], m_materialDetailLabels[3], m_materialDetailLabels[4], m_materialDetailEditControls[0], m_materialDetailEditControls[1], m_materialDetailEditControls[2], m_materialDetailEditControls[3], m_materialDetailEditControls[4] })
            {
                ShowWindow(control, showCommand);
            }
            ShowWindow(m_materialSummaryLabel, SW_HIDE);
            if (false == visible)
            {
                ShowWindow(m_materialTextureDropHighlight, SW_HIDE);
            }
            break;
        case EditorPanelId::Sprite:
            for (HWND control : { m_spritePanel, m_spriteDetailsSectionLabel, m_spriteDetailLabels[0], m_spriteDetailLabels[1], m_spriteDetailLabels[2], m_spriteDetailLabels[3], m_spriteDetailEditControls[0], m_spriteDetailEditControls[1], m_spriteDetailEditControls[2], m_spriteDetailEditControls[3] })
            {
                ShowWindow(control, showCommand);
            }
            ShowWindow(m_spriteSummaryLabel, SW_HIDE);
            break;
        case EditorPanelId::Collider2D:
            for (HWND control : { m_collider2DPanel, m_collider2DSummaryLabel, m_collider2DComponentSectionLabel, m_collider2DEnabledCheckBox, m_collider2DTriggerCheckBox, m_collider2DShapeTypeLabel, m_collider2DShapeTypeEdit, m_collider2DOffsetLabel, m_collider2DSizeLabel, m_collider2DEditControls[0], m_collider2DEditControls[1], m_collider2DEditControls[2], m_collider2DEditControls[3] })
            {
                ShowWindow(control, showCommand);
            }
            break;
        case EditorPanelId::LogOutput:
            for (HWND control : { m_logOutputPanel, m_logOutputTabControl, m_logClearButton, m_logFilterEdit, m_logListBox })
            {
                ShowWindow(control, showCommand);
            }
            ShowWindow(m_logCopyButton, SW_HIDE);
            break;
        default:
            break;
        }
    }

    void EditorShell::SetPanelParent(EditorPanelId panelId, HWND parentWindow) const
    {
        if (nullptr == parentWindow)
        {
            return;
        }

        const auto setParent =
            [parentWindow](HWND control)
            {
                if (nullptr != control && GetParent(control) != parentWindow)
                {
                    SetParent(control, parentWindow);
                }
            };

        switch (panelId)
        {
        case EditorPanelId::Hierarchy:
            for (HWND control : { m_hierarchyPanel, m_hierarchySummaryLabel, m_hierarchyListBox, m_hierarchyNameEdit, m_hierarchyCreateButton, m_hierarchyDuplicateButton, m_hierarchyDeleteButton })
            {
                setParent(control);
            }
            break;
        case EditorPanelId::Assets:
            for (HWND control : { m_assetsPanel, m_assetsSummaryLabel, m_assetsListView })
            {
                setParent(control);
            }
            break;
        case EditorPanelId::SceneView:
            for (HWND control : { m_sceneViewPanel, m_sceneViewPlanLabel, m_projectSummaryLabel, m_projectSceneListBox, m_projectSceneDetailLabel, m_sceneViewHost, m_sceneViewSizeLabel, m_buildAndPlayButton, m_pauseResumePlayButton, m_endPlayButton })
            {
                setParent(control);
            }
            break;
        case EditorPanelId::Inspector:
            for (HWND control : { m_inspectorPanel, m_inspectorSummaryLabel, m_transformSectionLabel, m_transformLabels[0], m_transformLabels[1], m_transformLabels[2], m_transformEditControls[0], m_transformEditControls[1], m_transformEditControls[2], m_transformEditControls[3], m_transformEditControls[4], m_transformEditControls[5], m_transformEditControls[6], m_transformEditControls[7], m_transformEditControls[8], m_spriteComponentSectionLabel, m_spriteRefLabel, m_spriteRefDropHighlight, m_spriteRefEdit, m_materialOpenButton, m_scriptAssetLabel, m_scriptAssetEdit, m_scriptCreateButton, m_scriptAssignButton, m_scriptClearButton, m_spriteComponentActionButton, m_materialSharedNoticeLabel, m_materialDetailsSectionLabel, m_materialDetailLabels[0], m_materialDetailLabels[1], m_materialDetailLabels[2], m_materialDetailLabels[3], m_materialDetailLabels[4], m_materialDetailEditControls[0], m_materialDetailEditControls[1], m_materialDetailEditControls[2], m_materialDetailEditControls[3], m_materialDetailEditControls[4], m_materialTextureDropHighlight, m_materialTextureBrowseButton, m_materialTintColorButton, m_materialOutlineEnabledCheckBox, m_materialOutlineColorButton, m_collider2DComponentSectionLabel, m_collider2DSummaryLabel, m_collider2DEnabledCheckBox, m_collider2DTriggerCheckBox, m_collider2DShapeTypeLabel, m_collider2DShapeTypeEdit, m_collider2DOffsetLabel, m_collider2DSizeLabel, m_collider2DRotationLabel, m_collider2DRotationEdit, m_collider2DEditControls[0], m_collider2DEditControls[1], m_collider2DEditControls[2], m_collider2DEditControls[3], m_collider2DEditButton, m_collider2DComponentActionButton, m_addComponentButton })
            {
                setParent(control);
            }
            break;
        case EditorPanelId::Material:
            for (HWND control : { m_materialPanel, m_materialSummaryLabel, m_materialSharedNoticeLabel, m_materialDetailsSectionLabel, m_materialDetailLabels[0], m_materialDetailLabels[1], m_materialDetailLabels[2], m_materialDetailLabels[3], m_materialDetailLabels[4], m_materialDetailEditControls[0], m_materialDetailEditControls[1], m_materialDetailEditControls[2], m_materialDetailEditControls[3], m_materialDetailEditControls[4], m_materialTextureDropHighlight })
            {
                setParent(control);
            }
            break;
        case EditorPanelId::Sprite:
            for (HWND control : { m_spritePanel, m_spriteSummaryLabel, m_spriteDetailsSectionLabel, m_spriteDetailLabels[0], m_spriteDetailLabels[1], m_spriteDetailLabels[2], m_spriteDetailLabels[3], m_spriteDetailEditControls[0], m_spriteDetailEditControls[1], m_spriteDetailEditControls[2], m_spriteDetailEditControls[3] })
            {
                setParent(control);
            }
            break;
        case EditorPanelId::Collider2D:
            for (HWND control : { m_collider2DPanel, m_collider2DSummaryLabel, m_collider2DComponentSectionLabel, m_collider2DEnabledCheckBox, m_collider2DTriggerCheckBox, m_collider2DShapeTypeLabel, m_collider2DShapeTypeEdit, m_collider2DOffsetLabel, m_collider2DSizeLabel, m_collider2DEditControls[0], m_collider2DEditControls[1], m_collider2DEditControls[2], m_collider2DEditControls[3] })
            {
                setParent(control);
            }
            break;
        case EditorPanelId::LogOutput:
            for (HWND control : { m_logOutputPanel, m_logOutputTabControl, m_logClearButton, m_logCopyButton, m_logFilterEdit, m_logListBox })
            {
                setParent(control);
            }
            break;
        default:
            break;
        }
    }

    RECT EditorShell::GetPanelCaptionRect(EditorPanelId panelId) const
    {
        HWND panelWindow = nullptr;
        switch (panelId)
        {
        case EditorPanelId::Hierarchy:
            panelWindow = m_hierarchyPanel;
            break;
        case EditorPanelId::Assets:
            panelWindow = m_assetsPanel;
            break;
        case EditorPanelId::SceneView:
            panelWindow = m_sceneViewPanel;
            break;
        case EditorPanelId::Inspector:
            panelWindow = m_inspectorPanel;
            break;
        case EditorPanelId::Sprite:
            panelWindow = m_spritePanel;
            break;
        case EditorPanelId::Material:
            panelWindow = m_materialPanel;
            break;
        case EditorPanelId::Collider2D:
            panelWindow = m_collider2DPanel;
            break;
        case EditorPanelId::LogOutput:
            panelWindow = m_logOutputPanel;
            break;
        default:
            break;
        }

        RECT captionRect{};
        if (nullptr != panelWindow && IsWindowVisible(panelWindow))
        {
            GetWindowRect(panelWindow, &captionRect);
            captionRect.bottom = captionRect.top + ScaleMetric(28);
        }

        return captionRect;
    }

    std::optional<EditorShell::EditorPanelId> EditorShell::HitTestPanelCaption(POINT cursorScreenPoint) const
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
        CollectReachableDockLeaves(m_rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
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

    std::optional<EditorShell::EditorPanelId> EditorShell::HitTestDockTab(POINT cursorScreenPoint) const
    {
        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
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
        CollectReachableDockLeaves(m_rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
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
        CollectReachableDockSplits(m_rootDockNodeId, dockSplitNodeIds);
        for (DockNodeId dockNodeId : dockSplitNodeIds)
        {
            const DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Split != dockNode.kind
                || dockNode.firstChild < 0
                || dockNode.secondChild < 0
                || static_cast<std::size_t>(dockNode.firstChild) >= m_dockNodes.size()
                || static_cast<std::size_t>(dockNode.secondChild) >= m_dockNodes.size())
            {
                continue;
            }

            const RECT firstRect = m_dockNodes[static_cast<std::size_t>(dockNode.firstChild)].rect;
            const RECT secondRect = m_dockNodes[static_cast<std::size_t>(dockNode.secondChild)].rect;
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
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_dockNodes.size())
        {
            return;
        }

        const DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
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
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_dockNodes.size())
        {
            return;
        }

        const DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
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
            || m_dragSplitNodeId < 0
            || static_cast<std::size_t>(m_dragSplitNodeId) >= m_dockNodes.size())
        {
            return false;
        }

        DockNode& splitNode = m_dockNodes[static_cast<std::size_t>(m_dragSplitNodeId)];
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
            ? cursorScreenPoint.x - m_dragStartScreenPoint.x
            : cursorScreenPoint.y - m_dragStartScreenPoint.y;
        const float nextRatio = ClampDockSplitRatio(
            m_dragSplitNodeId,
            m_dragStartSplitRatio + static_cast<float>(cursorDelta) / static_cast<float>(availableLength));
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
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_dockNodes.size())
        {
            return ratio;
        }

        const DockNode& splitNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
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
        CollectReachableDockLeaves(m_rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            const DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
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
        CollectReachableDockLeaves(m_rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
            if (DockNodeKind::Leaf != dockNode.kind)
            {
                continue;
            }

            RemovePanelFromDockNode(dockNode, panelId);
        }

        if (collapseEmptyLeaves && m_rootDockNodeId >= 0)
        {
            (void)CollapseEmptyDockLeaves(m_rootDockNodeId);
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
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_dockNodes.size())
        {
            return false;
        }

        DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
        if (DockNodeKind::Leaf == dockNode.kind)
        {
            return dockNode.panels.empty();
        }

        const bool firstEmpty = CollapseEmptyDockLeaves(dockNode.firstChild);
        const bool secondEmpty = CollapseEmptyDockLeaves(dockNode.secondChild);
        if (firstEmpty && false == secondEmpty)
        {
            dockNode = m_dockNodes[static_cast<std::size_t>(dockNode.secondChild)];
            return false;
        }

        if (secondEmpty && false == firstEmpty)
        {
            dockNode = m_dockNodes[static_cast<std::size_t>(dockNode.firstChild)];
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

        for (std::size_t index = 0; index < m_dockGuideWindows.size(); ++index)
        {
            if (false == hitVisibleGuide(m_dockGuideWindows[index]))
            {
                continue;
            }

            const DockGuideTargetKind kind = guideKinds[index];
            const DockNodeId dockNodeId = index < 5 ? m_currentGuideTarget.dockNodeId : m_rootDockNodeId;
            if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_dockNodes.size())
            {
                return DockGuideTarget{};
            }

            const RECT sourceRect = index < 5
                ? m_dockNodes[static_cast<std::size_t>(dockNodeId)].rect
                : m_dockNodes[static_cast<std::size_t>(m_rootDockNodeId)].rect;
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

        if (guideTarget.dockNodeId < 0 || static_cast<std::size_t>(guideTarget.dockNodeId) >= m_dockNodes.size())
        {
            return;
        }

        const DockNodeId sourceLeafNodeId = FindPanelDockLeaf(panelId);
        HWND sourceTabControl = GetDockLeafTabControl(sourceLeafNodeId);
        if (nullptr == sourceTabControl)
        {
            sourceTabControl = m_centerDockTab;
        }

        RemovePanelFromDockTree(panelId, false);
        if (guideTarget.dockNodeId < 0 || static_cast<std::size_t>(guideTarget.dockNodeId) >= m_dockNodes.size())
        {
            return;
        }

        DockNode& targetNode = m_dockNodes[static_cast<std::size_t>(guideTarget.dockNodeId)];
        if (DockGuideTargetKind::Tab == guideTarget.kind && DockNodeKind::Leaf == targetNode.kind)
        {
            DestroyFloatingWindow(panelId);
            SetPanelParent(panelId, parentWindow);
            targetNode.panels.push_back(panelId);
            targetNode.activeTabIndex = static_cast<int>(targetNode.panels.size()) - 1;
            if (m_rootDockNodeId >= 0)
            {
                (void)CollapseEmptyDockLeaves(m_rootDockNodeId);
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

        m_dockNodes[targetNodeIndex] = splitNode;
        if (m_rootDockNodeId >= 0)
        {
            (void)CollapseEmptyDockLeaves(m_rootDockNodeId);
        }
        SyncDockTabs();
        m_layoutInitialized = false;
    }

    void EditorShell::UpdateDockGuideWindows(HWND parentWindow, POINT cursorScreenPoint)
    {
        if (nullptr == parentWindow || DockDragKind::Panel != m_dragKind)
        {
            HideDockGuideWindows();
            return;
        }

        POINT cursorClientPoint = cursorScreenPoint;
        ScreenToClient(parentWindow, &cursorClientPoint);
        const DockNodeId hitLeafNodeId = HitTestDockLeaf(cursorClientPoint);
        m_currentGuideTarget.dockNodeId = hitLeafNodeId;

        const int guideSize = ScaleMetric(40);
        const int guideGap = ScaleMetric(3);
        if (0 <= hitLeafNodeId && static_cast<std::size_t>(hitLeafNodeId) < m_dockNodes.size())
        {
            const RECT leafRect = m_dockNodes[static_cast<std::size_t>(hitLeafNodeId)].rect;
            const int centerX = (leafRect.left + leafRect.right) / 2;
            const int centerY = (leafRect.top + leafRect.bottom) / 2;
            ShowDockGuideWindow(m_dockGuideWindows[4], RECT{ centerX - guideSize / 2, centerY - guideSize / 2, centerX + guideSize / 2, centerY + guideSize / 2 });
            ShowDockGuideWindow(m_dockGuideWindows[0], RECT{ centerX - guideSize / 2, centerY - guideSize - guideGap - guideSize / 2, centerX + guideSize / 2, centerY - guideGap - guideSize / 2 });
            ShowDockGuideWindow(m_dockGuideWindows[1], RECT{ centerX - guideSize / 2, centerY + guideGap + guideSize / 2, centerX + guideSize / 2, centerY + guideSize + guideGap + guideSize / 2 });
            ShowDockGuideWindow(m_dockGuideWindows[2], RECT{ centerX - guideSize - guideGap - guideSize / 2, centerY - guideSize / 2, centerX - guideGap - guideSize / 2, centerY + guideSize / 2 });
            ShowDockGuideWindow(m_dockGuideWindows[3], RECT{ centerX + guideGap + guideSize / 2, centerY - guideSize / 2, centerX + guideSize + guideGap + guideSize / 2, centerY + guideSize / 2 });
        }
        else
        {
            for (std::size_t index = 0; index < 5; ++index)
            {
                ShowWindow(m_dockGuideWindows[index], SW_HIDE);
            }
        }

        const RECT rootRect = 0 <= m_rootDockNodeId && static_cast<std::size_t>(m_rootDockNodeId) < m_dockNodes.size()
            ? m_dockNodes[static_cast<std::size_t>(m_rootDockNodeId)].rect
            : RECT{};
        const int rootCenterX = (rootRect.left + rootRect.right) / 2;
        const int rootCenterY = (rootRect.top + rootRect.bottom) / 2;
        ShowDockGuideWindow(m_dockGuideWindows[5], RECT{ rootCenterX - guideSize / 2, rootRect.top + guideGap, rootCenterX + guideSize / 2, rootRect.top + guideGap + guideSize });
        ShowDockGuideWindow(m_dockGuideWindows[6], RECT{ rootCenterX - guideSize / 2, rootRect.bottom - guideGap - guideSize, rootCenterX + guideSize / 2, rootRect.bottom - guideGap });
        ShowDockGuideWindow(m_dockGuideWindows[7], RECT{ rootRect.left + guideGap, rootCenterY - guideSize / 2, rootRect.left + guideGap + guideSize, rootCenterY + guideSize / 2 });
        ShowDockGuideWindow(m_dockGuideWindows[8], RECT{ rootRect.right - guideGap - guideSize, rootCenterY - guideSize / 2, rootRect.right - guideGap, rootCenterY + guideSize / 2 });
    }

    void EditorShell::HideDockGuideWindows()
    {
        for (HWND guideWindow : m_dockGuideWindows)
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
        if (cursorClientPoint.x < outerPadding + m_leftPaneWidth)
        {
            return cursorClientPoint.y < outerPadding + m_leftTopHeight + panelSpacing / 2
                ? DockAreaId::LeftTop
                : DockAreaId::LeftBottom;
        }

        if (cursorClientPoint.x > clientRect.right - outerPadding - m_rightPaneWidth)
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

        removePanel(m_leftTopDockPanels);
        removePanel(m_leftBottomDockPanels);
        removePanel(m_centerDockPanels);
        removePanel(m_rightDockPanels);

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
            m_leftTopActiveTabIndex = static_cast<int>(panels.size()) - 1;
            break;
        case DockAreaId::LeftBottom:
            m_leftBottomActiveTabIndex = static_cast<int>(panels.size()) - 1;
            break;
        case DockAreaId::Center:
            m_centerActiveTabIndex = static_cast<int>(panels.size()) - 1;
            break;
        case DockAreaId::Right:
            m_rightActiveTabIndex = static_cast<int>(panels.size()) - 1;
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

        m_dragPanelWindowOffset = POINT{
            static_cast<LONG>((std::max)(ScaleMetric(8), static_cast<int>(cursorScreenPoint.x - captionRect.left))),
            static_cast<LONG>((std::max)(ScaleMetric(8), static_cast<int>(cursorScreenPoint.y - captionRect.top)))
        };

        RemovePanelFromDockTree(panelId);
        const POINT floatingOrigin{
            cursorScreenPoint.x - m_dragPanelWindowOffset.x,
            cursorScreenPoint.y - m_dragPanelWindowOffset.y
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
            cursorScreenPoint.x - m_dragPanelWindowOffset.x,
            cursorScreenPoint.y - m_dragPanelWindowOffset.y,
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
        m_floatingPanelGroups.push_back(FloatingPanelGroup{
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

        if (DockDragKind::Panel == m_dragKind && m_dragPanelId.has_value() && *m_dragPanelId == panelId)
        {
            return;
        }

        m_dragKind = DockDragKind::Panel;
        m_dragPanelId = panelId;
        m_dragStartScreenPoint = GetCursorScreenPoint(m_cursor);
        m_currentGuideTarget = DockGuideTarget{};
        m_hasDockPreview = false;
    }

    void EditorShell::UpdateFloatingWindowDockDrag()
    {
        if (nullptr == m_parentWindow || DockDragKind::Panel != m_dragKind || false == m_dragPanelId.has_value())
        {
            return;
        }

        const POINT cursorScreenPoint = GetCursorScreenPoint(m_cursor);
        UpdateDockGuideWindows(m_parentWindow, cursorScreenPoint);
        m_currentGuideTarget = HitTestDockGuideTarget(m_parentWindow, cursorScreenPoint);
        m_hasDockPreview = DockGuideTargetKind::None != m_currentGuideTarget.kind
            && DockGuideTargetKind::Float != m_currentGuideTarget.kind;
        m_dockPreviewRect = m_currentGuideTarget.previewRect;
        UpdateDockPreviewWindow(m_parentWindow);
    }

    void EditorShell::CompleteFloatingWindowDockDrag(EditorPanelId panelId)
    {
        if (nullptr == m_parentWindow)
        {
            return;
        }

        UpdateFloatingWindowDockDrag();
        const DockGuideTarget guideTarget = m_currentGuideTarget;
        m_currentGuideTarget = DockGuideTarget{};
        m_hasDockPreview = false;
        HideDockGuideWindows();
        UpdateDockPreviewWindow(m_parentWindow);
        m_dragKind = DockDragKind::None;
        m_dragPanelId.reset();

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

        FloatingPanelGroup& group = m_floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
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

        switch (activePanelId)
        {
        case EditorPanelId::Hierarchy:
            LayoutHierarchyPanelInRect(panelRect);
            break;
        case EditorPanelId::Assets:
            LayoutAssetsPanelInRect(panelRect);
            break;
        case EditorPanelId::SceneView:
            LayoutSceneViewPanelInRect(panelRect);
            break;
        case EditorPanelId::Inspector:
            LayoutInspectorPanelInRect(panelRect);
            break;
        case EditorPanelId::Sprite:
            LayoutSpritePanelInRect(panelRect);
            break;
        case EditorPanelId::Material:
            LayoutMaterialPanelInRect(panelRect);
            break;
        case EditorPanelId::Collider2D:
            LayoutCollider2DPanelInRect(panelRect);
            break;
        case EditorPanelId::LogOutput:
            LayoutLogOutputPanelInRect(panelRect);
            break;
        default:
            break;
        }

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

        FloatingPanelGroup& targetGroup = m_floatingPanelGroups[static_cast<std::size_t>(targetGroupIndex)];
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
        for (const FloatingPanelGroup& group : m_floatingPanelGroups)
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

        const FloatingPanelGroup group = m_floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
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

        m_floatingPanelGroups.erase(m_floatingPanelGroups.begin() + groupIndex);
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

        FloatingPanelGroup& group = m_floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
        group.panels.erase(std::remove(group.panels.begin(), group.panels.end(), panelId), group.panels.end());
        if (group.activeTabIndex >= static_cast<int>(group.panels.size()))
        {
            group.activeTabIndex = (std::max)(0, static_cast<int>(group.panels.size()) - 1);
        }

        if (group.panels.empty())
        {
            m_floatingPanelGroups.erase(m_floatingPanelGroups.begin() + groupIndex);
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
            return m_hierarchyFloatingWindow;
        case EditorPanelId::Assets:
            return m_assetsFloatingWindow;
        case EditorPanelId::SceneView:
            return m_sceneViewFloatingWindow;
        case EditorPanelId::Inspector:
            return m_inspectorFloatingWindow;
        case EditorPanelId::Sprite:
            return m_spriteFloatingWindow;
        case EditorPanelId::Material:
            return m_materialFloatingWindow;
        case EditorPanelId::Collider2D:
            return m_collider2DFloatingWindow;
        case EditorPanelId::LogOutput:
            return m_logOutputFloatingWindow;
        default:
            return m_sceneViewFloatingWindow;
        }
    }

    void EditorShell::SyncFloatingPanelTabs(HWND floatingWindow)
    {
        const int groupIndex = FindFloatingPanelGroupIndex(floatingWindow);
        if (groupIndex < 0)
        {
            return;
        }

        FloatingPanelGroup& group = m_floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
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
        for (std::size_t index = 0; index < m_floatingPanelGroups.size(); ++index)
        {
            if (m_floatingPanelGroups[index].window == floatingWindow)
            {
                return static_cast<int>(index);
            }
        }

        return -1;
    }

    int EditorShell::FindFloatingPanelGroupIndex(EditorPanelId panelId) const
    {
        for (std::size_t index = 0; index < m_floatingPanelGroups.size(); ++index)
        {
            const std::vector<EditorPanelId>& panels = m_floatingPanelGroups[index].panels;
            if (panels.end() != std::find(panels.begin(), panels.end(), panelId))
            {
                return static_cast<int>(index);
            }
        }

        return -1;
    }

    EditorShell::EditorPanelId EditorShell::GetActiveFloatingPanel(HWND floatingWindow) const
    {
        const int groupIndex = FindFloatingPanelGroupIndex(floatingWindow);
        if (groupIndex < 0)
        {
            return EditorPanelId::SceneView;
        }

        const FloatingPanelGroup& group = m_floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
        if (group.panels.empty())
        {
            return EditorPanelId::SceneView;
        }

        const int activeIndex = (std::max)(0, (std::min)(group.activeTabIndex, static_cast<int>(group.panels.size()) - 1));
        return group.panels[static_cast<std::size_t>(activeIndex)];
    }

    void EditorShell::SyncDockTabs()
    {
        for (HWND tabControl : { m_leftTopDockTab, m_leftBottomDockTab, m_centerDockTab, m_rightDockTab })
        {
            if (nullptr != tabControl)
            {
                TabCtrl_DeleteAllItems(tabControl);
                ShowWindow(tabControl, SW_HIDE);
            }
        }
        for (HWND tabControl : m_dynamicDockTabs)
        {
            if (nullptr != tabControl)
            {
                TabCtrl_DeleteAllItems(tabControl);
                ShowWindow(tabControl, SW_HIDE);
            }
        }

        std::vector<DockNodeId> dockLeafNodeIds{};
        CollectReachableDockLeaves(m_rootDockNodeId, dockLeafNodeIds);
        for (DockNodeId dockNodeId : dockLeafNodeIds)
        {
            DockNode& dockNode = m_dockNodes[static_cast<std::size_t>(dockNodeId)];
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
        m_dynamicDockTabs.push_back(tabControl);
        return tabControl;
    }

    HWND EditorShell::CreateDockTabControlForLayoutKey(const std::wstring& layoutKey)
    {
        if (L"LeftTop" == layoutKey)
        {
            return m_leftTopDockTab;
        }
        if (L"LeftBottom" == layoutKey)
        {
            return m_leftBottomDockTab;
        }
        if (L"Center" == layoutKey)
        {
            return m_centerDockTab;
        }
        if (L"Right" == layoutKey)
        {
            return m_rightDockTab;
        }

        HWND tabControl = CreateAdditionalDockTabControl(m_parentWindow);
        if (L"LogOutput" == layoutKey)
        {
            m_logOutputDockTab = tabControl;
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

    bool EditorShell::DrawEditorButton(const DRAWITEMSTRUCT& drawItem) const
    {
        if (ODT_BUTTON != drawItem.CtlType || nullptr == drawItem.hwndItem || nullptr == drawItem.hDC)
        {
            return false;
        }

        wchar_t buttonText[128]{};
        GetWindowTextW(drawItem.hwndItem, buttonText, static_cast<int>(std::size(buttonText)));

        const bool isPressed = 0 != (drawItem.itemState & ODS_SELECTED);
        const bool isDisabled = 0 != (drawItem.itemState & ODS_DISABLED);
        const bool isDestructive =
            nullptr != wcsstr(buttonText, L"Remove")
            || nullptr != wcsstr(buttonText, L"Delete")
            || nullptr != wcsstr(buttonText, L"Clear");
        const bool isDefaultAction =
            drawItem.hwndItem == m_topBarPlayButton
            || drawItem.hwndItem == m_buildAndPlayButton
            || 0 == wcscmp(buttonText, L"All")
            || 0 == wcscmp(buttonText, L"Add Component")
            || 0 == wcscmp(buttonText, L"New");

        EditorColor backgroundColor = isDestructive
            ? EditorColor::FromRgb8(0x3A, 0x18, 0x24)
            : (isDefaultAction ? EditorThemes::XelqoriaDark.selection : EditorThemes::XelqoriaDark.panelHeaderBackground);
        if (isPressed)
        {
            backgroundColor = EditorThemes::XelqoriaDark.accent;
        }

        const EditorColor borderColor = isDestructive
            ? EditorThemes::XelqoriaDark.error
            : (isDefaultAction
            ? EditorThemes::XelqoriaDark.accent
            : EditorThemes::XelqoriaDark.panelBorder);
        const EditorColor textColor = isDisabled
            ? EditorThemes::XelqoriaDark.textSecondary
            : EditorThemes::XelqoriaDark.textPrimary;

        RECT buttonRect = drawItem.rcItem;
        FillRoundRectWithThemeColor(drawItem.hDC, buttonRect, backgroundColor, ScaleMetric(6));
        DrawRoundRectBorder(drawItem.hDC, buttonRect, borderColor, ScaleMetric(6));

        SetBkMode(drawItem.hDC, TRANSPARENT);
        SetTextColor(drawItem.hDC, ToColorRef(textColor));
        RECT textRect = buttonRect;
        textRect.left += ScaleMetric(8);
        textRect.right -= ScaleMetric(8);
        DrawTextW(
            drawItem.hDC,
            buttonText,
            -1,
            &textRect,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        return true;
    }

    bool EditorShell::DrawHierarchyListBoxItem(const DRAWITEMSTRUCT& drawItem) const
    {
        if (ODT_LISTBOX != drawItem.CtlType
            || drawItem.hwndItem != m_hierarchyListBox
            || nullptr == drawItem.hDC)
        {
            return false;
        }

        if (static_cast<UINT>(-1) == drawItem.itemID)
        {
            FillRectWithThemeColor(drawItem.hDC, drawItem.rcItem, EditorThemes::XelqoriaDark.panelBackground);
            return true;
        }

        const bool isSelected = 0 != (drawItem.itemState & ODS_SELECTED);
        const bool isHovered = static_cast<int>(drawItem.itemID) == GetHoveredListBoxIndex(drawItem.hwndItem);

        EditorColor backgroundColor = EditorThemes::XelqoriaDark.panelBackground;
        EditorColor textColor = EditorThemes::XelqoriaDark.textPrimary;
        if (isHovered)
        {
            backgroundColor = EditorThemes::XelqoriaDark.hover;
        }
        if (isSelected)
        {
            backgroundColor = EditorThemes::XelqoriaDark.selection;
        }

        FillRectWithThemeColor(drawItem.hDC, drawItem.rcItem, backgroundColor);

        wchar_t itemText[256]{};
        SendMessageW(
            drawItem.hwndItem,
            LB_GETTEXT,
            drawItem.itemID,
            reinterpret_cast<LPARAM>(itemText));

        SetBkMode(drawItem.hDC, TRANSPARENT);
        SetTextColor(drawItem.hDC, ToColorRef(textColor));

        RECT textRect = drawItem.rcItem;
        textRect.left += ScaleMetric(8);
        textRect.right -= ScaleMetric(8);
        DrawTextW(
            drawItem.hDC,
            itemText,
            -1,
            &textRect,
            DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        return true;
    }

    bool EditorShell::DrawInspectorSectionLabel(const DRAWITEMSTRUCT& drawItem) const
    {
        if (ODT_STATIC != drawItem.CtlType
            || nullptr == drawItem.hwndItem
            || nullptr == drawItem.hDC
            || (drawItem.hwndItem != m_transformSectionLabel
                && drawItem.hwndItem != m_spriteComponentSectionLabel
                && drawItem.hwndItem != m_materialDetailsSectionLabel
                && drawItem.hwndItem != m_collider2DComponentSectionLabel))
        {
            return false;
        }

        wchar_t sectionText[128]{};
        GetWindowTextW(drawItem.hwndItem, sectionText, static_cast<int>(std::size(sectionText)));
        const bool isHighlighted = 0 == wcsncmp(sectionText, L"> ", 2);

        RECT sectionRect = drawItem.rcItem;
        FillRectWithThemeColor(
            drawItem.hDC,
            sectionRect,
            isHighlighted ? EditorThemes::XelqoriaDark.selection : EditorThemes::XelqoriaDark.panelHeaderBackground);
        DrawRectBorder(
            drawItem.hDC,
            sectionRect,
            isHighlighted ? EditorThemes::XelqoriaDark.accent : EditorThemes::XelqoriaDark.panelBorder);

        RECT accentRect = sectionRect;
        const LONG accentWidth = static_cast<LONG>(ScaleMetric(4));
        accentRect.right = (std::min)(accentRect.right, accentRect.left + accentWidth);
        FillRectWithThemeColor(drawItem.hDC, accentRect, EditorThemes::XelqoriaDark.accent);

        SetBkMode(drawItem.hDC, TRANSPARENT);
        SetTextColor(drawItem.hDC, ToColorRef(EditorThemes::XelqoriaDark.textPrimary));

        RECT textRect = sectionRect;
        textRect.left += ScaleMetric(12);
        textRect.right -= ScaleMetric(8);
        DrawTextW(
            drawItem.hDC,
            sectionText,
            -1,
            &textRect,
            DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        return true;
    }

    std::optional<LRESULT> EditorShell::DrawAssetsListViewItem(LPARAM customDrawParameter) const
    {
        const NMLVCUSTOMDRAW* customDraw = reinterpret_cast<const NMLVCUSTOMDRAW*>(customDrawParameter);
        if (nullptr == customDraw
            || customDraw->nmcd.hdr.hwndFrom != m_assetsListView
            || customDraw->nmcd.hdr.code != NM_CUSTOMDRAW)
        {
            return std::nullopt;
        }

        if (CDDS_PREPAINT == customDraw->nmcd.dwDrawStage)
        {
            return CDRF_NOTIFYITEMDRAW;
        }

        if (CDDS_ITEMPREPAINT != customDraw->nmcd.dwDrawStage)
        {
            return CDRF_DODEFAULT;
        }

        NMLVCUSTOMDRAW* mutableCustomDraw = const_cast<NMLVCUSTOMDRAW*>(customDraw);
        mutableCustomDraw->nmcd.uItemState &= ~(CDIS_SELECTED | CDIS_HOT);

        mutableCustomDraw->clrText = ToColorRef(EditorThemes::XelqoriaDark.textPrimary);
        mutableCustomDraw->clrTextBk = ToColorRef(EditorThemes::XelqoriaDark.panelBackground);
        return CDRF_NEWFONT;
    }

    std::optional<LRESULT> EditorShell::DrawAssetsListViewHeader(LPARAM customDrawParameter) const
    {
        const NMCUSTOMDRAW* customDraw = reinterpret_cast<const NMCUSTOMDRAW*>(customDrawParameter);
        const HWND headerWindow = nullptr != m_assetsListView ? ListView_GetHeader(m_assetsListView) : nullptr;
        if (nullptr == customDraw
            || nullptr == headerWindow
            || customDraw->hdr.hwndFrom != headerWindow
            || customDraw->hdr.code != NM_CUSTOMDRAW)
        {
            return std::nullopt;
        }

        if (CDDS_PREPAINT == customDraw->dwDrawStage)
        {
            RECT clientRect{};
            GetClientRect(headerWindow, &clientRect);
            FillRectWithThemeColor(customDraw->hdc, clientRect, EditorThemes::XelqoriaDark.panelHeaderBackground);
            return CDRF_NOTIFYITEMDRAW;
        }

        if (CDDS_ITEMPREPAINT != customDraw->dwDrawStage)
        {
            return CDRF_DODEFAULT;
        }

        const int columnIndex = static_cast<int>(customDraw->dwItemSpec);
        RECT itemRect{};
        if (FALSE == Header_GetItemRect(headerWindow, columnIndex, &itemRect))
        {
            return CDRF_DODEFAULT;
        }

        FillRectWithThemeColor(customDraw->hdc, itemRect, EditorThemes::XelqoriaDark.panelHeaderBackground);
        DrawRectBorder(customDraw->hdc, itemRect, EditorThemes::XelqoriaDark.panelBorder);

        wchar_t text[128]{};
        HDITEMW item{};
        item.mask = HDI_TEXT | HDI_FORMAT;
        item.pszText = text;
        item.cchTextMax = static_cast<int>(std::size(text));
        Header_GetItem(headerWindow, columnIndex, &item);

        RECT textRect = itemRect;
        textRect.left += ScaleMetric(8);
        textRect.right -= ScaleMetric(8);

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

        SetBkMode(customDraw->hdc, TRANSPARENT);
        SetTextColor(customDraw->hdc, ToColorRef(EditorThemes::XelqoriaDark.textPrimary));
        DrawTextW(customDraw->hdc, text, -1, &textRect, textFormat);
        return CDRF_SKIPDEFAULT;
    }

    std::wstring EditorShell::GetDockTabLayoutKey(HWND tabControl) const
    {
        if (tabControl == m_leftTopDockTab)
        {
            return L"LeftTop";
        }
        if (tabControl == m_leftBottomDockTab)
        {
            return L"LeftBottom";
        }
        if (tabControl == m_centerDockTab)
        {
            return L"Center";
        }
        if (tabControl == m_rightDockTab)
        {
            return L"Right";
        }
        if (tabControl == m_logOutputDockTab)
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
            return m_leftTopDockTab;
        case DockAreaId::LeftBottom:
            return m_leftBottomDockTab;
        case DockAreaId::Center:
            return m_centerDockTab;
        case DockAreaId::Right:
            return m_rightDockTab;
        default:
            return nullptr;
        }
    }

    HWND EditorShell::GetDockLeafTabControl(DockNodeId dockNodeId) const
    {
        if (dockNodeId < 0 || static_cast<std::size_t>(dockNodeId) >= m_dockNodes.size())
        {
            return nullptr;
        }

        return m_dockNodes[static_cast<std::size_t>(dockNodeId)].tabControl;
    }

    std::vector<EditorShell::EditorPanelId>& EditorShell::GetDockAreaPanels(DockAreaId dockAreaId)
    {
        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            return m_leftTopDockPanels;
        case DockAreaId::LeftBottom:
            return m_leftBottomDockPanels;
        case DockAreaId::Center:
            return m_centerDockPanels;
        case DockAreaId::Right:
            return m_rightDockPanels;
        default:
            return m_centerDockPanels;
        }
    }

    const std::vector<EditorShell::EditorPanelId>& EditorShell::GetDockAreaPanels(DockAreaId dockAreaId) const
    {
        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            return m_leftTopDockPanels;
        case DockAreaId::LeftBottom:
            return m_leftBottomDockPanels;
        case DockAreaId::Center:
            return m_centerDockPanels;
        case DockAreaId::Right:
            return m_rightDockPanels;
        default:
            return m_centerDockPanels;
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
            activeIndex = m_leftTopActiveTabIndex;
            break;
        case DockAreaId::LeftBottom:
            activeIndex = m_leftBottomActiveTabIndex;
            break;
        case DockAreaId::Center:
            activeIndex = m_centerActiveTabIndex;
            break;
        case DockAreaId::Right:
            activeIndex = m_rightActiveTabIndex;
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
        const int centerWidth = (std::max)(ScaleMetric(120), availableColumnWidth - m_leftPaneWidth - m_rightPaneWidth);
        const int centerX = outerPadding + m_leftPaneWidth + panelSpacing;
        const int rightX = centerX + centerWidth + panelSpacing;
        const int dockHeight = (std::max)(0, clientHeight - outerPadding * 2);

        switch (dockAreaId)
        {
        case DockAreaId::LeftTop:
            return RECT{
                outerPadding,
                outerPadding,
                outerPadding + m_leftPaneWidth,
                outerPadding + m_leftTopHeight
            };
        case DockAreaId::LeftBottom:
            return RECT{
                outerPadding,
                outerPadding + m_leftTopHeight + panelSpacing,
                outerPadding + m_leftPaneWidth,
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
                rightX + m_rightPaneWidth,
                outerPadding + dockHeight
            };
        default:
            return RECT{};
        }
    }

    void EditorShell::UpdateDockPreviewWindow(HWND parentWindow)
    {
        if (nullptr == m_dockPreviewWindow || nullptr == parentWindow)
        {
            return;
        }

        if (false == m_hasDockPreview)
        {
            ShowWindow(m_dockPreviewWindow, SW_HIDE);
            return;
        }

        SetWindowPos(
            m_dockPreviewWindow,
            HWND_TOP,
            m_dockPreviewRect.left,
            m_dockPreviewRect.top,
            (std::max)(0, static_cast<int>(m_dockPreviewRect.right - m_dockPreviewRect.left)),
            (std::max)(0, static_cast<int>(m_dockPreviewRect.bottom - m_dockPreviewRect.top)),
            SWP_NOACTIVATE | SWP_SHOWWINDOW);
        InvalidateRect(m_dockPreviewWindow, nullptr, TRUE);
        UpdateWindow(m_dockPreviewWindow);

        for (HWND guideWindow : m_dockGuideWindows)
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

    void EditorShell::MoveChildWindowNoRedraw(HWND window, int x, int y, int width, int height) const
    {
        if (nullptr == window)
        {
            return;
        }

        const PendingLayoutMove pendingMove{
            window,
            x,
            y,
            (std::max)(0, width),
            (std::max)(0, height)
        };
        auto existingMove = std::find_if(
            m_pendingLayoutMoves.begin(),
            m_pendingLayoutMoves.end(),
            [window](const PendingLayoutMove& move)
            {
                return move.window == window;
            });
        if (existingMove == m_pendingLayoutMoves.end())
        {
            m_pendingLayoutMoves.push_back(pendingMove);
            return;
        }

        *existingMove = pendingMove;
    }

    void EditorShell::FlushLayoutMoves(HWND parentWindow) const
    {
        if (m_pendingLayoutMoves.empty())
        {
            return;
        }

        HDWP deferredPosition = BeginDeferWindowPos(static_cast<int>(m_pendingLayoutMoves.size()));
        if (nullptr == deferredPosition)
        {
            for (const PendingLayoutMove& move : m_pendingLayoutMoves)
            {
                SetWindowPos(
                    move.window,
                    nullptr,
                    move.x,
                    move.y,
                    move.width,
                    move.height,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }
            m_pendingLayoutMoves.clear();
            return;
        }

        bool deferFailed = false;
        for (const PendingLayoutMove& move : m_pendingLayoutMoves)
        {
            deferredPosition = DeferWindowPos(
                deferredPosition,
                move.window,
                nullptr,
                move.x,
                move.y,
                move.width,
                move.height,
                SWP_NOZORDER | SWP_NOACTIVATE);
            if (nullptr == deferredPosition)
            {
                deferFailed = true;
                break;
            }
        }

        if (deferFailed)
        {
            for (const PendingLayoutMove& move : m_pendingLayoutMoves)
            {
                SetWindowPos(
                    move.window,
                    nullptr,
                    move.x,
                    move.y,
                    move.width,
                    move.height,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        else
        {
            EndDeferWindowPos(deferredPosition);
        }
        m_pendingLayoutMoves.clear();
    }

    void EditorShell::RedrawLayout(HWND parentWindow) const
    {
        if (nullptr == parentWindow)
        {
            return;
        }

        FlushLayoutMoves(parentWindow);
        RedrawWindow(
            parentWindow,
            nullptr,
            nullptr,
            RDW_INVALIDATE | RDW_FRAME);
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

        const std::array<HWND, 121> controls = CollectControls();

        for (HWND control : controls)
        {
            if (nullptr != control)
            {
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(m_defaultFont), TRUE);
            }
        }

        if (nullptr != m_hierarchyListBox)
        {
            SendMessageW(m_hierarchyListBox, LB_SETITEMHEIGHT, 0, static_cast<LPARAM>(ScaleMetric(EditorRowHeight)));
        }

        if (ownedPreviousFont && nullptr != previousFont)
        {
            DeleteObject(previousFont);
        }

        return true;
    }

    std::array<HWND, 121> EditorShell::CollectControls() const
    {
        return {
            m_workspaceBackground,
            m_topBar,
            m_topBarProjectButton,
            m_topBarPlayButton,
            m_topBarLayoutButton,
            m_statusBar,
            m_leftTopDockTab,
            m_leftBottomDockTab,
            m_centerDockTab,
            m_rightDockTab,
            m_dockPreviewWindow,
            m_dockGuideWindows[0],
            m_dockGuideWindows[1],
            m_dockGuideWindows[2],
            m_dockGuideWindows[3],
            m_dockGuideWindows[4],
            m_dockGuideWindows[5],
            m_dockGuideWindows[6],
            m_dockGuideWindows[7],
            m_dockGuideWindows[8],
            m_hierarchyPanel,
            m_assetsPanel,
            m_inspectorPanel,
            m_spritePanel,
            m_materialPanel,
            m_collider2DPanel,
            m_sceneViewPanel,
            m_sceneViewPlanLabel,
            m_projectSummaryLabel,
            m_projectSceneListBox,
            m_projectSceneDetailLabel,
            m_sceneViewHost,
            m_sceneViewSizeLabel,
            m_buildAndPlayButton,
            m_pauseResumePlayButton,
            m_endPlayButton,
            m_logOutputPanel,
            m_logOutputTabControl,
            m_logClearButton,
            m_logCopyButton,
            m_logFilterEdit,
            m_logListBox,
            m_assetsListView,
            m_assetsSummaryLabel,
            m_hierarchySummaryLabel,
            m_hierarchySearchEdit,
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
            m_materialOpenButton,
            m_materialSummaryLabel,
            m_materialSharedNoticeLabel,
            m_materialDetailsSectionLabel,
            m_materialDetailLabels[0],
            m_materialDetailLabels[1],
            m_materialDetailLabels[2],
            m_materialDetailLabels[3],
            m_materialDetailLabels[4],
            m_materialDetailEditControls[0],
            m_materialDetailEditControls[1],
            m_materialDetailEditControls[2],
            m_materialDetailEditControls[3],
            m_materialDetailEditControls[4],
            m_materialTextureDropHighlight,
            m_materialTextureBrowseButton,
            m_materialTintColorButton,
            m_materialOutlineEnabledCheckBox,
            m_materialOutlineColorButton,
            m_collider2DSummaryLabel,
            m_spriteSummaryLabel,
            m_spriteDetailsSectionLabel,
            m_spriteDetailLabels[0],
            m_spriteDetailLabels[1],
            m_spriteDetailLabels[2],
            m_spriteDetailLabels[3],
            m_spriteDetailEditControls[0],
            m_spriteDetailEditControls[1],
            m_spriteDetailEditControls[2],
            m_spriteDetailEditControls[3],
            m_scriptAssetLabel,
            m_scriptAssetEdit,
            m_scriptCreateButton,
            m_scriptAssignButton,
            m_scriptClearButton,
            m_spriteComponentActionButton,
            m_collider2DComponentSectionLabel,
            m_collider2DEnabledCheckBox,
            m_collider2DTriggerCheckBox,
            m_collider2DShapeTypeLabel,
            m_collider2DShapeTypeEdit,
            m_collider2DOffsetLabel,
            m_collider2DSizeLabel,
            m_collider2DRotationLabel,
            m_collider2DRotationEdit,
            m_collider2DEditControls[0],
            m_collider2DEditControls[1],
            m_collider2DEditControls[2],
            m_collider2DEditControls[3],
            m_collider2DEditButton,
            m_collider2DComponentActionButton,
            m_addComponentButton
        };
    }

    int EditorShell::ScaleMetric(int value) const
    {
        return MulDiv(value, static_cast<int>(m_currentDpi), 96);
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
                    && notifyHeader->hwndFrom == windowData->shell->m_floatingPanelGroups[static_cast<std::size_t>(groupIndex)].tabControl)
                {
                    FloatingPanelGroup& group =
                        windowData->shell->m_floatingPanelGroups[static_cast<std::size_t>(groupIndex)];
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

    LRESULT CALLBACK EditorShell::EditorRowControlSubclassProc(
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
            RemoveWindowSubclass(window, EditorShell::EditorRowControlSubclassProc, EditorRowControlSubclassId);
        }

        return DefSubclassProc(window, message, wParam, lParam);
    }

    LRESULT CALLBACK EditorShell::EditorHeaderControlSubclassProc(
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
            RemoveWindowSubclass(window, EditorShell::EditorHeaderControlSubclassProc, EditorHeaderControlSubclassId);
        }

        return DefSubclassProc(window, message, wParam, lParam);
    }

    LRESULT CALLBACK EditorShell::ParentWindowSubclassProc(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam,
        UINT_PTR subclassId,
        DWORD_PTR referenceData)
    {
        (void)subclassId;

        const EditorShell* shell = reinterpret_cast<const EditorShell*>(referenceData);
        if (nullptr != shell)
        {
            const std::optional<LRESULT> result = shell->HandleThemeMessage(message, wParam, lParam);
            if (true == result.has_value())
            {
                return *result;
            }
        }

        return DefSubclassProc(window, message, wParam, lParam);
    }

    std::optional<LRESULT> EditorShell::HandleThemeMessage(UINT message, WPARAM wParam, LPARAM lParam) const
    {
        if (WM_NOTIFY == message)
        {
            const NMHDR* notifyHeader = reinterpret_cast<const NMHDR*>(lParam);
            if (nullptr != notifyHeader
                && notifyHeader->hwndFrom == m_assetsListView
                && notifyHeader->code == NM_CUSTOMDRAW)
            {
                return DrawAssetsListViewItem(lParam);
            }

            const HWND assetsHeader = nullptr != m_assetsListView ? ListView_GetHeader(m_assetsListView) : nullptr;
            if (nullptr != notifyHeader
                && nullptr != assetsHeader
                && notifyHeader->hwndFrom == assetsHeader
                && notifyHeader->code == NM_CUSTOMDRAW)
            {
                return DrawAssetsListViewHeader(lParam);
            }
        }

        if (WM_ERASEBKGND == message)
        {
            HDC deviceContext = reinterpret_cast<HDC>(wParam);
            if (nullptr == deviceContext || nullptr == m_windowBackgroundBrush)
            {
                return std::nullopt;
            }

            RECT clientRect{};
            GetClientRect(m_parentWindow, &clientRect);
            FillRect(deviceContext, &clientRect, m_windowBackgroundBrush);
            return 1;
        }

        if (WM_CTLCOLORSTATIC == message
            || WM_CTLCOLOREDIT == message
            || WM_CTLCOLORLISTBOX == message)
        {
            HDC deviceContext = reinterpret_cast<HDC>(wParam);
            if (nullptr == deviceContext || nullptr == m_panelBackgroundBrush)
            {
                return std::nullopt;
            }

            HWND control = reinterpret_cast<HWND>(lParam);
            if (WM_CTLCOLOREDIT == message && IsInspectorInputControl(control) && nullptr != m_inputBackgroundBrush)
            {
                SetBkMode(deviceContext, OPAQUE);
                SetBkColor(deviceContext, ToColorRef(EditorThemes::XelqoriaDark.panelHeaderBackground));
                SetTextColor(
                    deviceContext,
                    ToColorRef(IsWindowEnabled(control) ? EditorThemes::XelqoriaDark.textPrimary : EditorThemes::XelqoriaDark.textSecondary));
                return reinterpret_cast<LRESULT>(m_inputBackgroundBrush);
            }

            SetBkMode(deviceContext, TRANSPARENT);
            SetBkColor(deviceContext, ToColorRef(EditorThemes::XelqoriaDark.panelBackground));
            SetTextColor(
                deviceContext,
                ToColorRef(IsInspectorSecondaryLabel(control) ? EditorThemes::XelqoriaDark.textSecondary : EditorThemes::XelqoriaDark.textPrimary));
            return reinterpret_cast<LRESULT>(m_panelBackgroundBrush);
        }

        return std::nullopt;
    }

    bool EditorShell::IsInspectorInputControl(HWND window) const
    {
        if (nullptr == window)
        {
            return false;
        }

        for (HWND transformEdit : m_transformEditControls)
        {
            if (window == transformEdit)
            {
                return true;
            }
        }

        for (HWND colliderEdit : m_collider2DEditControls)
        {
            if (window == colliderEdit)
            {
                return true;
            }
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

    bool EditorShell::IsInspectorSecondaryLabel(HWND window) const
    {
        if (nullptr == window)
        {
            return false;
        }

        for (HWND transformLabel : m_transformLabels)
        {
            if (window == transformLabel)
            {
                return true;
            }
        }

        return window == m_inspectorSummaryLabel
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

    HWND EditorShell::GetHierarchySearchEdit() const
    {
        return m_hierarchySearchEdit;
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

    void EditorShell::ConfigureAssetsListHeaderTheme() const
    {
        if (nullptr == m_assetsListView)
        {
            return;
        }

        HWND assetsHeader = ListView_GetHeader(m_assetsListView);
        if (nullptr == assetsHeader)
        {
            return;
        }

        ApplyDarkExplorerTheme(assetsHeader);
        SetWindowSubclass(assetsHeader, EditorHeaderControlSubclassProc, EditorHeaderControlSubclassId, 0);
        RedrawWindow(assetsHeader, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
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

    HWND EditorShell::GetMaterialOpenButton() const
    {
        return m_materialOpenButton;
    }

    HWND EditorShell::GetSpriteRefDropHighlight() const
    {
        return m_spriteRefDropHighlight;
    }

    HWND EditorShell::GetMaterialSummaryLabel() const
    {
        return m_materialSummaryLabel;
    }

    HWND EditorShell::GetMaterialSharedNoticeLabel() const
    {
        return m_materialSharedNoticeLabel;
    }

    HWND EditorShell::GetMaterialDetailsSectionLabel() const
    {
        return m_materialDetailsSectionLabel;
    }

    const std::array<HWND, 5>& EditorShell::GetMaterialDetailLabels() const
    {
        return m_materialDetailLabels;
    }

    const std::array<HWND, 5>& EditorShell::GetMaterialDetailEditControls() const
    {
        return m_materialDetailEditControls;
    }

    HWND EditorShell::GetMaterialTextureDropHighlight() const
    {
        return m_materialTextureDropHighlight;
    }

    HWND EditorShell::GetMaterialTextureBrowseButton() const
    {
        return m_materialTextureBrowseButton;
    }

    HWND EditorShell::GetMaterialTintColorButton() const
    {
        return m_materialTintColorButton;
    }

    HWND EditorShell::GetMaterialOutlineEnabledCheckBox() const
    {
        return m_materialOutlineEnabledCheckBox;
    }

    HWND EditorShell::GetMaterialOutlineColorButton() const
    {
        return m_materialOutlineColorButton;
    }

    HWND EditorShell::GetCollider2DSummaryLabel() const
    {
        return m_collider2DSummaryLabel;
    }

    HWND EditorShell::GetSpriteSummaryLabel() const
    {
        return m_spriteSummaryLabel;
    }

    HWND EditorShell::GetSpriteDetailsSectionLabel() const
    {
        return m_spriteDetailsSectionLabel;
    }

    const std::array<HWND, 4>& EditorShell::GetSpriteDetailLabels() const
    {
        return m_spriteDetailLabels;
    }

    const std::array<HWND, 4>& EditorShell::GetSpriteDetailEditControls() const
    {
        return m_spriteDetailEditControls;
    }

    HWND EditorShell::GetScriptAssetLabel() const
    {
        return m_scriptAssetLabel;
    }

    HWND EditorShell::GetScriptAssetEdit() const
    {
        return m_scriptAssetEdit;
    }

    HWND EditorShell::GetScriptCreateButton() const
    {
        return m_scriptCreateButton;
    }

    HWND EditorShell::GetScriptAssignButton() const
    {
        return m_scriptAssignButton;
    }

    HWND EditorShell::GetScriptClearButton() const
    {
        return m_scriptClearButton;
    }

    HWND EditorShell::GetSpriteComponentActionButton() const
    {
        return m_spriteComponentActionButton;
    }

    HWND EditorShell::GetCollider2DComponentSectionLabel() const
    {
        return m_collider2DComponentSectionLabel;
    }

    HWND EditorShell::GetCollider2DEnabledCheckBox() const
    {
        return m_collider2DEnabledCheckBox;
    }

    HWND EditorShell::GetCollider2DTriggerCheckBox() const
    {
        return m_collider2DTriggerCheckBox;
    }

    HWND EditorShell::GetCollider2DShapeTypeLabel() const
    {
        return m_collider2DShapeTypeLabel;
    }

    HWND EditorShell::GetCollider2DShapeTypeEdit() const
    {
        return m_collider2DShapeTypeEdit;
    }

    HWND EditorShell::GetCollider2DOffsetLabel() const
    {
        return m_collider2DOffsetLabel;
    }

    HWND EditorShell::GetCollider2DSizeLabel() const
    {
        return m_collider2DSizeLabel;
    }

    HWND EditorShell::GetCollider2DRotationLabel() const
    {
        return m_collider2DRotationLabel;
    }

    HWND EditorShell::GetCollider2DRotationEdit() const
    {
        return m_collider2DRotationEdit;
    }

    const std::array<HWND, 4>& EditorShell::GetCollider2DEditControls() const
    {
        return m_collider2DEditControls;
    }

    HWND EditorShell::GetCollider2DEditButton() const
    {
        return m_collider2DEditButton;
    }

    HWND EditorShell::GetCollider2DComponentActionButton() const
    {
        return m_collider2DComponentActionButton;
    }

    HWND EditorShell::GetAddComponentButton() const
    {
        return m_addComponentButton;
    }

    HWND EditorShell::GetSceneViewSizeLabel() const
    {
        return m_sceneViewSizeLabel;
    }

    HWND EditorShell::GetBuildAndPlayButton() const
    {
        return m_buildAndPlayButton;
    }

    HWND EditorShell::GetPauseResumePlayButton() const
    {
        return m_pauseResumePlayButton;
    }

    HWND EditorShell::GetEndPlayButton() const
    {
        return m_endPlayButton;
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

    SceneViewSurface EditorShell::GetSceneViewSurface() const
    {
        return SceneViewSurface{
            m_sceneViewHost,
            m_sceneViewWidth,
            m_sceneViewHeight
        };
    }

    HWND EditorShell::GetLogOutputTabControl() const
    {
        return m_logOutputTabControl;
    }

    HWND EditorShell::GetLogClearButton() const
    {
        return m_logClearButton;
    }

    HWND EditorShell::GetLogCopyButton() const
    {
        return m_logCopyButton;
    }

    HWND EditorShell::GetLogFilterEdit() const
    {
        return m_logFilterEdit;
    }

    HWND EditorShell::GetLogListBox() const
    {
        return m_logListBox;
    }

    HWND EditorShell::CreateChildWindow(
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

        if (handle != nullptr && m_defaultFont != nullptr)
        {
            SendMessageW(handle, WM_SETFONT, reinterpret_cast<WPARAM>(m_defaultFont), TRUE);
        }

        return handle;
    }
}
