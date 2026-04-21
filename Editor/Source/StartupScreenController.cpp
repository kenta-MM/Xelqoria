#include "StartupScreenController.h"

#include <algorithm>
#include <array>
#include <commdlg.h>
#include <cwchar>
#include <shlobj.h>
#include <string>

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr int CreateProjectButtonId = 4301;
        constexpr int OpenProjectButtonId = 4302;
        constexpr int BrowseFolderButtonId = 4303;

        [[nodiscard]] std::wstring BuildRecentProjectLabel(const EditorProjectInfo& project)
        {
            return project.name + L" - " + project.projectFilePath.parent_path().wstring();
        }
    }

    bool StartupScreenController::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        m_defaultFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        m_titleLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"Xelqoria Editor", WS_CHILD | WS_VISIBLE);
        m_nameLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"プロジェクト名", WS_CHILD | WS_VISIBLE);
        m_projectNameEdit = CreateChildWindow(parentWindow, hInstance, L"Edit", L"NewProject", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
        m_folderLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"保存先フォルダ", WS_CHILD | WS_VISIBLE);
        m_projectFolderEdit = CreateChildWindow(parentWindow, hInstance, L"Edit", L".", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
        m_browseFolderButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"参照", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_createButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"プロジェクト作成", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_openButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"プロジェクトオープン", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_recentLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"最近使ったプロジェクト一覧", WS_CHILD | WS_VISIBLE);
        m_recentListBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER);

        if (nullptr == m_titleLabel
            || nullptr == m_nameLabel
            || nullptr == m_projectNameEdit
            || nullptr == m_folderLabel
            || nullptr == m_projectFolderEdit
            || nullptr == m_browseFolderButton
            || nullptr == m_createButton
            || nullptr == m_openButton
            || nullptr == m_recentLabel
            || nullptr == m_recentListBox)
        {
            return false;
        }

        SetWindowLongPtrW(m_createButton, GWLP_ID, CreateProjectButtonId);
        SetWindowLongPtrW(m_openButton, GWLP_ID, OpenProjectButtonId);
        SetWindowLongPtrW(m_browseFolderButton, GWLP_ID, BrowseFolderButtonId);
        RefreshRecentProjects();
        return true;
    }

    void StartupScreenController::UpdateLayout(HWND parentWindow)
    {
        RECT clientRect{};
        GetClientRect(parentWindow, &clientRect);
        const int clientWidth = clientRect.right - clientRect.left;
        const int clientHeight = clientRect.bottom - clientRect.top;
        const int panelWidth = 520;
        const int panelLeft = (std::max)(24, (clientWidth - panelWidth) / 2);
        const int top = (std::max)(24, (clientHeight - 420) / 2);

        MoveWindow(m_titleLabel, panelLeft, top, panelWidth, 36, TRUE);
        MoveWindow(m_nameLabel, panelLeft, top + 56, 160, 24, TRUE);
        MoveWindow(m_projectNameEdit, panelLeft + 160, top + 52, panelWidth - 160, 28, TRUE);
        MoveWindow(m_folderLabel, panelLeft, top + 96, 160, 24, TRUE);
        MoveWindow(m_projectFolderEdit, panelLeft + 160, top + 92, panelWidth - 238, 28, TRUE);
        MoveWindow(m_browseFolderButton, panelLeft + panelWidth - 72, top + 92, 72, 28, TRUE);
        MoveWindow(m_createButton, panelLeft, top + 140, 250, 32, TRUE);
        MoveWindow(m_openButton, panelLeft + 270, top + 140, 250, 32, TRUE);
        MoveWindow(m_recentLabel, panelLeft, top + 196, panelWidth, 24, TRUE);
        MoveWindow(m_recentListBox, panelLeft, top + 224, panelWidth, 180, TRUE);
    }

    void StartupScreenController::Update()
    {
        const bool isLeftMouseDown = 0 != (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
        if (false == m_wasLeftMouseDown || isLeftMouseDown)
        {
            m_wasLeftMouseDown = isLeftMouseDown;
            return;
        }

        m_wasLeftMouseDown = false;

        POINT cursorPosition{};
        GetCursorPos(&cursorPosition);
        const HWND clickedWindow = WindowFromPoint(cursorPosition);
        if (clickedWindow == m_createButton)
        {
            m_createRequested = true;
            return;
        }

        if (clickedWindow == m_browseFolderButton)
        {
            BROWSEINFOW browseInfo{};
            browseInfo.hwndOwner = GetParent(m_browseFolderButton);
            browseInfo.lpszTitle = L"プロジェクトの保存先フォルダを選択";
            browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

            PIDLIST_ABSOLUTE itemList = SHBrowseForFolderW(&browseInfo);
            if (nullptr != itemList)
            {
                std::array<wchar_t, MAX_PATH> folderPath{};
                if (SHGetPathFromIDListW(itemList, folderPath.data()))
                {
                    SetWindowTextW(m_projectFolderEdit, folderPath.data());
                }

                CoTaskMemFree(itemList);
            }

            return;
        }

        if (clickedWindow == m_openButton)
        {
            const std::optional<EditorProjectInfo> selectedProject = GetSelectedRecentProject();
            if (selectedProject.has_value())
            {
                m_openProjectFilePath = selectedProject->projectFilePath;
                return;
            }

            std::array<wchar_t, MAX_PATH> filePath{};
            OPENFILENAMEW openFileName{};
            openFileName.lStructSize = sizeof(openFileName);
            openFileName.hwndOwner = GetParent(m_openButton);
            openFileName.lpstrFilter = L"Xelqoria Project (*.proj)\0*.proj\0All Files (*.*)\0*.*\0";
            openFileName.lpstrFile = filePath.data();
            openFileName.nMaxFile = static_cast<DWORD>(filePath.size());
            openFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            if (GetOpenFileNameW(&openFileName))
            {
                m_openProjectFilePath = filePath.data();
            }
        }
    }

    void StartupScreenController::Hide()
    {
        std::array<HWND, 10> controls{
            m_titleLabel,
            m_nameLabel,
            m_projectNameEdit,
            m_folderLabel,
            m_projectFolderEdit,
            m_browseFolderButton,
            m_createButton,
            m_openButton,
            m_recentLabel,
            m_recentListBox
        };

        for (HWND control : controls)
        {
            ShowWindow(control, SW_HIDE);
        }
    }

    bool StartupScreenController::HasCreateRequest() const
    {
        return m_createRequested;
    }

    bool StartupScreenController::HasOpenRequest() const
    {
        return false == m_openProjectFilePath.empty();
    }

    std::wstring StartupScreenController::GetProjectName() const
    {
        return GetText(m_projectNameEdit);
    }

    std::filesystem::path StartupScreenController::GetProjectParentDirectory() const
    {
        return std::filesystem::path(GetText(m_projectFolderEdit));
    }

    std::filesystem::path StartupScreenController::GetOpenProjectFilePath() const
    {
        return m_openProjectFilePath;
    }

    void StartupScreenController::ClearRequests()
    {
        m_createRequested = false;
        m_openProjectFilePath.clear();
    }

    void StartupScreenController::RefreshRecentProjects()
    {
        m_recentProjects = m_recentProjectsStore.Load();
        SendMessageW(m_recentListBox, LB_RESETCONTENT, 0, 0);
        for (const EditorProjectInfo& project : m_recentProjects)
        {
            const std::wstring label = BuildRecentProjectLabel(project);
            SendMessageW(m_recentListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        }
    }

    std::optional<EditorProjectInfo> StartupScreenController::GetSelectedRecentProject() const
    {
        const LRESULT selectedIndex = SendMessageW(m_recentListBox, LB_GETCURSEL, 0, 0);
        if (selectedIndex == LB_ERR || selectedIndex < 0)
        {
            return std::nullopt;
        }

        const auto index = static_cast<std::size_t>(selectedIndex);
        if (index >= m_recentProjects.size())
        {
            return std::nullopt;
        }

        return m_recentProjects[index];
    }

    HWND StartupScreenController::CreateChildWindow(
        HWND parentWindow,
        HINSTANCE hInstance,
        const wchar_t* className,
        const wchar_t* text,
        DWORD style,
        DWORD exStyle) const
    {
        HWND handle = CreateWindowExW(
            exStyle,
            className,
            text,
            style,
            0,
            0,
            0,
            0,
            parentWindow,
            nullptr,
            hInstance,
            nullptr);

        if (nullptr != handle && nullptr != m_defaultFont)
        {
            SendMessageW(handle, WM_SETFONT, reinterpret_cast<WPARAM>(m_defaultFont), TRUE);
        }

        return handle;
    }

    std::wstring StartupScreenController::GetText(HWND control) const
    {
        const int length = GetWindowTextLengthW(control);
        std::wstring text(static_cast<std::size_t>(length) + 1u, L'\0');
        GetWindowTextW(control, text.data(), length + 1);
        text.resize(static_cast<std::size_t>(length));
        return text;
    }
}
