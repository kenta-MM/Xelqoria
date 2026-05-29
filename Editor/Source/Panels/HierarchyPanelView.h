#pragma once

#include "Panels/EditorPanelViewBase.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Hierarchy Panel の HWND 群を管理する View。
    /// </summary>
    class HierarchyPanelView final : public EditorPanelViewBase
    {
    public:
        /// <summary>
        /// Hierarchy control 群を生成・管理する View を作成する。
        /// </summary>
        explicit HierarchyPanelView(EditorPanelHostContext& hostContext);

        bool Initialize(HWND parentWindow, HINSTANCE hInstance) override;
        void Layout(const RECT& bounds) override;

        [[nodiscard]] HWND GetListBox() const;
        [[nodiscard]] HWND GetSummaryLabel() const;
        [[nodiscard]] HWND GetSearchEdit() const;
        [[nodiscard]] HWND GetNameEdit() const;
        [[nodiscard]] HWND GetCreateButton() const;
        [[nodiscard]] HWND GetDuplicateButton() const;
        [[nodiscard]] HWND GetDeleteButton() const;

    private:
        HWND m_panel = nullptr;
        HWND m_summaryLabel = nullptr;
        HWND m_listBox = nullptr;
        HWND m_searchEdit = nullptr;
        HWND m_nameEdit = nullptr;
        HWND m_createButton = nullptr;
        HWND m_duplicateButton = nullptr;
        HWND m_deleteButton = nullptr;
    };
}
