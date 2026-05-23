#include "StartupScreenController.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cwchar>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>

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
        constexpr int ProjectTabButtonId = 4306;
        constexpr int ViewTabButtonId = 4307;
        constexpr int RecentProjectsListBoxId = 4311;
        constexpr int AllProjectsListBoxId = 4312;
        constexpr const wchar_t* CreateProjectWindowClassName = L"XelqoriaCreateProjectWindow";
        constexpr const wchar_t* ParentControllerPropertyName = L"XelqoriaStartupScreenController";
        constexpr COLORREF BackgroundColor = RGB(4, 7, 20);
        constexpr COLORREF HeaderColor = RGB(7, 10, 25);
        constexpr COLORREF PanelColor = RGB(15, 16, 44);
        constexpr COLORREF PanelHoverColor = RGB(25, 24, 68);
        constexpr COLORREF TextColor = RGB(238, 241, 255);
        constexpr COLORREF MutedTextColor = RGB(170, 178, 216);
        constexpr COLORREF AccentColor = RGB(160, 82, 255);
        constexpr COLORREF AccentBlueColor = RGB(44, 113, 255);
        constexpr COLORREF BorderColor = RGB(91, 75, 210);
        constexpr COLORREF EditColor = RGB(13, 15, 38);

        [[nodiscard]] POINT ToWin32Point(Platform::Point point)
        {
            return POINT{ static_cast<LONG>(point.x), static_cast<LONG>(point.y) };
        }

        [[nodiscard]] UINT GetWindowDpi(HWND window)
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

        [[nodiscard]] StartupScreenController* GetControllerFromWindow(HWND window)
        {
            return reinterpret_cast<StartupScreenController*>(
                GetPropW(window, ParentControllerPropertyName));
        }

        LRESULT CALLBACK StartupParentWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            StartupScreenController* controller = GetControllerFromWindow(window);
            if (nullptr != controller)
            {
                return controller->HandleParentWindowMessage(window, message, wParam, lParam);
            }

            return DefWindowProcW(window, message, wParam, lParam);
        }

        LRESULT CALLBACK CreateProjectWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            StartupScreenController* controller = GetControllerFromWindow(window);
            if (nullptr != controller)
            {
                return controller->HandleCreateProjectWindowMessage(window, message, wParam, lParam);
            }

            if (message == WM_CLOSE)
            {
                ShowWindow(window, SW_HIDE);
                return 0;
            }

            return DefWindowProcW(window, message, wParam, lParam);
        }

        [[nodiscard]] bool RegisterCreateProjectWindowClass(HINSTANCE hInstance)
        {
            WNDCLASSW windowClass{};
            if (GetClassInfoW(hInstance, CreateProjectWindowClassName, &windowClass))
            {
                return true;
            }

            windowClass.lpfnWndProc = CreateProjectWindowProc;
            windowClass.hInstance = hInstance;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            windowClass.hbrBackground = nullptr;
            windowClass.lpszClassName = CreateProjectWindowClassName;
            return 0 != RegisterClassW(&windowClass);
        }

        [[nodiscard]] std::wstring BuildRecentProjectLabel(const EditorProjectInfo& project)
        {
            return project.name + L" - " + project.projectFilePath.parent_path().wstring();
        }

        [[nodiscard]] std::wstring FormatProjectFileTime(const std::filesystem::path& projectFilePath)
        {
            std::error_code errorCode;
            const std::filesystem::file_time_type fileTime = std::filesystem::last_write_time(projectFilePath, errorCode);
            if (errorCode)
            {
                return L"-";
            }

            const auto systemTime =
                std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    fileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
            const std::time_t timeValue = std::chrono::system_clock::to_time_t(systemTime);
            std::tm localTime{};
            if (0 != localtime_s(&localTime, &timeValue))
            {
                return L"-";
            }

            std::wstringstream stream{};
            stream << std::put_time(&localTime, L"%Y-%m-%d %H:%M");
            return stream.str();
        }

        [[nodiscard]] std::wstring BuildAllProjectLabel(const EditorProjectInfo& project)
        {
            return project.name
                + L"    "
                + project.projectFilePath.parent_path().wstring()
                + L"    "
                + FormatProjectFileTime(project.projectFilePath);
        }

        [[nodiscard]] bool IsExistingValidProject(const EditorProjectInfo& project)
        {
            EditorProject validator{};
            return validator.Open(project.projectFilePath);
        }

        [[nodiscard]] std::filesystem::file_time_type GetSortableProjectTime(const EditorProjectInfo& project)
        {
            std::error_code errorCode;
            const std::filesystem::file_time_type fileTime = std::filesystem::last_write_time(project.projectFilePath, errorCode);
            return errorCode ? (std::filesystem::file_time_type::min)() : fileTime;
        }

        void SelectPenAndBrush(HDC deviceContext, HPEN pen, HBRUSH brush)
        {
            SelectObject(deviceContext, pen);
            SelectObject(deviceContext, brush);
        }
    }

    bool StartupScreenController::Initialize(HWND parentWindow, HINSTANCE hInstance, Platform::IFileDialog& fileDialog)
    {
        m_fileDialog = &fileDialog;
        m_hidden = false;
        EnsureThemeResources();
        SubclassParentWindow(parentWindow);
        (void)RefreshDpiResources(parentWindow);

        m_projectTabButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"プロジェクト", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW);
        m_viewTabButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"表示", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW);
        m_createButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"プロジェクト作成", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW);
        m_openButton = CreateChildWindow(parentWindow, hInstance, L"Button", L"プロジェクトを開く", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW);
        m_recentLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"最近使ったプロジェクト一覧", WS_CHILD | WS_VISIBLE);
        m_recentListBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS);
        m_allProjectsLabel = CreateChildWindow(parentWindow, hInstance, L"Static", L"作成済みプロジェクト", WS_CHILD);
        m_allProjectsListBox = CreateChildWindow(
            parentWindow,
            hInstance,
            L"ListBox",
            L"",
            WS_CHILD | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS);

        if (nullptr == m_projectTabButton
            || nullptr == m_viewTabButton
            || nullptr == m_createButton
            || nullptr == m_openButton
            || nullptr == m_recentLabel
            || nullptr == m_recentListBox
            || nullptr == m_allProjectsLabel
            || nullptr == m_allProjectsListBox)
        {
            return false;
        }

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
            ScaleMetric(640),
            ScaleMetric(260),
            parentWindow,
            nullptr,
            hInstance,
            nullptr);

        if (nullptr == m_createProjectWindow)
        {
            return false;
        }

        SetPropW(m_createProjectWindow, ParentControllerPropertyName, this);

        m_nameLabel = CreateChildWindow(m_createProjectWindow, hInstance, L"Static", L"プロジェクト名", WS_CHILD | WS_VISIBLE);
        m_projectNameEdit = CreateChildWindow(m_createProjectWindow, hInstance, L"Edit", L"NewProject", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
        m_folderLabel = CreateChildWindow(m_createProjectWindow, hInstance, L"Static", L"保存先フォルダ", WS_CHILD | WS_VISIBLE);
        m_projectFolderEdit = CreateChildWindow(m_createProjectWindow, hInstance, L"Edit", L".", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL);
        m_browseFolderButton = CreateChildWindow(m_createProjectWindow, hInstance, L"Button", L"参照", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW);
        m_createConfirmButton = CreateChildWindow(m_createProjectWindow, hInstance, L"Button", L"作成", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW);
        m_createCancelButton = CreateChildWindow(m_createProjectWindow, hInstance, L"Button", L"キャンセル", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW);

        if (nullptr == m_nameLabel
            || nullptr == m_projectNameEdit
            || nullptr == m_folderLabel
            || nullptr == m_projectFolderEdit
            || nullptr == m_browseFolderButton
            || nullptr == m_createConfirmButton
            || nullptr == m_createCancelButton)
        {
            return false;
        }

        SetWindowLongPtrW(m_projectTabButton, GWLP_ID, ProjectTabButtonId);
        SetWindowLongPtrW(m_viewTabButton, GWLP_ID, ViewTabButtonId);
        SetWindowLongPtrW(m_createButton, GWLP_ID, CreateProjectButtonId);
        SetWindowLongPtrW(m_openButton, GWLP_ID, OpenProjectButtonId);
        SetWindowLongPtrW(m_recentListBox, GWLP_ID, RecentProjectsListBoxId);
        SetWindowLongPtrW(m_allProjectsListBox, GWLP_ID, AllProjectsListBoxId);
        SetWindowLongPtrW(m_browseFolderButton, GWLP_ID, BrowseFolderButtonId);
        SetWindowLongPtrW(m_createConfirmButton, GWLP_ID, CreateConfirmButtonId);
        SetWindowLongPtrW(m_createCancelButton, GWLP_ID, CreateCancelButtonId);

        SendMessageW(m_recentListBox, LB_SETITEMHEIGHT, 0, ScaleMetric(44));
        SendMessageW(m_allProjectsListBox, LB_SETITEMHEIGHT, 0, ScaleMetric(44));
        UpdateCreateProjectWindowLayout();
        RefreshRecentProjects();
        RefreshAllProjects();
        SetActiveTab(StartupTab::Projects);
        return true;
    }

    StartupScreenController::~StartupScreenController()
    {
        DestroyThemeResources();

        if (m_ownsDefaultFont && nullptr != m_defaultFont)
        {
            HFONT stockFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            const std::array<HWND, 16> controls{
                m_projectTabButton,
                m_viewTabButton,
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
                m_recentListBox,
                m_allProjectsLabel,
                m_allProjectsListBox
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

        const int headerHeight = ScaleMetric(48);
        const int tabHeight = ScaleMetric(52);
        const int margin = ScaleMetric(28);
        const int contentTop = headerHeight + tabHeight + ScaleMetric(28);
        const int contentWidth = (std::min)(ScaleMetric(1360), (std::max)(ScaleMetric(420), clientWidth - margin * 2));
        const int contentLeft = (std::max)(margin, (clientWidth - contentWidth) / 2);
        const int cardGap = ScaleMetric(40);
        const int cardHeight = ScaleMetric(118);
        const int cardWidth = (contentWidth - cardGap) / 2;
        const bool twoColumns = cardWidth >= ScaleMetric(320);
        const int actualCardWidth = twoColumns ? cardWidth : contentWidth;
        const int secondCardLeft = twoColumns ? contentLeft + actualCardWidth + cardGap : contentLeft;
        const int secondCardTop = twoColumns ? contentTop : contentTop + cardHeight + ScaleMetric(16);
        const int listTop = (twoColumns ? contentTop + cardHeight : secondCardTop + cardHeight) + ScaleMetric(36);
        const int listHeight = (std::max)(ScaleMetric(180), clientHeight - listTop - ScaleMetric(54));

        MoveWindow(m_projectTabButton, ScaleMetric(16), headerHeight, ScaleMetric(130), tabHeight, TRUE);
        MoveWindow(m_viewTabButton, ScaleMetric(154), headerHeight, ScaleMetric(90), tabHeight, TRUE);
        MoveWindow(m_createButton, contentLeft, contentTop, actualCardWidth, cardHeight, TRUE);
        MoveWindow(m_openButton, secondCardLeft, secondCardTop, actualCardWidth, cardHeight, TRUE);
        MoveWindow(m_recentLabel, contentLeft + ScaleMetric(20), listTop - ScaleMetric(38), contentWidth, ScaleMetric(28), TRUE);
        MoveWindow(m_recentListBox, contentLeft, listTop, contentWidth, listHeight, TRUE);
        MoveWindow(m_allProjectsLabel, contentLeft + ScaleMetric(20), contentTop, contentWidth, ScaleMetric(28), TRUE);
        MoveWindow(m_allProjectsListBox, contentLeft, contentTop + ScaleMetric(42), contentWidth, (std::max)(ScaleMetric(260), clientHeight - contentTop - ScaleMetric(88)), TRUE);

        m_layoutInitialized = true;
        m_lastLayoutClientWidth = clientWidth;
        m_lastLayoutClientHeight = clientHeight;
        m_lastLayoutDpi = m_currentDpi;
        SetActiveTab(m_activeTab);
        InvalidateRect(parentWindow, nullptr, TRUE);
    }

    bool StartupScreenController::HandleDrawItem(LPARAM drawItemParameter)
    {
        const DRAWITEMSTRUCT* drawItem = reinterpret_cast<DRAWITEMSTRUCT*>(drawItemParameter);
        if (nullptr == drawItem)
        {
            return false;
        }

        switch (drawItem->CtlID)
        {
        case CreateProjectButtonId:
        case OpenProjectButtonId:
        case BrowseFolderButtonId:
        case CreateConfirmButtonId:
        case CreateCancelButtonId:
            DrawThemedButton(*drawItem);
            return true;
        case ProjectTabButtonId:
            DrawTabButton(*drawItem, StartupTab::Projects);
            return true;
        case ViewTabButtonId:
            DrawTabButton(*drawItem, StartupTab::View);
            return true;
        case RecentProjectsListBoxId:
        case AllProjectsListBoxId:
            DrawProjectListItem(*drawItem);
            return true;
        default:
            return false;
        }
    }

    void StartupScreenController::Update(const Core::InputSnapshot& inputSnapshot)
    {
        if (m_hidden)
        {
            return;
        }

        const bool isLeftMouseDown = inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left);
        if (false == m_wasLeftMouseDown || isLeftMouseDown)
        {
            m_wasLeftMouseDown = isLeftMouseDown;
            return;
        }

        m_wasLeftMouseDown = false;

        const POINT cursorPosition = ToWin32Point(inputSnapshot.GetCursorScreenPoint());
        const HWND clickedWindow = WindowFromPoint(cursorPosition);
        if (clickedWindow == m_projectTabButton)
        {
            SetActiveTab(StartupTab::Projects);
            return;
        }

        if (clickedWindow == m_viewTabButton)
        {
            RefreshAllProjects();
            SetActiveTab(StartupTab::View);
            return;
        }

        if (clickedWindow == m_createButton)
        {
            ShowCreateProjectWindow();
            return;
        }

        if (clickedWindow == m_browseFolderButton)
        {
            if (nullptr != m_fileDialog)
            {
                const std::optional<std::filesystem::path> folderPath =
                    m_fileDialog->OpenFolder(
                        Platform::FolderDialogOptions{
                            GetParent(m_browseFolderButton),
                            L"プロジェクトの保存先フォルダを選択"
                        });
                if (folderPath.has_value())
                {
                    SetWindowTextW(m_projectFolderEdit, folderPath->c_str());
                }
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

        if (clickedWindow == m_recentListBox
            && HandleProjectListDoubleClick(m_recentListBox, m_recentProjects, cursorPosition))
        {
            return;
        }

        if (clickedWindow == m_allProjectsListBox
            && HandleProjectListDoubleClick(m_allProjectsListBox, m_allProjects, cursorPosition))
        {
            return;
        }

        if (clickedWindow == m_openButton)
        {
            const std::optional<EditorProjectInfo> selectedProject =
                StartupTab::View == m_activeTab ? GetSelectedAllProject() : GetSelectedRecentProject();
            if (selectedProject.has_value())
            {
                m_openProjectFilePath = selectedProject->projectFilePath;
                return;
            }

            if (nullptr == m_fileDialog)
            {
                return;
            }

            const std::optional<std::filesystem::path> filePath =
                m_fileDialog->OpenFile(
                    Platform::FileDialogOptions{
                        GetParent(m_openButton),
                        {},
                        {
                            Platform::FileDialogFilter{ L"Xelqoria Project (*.proj)", L"*.proj" },
                            Platform::FileDialogFilter{ L"All Files (*.*)", L"*.*" }
                        },
                        {}
                    });
            if (filePath.has_value())
            {
                m_openProjectFilePath = *filePath;
            }
        }
    }

    void StartupScreenController::Hide()
    {
        m_hidden = true;
        std::array<HWND, 8> controls{
            m_projectTabButton,
            m_viewTabButton,
            m_createButton,
            m_openButton,
            m_recentLabel,
            m_recentListBox,
            m_allProjectsLabel,
            m_allProjectsListBox
        };

        for (HWND control : controls)
        {
            ShowWindow(control, SW_HIDE);
        }

        HideCreateProjectWindow();
        if (nullptr != m_parentWindow)
        {
            RedrawWindow(m_parentWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        }
    }

    void StartupScreenController::Destroy()
    {
        HWND parentWindow = m_parentWindow;

        std::array<HWND*, 8> startupControls{
            &m_projectTabButton,
            &m_viewTabButton,
            &m_createButton,
            &m_openButton,
            &m_recentLabel,
            &m_recentListBox,
            &m_allProjectsLabel,
            &m_allProjectsListBox
        };

        for (HWND* control : startupControls)
        {
            DestroyWindowHandle(*control);
        }

        if (nullptr != m_createProjectWindow)
        {
            RemovePropW(m_createProjectWindow, ParentControllerPropertyName);
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
        UnsubclassParentWindow();

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
        RefreshRecentProjects();
        RefreshAllProjects();
    }

    void StartupScreenController::RefreshRecentProjects()
    {
        m_recentProjects = m_recentProjectsStore.Load();
        SendMessageW(m_recentListBox, LB_RESETCONTENT, 0, 0);
        if (m_recentProjects.empty())
        {
            SendMessageW(m_recentListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"最近使ったプロジェクトはありません"));
            return;
        }

        for (const EditorProjectInfo& project : m_recentProjects)
        {
            const std::wstring label = BuildRecentProjectLabel(project);
            SendMessageW(m_recentListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        }
    }

    void StartupScreenController::RefreshAllProjects()
    {
        m_allProjects = m_recentProjectsStore.Load();
        m_allProjects.erase(
            std::remove_if(
                m_allProjects.begin(),
                m_allProjects.end(),
                [](const EditorProjectInfo& project)
                {
                    return false == IsExistingValidProject(project);
                }),
            m_allProjects.end());
        std::sort(
            m_allProjects.begin(),
            m_allProjects.end(),
            [](const EditorProjectInfo& left, const EditorProjectInfo& right)
            {
                return GetSortableProjectTime(right) < GetSortableProjectTime(left);
            });

        SendMessageW(m_allProjectsListBox, LB_RESETCONTENT, 0, 0);
        if (m_allProjects.empty())
        {
            SendMessageW(m_allProjectsListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"表示できるプロジェクトはありません"));
            return;
        }

        for (const EditorProjectInfo& project : m_allProjects)
        {
            const std::wstring label = BuildAllProjectLabel(project);
            SendMessageW(m_allProjectsListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        }
    }

    void StartupScreenController::SetActiveTab(StartupTab tab)
    {
        m_activeTab = tab;
        const bool showProjects = StartupTab::Projects == m_activeTab;
        ShowWindow(m_createButton, showProjects ? SW_SHOW : SW_HIDE);
        ShowWindow(m_openButton, showProjects ? SW_SHOW : SW_HIDE);
        ShowWindow(m_recentLabel, showProjects ? SW_SHOW : SW_HIDE);
        ShowWindow(m_recentListBox, showProjects ? SW_SHOW : SW_HIDE);
        ShowWindow(m_allProjectsLabel, showProjects ? SW_HIDE : SW_SHOW);
        ShowWindow(m_allProjectsListBox, showProjects ? SW_HIDE : SW_SHOW);
        InvalidateRect(m_projectTabButton, nullptr, TRUE);
        InvalidateRect(m_viewTabButton, nullptr, TRUE);
        if (nullptr != m_parentWindow)
        {
            InvalidateRect(m_parentWindow, nullptr, TRUE);
        }
    }

    void StartupScreenController::EnsureThemeResources()
    {
        if (nullptr != m_darkBackgroundBrush)
        {
            return;
        }

        m_darkBackgroundBrush = CreateSolidBrush(BackgroundColor);
        m_panelBackgroundBrush = CreateSolidBrush(PanelColor);
        m_editBackgroundBrush = CreateSolidBrush(EditColor);
        m_accentPen = CreatePen(PS_SOLID, 1, AccentColor);
        m_dimAccentPen = CreatePen(PS_SOLID, 1, BorderColor);
        m_blueAccentPen = CreatePen(PS_SOLID, 1, AccentBlueColor);
    }

    void StartupScreenController::DestroyThemeResources()
    {
        if (nullptr != m_darkBackgroundBrush)
        {
            DeleteObject(m_darkBackgroundBrush);
            m_darkBackgroundBrush = nullptr;
        }

        if (nullptr != m_panelBackgroundBrush)
        {
            DeleteObject(m_panelBackgroundBrush);
            m_panelBackgroundBrush = nullptr;
        }

        if (nullptr != m_editBackgroundBrush)
        {
            DeleteObject(m_editBackgroundBrush);
            m_editBackgroundBrush = nullptr;
        }

        if (nullptr != m_accentPen)
        {
            DeleteObject(m_accentPen);
            m_accentPen = nullptr;
        }

        if (nullptr != m_dimAccentPen)
        {
            DeleteObject(m_dimAccentPen);
            m_dimAccentPen = nullptr;
        }

        if (nullptr != m_blueAccentPen)
        {
            DeleteObject(m_blueAccentPen);
            m_blueAccentPen = nullptr;
        }
    }

    void StartupScreenController::SubclassParentWindow(HWND parentWindow)
    {
        if (nullptr == parentWindow || m_parentWindow == parentWindow)
        {
            return;
        }

        m_parentWindow = parentWindow;
        SetPropW(m_parentWindow, ParentControllerPropertyName, this);
        m_originalParentWindowProc =
            reinterpret_cast<WNDPROC>(SetWindowLongPtrW(m_parentWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(StartupParentWindowProc)));
    }

    void StartupScreenController::UnsubclassParentWindow()
    {
        if (nullptr != m_parentWindow && nullptr != m_originalParentWindowProc)
        {
            SetWindowLongPtrW(m_parentWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalParentWindowProc));
        }

        if (nullptr != m_parentWindow)
        {
            RemovePropW(m_parentWindow, ParentControllerPropertyName);
        }

        m_parentWindow = nullptr;
        m_originalParentWindowProc = nullptr;
    }

    void StartupScreenController::PaintStartupBackground(HDC deviceContext)
    {
        RECT clientRect{};
        GetClientRect(m_parentWindow, &clientRect);
        FillRect(deviceContext, &clientRect, m_darkBackgroundBrush);

        RECT headerRect{ clientRect.left, clientRect.top, clientRect.right, ScaleMetric(48) };
        HBRUSH headerBrush = CreateSolidBrush(HeaderColor);
        FillRect(deviceContext, &headerRect, headerBrush);
        DeleteObject(headerBrush);

        RECT tabRect{ clientRect.left, ScaleMetric(48), clientRect.right, ScaleMetric(100) };
        HBRUSH tabBrush = CreateSolidBrush(RGB(8, 10, 28));
        FillRect(deviceContext, &tabRect, tabBrush);
        DeleteObject(tabBrush);

        SelectObject(deviceContext, m_dimAccentPen);
        MoveToEx(deviceContext, 0, ScaleMetric(99), nullptr);
        LineTo(deviceContext, clientRect.right, ScaleMetric(99));

        SetBkMode(deviceContext, TRANSPARENT);
        SetTextColor(deviceContext, TextColor);

        SelectObject(deviceContext, m_blueAccentPen);
        const int baseX = clientRect.right - ScaleMetric(210);
        const int baseY = clientRect.bottom - ScaleMetric(160);
        for (int index = 0; index < 6; ++index)
        {
            const int offset = ScaleMetric(index * 22);
            POINT points[7]{
                { baseX + offset, baseY - ScaleMetric(40) - offset / 2 },
                { baseX + ScaleMetric(58) + offset, baseY - ScaleMetric(12) - offset / 2 },
                { baseX + ScaleMetric(58) + offset, baseY + ScaleMetric(48) - offset / 2 },
                { baseX + offset, baseY + ScaleMetric(78) - offset / 2 },
                { baseX - ScaleMetric(58) + offset, baseY + ScaleMetric(48) - offset / 2 },
                { baseX - ScaleMetric(58) + offset, baseY - ScaleMetric(12) - offset / 2 },
                { baseX + offset, baseY - ScaleMetric(40) - offset / 2 }
            };
            Polyline(deviceContext, points, 7);
        }
    }

    void StartupScreenController::PaintCreateProjectBackground(HDC deviceContext)
    {
        RECT clientRect{};
        GetClientRect(m_createProjectWindow, &clientRect);
        FillRect(deviceContext, &clientRect, m_darkBackgroundBrush);

        SetBkMode(deviceContext, TRANSPARENT);
        SetTextColor(deviceContext, TextColor);
        SelectObject(deviceContext, m_blueAccentPen);
        const int baseX = clientRect.right - ScaleMetric(82);
        const int baseY = clientRect.bottom - ScaleMetric(36);
        for (int index = 0; index < 4; ++index)
        {
            RECT rect{
                baseX - ScaleMetric(18 * index),
                baseY - ScaleMetric(18 * index),
                baseX + ScaleMetric(48 + 18 * index),
                baseY + ScaleMetric(48 + 18 * index)
            };
            RoundRect(deviceContext, rect.left, rect.top, rect.right, rect.bottom, ScaleMetric(8), ScaleMetric(8));
        }
    }

    void StartupScreenController::DrawThemedButton(const DRAWITEMSTRUCT& drawItem) const
    {
        const bool selected = 0 != (drawItem.itemState & ODS_SELECTED);
        const bool hot = 0 != (drawItem.itemState & ODS_HOTLIGHT);
        HBRUSH fillBrush = CreateSolidBrush(selected ? RGB(18, 16, 50) : (hot ? PanelHoverColor : PanelColor));
        HPEN borderPen = CreatePen(PS_SOLID, 1, drawItem.CtlID == CreateConfirmButtonId ? AccentColor : BorderColor);
        SelectPenAndBrush(drawItem.hDC, borderPen, fillBrush);
        const RECT rect = drawItem.rcItem;
        RoundRect(drawItem.hDC, rect.left, rect.top, rect.right, rect.bottom, ScaleMetric(8), ScaleMetric(8));

        SetBkMode(drawItem.hDC, TRANSPARENT);
        SetTextColor(drawItem.hDC, drawItem.CtlID == CreateCancelButtonId ? MutedTextColor : TextColor);
        const int textLength = GetWindowTextLengthW(drawItem.hwndItem);
        std::wstring text(static_cast<std::size_t>(textLength) + 1u, L'\0');
        GetWindowTextW(drawItem.hwndItem, text.data(), textLength + 1);
        text.resize(static_cast<std::size_t>(textLength));

        RECT textRect = rect;
        if (drawItem.CtlID == CreateProjectButtonId || drawItem.CtlID == OpenProjectButtonId)
        {
            textRect.left += ScaleMetric(132);
            textRect.right -= ScaleMetric(24);
            DrawTextW(drawItem.hDC, text.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            SelectObject(drawItem.hDC, m_accentPen);
            if (drawItem.CtlID == CreateProjectButtonId)
            {
                const int centerX = rect.left + ScaleMetric(78);
                const int centerY = rect.top + (rect.bottom - rect.top) / 2;
                POINT hex[7]{
                    { centerX, centerY - ScaleMetric(34) },
                    { centerX + ScaleMetric(30), centerY - ScaleMetric(17) },
                    { centerX + ScaleMetric(30), centerY + ScaleMetric(17) },
                    { centerX, centerY + ScaleMetric(34) },
                    { centerX - ScaleMetric(30), centerY + ScaleMetric(17) },
                    { centerX - ScaleMetric(30), centerY - ScaleMetric(17) },
                    { centerX, centerY - ScaleMetric(34) }
                };
                Polyline(drawItem.hDC, hex, 7);
                MoveToEx(drawItem.hDC, centerX - ScaleMetric(14), centerY, nullptr);
                LineTo(drawItem.hDC, centerX + ScaleMetric(14), centerY);
                MoveToEx(drawItem.hDC, centerX, centerY - ScaleMetric(14), nullptr);
                LineTo(drawItem.hDC, centerX, centerY + ScaleMetric(14));
            }
            else
            {
                RECT folder{ rect.left + ScaleMetric(58), rect.top + ScaleMetric(38), rect.left + ScaleMetric(120), rect.top + ScaleMetric(82) };
                MoveToEx(drawItem.hDC, folder.left, folder.top + ScaleMetric(10), nullptr);
                LineTo(drawItem.hDC, folder.left + ScaleMetric(18), folder.top + ScaleMetric(10));
                LineTo(drawItem.hDC, folder.left + ScaleMetric(24), folder.top);
                LineTo(drawItem.hDC, folder.right - ScaleMetric(10), folder.top);
                LineTo(drawItem.hDC, folder.right - ScaleMetric(10), folder.top + ScaleMetric(16));
                LineTo(drawItem.hDC, folder.right, folder.top + ScaleMetric(16));
                LineTo(drawItem.hDC, folder.right, folder.bottom);
                LineTo(drawItem.hDC, folder.left, folder.bottom);
                LineTo(drawItem.hDC, folder.left, folder.top + ScaleMetric(10));
            }
        }
        else
        {
            DrawTextW(drawItem.hDC, text.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        DeleteObject(fillBrush);
        DeleteObject(borderPen);
    }

    void StartupScreenController::DrawTabButton(const DRAWITEMSTRUCT& drawItem, StartupTab tab) const
    {
        const bool active = m_activeTab == tab;
        FillRect(drawItem.hDC, &drawItem.rcItem, m_darkBackgroundBrush);
        SetBkMode(drawItem.hDC, TRANSPARENT);
        SetTextColor(drawItem.hDC, active ? TextColor : MutedTextColor);
        const int textLength = GetWindowTextLengthW(drawItem.hwndItem);
        std::wstring text(static_cast<std::size_t>(textLength) + 1u, L'\0');
        GetWindowTextW(drawItem.hwndItem, text.data(), textLength + 1);
        text.resize(static_cast<std::size_t>(textLength));
        RECT textRect = drawItem.rcItem;
        DrawTextW(drawItem.hDC, text.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (active)
        {
            HPEN glowPen = CreatePen(PS_SOLID, ScaleMetric(3), AccentColor);
            SelectObject(drawItem.hDC, glowPen);
            MoveToEx(drawItem.hDC, drawItem.rcItem.left + ScaleMetric(10), drawItem.rcItem.bottom - ScaleMetric(4), nullptr);
            LineTo(drawItem.hDC, drawItem.rcItem.right - ScaleMetric(10), drawItem.rcItem.bottom - ScaleMetric(4));
            DeleteObject(glowPen);
        }
    }

    void StartupScreenController::DrawProjectListItem(const DRAWITEMSTRUCT& drawItem) const
    {
        if (drawItem.itemID == static_cast<UINT>(-1))
        {
            FillRect(drawItem.hDC, &drawItem.rcItem, m_panelBackgroundBrush);
            return;
        }

        const bool selected = 0 != (drawItem.itemState & ODS_SELECTED);
        HBRUSH rowBrush = CreateSolidBrush(selected ? RGB(34, 24, 80) : RGB(18, 18, 48));
        RECT rowRect = drawItem.rcItem;
        rowRect.left += ScaleMetric(8);
        rowRect.right -= ScaleMetric(8);
        rowRect.top += ScaleMetric(4);
        rowRect.bottom -= ScaleMetric(4);
        FillRect(drawItem.hDC, &drawItem.rcItem, m_panelBackgroundBrush);
        SelectPenAndBrush(drawItem.hDC, selected ? m_accentPen : m_dimAccentPen, rowBrush);
        RoundRect(drawItem.hDC, rowRect.left, rowRect.top, rowRect.right, rowRect.bottom, ScaleMetric(6), ScaleMetric(6));
        DeleteObject(rowBrush);

        wchar_t buffer[1024]{};
        SendMessageW(drawItem.hwndItem, LB_GETTEXT, drawItem.itemID, reinterpret_cast<LPARAM>(buffer));
        SetBkMode(drawItem.hDC, TRANSPARENT);
        SetTextColor(drawItem.hDC, TextColor);
        RECT textRect = rowRect;
        textRect.left += ScaleMetric(18);
        textRect.right -= ScaleMetric(18);
        DrawTextW(drawItem.hDC, buffer, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    LRESULT StartupScreenController::HandleParentWindowMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (false == m_hidden)
        {
            if (WM_ERASEBKGND == message)
            {
                return 1;
            }

            if (WM_PAINT == message)
            {
                PAINTSTRUCT paintStruct{};
                HDC deviceContext = BeginPaint(window, &paintStruct);
                PaintStartupBackground(deviceContext);
                EndPaint(window, &paintStruct);
                return 0;
            }

            if (WM_CTLCOLORSTATIC == message || WM_CTLCOLOREDIT == message || WM_CTLCOLORLISTBOX == message)
            {
                HDC deviceContext = reinterpret_cast<HDC>(wParam);
                SetBkMode(deviceContext, TRANSPARENT);
                SetTextColor(deviceContext, TextColor);
                if (WM_CTLCOLOREDIT == message)
                {
                    SetBkColor(deviceContext, EditColor);
                    return reinterpret_cast<LRESULT>(m_editBackgroundBrush);
                }

                if (WM_CTLCOLORLISTBOX == message)
                {
                    SetBkColor(deviceContext, PanelColor);
                    return reinterpret_cast<LRESULT>(m_panelBackgroundBrush);
                }

                return reinterpret_cast<LRESULT>(m_darkBackgroundBrush);
            }
        }

        return CallWindowProcW(m_originalParentWindowProc, window, message, wParam, lParam);
    }

    LRESULT StartupScreenController::HandleCreateProjectWindowMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (WM_CLOSE == message)
        {
            ShowWindow(window, SW_HIDE);
            return 0;
        }

        if (WM_ERASEBKGND == message)
        {
            return 1;
        }

        if (WM_PAINT == message)
        {
            PAINTSTRUCT paintStruct{};
            HDC deviceContext = BeginPaint(window, &paintStruct);
            PaintCreateProjectBackground(deviceContext);
            EndPaint(window, &paintStruct);
            return 0;
        }

        if (WM_DRAWITEM == message)
        {
            return HandleDrawItem(lParam) ? TRUE : FALSE;
        }

        if (WM_CTLCOLORSTATIC == message || WM_CTLCOLOREDIT == message)
        {
            HDC deviceContext = reinterpret_cast<HDC>(wParam);
            SetBkMode(deviceContext, TRANSPARENT);
            SetTextColor(deviceContext, TextColor);
            if (WM_CTLCOLOREDIT == message)
            {
                SetBkColor(deviceContext, EditColor);
                return reinterpret_cast<LRESULT>(m_editBackgroundBrush);
            }

            return reinterpret_cast<LRESULT>(m_darkBackgroundBrush);
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

    bool StartupScreenController::HandleProjectListDoubleClick(
        HWND listBox,
        const std::vector<EditorProjectInfo>& projects,
        POINT cursorPosition)
    {
        POINT listBoxPosition = cursorPosition;
        ScreenToClient(listBox, &listBoxPosition);

        const LPARAM positionParameter = MAKELPARAM(listBoxPosition.x, listBoxPosition.y);
        const LRESULT itemResult = SendMessageW(listBox, LB_ITEMFROMPOINT, 0, positionParameter);
        if (HIWORD(itemResult) != 0)
        {
            if (listBox == m_recentListBox)
            {
                m_lastRecentClickIndex = -1;
            }
            else
            {
                m_lastAllProjectsClickIndex = -1;
            }
            return false;
        }

        const int clickedIndex = LOWORD(itemResult);
        DWORD& lastClickTime = listBox == m_recentListBox ? m_lastRecentClickTime : m_lastAllProjectsClickTime;
        int& lastClickIndex = listBox == m_recentListBox ? m_lastRecentClickIndex : m_lastAllProjectsClickIndex;
        const DWORD currentTime = GetTickCount();
        const bool isDoubleClick =
            clickedIndex == lastClickIndex
            && currentTime - lastClickTime <= GetDoubleClickTime();

        lastClickIndex = clickedIndex;
        lastClickTime = currentTime;

        if (false == isDoubleClick)
        {
            return false;
        }

        const auto projectIndex = static_cast<std::size_t>(clickedIndex);
        if (projectIndex >= projects.size())
        {
            return false;
        }

        m_openProjectFilePath = projects[projectIndex].projectFilePath;
        return true;
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

        const int width = ScaleMetric(640);
        const int height = ScaleMetric(260);
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
        MoveWindow(m_nameLabel, ScaleMetric(32), ScaleMetric(48), ScaleMetric(150), ScaleMetric(28), TRUE);
        MoveWindow(m_projectNameEdit, ScaleMetric(190), ScaleMetric(44), ScaleMetric(400), ScaleMetric(32), TRUE);
        MoveWindow(m_folderLabel, ScaleMetric(32), ScaleMetric(98), ScaleMetric(150), ScaleMetric(28), TRUE);
        MoveWindow(m_projectFolderEdit, ScaleMetric(190), ScaleMetric(94), ScaleMetric(286), ScaleMetric(32), TRUE);
        MoveWindow(m_browseFolderButton, ScaleMetric(492), ScaleMetric(94), ScaleMetric(98), ScaleMetric(32), TRUE);
        MoveWindow(m_createConfirmButton, ScaleMetric(356), ScaleMetric(160), ScaleMetric(112), ScaleMetric(36), TRUE);
        MoveWindow(m_createCancelButton, ScaleMetric(478), ScaleMetric(160), ScaleMetric(112), ScaleMetric(36), TRUE);
        if (nullptr != m_createProjectWindow)
        {
            InvalidateRect(m_createProjectWindow, nullptr, TRUE);
        }
    }

    bool StartupScreenController::HandleRecentProjectDoubleClick(POINT cursorPosition)
    {
        return HandleProjectListDoubleClick(m_recentListBox, m_recentProjects, cursorPosition);
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

    std::optional<EditorProjectInfo> StartupScreenController::GetSelectedAllProject() const
    {
        const LRESULT selectedIndex = SendMessageW(m_allProjectsListBox, LB_GETCURSEL, 0, 0);
        if (selectedIndex == LB_ERR || selectedIndex < 0)
        {
            return std::nullopt;
        }

        const auto index = static_cast<std::size_t>(selectedIndex);
        if (index >= m_allProjects.size())
        {
            return std::nullopt;
        }

        return m_allProjects[index];
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
        fontDesc.lfHeight = -MulDiv(11, static_cast<int>(m_currentDpi), 72);
        fontDesc.lfWeight = FW_NORMAL;
        wcscpy_s(fontDesc.lfFaceName, L"Segoe UI");
        m_defaultFont = CreateFontIndirectW(&fontDesc);
        m_ownsDefaultFont = nullptr != m_defaultFont;
        if (nullptr == m_defaultFont)
        {
            m_defaultFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            m_ownsDefaultFont = false;
        }

        const std::array<HWND, 16> controls{
            m_projectTabButton,
            m_viewTabButton,
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
            m_recentListBox,
            m_allProjectsLabel,
            m_allProjectsListBox
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
