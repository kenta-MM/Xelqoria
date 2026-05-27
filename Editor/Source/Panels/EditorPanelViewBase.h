#pragma once

#include <functional>
#include <vector>

#include "Panels/EditorPanelHostContext.h"
#include "Panels/IEditorPanelView.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// 既存 HWND 群を PanelView 契約へ接続する共通基底。
    /// </summary>
    class EditorPanelViewBase : public IEditorPanelView
    {
    public:
        /// <summary>
        /// Panel 識別子、表示名、共通 host を指定して View 境界を作成する。
        /// </summary>
        EditorPanelViewBase(
            EditorPanelHostContext& hostContext,
            EditorPanelId panelId,
            const wchar_t* title);

        [[nodiscard]] EditorPanelId GetPanelId() const override;
        [[nodiscard]] const wchar_t* GetTitle() const override;
        [[nodiscard]] HWND GetRootWindow() const override;
        bool Initialize(HWND parentWindow, HINSTANCE hInstance) override;
        void Layout(const RECT& bounds) override;
        void Show(bool visible) override;
        void SetParent(HWND parentWindow) override;
        void CollectControls(std::vector<HWND>& controls) const override;

    protected:
        [[nodiscard]] HWND CreateChildWindow(
            HWND parentWindow,
            HINSTANCE hInstance,
            const wchar_t* className,
            const wchar_t* text,
            DWORD style,
            DWORD exStyle = 0) const;
        void MoveChildWindowNoRedraw(HWND window, int x, int y, int width, int height) const;
        [[nodiscard]] int ScaleMetric(int value) const;

        void SetRootWindow(HWND rootWindow);
        void SetControls(std::vector<HWND> controls);
        void SetVisibleControls(std::vector<HWND> visibleControls);
        void SetAlwaysHiddenControls(std::vector<HWND> alwaysHiddenControls);
        void SetHideWhenInvisibleControls(std::vector<HWND> hideWhenInvisibleControls);

    private:
        EditorPanelHostContext& m_hostContext;
        EditorPanelId m_panelId;
        const wchar_t* m_title = L"";
        HWND m_rootWindow = nullptr;
        std::vector<HWND> m_controls{};
        std::vector<HWND> m_visibleControls{};
        std::vector<HWND> m_alwaysHiddenControls{};
        std::vector<HWND> m_hideWhenInvisibleControls{};
    };
}
