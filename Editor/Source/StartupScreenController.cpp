#include "StartupScreenController.h"

#include <algorithm>
#include <array>
#include <cwchar>
#include <string>
#include <ShlObj_core.h>
#include <shtypes.h>
#include <Windows.h>
#include <filesystem>
#include <optional>
#include "EditorProject.h"
#include <InputSystem.h>

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr int CreateProjectButtonId = 4301;
        constexpr int OpenProjectButtonId = 4302;
        constexpr int BrowseFolderButtonId = 4303;
        constexpr int CreateConfirmButtonId = 4304;
        constexpr int CreateCancelButtonId = 4305;
        constexpr const wchar_t* CreateProjectWindowClassName = L"XelqoriaCreateProjectWindow";

        [[nodiscard]] POINT ToWin32Point(Platform::Point point)
        {
            return POINT{ static_cast<LONG>(point.x), static_cast<LONG>(point.y) };
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

        LRESULT CALLBACK CreateProjectWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (message == WM_CLOSE)
            {
                ShowWindow(window, SW_HIDE);
                return 0;
            }

            return DefWindowProcW(window, message, wParam, lParam);
        }

        [[nodiscard]] std::wstring BuildRecentProjectLabel(const EditorProjectInfo& project)
        {
            return project.name + L" - " + project.projectFilePath.parent_path().wstring();
        }

        bool RegisterCreateProjectWindowClass(HINSTANCE hInstance)
        {
            WNDCLASSW windowClass{};
            if (GetClassInfoW(hInstance, CreateProjectWindowClassName, &windowClass))
            {
                return true;
            }

            windowClass.lpfnWndProc = CreateProjectWindowProc;
            windowClass.hInstance = hInstance;
            windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
            windowClass.lpszClassName = CreateProjectWindowClassName;
            return 0 != RegisterClassW(&windowClass);
        }
    }

    bool StartupScreenController::Initialize(HWND parentWindow, HINSTANCE hInstance)
    {
        (void)RefreshDpiResources(parentWindow);
        m_createButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"プロジェクト作成", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_openButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"プロジェクトを開く", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
        m_recentLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"最近使ったプロジェクト一覧", WS_CHILD | WS_VISIBLE);
        m_recentListBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER);

        if (false == RegisterCreateProjectWindowClass(hInstance))
        {
            return false;
        }

        m_createProjectWindow = CreateWindowExW(
            WS_EX_DLGMODALFRAME,
            CreateProjectWindowClassName,
            L"プロジェクト作成",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            ScaleMetric(560),
            ScaleMetric(180),
            parentWindow,
            nullptr,
            hInstance,
            nullptr);

        if (nullptr == m_createProjectWindow)
        {
            return false;
        }

        if (nullptr == (m_nameLabel = CreateChildWindow(m_createProjectWindow, hInstance, L"Static", L"プロジェクト名", WS_CHILD | WS_VISIBLE))) {
            return false;
        }
        if (nullptr == (m_projectNameEdit = CreateChildWindow(m_createProjectWindow, hInstance, L"Edit", L"NewProject", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL))) {
            return false;
        }
        if (nullptr == (m_folderLabel = CreateChildWindow(m_createProjectWindow, hInstance, L"Static", L"保存先フォルダ", WS_CHILD | WS_VISIBLE))) {
            return false;
        }
        if (nullptr == (m_projectFolderEdit = CreateChildWindow(m_createProjectWindow, hInstance, L"Edit", L".", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL))) {
            return false;
        }
        if (nullptr == (m_browseFolderButton = CreateChildWindow(m_createProjectWindow, hInstance, L"Button", L"参照", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON))) {
            return false;
        }
        if (nullptr == (m_createConfirmButton = CreateChildWindow(m_createProjectWindow, hInstance, L"Button", L"作成", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON))) {
            return false;
        }
        if (nullptr == (m_createCancelButton = CreateChildWindow(m_createProjectWindow, hInstance, L"Button", L"キャンセル", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON))) {
            return false;
        }

        SetWindowLongPtrW(m_createButton, GWLP_ID, CreateProjectButtonId);
        SetWindowLongPtrW(m_openButton, GWLP_ID, OpenProjectButtonId);
        SetWindowLongPtrW(m_browseFolderButton, GWLP_ID, BrowseFolderButtonId);
        SetWindowLongPtrW(m_createConfirmButton, GWLP_ID, CreateConfirmButtonId);
        SetWindowLongPtrW(m_createCancelButton, GWLP_ID, CreateCancelButtonId);
        UpdateCreateProjectWindowLayout();
        RefreshRecentProjects();
        return true;
    }

    StartupScreenController::~StartupScreenController()
    {
        if (m_ownsDefaultFont && nullptr != m_defaultFont)
        {
            HFONT stockFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            const std::array<HWND, 12> controls{
                m_nameLabel,
                m_projectNameEdit,
                m_folderLabel,
                m_projectFolderEdit,
                m_browseFolderButton,
                m_createProjectWindow,
                m_createConfirmButton,
                m_createCancelButton,
                m_createButton,
                m_openButton,
                m_recentLabel,
                m_recentListBox
            };
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

    void StartupScreenController::UpdateLayout(HWND parentWindow)
    {
        const bool dpiChanged = RefreshDpiResources(parentWindow);

        RECT clientRect{};
        GetClientRect(parentWindow, &clientRect);
        const int clientWidth = clientRect.right - clientRect.left;
        const int clientHeight = clientRect.bottom - clientRect.top;
        if (m_layoutInitialized
            && false == dpiChanged
            && m_lastLayoutClientWidth == clientWidth
            && m_lastLayoutClientHeight == clientHeight
            && m_lastLayoutDpi == m_currentDpi)
        {
            return;
        }

        const int panelWidth = ScaleMetric(520);
        const int panelLeft = (std::max)(ScaleMetric(24), (clientWidth - panelWidth) / 2);
        const int top = (std::max)(ScaleMetric(24), (clientHeight - ScaleMetric(264)) / 2);

        MoveWindow(m_createButton, panelLeft, top, ScaleMetric(250), ScaleMetric(32), TRUE);
        MoveWindow(m_openButton, panelLeft + ScaleMetric(270), top, ScaleMetric(250), ScaleMetric(32), TRUE);
        MoveWindow(m_recentLabel, panelLeft, top + ScaleMetric(56), panelWidth, ScaleMetric(24), TRUE);
        MoveWindow(m_recentListBox, panelLeft, top + ScaleMetric(84), panelWidth, ScaleMetric(180), TRUE);
        m_layoutInitialized = true;
        m_lastLayoutClientWidth = clientWidth;
        m_lastLayoutClientHeight = clientHeight;
        m_lastLayoutDpi = m_currentDpi;
    }

    void StartupScreenController::Update(const Core::InputSnapshot& inputSnapshot)
    {
        const bool isLeftMouseDown = inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left);
        if (false == m_wasLeftMouseDown || isLeftMouseDown)
        {
            m_wasLeftMouseDown = isLeftMouseDown;
            return;
        }

        m_wasLeftMouseDown = false;

        const POINT cursorPosition = ToWin32Point(inputSnapshot.GetCursorScreenPoint());
        const HWND clickedWindow = WindowFromPoint(cursorPosition);
        if (clickedWindow == m_createButton)
        {
            ShowCreateProjectWindow();
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

        if (clickedWindow == m_createConfirmButton)
        {
            HideCreateProjectWindow();
            m_createRequested = true;
            return;
        }

        if (clickedWindow == m_createCancelButton)
        {
            HideCreateProjectWindow();
            return;
        }

        if (clickedWindow == m_recentListBox && HandleRecentProjectDoubleClick(cursorPosition))
        {
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
        std::array<HWND, 4> controls{
            m_createButton,
            m_openButton,
            m_recentLabel,
            m_recentListBox
        };

        for (HWND control : controls)
        {
            ShowWindow(control, SW_HIDE);
        }

        HideCreateProjectWindow();
    }

    void StartupScreenController::Destroy()
    {
        HWND parentWindow = nullptr;
        if (nullptr != m_createButton)
        {
            parentWindow = GetParent(m_createButton);
        }

        std::array<HWND*, 4> startupControls{
            &m_createButton,
            &m_openButton,
            &m_recentLabel,
            &m_recentListBox
        };

        for (HWND* control : startupControls)
        {
            DestroyWindowHandle(*control);
        }

        DestroyWindowHandle(m_createProjectWindow);
        m_nameLabel = nullptr;
        m_projectNameEdit = nullptr;
        m_folderLabel = nullptr;
        m_projectFolderEdit = nullptr;
        m_browseFolderButton = nullptr;
        m_createConfirmButton = nullptr;
        m_createCancelButton = nullptr;
        m_createRequested = false;
        m_openProjectFilePath.clear();

        if (nullptr != parentWindow)
        {
            RedrawWindow(parentWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
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

    void StartupScreenController::ShowCreateProjectWindow()
    {
        if (nullptr == m_createProjectWindow)
        {
            return;
        }

        HWND parentWindow = GetParent(m_createButton);
        RefreshDpiResources(parentWindow);
        RECT parentRect{};
        GetWindowRect(parentWindow, &parentRect);

        const int width = ScaleMetric(560);
        const int height = ScaleMetric(180);
        const int left = parentRect.left + ((parentRect.right - parentRect.left) - width) / 2;
        const int top = parentRect.top + ((parentRect.bottom - parentRect.top) - height) / 2;

        SetWindowPos(m_createProjectWindow, HWND_TOP, left, top, width, height, SWP_SHOWWINDOW);
        UpdateCreateProjectWindowLayout();
        SetFocus(m_projectNameEdit);
    }

    void StartupScreenController::HideCreateProjectWindow()
    {
        if (nullptr != m_createProjectWindow)
        {
            ShowWindow(m_createProjectWindow, SW_HIDE);
        }
    }

    void StartupScreenController::DestroyWindowHandle(HWND& window)
    {
        if (nullptr == window)
        {
            return;
        }

        DestroyWindow(window);
        window = nullptr;
    }

    void StartupScreenController::UpdateCreateProjectWindowLayout()
    {
        MoveWindow(m_nameLabel, ScaleMetric(16), ScaleMetric(18), ScaleMetric(140), ScaleMetric(24), TRUE);
        MoveWindow(m_projectNameEdit, ScaleMetric(156), ScaleMetric(14), ScaleMetric(360), ScaleMetric(28), TRUE);
        MoveWindow(m_folderLabel, ScaleMetric(16), ScaleMetric(58), ScaleMetric(140), ScaleMetric(24), TRUE);
        MoveWindow(m_projectFolderEdit, ScaleMetric(156), ScaleMetric(54), ScaleMetric(276), ScaleMetric(28), TRUE);
        MoveWindow(m_browseFolderButton, ScaleMetric(444), ScaleMetric(54), ScaleMetric(72), ScaleMetric(28), TRUE);
        MoveWindow(m_createConfirmButton, ScaleMetric(316), ScaleMetric(100), ScaleMetric(96), ScaleMetric(30), TRUE);
        MoveWindow(m_createCancelButton, ScaleMetric(420), ScaleMetric(100), ScaleMetric(96), ScaleMetric(30), TRUE);
    }

    bool StartupScreenController::HandleRecentProjectDoubleClick(POINT cursorPosition)
    {
        POINT listBoxPosition = cursorPosition;
        ScreenToClient(m_recentListBox, &listBoxPosition);

        const LPARAM positionParameter = MAKELPARAM(listBoxPosition.x, listBoxPosition.y);
        const LRESULT itemResult = SendMessageW(m_recentListBox, LB_ITEMFROMPOINT, 0, positionParameter);
        if (HIWORD(itemResult) != 0)
        {
            m_lastRecentClickIndex = -1;
            return false;
        }

        const int clickedIndex = LOWORD(itemResult);
        const DWORD currentTime = GetTickCount();
        const bool isDoubleClick =
            clickedIndex == m_lastRecentClickIndex
            && currentTime - m_lastRecentClickTime <= GetDoubleClickTime();

        m_lastRecentClickIndex = clickedIndex;
        m_lastRecentClickTime = currentTime;

        if (false == isDoubleClick)
        {
            return false;
        }

        const auto projectIndex = static_cast<std::size_t>(clickedIndex);
        if (projectIndex >= m_recentProjects.size())
        {
            return false;
        }

        m_openProjectFilePath = m_recentProjects[projectIndex].projectFilePath;
        return true;
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

    bool StartupScreenController::RefreshDpiResources(HWND parentWindow)
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

        const std::array<HWND, 12> controls{
            m_nameLabel,
            m_projectNameEdit,
            m_folderLabel,
            m_projectFolderEdit,
            m_browseFolderButton,
            m_createProjectWindow,
            m_createConfirmButton,
            m_createCancelButton,
            m_createButton,
            m_openButton,
            m_recentLabel,
            m_recentListBox
        };

        for (HWND control : controls)
        {
            if (nullptr != control)
            {
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(m_defaultFont), TRUE);
            }
        }

        if (ownedPreviousFont && nullptr != previousFont)
        {
            DeleteObject(previousFont);
        }

        return true;
    }

    int StartupScreenController::ScaleMetric(int value) const
    {
        return MulDiv(value, static_cast<int>(m_currentDpi), 96);
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
