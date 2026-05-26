#pragma once

#include <functional>
#include <vector>

#include "Panels/IEditorPanelView.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// 既存 HWND 群を PanelView 契約へ接続する共通基底。
    /// </summary>
    class EditorPanelViewBase : public IEditorPanelView
    {
    public:
        using ControlProvider = std::function<std::vector<HWND>()>;

        /// <summary>
        /// Panel 識別子、表示名、対象 control 群を指定して View 境界を作成する。
        /// </summary>
        EditorPanelViewBase(
            EditorPanelId panelId,
            const wchar_t* title,
            ControlProvider controls,
            ControlProvider visibleControls,
            ControlProvider alwaysHiddenControls,
            ControlProvider hideWhenInvisibleControls);

        [[nodiscard]] EditorPanelId GetPanelId() const override;
        [[nodiscard]] const wchar_t* GetTitle() const override;
        bool Initialize(HWND parentWindow, HINSTANCE hInstance) override;
        void Layout(const RECT& bounds) override;
        void Show(bool visible) override;
        void SetParent(HWND parentWindow) override;
        void CollectControls(std::vector<HWND>& controls) const override;

    private:
        [[nodiscard]] std::vector<HWND> GetControls(const ControlProvider& provider) const;

        EditorPanelId m_panelId;
        const wchar_t* m_title = L"";
        ControlProvider m_controls;
        ControlProvider m_visibleControls;
        ControlProvider m_alwaysHiddenControls;
        ControlProvider m_hideWhenInvisibleControls;
    };
}
