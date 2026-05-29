#include "Shell/EditorShell.h"

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

#include "EditorTheme.h"
#include "Panels/Assets/AssetsPanelView.h"
#include "Panels/Hierarchy/HierarchyPanelView.h"
#include "Panels/Inspector/InspectorPanelView.h"
#include "Panels/LogOutput/LogOutputPanelView.h"
#include "Panels/SceneView/SceneViewPanelView.h"

#pragma comment(lib, "uxtheme.lib")

namespace Xelqoria::Editor
{
    EditorShell::EditorShell() = default;

    namespace
    {
        constexpr UINT_PTR ParentWindowSubclassId = 1;
        constexpr UINT_PTR EditorTabControlSubclassId = 2;
        constexpr UINT_PTR EditorRowControlSubclassId = 3;
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
            std::vector<EditorPanelId> panels{};
        };

        struct SavedFloatingGroup
        {
            RECT rect{};
            int activeTabIndex = 0;
            std::vector<EditorPanelId> panels{};
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
            case EditorPanelId::Sprite:
            case EditorPanelId::Material:
            case EditorPanelId::Collider2D:
            default:
                return false;
            }
        }

        [[nodiscard]] const wchar_t* GetPanelLayoutName(EditorPanelId panelId)
        {
            switch (panelId)
            {
            case EditorPanelId::Hierarchy:
                return L"Hierarchy";
            case EditorPanelId::Assets:
                return L"Assets";
            case EditorPanelId::SceneView:
                return L"SceneView";
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
                return L"SceneView";
            }
        }

        [[nodiscard]] std::optional<EditorPanelId> TryParsePanelLayoutName(const std::wstring& name)
        {
            for (EditorPanelId panelId : GetAllEditorPanels())
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
        m_panelViewsInitialized = false;
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
        m_panelHostContext.Bind(m_currentDpi, m_defaultFont, m_pendingLayoutMoves);
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

        m_hierarchyPanelView = std::make_unique<HierarchyPanelView>(m_panelHostContext);
        m_assetsPanelView = std::make_unique<AssetsPanelView>(m_panelHostContext);
        m_inspectorPanelView = std::make_unique<InspectorPanelView>(m_panelHostContext);
        m_sceneViewPanelView = std::make_unique<SceneViewPanelView>(m_panelHostContext);
        m_logOutputPanelView = std::make_unique<LogOutputPanelView>(m_panelHostContext);

        const bool initialized = m_hierarchyPanelView->Initialize(parentWindow, hInstance)
            && m_assetsPanelView->Initialize(parentWindow, hInstance)
            && m_inspectorPanelView->Initialize(parentWindow, hInstance)
            && m_sceneViewPanelView->Initialize(parentWindow, hInstance)
            && m_logOutputPanelView->Initialize(parentWindow, hInstance);
        if (initialized)
        {
            m_panelViewsInitialized = true;
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
            const std::vector<HWND> controls = CollectControls();
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

    #include "EditorShell.Docking.cpp"

    void EditorShell::SendGroupBoxesToBack()
    {
        const std::array<const IEditorPanelView*, 5> panelViews{
            m_hierarchyPanelView.get(),
            m_assetsPanelView.get(),
            m_inspectorPanelView.get(),
            m_sceneViewPanelView.get(),
            m_logOutputPanelView.get()
        };

        for (const IEditorPanelView* panelView : panelViews)
        {
            const HWND rootWindow = nullptr != panelView ? panelView->GetRootWindow() : nullptr;
            if (nullptr != rootWindow)
            {
                SetWindowPos(
                    rootWindow,
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


    bool EditorShell::HandleDrawItem(LPARAM drawItemParameter) const
    {
        if (false == m_panelViewsInitialized)
        {
            return false;
        }

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

    IEditorPanelView& EditorShell::GetPanelView(EditorPanelId panelId) const
    {
        switch (panelId)
        {
        case EditorPanelId::Hierarchy:
            return GetHierarchyPanelView();
        case EditorPanelId::Assets:
            return GetAssetsPanelView();
        case EditorPanelId::SceneView:
            return GetSceneViewPanelView();
        case EditorPanelId::Inspector:
            return GetInspectorPanelView();
        case EditorPanelId::LogOutput:
            return GetLogOutputPanelView();
        default:
            return GetSceneViewPanelView();
        }
    }

    void EditorShell::ShowPanelControls(EditorPanelId panelId, bool visible) const
    {
        GetPanelView(panelId).Show(visible);
    }

    void EditorShell::SetPanelParent(EditorPanelId panelId, HWND parentWindow) const
    {
        GetPanelView(panelId).SetParent(parentWindow);
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
            || drawItem.hwndItem == GetSceneViewPanelView().GetBuildAndPlayButton()
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
            || drawItem.hwndItem != GetHierarchyPanelView().GetListBox()
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
            || false == GetInspectorPanelView().IsSectionLabel(drawItem.hwndItem))
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
            || customDraw->nmcd.hdr.hwndFrom != GetAssetsPanelView().GetListView()
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
        const HWND assetsListView = GetAssetsPanelView().GetListView();
        const HWND headerWindow = nullptr != assetsListView ? ListView_GetHeader(assetsListView) : nullptr;
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

    void EditorShell::MoveChildWindowNoRedraw(HWND window, int x, int y, int width, int height) const
    {
        if (nullptr == window)
        {
            return;
        }

        const EditorPanelLayoutMove pendingMove{
            window,
            x,
            y,
            (std::max)(0, width),
            (std::max)(0, height)
        };
        auto existingMove = std::find_if(
            m_pendingLayoutMoves.begin(),
            m_pendingLayoutMoves.end(),
            [window](const EditorPanelLayoutMove& move)
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
        (void)parentWindow;

        if (m_pendingLayoutMoves.empty())
        {
            return;
        }

        constexpr UINT LayoutMoveFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOCOPYBITS;
        HDWP deferredPosition = BeginDeferWindowPos(static_cast<int>(m_pendingLayoutMoves.size()));
        if (nullptr == deferredPosition)
        {
            for (const EditorPanelLayoutMove& move : m_pendingLayoutMoves)
            {
                SetWindowPos(
                    move.window,
                    nullptr,
                    move.x,
                    move.y,
                    move.width,
                    move.height,
                    LayoutMoveFlags);
            }
            m_pendingLayoutMoves.clear();
            return;
        }

        bool deferFailed = false;
        for (const EditorPanelLayoutMove& move : m_pendingLayoutMoves)
        {
            deferredPosition = DeferWindowPos(
                deferredPosition,
                move.window,
                nullptr,
                move.x,
                move.y,
                move.width,
                move.height,
                LayoutMoveFlags);
            if (nullptr == deferredPosition)
            {
                deferFailed = true;
                break;
            }
        }

        if (deferFailed)
        {
            for (const EditorPanelLayoutMove& move : m_pendingLayoutMoves)
            {
                SetWindowPos(
                    move.window,
                    nullptr,
                    move.x,
                    move.y,
                    move.width,
                    move.height,
                    LayoutMoveFlags);
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
            RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW | RDW_NOERASE);
    }

    bool EditorShell::UpdateSceneViewHostSize()
    {
        const SceneViewSurface sceneViewSurface = GetSceneViewPanelView().GetSceneViewSurface();
        const auto newWidth = static_cast<std::uint32_t>(sceneViewSurface.width);
        const auto newHeight = static_cast<std::uint32_t>(sceneViewSurface.height);
        static std::uint32_t previousWidth = 0;
        static std::uint32_t previousHeight = 0;
        const bool sizeChanged = newWidth != previousWidth || newHeight != previousHeight;

        if (sizeChanged)
        {
            previousWidth = newWidth;
            previousHeight = newHeight;

            wchar_t sizeText[128]{};
            std::swprintf(
                sizeText,
                std::size(sizeText),
                L"SceneView size: %u x %u / host: child HWND",
                newWidth,
                newHeight);
            SetWindowTextW(GetSceneViewPanelView().GetSceneViewSizeLabel(), sizeText);
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

        const std::vector<HWND> controls = CollectControls();

        for (HWND control : controls)
        {
            if (nullptr != control)
            {
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(m_defaultFont), TRUE);
            }
        }

        if (m_panelViewsInitialized && nullptr != m_hierarchyPanelView)
        {
            ConfigureEditorHierarchyListBox(GetHierarchyPanelView().GetListBox(), ScaleMetric(EditorPanelRowHeight));
        }

        if (ownedPreviousFont && nullptr != previousFont)
        {
            DeleteObject(previousFont);
        }

        return true;
    }

    std::vector<HWND> EditorShell::CollectControls() const
    {
        std::vector<HWND> controls{
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
            m_dockGuideWindows[8]
        };

        const std::array<const IEditorPanelView*, 5> panelViews{
            m_hierarchyPanelView.get(),
            m_assetsPanelView.get(),
            m_inspectorPanelView.get(),
            m_sceneViewPanelView.get(),
            m_logOutputPanelView.get()
        };
        for (const IEditorPanelView* panelView : panelViews)
        {
            if (nullptr != panelView)
            {
                panelView->CollectControls(controls);
            }
        }

        return controls;
    }

    int EditorShell::ScaleMetric(int value) const
    {
        return MulDiv(value, static_cast<int>(m_currentDpi), 96);
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
            const HWND assetsListView =
                m_panelViewsInitialized && nullptr != m_assetsPanelView ? GetAssetsPanelView().GetListView() : nullptr;
            if (nullptr != notifyHeader
                && notifyHeader->hwndFrom == assetsListView
                && notifyHeader->code == NM_CUSTOMDRAW)
            {
                return DrawAssetsListViewItem(lParam);
            }

            const HWND assetsHeader = nullptr != assetsListView ? ListView_GetHeader(assetsListView) : nullptr;
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
            if (m_panelViewsInitialized
                && WM_CTLCOLOREDIT == message
                && IsInspectorInputControl(control)
                && nullptr != m_inputBackgroundBrush)
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
            const bool useSecondaryText = m_panelViewsInitialized && IsInspectorSecondaryLabel(control);
            SetTextColor(
                deviceContext,
                ToColorRef(useSecondaryText ? EditorThemes::XelqoriaDark.textSecondary : EditorThemes::XelqoriaDark.textPrimary));
            return reinterpret_cast<LRESULT>(m_panelBackgroundBrush);
        }

        return std::nullopt;
    }
    bool EditorShell::IsInspectorInputControl(HWND window) const
    {
        return GetInspectorPanelView().IsInputControl(window);
    }

    bool EditorShell::IsInspectorSecondaryLabel(HWND window) const
    {
        return GetInspectorPanelView().IsSecondaryLabel(window);
    }

    HWND EditorShell::GetHierarchyListBox() const { return GetHierarchyPanelView().GetListBox(); }
    HWND EditorShell::GetHierarchySummaryLabel() const { return GetHierarchyPanelView().GetSummaryLabel(); }
    HWND EditorShell::GetHierarchyNameEdit() const { return GetHierarchyPanelView().GetNameEdit(); }
    HWND EditorShell::GetHierarchySearchEdit() const { return GetHierarchyPanelView().GetSearchEdit(); }
    HWND EditorShell::GetHierarchyCreateButton() const { return GetHierarchyPanelView().GetCreateButton(); }
    HWND EditorShell::GetHierarchyDuplicateButton() const { return GetHierarchyPanelView().GetDuplicateButton(); }
    HWND EditorShell::GetHierarchyDeleteButton() const { return GetHierarchyPanelView().GetDeleteButton(); }
    HWND EditorShell::GetAssetsListView() const { return GetAssetsPanelView().GetListView(); }
    void EditorShell::ConfigureAssetsListHeaderTheme() const { GetAssetsPanelView().ConfigureListHeaderTheme(); }
    HWND EditorShell::GetAssetsSummaryLabel() const { return GetAssetsPanelView().GetSummaryLabel(); }
    HWND EditorShell::GetInspectorSummaryLabel() const { return GetInspectorPanelView().GetSummaryLabel(); }
    HWND EditorShell::GetTransformSectionLabel() const { return GetInspectorPanelView().GetTransformSectionLabel(); }
    const std::array<HWND, 3>& EditorShell::GetTransformLabels() const { return GetInspectorPanelView().GetTransformLabels(); }
    const std::array<HWND, 9>& EditorShell::GetTransformEditControls() const { return GetInspectorPanelView().GetTransformEditControls(); }
    HWND EditorShell::GetSpriteRefLabel() const { return GetInspectorPanelView().GetSpriteRefLabel(); }
    HWND EditorShell::GetSpriteComponentSectionLabel() const { return GetInspectorPanelView().GetSpriteComponentSectionLabel(); }
    HWND EditorShell::GetSpriteRefEdit() const { return GetInspectorPanelView().GetSpriteRefEdit(); }
    HWND EditorShell::GetMaterialOpenButton() const { return GetInspectorPanelView().GetMaterialOpenButton(); }
    HWND EditorShell::GetSpriteRefDropHighlight() const { return GetInspectorPanelView().GetSpriteRefDropHighlight(); }
    HWND EditorShell::GetMaterialSharedNoticeLabel() const { return GetInspectorPanelView().GetMaterialSharedNoticeLabel(); }
    HWND EditorShell::GetMaterialDetailsSectionLabel() const { return GetInspectorPanelView().GetMaterialDetailsSectionLabel(); }
    const std::array<HWND, 5>& EditorShell::GetMaterialDetailLabels() const { return GetInspectorPanelView().GetMaterialDetailLabels(); }
    const std::array<HWND, 5>& EditorShell::GetMaterialDetailEditControls() const { return GetInspectorPanelView().GetMaterialDetailEditControls(); }
    HWND EditorShell::GetMaterialTextureDropHighlight() const { return GetInspectorPanelView().GetMaterialTextureDropHighlight(); }
    HWND EditorShell::GetMaterialTextureBrowseButton() const { return GetInspectorPanelView().GetMaterialTextureBrowseButton(); }
    HWND EditorShell::GetMaterialTintColorButton() const { return GetInspectorPanelView().GetMaterialTintColorButton(); }
    HWND EditorShell::GetMaterialOutlineEnabledCheckBox() const { return GetInspectorPanelView().GetMaterialOutlineEnabledCheckBox(); }
    HWND EditorShell::GetMaterialOutlineColorButton() const { return GetInspectorPanelView().GetMaterialOutlineColorButton(); }
    HWND EditorShell::GetCollider2DSummaryLabel() const { return GetInspectorPanelView().GetCollider2DSummaryLabel(); }
    HWND EditorShell::GetScriptAssetLabel() const { return GetInspectorPanelView().GetScriptAssetLabel(); }
    HWND EditorShell::GetScriptAssetEdit() const { return GetInspectorPanelView().GetScriptAssetEdit(); }
    HWND EditorShell::GetScriptCreateButton() const { return GetInspectorPanelView().GetScriptCreateButton(); }
    HWND EditorShell::GetScriptAssignButton() const { return GetInspectorPanelView().GetScriptAssignButton(); }
    HWND EditorShell::GetScriptClearButton() const { return GetInspectorPanelView().GetScriptClearButton(); }
    HWND EditorShell::GetSpriteComponentActionButton() const { return GetInspectorPanelView().GetSpriteComponentActionButton(); }
    HWND EditorShell::GetCollider2DComponentSectionLabel() const { return GetInspectorPanelView().GetCollider2DComponentSectionLabel(); }
    HWND EditorShell::GetCollider2DEnabledCheckBox() const { return GetInspectorPanelView().GetCollider2DEnabledCheckBox(); }
    HWND EditorShell::GetCollider2DTriggerCheckBox() const { return GetInspectorPanelView().GetCollider2DTriggerCheckBox(); }
    HWND EditorShell::GetCollider2DShapeTypeLabel() const { return GetInspectorPanelView().GetCollider2DShapeTypeLabel(); }
    HWND EditorShell::GetCollider2DShapeTypeEdit() const { return GetInspectorPanelView().GetCollider2DShapeTypeEdit(); }
    HWND EditorShell::GetCollider2DOffsetLabel() const { return GetInspectorPanelView().GetCollider2DOffsetLabel(); }
    HWND EditorShell::GetCollider2DSizeLabel() const { return GetInspectorPanelView().GetCollider2DSizeLabel(); }
    HWND EditorShell::GetCollider2DRotationLabel() const { return GetInspectorPanelView().GetCollider2DRotationLabel(); }
    HWND EditorShell::GetCollider2DRotationEdit() const { return GetInspectorPanelView().GetCollider2DRotationEdit(); }
    const std::array<HWND, 4>& EditorShell::GetCollider2DEditControls() const { return GetInspectorPanelView().GetCollider2DEditControls(); }
    HWND EditorShell::GetCollider2DEditButton() const { return GetInspectorPanelView().GetCollider2DEditButton(); }
    HWND EditorShell::GetCollider2DComponentActionButton() const { return GetInspectorPanelView().GetCollider2DComponentActionButton(); }
    HWND EditorShell::GetAddComponentButton() const { return GetInspectorPanelView().GetAddComponentButton(); }
    HWND EditorShell::GetSceneViewSizeLabel() const { return GetSceneViewPanelView().GetSceneViewSizeLabel(); }
    HWND EditorShell::GetBuildAndPlayButton() const { return GetSceneViewPanelView().GetBuildAndPlayButton(); }
    HWND EditorShell::GetPauseResumePlayButton() const { return GetSceneViewPanelView().GetPauseResumePlayButton(); }
    HWND EditorShell::GetEndPlayButton() const { return GetSceneViewPanelView().GetEndPlayButton(); }
    HWND EditorShell::GetSceneViewPlanLabel() const { return GetSceneViewPanelView().GetSceneViewPlanLabel(); }
    HWND EditorShell::GetProjectSummaryLabel() const { return GetSceneViewPanelView().GetProjectSummaryLabel(); }
    HWND EditorShell::GetProjectSceneListBox() const { return GetSceneViewPanelView().GetProjectSceneListBox(); }
    HWND EditorShell::GetProjectSceneDetailLabel() const { return GetSceneViewPanelView().GetProjectSceneDetailLabel(); }
    SceneViewSurface EditorShell::GetSceneViewSurface() const { return GetSceneViewPanelView().GetSceneViewSurface(); }
    HWND EditorShell::GetLogOutputTabControl() const { return GetLogOutputPanelView().GetTabControl(); }
    HWND EditorShell::GetLogClearButton() const { return GetLogOutputPanelView().GetClearButton(); }
    HWND EditorShell::GetLogCopyButton() const { return GetLogOutputPanelView().GetCopyButton(); }
    HWND EditorShell::GetLogFilterEdit() const { return GetLogOutputPanelView().GetFilterEdit(); }
    HWND EditorShell::GetLogListBox() const { return GetLogOutputPanelView().GetListBox(); }
    HierarchyPanelView& EditorShell::GetHierarchyPanelView() const
    {
        return *m_hierarchyPanelView;
    }

    AssetsPanelView& EditorShell::GetAssetsPanelView() const
    {
        return *m_assetsPanelView;
    }

    InspectorPanelView& EditorShell::GetInspectorPanelView() const
    {
        return *m_inspectorPanelView;
    }

    SceneViewPanelView& EditorShell::GetSceneViewPanelView() const
    {
        return *m_sceneViewPanelView;
    }

    LogOutputPanelView& EditorShell::GetLogOutputPanelView() const
    {
        return *m_logOutputPanelView;
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
