#pragma once

#include <Windows.h>
#include <filesystem>
#include <optional>
#include <vector>

#include "EditorProject.h"
#include "IFileDialog.h"
#include "InputSystem.h"
#include "RecentProjectsStore.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// 起動時のプロジェクト選択画面を管理する。
    /// </summary>
    class StartupScreenController
    {
    public:
        /// <summary>
        /// 起動画面を初期化する。
        /// </summary>
        /// <param name="parentWindow">親ウィンドウ。</param>
        /// <param name="hInstance">アプリケーションインスタンス。</param>
        /// <returns>初期化に成功した場合は true。</returns>
        bool Initialize(HWND parentWindow, HINSTANCE hInstance, Platform::IFileDialog& fileDialog);

        /// <summary>
        /// StartupScreenController が所有する UI リソースを破棄する。
        /// </summary>
        ~StartupScreenController();

        /// <summary>
        /// 起動画面を現在のウィンドウサイズへ合わせる。
        /// </summary>
        /// <param name="parentWindow">親ウィンドウ。</param>
        void UpdateLayout(HWND parentWindow);

        /// <summary>
        /// 起動画面に属する owner draw コントロールを描画する。
        /// </summary>
        /// <param name="drawItemParameter">WM_DRAWITEM の LPARAM。</param>
        /// <returns>描画を処理した場合は true。</returns>
        bool HandleDrawItem(LPARAM drawItemParameter);

        /// <summary>
        /// 入力状態を更新し、プロジェクト操作要求を検出する。
        /// </summary>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        void Update(const Core::InputSnapshot& inputSnapshot);

        /// <summary>
        /// 起動画面を非表示にする。
        /// </summary>
        void Hide();

        /// <summary>
        /// 起動画面の親ウィンドウメッセージを処理する。
        /// </summary>
        LRESULT HandleParentWindowMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

        /// <summary>
        /// プロジェクト作成ダイアログのメッセージを処理する。
        /// </summary>
        LRESULT HandleCreateProjectWindowMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

        /// <summary>
        /// 起動画面のウィンドウ群を破棄する。
        /// </summary>
        void Destroy();

        /// <summary>
        /// 新規作成要求があるかを取得する。
        /// </summary>
        /// <returns>新規作成要求がある場合は true。</returns>
        [[nodiscard]] bool HasCreateRequest() const;

        /// <summary>
        /// 開く要求があるかを取得する。
        /// </summary>
        /// <returns>開く要求がある場合は true。</returns>
        [[nodiscard]] bool HasOpenRequest() const;

        /// <summary>
        /// 入力されたプロジェクト名を取得する。
        /// </summary>
        /// <returns>プロジェクト名。</returns>
        [[nodiscard]] std::wstring GetProjectName() const;

        /// <summary>
        /// 入力された保存先フォルダを取得する。
        /// </summary>
        /// <returns>保存先親フォルダ。</returns>
        [[nodiscard]] std::filesystem::path GetProjectParentDirectory() const;

        /// <summary>
        /// 開く対象の `.proj` ファイルを取得する。
        /// </summary>
        /// <returns>開く対象パス。</returns>
        [[nodiscard]] std::filesystem::path GetOpenProjectFilePath() const;

        /// <summary>
        /// 処理済み要求をクリアする。
        /// </summary>
        void ClearRequests();

    private:
        enum class StartupTab
        {
            Projects,
            View
        };

        /// <summary>
        /// 最近使ったプロジェクト一覧を再表示する。
        /// </summary>
        void RefreshRecentProjects();

        /// <summary>
        /// 表示タブのプロジェクト一覧を再表示する。
        /// </summary>
        void RefreshAllProjects();

        /// <summary>
        /// 起動画面の現在タブを変更する。
        /// </summary>
        /// <param name="tab">変更後のタブ。</param>
        void SetActiveTab(StartupTab tab);

        /// <summary>
        /// 起動画面用のブラシなどの GDI リソースを作成する。
        /// </summary>
        void EnsureThemeResources();

        /// <summary>
        /// 起動画面用の GDI リソースを破棄する。
        /// </summary>
        void DestroyThemeResources();

        /// <summary>
        /// 起動画面の親ウィンドウをサブクラス化する。
        /// </summary>
        /// <param name="parentWindow">サブクラス化する親ウィンドウ。</param>
        void SubclassParentWindow(HWND parentWindow);

        /// <summary>
        /// 起動画面の親ウィンドウのサブクラス化を解除する。
        /// </summary>
        void UnsubclassParentWindow();

        /// <summary>
        /// 起動画面の背景を描画する。
        /// </summary>
        /// <param name="deviceContext">描画先。</param>
        void PaintStartupBackground(HDC deviceContext);

        /// <summary>
        /// プロジェクト作成ダイアログの背景を描画する。
        /// </summary>
        /// <param name="deviceContext">描画先。</param>
        void PaintCreateProjectBackground(HDC deviceContext);

        /// <summary>
        /// テーマ適用済みボタンを描画する。
        /// </summary>
        /// <param name="drawItem">owner draw 情報。</param>
        void DrawThemedButton(const DRAWITEMSTRUCT& drawItem) const;

        /// <summary>
        /// テーマ適用済みタブを描画する。
        /// </summary>
        /// <param name="drawItem">owner draw 情報。</param>
        /// <param name="tab">描画対象タブ。</param>
        void DrawTabButton(const DRAWITEMSTRUCT& drawItem, StartupTab tab) const;

        /// <summary>
        /// テーマ適用済みリスト行を描画する。
        /// </summary>
        /// <param name="drawItem">owner draw 情報。</param>
        void DrawProjectListItem(const DRAWITEMSTRUCT& drawItem) const;

        /// <summary>
        /// プロジェクト一覧行に対応するプロジェクトを開く要求として設定する。
        /// </summary>
        /// <param name="listBox">対象 ListBox。</param>
        /// <param name="projects">ListBox に対応するプロジェクト一覧。</param>
        /// <param name="cursorPosition">スクリーン座標のカーソル位置。</param>
        /// <returns>要求を設定した場合は true。</returns>
        bool HandleProjectListDoubleClick(
            HWND listBox,
            const std::vector<EditorProjectInfo>& projects,
            POINT cursorPosition);

        /// <summary>
        /// プロジェクト作成ウィンドウを表示する。
        /// </summary>
        void ShowCreateProjectWindow();

        /// <summary>
        /// プロジェクト作成ウィンドウを閉じる。
        /// </summary>
        void HideCreateProjectWindow();

        /// <summary>
        /// ウィンドウを破棄し、保持ハンドルを空にする。
        /// </summary>
        /// <param name="window">破棄対象のウィンドウハンドル。</param>
        void DestroyWindowHandle(HWND& window);

        /// <summary>
        /// プロジェクト作成ウィンドウのレイアウトを更新する。
        /// </summary>
        void UpdateCreateProjectWindowLayout();

        /// <summary>
        /// DPI に合わせた UI フォントを作成して各 child window へ適用する。
        /// </summary>
        /// <param name="parentWindow">DPI の基準にする親ウィンドウ。</param>
        /// <returns>DPI リソースを更新した場合は true。</returns>
        bool RefreshDpiResources(HWND parentWindow);

        /// <summary>
        /// 指定値を現在 DPI に合わせて拡大縮小する。
        /// </summary>
        /// <param name="value">96 DPI 基準の値。</param>
        /// <returns>DPI 適用後の値。</returns>
        [[nodiscard]] int ScaleMetric(int value) const;

        /// <summary>
        /// 最近使ったプロジェクト一覧のダブルクリックを処理する。
        /// </summary>
        /// <param name="cursorPosition">スクリーン座標のカーソル位置。</param>
        /// <returns>ダブルクリックでプロジェクトを開く要求を設定した場合は true。</returns>
        bool HandleRecentProjectDoubleClick(POINT cursorPosition);

        /// <summary>
        /// ListBox の選択行に対応するプロジェクトを取得する。
        /// </summary>
        /// <returns>選択中プロジェクト。未選択時は空。</returns>
        [[nodiscard]] std::optional<EditorProjectInfo> GetSelectedRecentProject() const;

        /// <summary>
        /// 表示タブ ListBox の選択行に対応するプロジェクトを取得する。
        /// </summary>
        /// <returns>選択中プロジェクト。未選択時は空。</returns>
        [[nodiscard]] std::optional<EditorProjectInfo> GetSelectedAllProject() const;

        /// <summary>
        /// 子ウィンドウを生成する。
        /// </summary>
        HWND CreateChildWindow(
            HWND parentWindow,
            HINSTANCE hInstance,
            const wchar_t* className,
            const wchar_t* text,
            DWORD style,
            DWORD exStyle = 0) const;

        /// <summary>
        /// Edit の文字列を取得する。
        /// </summary>
        [[nodiscard]] std::wstring GetText(HWND control) const;

    private:
        HFONT m_defaultFont = nullptr;
        UINT m_currentDpi = 96;
        bool m_ownsDefaultFont = false;
        bool m_hidden = false;
        StartupTab m_activeTab = StartupTab::Projects;
        HWND m_parentWindow = nullptr;
        WNDPROC m_originalParentWindowProc = nullptr;
        HBRUSH m_darkBackgroundBrush = nullptr;
        HBRUSH m_panelBackgroundBrush = nullptr;
        HBRUSH m_editBackgroundBrush = nullptr;
        HPEN m_accentPen = nullptr;
        HPEN m_dimAccentPen = nullptr;
        HPEN m_blueAccentPen = nullptr;
        HWND m_projectTabButton = nullptr;
        HWND m_viewTabButton = nullptr;
        HWND m_minimizeButton = nullptr;
        HWND m_maximizeButton = nullptr;
        HWND m_closeButton = nullptr;
        HWND m_nameLabel = nullptr;
        HWND m_projectNameEdit = nullptr;
        HWND m_folderLabel = nullptr;
        HWND m_projectFolderEdit = nullptr;
        HWND m_browseFolderButton = nullptr;
        HWND m_createProjectWindow = nullptr;
        HWND m_createConfirmButton = nullptr;
        HWND m_createCancelButton = nullptr;
        HWND m_createButton = nullptr;
        HWND m_openButton = nullptr;
        HWND m_recentLabel = nullptr;
        HWND m_recentListBox = nullptr;
        HWND m_allProjectsLabel = nullptr;
        HWND m_allProjectsListBox = nullptr;
        RecentProjectsStore m_recentProjectsStore{};
        Platform::IFileDialog* m_fileDialog = nullptr;
        std::vector<EditorProjectInfo> m_recentProjects{};
        std::vector<EditorProjectInfo> m_allProjects{};
        bool m_createRequested = false;
        bool m_wasLeftMouseDown = false;
        DWORD m_lastRecentClickTime = 0;
        int m_lastRecentClickIndex = -1;
        DWORD m_lastAllProjectsClickTime = 0;
        int m_lastAllProjectsClickIndex = -1;
        bool m_layoutInitialized = false;
        int m_lastLayoutClientWidth = 0;
        int m_lastLayoutClientHeight = 0;
        UINT m_lastLayoutDpi = 0;
        std::filesystem::path m_openProjectFilePath{};
    };
}
