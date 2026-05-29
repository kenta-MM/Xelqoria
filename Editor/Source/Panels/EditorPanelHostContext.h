#pragma once

#include <Windows.h>
#include <vector>

namespace Xelqoria::Editor
{
    constexpr int EditorPanelRowHeight = 22;
    constexpr const wchar_t* EditorPanelWindowClassName = L"XelqoriaEditorPanel";

    struct EditorPanelLayoutMove
    {
        HWND window = nullptr;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    class EditorPanelHostContext
    {
    public:
        void Bind(UINT& currentDpi, HFONT& defaultFont, std::vector<EditorPanelLayoutMove>& pendingLayoutMoves);

        [[nodiscard]] HWND CreateChildWindow(
            HWND parentWindow,
            HINSTANCE hInstance,
            const wchar_t* className,
            const wchar_t* text,
            DWORD style,
            DWORD exStyle = 0) const;

        void MoveChildWindowNoRedraw(HWND window, int x, int y, int width, int height) const;
        [[nodiscard]] int ScaleMetric(int value) const;

    private:
        UINT* m_currentDpi = nullptr;
        HFONT* m_defaultFont = nullptr;
        std::vector<EditorPanelLayoutMove>* m_pendingLayoutMoves = nullptr;
    };

    void ApplyEditorDarkExplorerTheme(HWND window);
    void ConfigureEditorTabControl(HWND tabControl, int itemWidth, int itemHeight);
    void ConfigureEditorHierarchyListBox(HWND listBox, int rowHeight);
    void ConfigureEditorAssetsListView(HWND listView);
    void ConfigureEditorAssetsListHeaderTheme(HWND listView);
    [[nodiscard]] bool ApplySpriteRefEditReadOnlySubclass(HWND editControl);
}
