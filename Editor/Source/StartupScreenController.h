#pragma once

#include <Windows.h>
#include <filesystem>
#include <optional>
#include <vector>

#include "EditorProject.h"
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
        bool Initialize(HWND parentWindow, HINSTANCE hInstance);

        /// <summary>
        /// 起動画面を現在のウィンドウサイズへ合わせる。
        /// </summary>
        /// <param name="parentWindow">親ウィンドウ。</param>
        void UpdateLayout(HWND parentWindow);

        /// <summary>
        /// 入力状態を更新し、プロジェクト操作要求を検出する。
        /// </summary>
        void Update();

        /// <summary>
        /// 起動画面を非表示にする。
        /// </summary>
        void Hide();

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
        /// <summary>
        /// 最近使ったプロジェクト一覧を再表示する。
        /// </summary>
        void RefreshRecentProjects();

        /// <summary>
        /// ListBox の選択行に対応するプロジェクトを取得する。
        /// </summary>
        /// <returns>選択中プロジェクト。未選択時は空。</returns>
        [[nodiscard]] std::optional<EditorProjectInfo> GetSelectedRecentProject() const;

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
        HWND m_titleLabel = nullptr;
        HWND m_nameLabel = nullptr;
        HWND m_projectNameEdit = nullptr;
        HWND m_folderLabel = nullptr;
        HWND m_projectFolderEdit = nullptr;
        HWND m_browseFolderButton = nullptr;
        HWND m_createButton = nullptr;
        HWND m_openButton = nullptr;
        HWND m_recentLabel = nullptr;
        HWND m_recentListBox = nullptr;
        RecentProjectsStore m_recentProjectsStore{};
        std::vector<EditorProjectInfo> m_recentProjects{};
        bool m_createRequested = false;
        bool m_wasLeftMouseDown = false;
        std::filesystem::path m_openProjectFilePath{};
    };
}
