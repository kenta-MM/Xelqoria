#include "Application.h"

#include <array>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <functional>
#include <span>
#include <string>

#include "GraphicsAPI.h"
#include <Windows.h>
#include <optional>
#include <InputSystem.h>
#include "EditorCommandController.h"
#include "EditorStringUtils.h"
#include "InspectorPanelController.h"
#include "SceneEditingOperations.h"
#include "SceneViewInteractionTypes.h"
#include "ScriptAssetService.h"
#include "Win32PlatformFactory.h"
#include <Entity.h>
#include <cstdint>

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr unsigned ProjectMenuCreateCommandId = 5101;
        constexpr unsigned ProjectMenuSaveCommandId = 5102;
        constexpr unsigned ProjectMenuSaveAsCommandId = 5103;
        constexpr unsigned ProjectMenuOpenCommandId = 5104;
        constexpr unsigned ProjectMenuSettingsCommandId = 5105;
        constexpr unsigned ProjectMenuResetLayoutCommandId = 5106;

        [[nodiscard]] POINT ToWin32Point(Platform::Point point)
        {
            return POINT{ static_cast<LONG>(point.x), static_cast<LONG>(point.y) };
        }

        [[nodiscard]] std::filesystem::path SelectProjectFile(
            Platform::IFileDialog& fileDialog,
            HWND ownerWindow)
        {
            const std::optional<std::filesystem::path> filePath =
                fileDialog.OpenFile(
                    Platform::FileDialogOptions{
                        ownerWindow,
                        {},
                        {
                            Platform::FileDialogFilter{ L"Xelqoria Project (*.proj)", L"*.proj" },
                            Platform::FileDialogFilter{ L"All Files (*.*)", L"*.*" }
                        },
                        {}
                    });
            return filePath.value_or(std::filesystem::path{});
        }

        [[nodiscard]] std::filesystem::path SelectScriptAssetFile(
            Platform::IFileDialog& fileDialog,
            HWND ownerWindow,
            const std::filesystem::path& initialDirectory)
        {
            const std::optional<std::filesystem::path> filePath =
                fileDialog.OpenFile(
                    Platform::FileDialogOptions{
                        ownerWindow,
                        {},
                        {
                            Platform::FileDialogFilter{ L"Xelqoria Script Asset (*.script)", L"*.script" },
                            Platform::FileDialogFilter{ L"All Files (*.*)", L"*.*" }
                        },
                        initialDirectory
                    });
            return filePath.value_or(std::filesystem::path{});
        }


        [[nodiscard]] std::filesystem::path SelectFolder(
            Platform::IFileDialog& fileDialog,
            HWND ownerWindow,
            const wchar_t* title)
        {
            const std::optional<std::filesystem::path> folderPath =
                fileDialog.OpenFolder(
                    Platform::FolderDialogOptions{
                        ownerWindow,
                        title
                    });
            return folderPath.value_or(std::filesystem::path{});
        }

        [[nodiscard]] std::wstring BuildNewProjectName(const std::filesystem::path& parentDirectory)
        {
            std::wstring projectName = L"NewProject";
            for (int index = 1; std::filesystem::exists(parentDirectory / projectName); ++index)
            {
                projectName = L"NewProject" + std::to_wstring(index);
            }

            return projectName;
        }

        /// <summary>
        /// Script ビルドに使用する C++ コンパイラを取得する。
        /// </summary>
        /// <returns>コンパイラ実行ファイルまたはコマンド。</returns>
        [[nodiscard]] std::filesystem::path GetScriptCompilerExecutable()
        {
            wchar_t* configuredCompiler = nullptr;
            std::size_t configuredCompilerLength = 0;
            const errno_t environmentResult =
                _wdupenv_s(&configuredCompiler, &configuredCompilerLength, L"XELQORIA_SCRIPT_COMPILER");
            if (0 == environmentResult
                && nullptr != configuredCompiler
                && 0 < configuredCompilerLength
                && L'\0' != configuredCompiler[0])
            {
                const std::filesystem::path compilerPath = configuredCompiler;
                free(configuredCompiler);
                return compilerPath;
            }

            free(configuredCompiler);
            return L"cl.exe";
        }

        /// <summary>
        /// Visual Studio の開発者環境セットアップバッチを取得する。
        /// </summary>
        /// <returns>VsDevCmd.bat のパス。見つからない場合は空。</returns>
        [[nodiscard]] std::filesystem::path GetScriptEnvironmentSetupBatch()
        {
            wchar_t* configuredBatch = nullptr;
            std::size_t configuredBatchLength = 0;
            const errno_t environmentResult =
                _wdupenv_s(&configuredBatch, &configuredBatchLength, L"XELQORIA_SCRIPT_ENVIRONMENT");
            if (0 == environmentResult
                && nullptr != configuredBatch
                && 0 < configuredBatchLength
                && L'\0' != configuredBatch[0])
            {
                const std::filesystem::path batchPath = configuredBatch;
                free(configuredBatch);
                return batchPath;
            }

            free(configuredBatch);

            const std::array<std::filesystem::path, 4> candidates{
                L"C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/VsDevCmd.bat",
                L"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/Tools/VsDevCmd.bat",
                L"C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/Common7/Tools/VsDevCmd.bat",
                L"C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/VsDevCmd.bat"
            };

            for (const std::filesystem::path& candidate : candidates)
            {
                std::error_code errorCode;
                if (std::filesystem::exists(candidate, errorCode) && false == static_cast<bool>(errorCode))
                {
                    return candidate;
                }
            }

            return {};
        }

        /// <summary>
        /// Script ビルド結果を SceneView 表示用の短い文面へ変換する。
        /// </summary>
        /// <param name="buildResult">Script ビルド結果。</param>
        /// <returns>表示文面。</returns>
        [[nodiscard]] std::wstring BuildScriptBuildStatusText(const ScriptBuildResult& buildResult)
        {
            if (buildResult.succeeded)
            {
                return L"再生開始前 Script ビルドに成功しました。";
            }

            std::wstring text = L"再生開始前 Script ビルドに失敗しました。詳細: ";
            text += buildResult.diagnostics;
            constexpr std::size_t MaxLabelLength = 180;
            if (MaxLabelLength < text.size())
            {
                text.resize(MaxLabelLength - 3);
                text += L"...";
            }

            return text;
        }

        /// <summary>
        /// Script Runtime 診断を短い表示文面へ変換する。
        /// </summary>
        /// <param name="diagnostics">Script Runtime 診断一覧。</param>
        /// <returns>表示文面。</returns>
        [[nodiscard]] std::wstring BuildScriptRuntimeStatusText(
            std::span<const ScriptRuntimeDiagnostic> diagnostics)
        {
            if (diagnostics.empty())
            {
                return L"Script 再生を開始しました。";
            }

            std::wstring text = L"Script 再生開始時に問題が発生しました。詳細: ";
            text += diagnostics.front().message;
            constexpr std::size_t MaxLabelLength = 180;
            if (MaxLabelLength < text.size())
            {
                text.resize(MaxLabelLength - 3);
                text += L"...";
            }

            return text;
        }
    }

    Application::Application(HINSTANCE hInstance)
        : m_hInstance(hInstance)
    {
    }

    Application::~Application()
    {
        Shutdown();
    }

    int Application::Run()
    {
        if (false == Initialize())
        {
            return -1;
        }

        using clock = std::chrono::steady_clock;
        auto lastTime = clock::now();

        while (m_running)
        {
            if (false == m_window.PumpMessages())
            {
                m_running = false;
                continue;
            }

            const auto currentTime = clock::now();
            const std::chrono::duration<float> delta = currentTime - lastTime;
            lastTime = currentTime;

            Update(delta.count());
            Render();
        }

        return 0;
    }

    bool Application::Initialize()
    {
        constexpr auto clientWidth = 1600u;
        constexpr auto clientHeight = 900u;

        const std::wstring title = L"Xelqoria Editor";
        m_window.SetPlatformWindow(
            Platform::Win32::CreateWin32Window(m_hInstance),
            Platform::Win32::CreateWin32EventLoop());
        m_inputSystem.SetPlatformInput(Platform::Win32::CreateWin32Input());

        if (false == m_window.Create(title.c_str(), clientWidth, clientHeight))
        {
            return false;
        }
        m_inputSystem.SetMouseWheelDeltaReader(
            [this]()
            {
                return m_window.ConsumeMouseWheelDelta();
            });

        InitializeProjectMenu();
        m_window.SetCommandHandler(
            [this](unsigned commandId)
            {
                HandleProjectMenuCommand(commandId);
            });
        m_window.SetNotifyHandler(
            [this](Platform::NativeMessageParameter notifyParameter)
            {
                if (m_editorShell.HandleNotify(static_cast<LPARAM>(notifyParameter)))
                {
                    return true;
                }

                return m_assetsPanelController.HandleNotify(static_cast<LPARAM>(notifyParameter));
            });
        m_window.SetDrawItemHandler(
            [this](Platform::NativeMessageParameter drawItemParameter)
            {
                return m_logOutputPanelController.HandleDrawItem(static_cast<LPARAM>(drawItemParameter));
            });
        m_window.SetCloseRequestHandler(
            [this]()
            {
                return HandleCloseRequest();
            });
        m_window.SetResizeHandler(
            [this](std::uint32_t width, std::uint32_t height)
            {
                HandleWindowResized(width, height);
            });

        if (false == m_startupScreenController.Initialize(GetMainWindowHandle(), m_hInstance, m_fileDialog))
        {
            return false;
        }

        m_startupScreenController.UpdateLayout(GetMainWindowHandle());
        m_window.Show();
        return true;
    }

    bool Application::InitializeEditorWorkspace()
    {
        if (m_editorInitialized)
        {
            return true;
        }

        constexpr RHI::GraphicsAPI api = RHI::GraphicsAPI::D3D11;

        if (false == m_editorShell.Initialize(GetMainWindowHandle(), m_hInstance, m_cursor))
        {
            return false;
        }

        m_assetsPanelController.Bind(m_editorShell, m_cursor);
        m_hierarchyPanelController.Bind(m_editorShell);
        m_inspectorPanelController.Bind(m_editorShell, m_cursor);
        m_logOutputPanelController.Bind(m_editorShell, m_clipboard);
        m_projectPanelController.Bind(m_editorShell);
        m_sceneViewController.Bind(m_editorShell);
        m_buildAndPlayButton = m_editorShell.GetBuildAndPlayButton();
        m_pauseResumePlayButton = m_editorShell.GetPauseResumePlayButton();
        m_endPlayButton = m_editorShell.GetEndPlayButton();

        const bool sceneViewSizeChanged = m_editorShell.UpdateLayout(GetMainWindowHandle());
        if (true == sceneViewSizeChanged)
        {
            m_sceneViewController.OnViewportChanged(
                m_editorShell.GetSceneViewWidth(),
                m_editorShell.GetSceneViewHeight());
        }

        if (false == m_sceneViewRenderer.Initialize(
                m_hInstance,
                m_editorShell.GetSceneViewHost(),
                m_editorShell.GetSceneViewWidth(),
                m_editorShell.GetSceneViewHeight(),
                api))
        {
            return false;
        }

        if (false == m_sceneDocument.Initialize(m_sceneViewRenderer.GetGraphicsContext()))
        {
            return false;
        }

        m_assetsPanelController.Refresh(m_sceneDocument.GetProjectInfo());
        const bool canAddSpriteComponent = m_assetsPanelController.HasVisibleSpriteAssets();
        RefreshEditorPanels(canAddSpriteComponent, false);
        RefreshSceneViewSelectionStatus();
        RefreshEditorPlayControls();
        AppendEditorLog(L"Editor ワークスペースを初期化しました。");
        m_editorCommandController.Reset(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
        m_editorInitialized = true;
        RedrawWindow(GetMainWindowHandle(), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        return true;
    }

    void Application::InitializeProjectMenu()
    {
        m_projectMenu = CreatePopupMenu();
        AppendMenuW(m_projectMenu, MF_STRING, ProjectMenuCreateCommandId, L"プロジェクトを作成する");
        AppendMenuW(m_projectMenu, MF_STRING, ProjectMenuSaveCommandId, L"プロジェクトを保存する");
        AppendMenuW(m_projectMenu, MF_STRING, ProjectMenuSaveAsCommandId, L"プロジェクトを別名で保存する");
        AppendMenuW(m_projectMenu, MF_STRING, ProjectMenuOpenCommandId, L"プロジェクトを開く");
        AppendMenuW(m_projectMenu, MF_STRING, ProjectMenuSettingsCommandId, L"プロジェクトの設定を開く");
        AppendMenuW(m_projectMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(m_projectMenu, MF_STRING, ProjectMenuResetLayoutCommandId, L"画面レイアウトを初期状態に戻す");

        HMENU menuBar = CreateMenu();
        AppendMenuW(menuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(m_projectMenu), L"プロジェクト");
        SetMenu(GetMainWindowHandle(), menuBar);
    }

    void Application::HandleProjectMenuCommand(unsigned commandId)
    {
        switch (commandId)
        {
        case ProjectMenuCreateCommandId:
            CreateProjectFromMenu();
            break;
        case ProjectMenuSaveCommandId:
            SaveProjectFromMenu();
            break;
        case ProjectMenuSaveAsCommandId:
            SaveProjectAsFromMenu();
            break;
        case ProjectMenuOpenCommandId:
            OpenProjectFromMenu();
            break;
        case ProjectMenuSettingsCommandId:
            if (m_editorInitialized)
            {
                SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクト設定画面は未実装です。");
            }
            break;
        case ProjectMenuResetLayoutCommandId:
            if (m_editorInitialized)
            {
                m_editorShell.ResetDockLayout();
                SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"画面レイアウトを初期状態に戻しました。");
            }
            break;
        default:
            break;
        }
    }

    bool Application::HandleCloseRequest()
    {
        return ConfirmSaveIfDirty();
    }

    void Application::HandleWindowResized(std::uint32_t width, std::uint32_t height)
    {
        if (0 == width || 0 == height)
        {
            return;
        }

        if (false == m_editorInitialized)
        {
            m_startupScreenController.UpdateLayout(GetMainWindowHandle());
            return;
        }

        const bool sceneViewSizeChanged = m_editorShell.UpdateLayout(GetMainWindowHandle());
        if (sceneViewSizeChanged)
        {
            m_sceneViewController.OnViewportChanged(
                m_editorShell.GetSceneViewWidth(),
                m_editorShell.GetSceneViewHeight());
            m_sceneViewRenderer.Resize(
                m_editorShell.GetSceneViewWidth(),
                m_editorShell.GetSceneViewHeight());
        }
    }

    bool Application::EnterEditorWithNewProject()
    {
        const std::wstring projectName = m_startupScreenController.GetProjectName();
        const std::filesystem::path projectParentDirectory = m_startupScreenController.GetProjectParentDirectory();
        m_startupScreenController.Destroy();

        if (false == InitializeEditorWorkspace())
        {
            return false;
        }

        if (false == m_sceneDocument.CreateProject(projectName, projectParentDirectory))
        {
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクト作成に失敗しました。");
            AppendEditorLog(L"プロジェクト作成に失敗しました。");
            return false;
        }

        RecordCurrentProject();
        m_assetsPanelController.Refresh(m_sceneDocument.GetProjectInfo());
        m_sceneDocument.RefreshProjectAssetRegistries();
        m_projectPanelController.Refresh(m_sceneDocument);
        ClearProjectDirty();
        AppendEditorLog(L"プロジェクトを作成しました。");
        return true;
    }

    bool Application::EnterEditorWithExistingProject()
    {
        const std::filesystem::path projectFilePath = m_startupScreenController.GetOpenProjectFilePath();
        m_startupScreenController.Destroy();

        if (false == InitializeEditorWorkspace())
        {
            return false;
        }

        if (false == m_sceneDocument.OpenProject(projectFilePath))
        {
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクトを開けませんでした。");
            AppendEditorLog(L"プロジェクトを開けませんでした。");
            return false;
        }

        RecordCurrentProject();
        m_assetsPanelController.Refresh(m_sceneDocument.GetProjectInfo());
        m_sceneDocument.RefreshProjectAssetRegistries();
        const bool canAddSpriteComponent = m_assetsPanelController.HasVisibleSpriteAssets();
        m_projectPanelController.Refresh(m_sceneDocument);
        ApplySelectionChange(std::nullopt, canAddSpriteComponent, true, true);
        ClearProjectDirty();
        AppendEditorLog(L"プロジェクトを開きました。");
        return true;
    }

    void Application::CreateProjectFromMenu()
    {
        if (false == ConfirmSaveIfDirty())
        {
            return;
        }

        const std::filesystem::path parentDirectory =
            SelectFolder(m_fileDialog, GetMainWindowHandle(), L"プロジェクトの保存先フォルダを選択");
        if (parentDirectory.empty())
        {
            return;
        }

        const std::wstring projectName = BuildNewProjectName(parentDirectory);
        m_startupScreenController.Destroy();

        if (false == InitializeEditorWorkspace())
        {
            return;
        }

        if (false == m_sceneDocument.CreateProject(projectName, parentDirectory))
        {
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクト作成に失敗しました。");
            AppendEditorLog(L"プロジェクト作成に失敗しました。");
            return;
        }

        RecordCurrentProject();
        m_assetsPanelController.Refresh(m_sceneDocument.GetProjectInfo());
        m_sceneDocument.RefreshProjectAssetRegistries();
        m_projectPanelController.Refresh(m_sceneDocument);
        ApplySelectionChange(std::nullopt, m_assetsPanelController.HasVisibleSpriteAssets(), true, true);
        m_editorCommandController.Reset(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
        ClearProjectDirty();
        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクトを作成しました。");
        AppendEditorLog(L"プロジェクトを作成しました。");
    }

    void Application::OpenProjectFromMenu()
    {
        if (false == ConfirmSaveIfDirty())
        {
            return;
        }

        const std::filesystem::path projectFilePath = SelectProjectFile(m_fileDialog, GetMainWindowHandle());
        if (projectFilePath.empty())
        {
            return;
        }

        m_startupScreenController.Destroy();

        if (false == InitializeEditorWorkspace())
        {
            return;
        }

        if (false == m_sceneDocument.OpenProject(projectFilePath))
        {
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクトを開けませんでした。");
            AppendEditorLog(L"プロジェクトを開けませんでした。");
            return;
        }

        RecordCurrentProject();
        m_assetsPanelController.Refresh(m_sceneDocument.GetProjectInfo());
        m_sceneDocument.RefreshProjectAssetRegistries();
        m_projectPanelController.Refresh(m_sceneDocument);
        ApplySelectionChange(std::nullopt, m_assetsPanelController.HasVisibleSpriteAssets(), true, true);
        m_editorCommandController.Reset(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
        ClearProjectDirty();
        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクトを開きました。");
        AppendEditorLog(L"プロジェクトを開きました。");
    }

    bool Application::SaveProjectFromMenu()
    {
        if (false == m_editorInitialized)
        {
            return false;
        }

        if (false == m_sceneDocument.Save())
        {
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクトの保存に失敗しました。");
            AppendEditorLog(L"プロジェクトの保存に失敗しました。");
            return false;
        }

        m_editorCommandController.PushSnapshot(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
        m_projectPanelController.Refresh(m_sceneDocument);
        ClearProjectDirty();
        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクトを保存しました。");
        AppendEditorLog(L"プロジェクトを保存しました。");
        return true;
    }

    void Application::SaveProjectAsFromMenu()
    {
        if (false == m_editorInitialized)
        {
            return;
        }

        const std::filesystem::path parentDirectory =
            SelectFolder(m_fileDialog, GetMainWindowHandle(), L"別名保存先フォルダを選択");
        if (parentDirectory.empty())
        {
            return;
        }

        std::wstring projectName = L"NewProject";
        if (m_sceneDocument.GetProjectInfo().has_value())
        {
            projectName = m_sceneDocument.GetProjectInfo()->name + L"_Copy";
        }

        if (false == m_sceneDocument.SaveProjectAs(projectName, parentDirectory))
        {
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクトの別名保存に失敗しました。");
            AppendEditorLog(L"プロジェクトの別名保存に失敗しました。");
            return;
        }

        RecordCurrentProject();
        m_assetsPanelController.Refresh(m_sceneDocument.GetProjectInfo());
        m_sceneDocument.RefreshProjectAssetRegistries();
        m_projectPanelController.Refresh(m_sceneDocument);
        ClearProjectDirty();
        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクトを別名で保存しました。");
        AppendEditorLog(L"プロジェクトを別名で保存しました。");
    }

    bool Application::ConfirmSaveIfDirty()
    {
        if (false == m_projectDirty)
        {
            return true;
        }

        const int result = MessageBoxW(
            GetMainWindowHandle(),
            L"プロジェクトを保存しますか？",
            L"保存確認",
            MB_YESNOCANCEL | MB_ICONQUESTION);
        if (result == IDYES)
        {
            return SaveProjectFromMenu();
        }

        if (result == IDCANCEL)
        {
            return false;
        }

        return true;
    }

    void Application::MarkProjectDirty()
    {
        m_projectDirty = true;
    }

    bool Application::StartEditorPlay()
    {
        if (m_scriptBuildInProgress || m_scriptRuntimeSession.IsPlaying())
        {
            return false;
        }

        StartEditorPlayBuild();
        return m_scriptBuildInProgress;
    }

    void Application::StartEditorPlayBuild()
    {
        if (m_scriptBuildInProgress || m_scriptRuntimeSession.IsPlaying())
        {
            return;
        }

        if (false == m_sceneDocument.GetProjectInfo().has_value() || nullptr == m_sceneDocument.GetScene())
        {
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"再生開始前 Script ビルドにはプロジェクトが必要です。");
            AppendBuildLog(L"再生開始前 Script ビルドにはプロジェクトが必要です。");
            return;
        }

        const std::filesystem::path projectRootDirectory =
            m_sceneDocument.GetProjectInfo()->projectFilePath.parent_path();
        const ScriptBuildOptions buildOptions{
            GetScriptCompilerExecutable(),
            GetScriptEnvironmentSetupBatch()
        };
        m_scriptBuildProjectRootDirectory = projectRootDirectory;
        m_scriptBuildInProgress = true;
        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Script ビルドをバックグラウンドで開始しました。");
        AppendBuildLog(L"Script ビルドをバックグラウンドで開始しました。");
        RefreshEditorPlayControls();
        m_scriptBuildFuture = std::async(
            std::launch::async,
            [projectRootDirectory, buildOptions]()
            {
                return ScriptAssetService::BuildProjectScripts(projectRootDirectory, buildOptions);
            });
    }

    void Application::PollEditorPlayBuild()
    {
        if (false == m_scriptBuildInProgress || false == m_scriptBuildFuture.valid())
        {
            return;
        }

        if (m_scriptBuildFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            return;
        }

        ScriptBuildResult buildResult{};
        try
        {
            buildResult = m_scriptBuildFuture.get();
        }
        catch (const std::exception& exception)
        {
            buildResult.succeeded = false;
            buildResult.diagnostics = L"Script ビルド中に例外が発生しました: ";
            buildResult.diagnostics += ToWideString(exception.what());
        }
        catch (...)
        {
            buildResult.succeeded = false;
            buildResult.diagnostics = L"Script ビルド中に不明な例外が発生しました。";
        }

        m_scriptBuildInProgress = false;
        const std::filesystem::path projectRootDirectory =
            m_scriptBuildProjectRootDirectory.value_or(std::filesystem::path{});
        m_scriptBuildProjectRootDirectory.reset();

        const std::wstring statusText = BuildScriptBuildStatusText(buildResult);
        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), statusText.c_str());
        AppendBuildLog(statusText, false == buildResult.succeeded);
        if (false == buildResult.diagnostics.empty())
        {
            AppendBuildLog(buildResult.diagnostics, false == buildResult.succeeded);
        }

        if (false == buildResult.succeeded)
        {
            m_logOutputPanelController.SelectCategory(LogOutputCategory::Build);
            RefreshEditorPlayControls();
            return;
        }

        (void)BeginEditorPlayRuntime(buildResult, projectRootDirectory);
        RefreshEditorPlayControls();
    }

    bool Application::BeginEditorPlayRuntime(
        const ScriptBuildResult& buildResult,
        const std::filesystem::path& projectRootDirectory)
    {
        if (nullptr == m_sceneDocument.GetScene())
        {
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Script 再生には Scene が必要です。");
            AppendGameLog(L"Script 再生には Scene が必要です。");
            return false;
        }

        const bool runtimeStarted = m_scriptRuntimeSession.Begin(
            *m_sceneDocument.GetScene(),
            m_sceneDocument.GetSpriteAssetRegistry(),
            buildResult,
            projectRootDirectory);
        const std::wstring runtimeStatusText =
            BuildScriptRuntimeStatusText(m_scriptRuntimeSession.GetDiagnostics());
        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), runtimeStatusText.c_str());
        AppendGameLog(runtimeStatusText);
        return runtimeStarted;
    }

    void Application::ToggleEditorPlayPause()
    {
        if (false == m_scriptRuntimeSession.IsPlaying())
        {
            return;
        }

        if (m_scriptRuntimeSession.IsPaused())
        {
            m_scriptRuntimeSession.Resume();
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Script 再生を再開しました。");
            AppendGameLog(L"Script 再生を再開しました。");
        }
        else
        {
            m_scriptRuntimeSession.Pause();
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Script 再生を停止しました。");
            AppendGameLog(L"Script 再生を停止しました。");
        }

        RefreshEditorPlayControls();
    }

    void Application::EndEditorPlay()
    {
        if (false == m_scriptRuntimeSession.IsPlaying())
        {
            return;
        }

        m_scriptRuntimeSession.End();
        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Script 再生を終了しました。");
        AppendGameLog(L"Script 再生を終了しました。");
        RefreshEditorPlayControls();
    }

    void Application::RefreshEditorPlayControls()
    {
        if (nullptr == m_buildAndPlayButton || nullptr == m_pauseResumePlayButton || nullptr == m_endPlayButton)
        {
            return;
        }

        const bool isPlaying = m_scriptRuntimeSession.IsPlaying();
        EnableWindow(m_buildAndPlayButton, false == m_scriptBuildInProgress && false == isPlaying ? TRUE : FALSE);
        EnableWindow(m_pauseResumePlayButton, isPlaying ? TRUE : FALSE);
        EnableWindow(m_endPlayButton, isPlaying ? TRUE : FALSE);
        SetWindowTextW(m_buildAndPlayButton, m_scriptBuildInProgress ? L"ビルド中" : L"ビルドして開始");
        SetWindowTextW(m_pauseResumePlayButton, m_scriptRuntimeSession.IsPaused() ? L"再開" : L"停止");
    }

    void Application::UpdateEditorPlayControls(const Core::InputSnapshot& inputSnapshot)
    {
        const ButtonClickFrameInput frameInput{
            inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left),
            ToWin32Point(inputSnapshot.GetCursorScreenPoint())
        };

        if (TryConsumeButtonClick(m_buildAndPlayButton, frameInput, m_editorPlayButtonInputState))
        {
            (void)StartEditorPlay();
            m_editorPlayButtonInputState.pressedButtonHandle = nullptr;
        }
        else if (TryConsumeButtonClick(m_pauseResumePlayButton, frameInput, m_editorPlayButtonInputState))
        {
            ToggleEditorPlayPause();
            m_editorPlayButtonInputState.pressedButtonHandle = nullptr;
        }
        else if (TryConsumeButtonClick(m_endPlayButton, frameInput, m_editorPlayButtonInputState))
        {
            EndEditorPlay();
            m_editorPlayButtonInputState.pressedButtonHandle = nullptr;
        }

        if (false == frameInput.isLeftMouseButtonDown && true == m_editorPlayButtonInputState.wasLeftMouseButtonDown)
        {
            m_editorPlayButtonInputState.pressedButtonHandle = nullptr;
        }

        m_editorPlayButtonInputState.wasLeftMouseButtonDown = frameInput.isLeftMouseButtonDown;
    }

    void Application::ClearProjectDirty()
    {
        m_projectDirty = false;
    }

    void Application::Shutdown()
    {
        m_sceneViewRenderer.Shutdown();
    }

    void Application::Update(float deltaTime)
    {
        m_inputSystem.Update();
        const Core::InputSnapshot& inputSnapshot = m_inputSystem.GetSnapshot();

        if (false == m_editorInitialized)
        {
            m_startupScreenController.UpdateLayout(GetMainWindowHandle());
            m_startupScreenController.Update(inputSnapshot);
            if (m_startupScreenController.HasCreateRequest())
            {
                if (EnterEditorWithNewProject())
                {
                    m_startupScreenController.ClearRequests();
                }
            }
            else if (m_startupScreenController.HasOpenRequest())
            {
                if (EnterEditorWithExistingProject())
                {
                    m_startupScreenController.ClearRequests();
                }
            }

            return;
        }

        (void)m_editorShell.UpdateDocking(GetMainWindowHandle(), inputSnapshot);
        m_logOutputPanelController.Update(inputSnapshot);
        const bool sceneViewSizeChanged = m_editorShell.UpdateLayout(GetMainWindowHandle());
        if (true == sceneViewSizeChanged)
        {
            m_sceneViewController.OnViewportChanged(
                m_editorShell.GetSceneViewWidth(),
                m_editorShell.GetSceneViewHeight());
            m_sceneViewRenderer.Resize(
                m_editorShell.GetSceneViewWidth(),
                m_editorShell.GetSceneViewHeight());
        }

        m_assetsPanelController.SyncSelection();
        UpdateEditorPlayControls(inputSnapshot);
        PollEditorPlayBuild();
        if (true == m_projectPanelController.Update(m_sceneDocument))
        {
            const bool canAddSpriteComponentAfterSceneLoad = m_assetsPanelController.HasVisibleSpriteAssets();
            ApplySelectionChange(std::nullopt, canAddSpriteComponentAfterSceneLoad, true, true);
            m_editorCommandController.Reset(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
            MarkProjectDirty();
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"選択した Scene を読み込みました。");
            AppendEditorLog(L"選択した Scene を読み込みました。");
        }

        m_scriptRuntimeSession.Update(deltaTime);
        if (false == m_scriptRuntimeSession.GetDiagnostics().empty())
        {
            const std::wstring runtimeStatusText =
                BuildScriptRuntimeStatusText(m_scriptRuntimeSession.GetDiagnostics());
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), runtimeStatusText.c_str());
            AppendGameLog(runtimeStatusText);
        }

        m_assetsPanelController.UpdateDragState(inputSnapshot);
        m_inspectorPanelController.UpdateTextureDropHighlight(m_assetsPanelController);
        const bool canAddSpriteComponent = m_assetsPanelController.HasVisibleSpriteAssets();

        if (true == m_assetsPanelController.HasCreateSpriteRequest())
        {
            const std::filesystem::path createSpriteTargetDirectory =
                m_assetsPanelController.GetCreateSpriteTargetDirectory();
            m_assetsPanelController.ClearCreateSpriteRequest();

            if (nullptr != m_sceneDocument.GetScene())
            {
                const EditorCamera2D& sceneViewCamera = m_sceneViewController.GetCamera();
                const SceneEditResult createSpriteResult =
                    SceneEditingOperations::CreateUntexturedSprite(
                        *m_sceneDocument.GetScene(),
                        sceneViewCamera.GetCenterX(),
                        sceneViewCamera.GetCenterY());
                if (true == createSpriteResult.changed)
                {
                    const auto createdEntity = createSpriteResult.selectedEntityId.has_value()
                        ? m_sceneDocument.GetScene()->FindEntity(*createSpriteResult.selectedEntityId)
                        : std::optional<std::reference_wrapper<Game::Entity>>{};
                    if (true == createdEntity.has_value()
                        && true == m_sceneDocument.CreateSpriteAssetFile(
                            createdEntity->get(),
                            createSpriteTargetDirectory))
                    {
                        m_assetsPanelController.Refresh(m_sceneDocument.GetProjectInfo());
                    }

                    ApplySelectionChange(createSpriteResult.selectedEntityId, canAddSpriteComponent, true, true);
                    PersistSceneChanges(
                        L"Assets から Sprite を作成しました。",
                        L"Sprite は作成されましたが、Scene の保存に失敗しました。",
                        true);
                }
            }
        }

        if (true == m_assetsPanelController.HasCreateScriptRequest())
        {
            const std::filesystem::path createScriptTargetDirectory =
                m_assetsPanelController.GetCreateScriptTargetDirectory();
            m_assetsPanelController.ClearCreateScriptRequest();

            const ScriptAssetCreationResult createScriptResult =
                m_sceneDocument.CreateScriptAssetFile(createScriptTargetDirectory);
            if (true == createScriptResult.succeeded)
            {
                m_assetsPanelController.Refresh(m_sceneDocument.GetProjectInfo());
                SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Script Asset を作成しました。");
                AppendEditorLog(L"Script Asset を作成しました。");
            }
            else
            {
                SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Script Asset の作成に失敗しました。");
                AppendEditorLog(L"Script Asset の作成に失敗しました。");
            }
        }

        if (true == m_assetsPanelController.HasAssignScriptRequest())
        {
            const std::filesystem::path spriteAssetPath =
                m_assetsPanelController.GetAssignScriptSpriteAssetPath();
            m_assetsPanelController.ClearAssignScriptRequest();

            const std::filesystem::path projectRootDirectory =
                m_sceneDocument.GetProjectInfo().has_value()
                ? m_sceneDocument.GetProjectInfo()->projectFilePath.parent_path()
                : std::filesystem::path{};
            const std::filesystem::path scriptAssetPath =
                SelectScriptAssetFile(m_fileDialog, GetMainWindowHandle(), projectRootDirectory);
            if (false == scriptAssetPath.empty())
            {
                if (true == m_sceneDocument.AssignScriptAssetToSpriteAssetFile(spriteAssetPath, scriptAssetPath))
                {
                    m_assetsPanelController.Refresh(m_sceneDocument.GetProjectInfo());
                    SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Sprite Asset に Script Asset を割り当てました。");
                    AppendEditorLog(L"Sprite Asset に Script Asset を割り当てました。");
                }
                else
                {
                    SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Script Asset の割り当てに失敗しました。");
                    AppendEditorLog(L"Script Asset の割り当てに失敗しました。");
                }
            }
        }

        if (true == m_hierarchyPanelController.SyncSelection())
        {
            RefreshEditorPanels(canAddSpriteComponent, true);
        }

        const SceneEditResult hierarchyEditResult =
            m_hierarchyPanelController.ApplyEdits(m_sceneDocument.GetScene(), inputSnapshot);
        if (true == hierarchyEditResult.changed)
        {
            if (true == hierarchyEditResult.createdSpriteEntity)
            {
                const auto createdEntity = hierarchyEditResult.selectedEntityId.has_value()
                    ? m_sceneDocument.GetScene()->FindEntity(*hierarchyEditResult.selectedEntityId)
                    : std::optional<std::reference_wrapper<Game::Entity>>{};
                if (true == createdEntity.has_value()
                    && true == m_sceneDocument.CreateSpriteAssetFile(createdEntity->get(), {}))
                {
                    m_assetsPanelController.Refresh(m_sceneDocument.GetProjectInfo());
                }
            }

            ApplySelectionChange(hierarchyEditResult.selectedEntityId, canAddSpriteComponent, true, true);
            PersistSceneChanges(
                L"Hierarchy の編集内容を Scene へ保存しました。",
                L"Hierarchy の編集内容は反映されましたが、Scene の保存に失敗しました。",
                true);
        }

        const InspectorApplyResult inspectorResult = m_inspectorPanelController.ApplyEdits(
            m_sceneDocument.GetScene(),
            m_hierarchyPanelController.GetSelectedEntityId(),
            canAddSpriteComponent,
            inputSnapshot,
            m_sceneDocument.GetSpriteAssetRegistry());
        if (true == inspectorResult.changed)
        {
            PersistSceneChanges(
                L"Inspector の編集内容を Scene へ保存しました。",
                L"Inspector の編集内容は反映されましたが、Scene の保存に失敗しました。",
                true);
            RefreshEditorPanels(canAddSpriteComponent, false);
            RefreshSceneViewSelectionStatus();
        }

        if (InspectorScriptAction::None != inspectorResult.scriptAction)
        {
            bool scriptActionSucceeded = false;
            bool scriptActionChangedScene = false;
            switch (inspectorResult.scriptAction)
            {
            case InspectorScriptAction::Create:
            {
                const std::optional<Core::AssetId> scriptTargetSpriteAssetId =
                    EnsureSelectedSpriteAssetFileForScript(
                        inspectorResult.scriptTargetSpriteAssetId,
                        scriptActionChangedScene);
                const ScriptAssetCreationResult createResult =
                    scriptTargetSpriteAssetId.has_value()
                    ? m_sceneDocument.CreateAndAssignScriptAssetToSpriteAsset(*scriptTargetSpriteAssetId)
                    : ScriptAssetCreationResult{};
                scriptActionSucceeded = createResult.succeeded;
                SetWindowTextW(
                    m_editorShell.GetSceneViewPlanLabel(),
                    scriptActionSucceeded
                        ? L"Script Asset を作成し、Sprite Asset に割り当てました。"
                        : L"Script Asset の作成または割り当てに失敗しました。");
                break;
            }
            case InspectorScriptAction::Assign:
            {
                const std::filesystem::path projectRootDirectory =
                    m_sceneDocument.GetProjectInfo().has_value()
                    ? m_sceneDocument.GetProjectInfo()->projectFilePath.parent_path()
                    : std::filesystem::path{};
                const std::filesystem::path scriptAssetPath =
                    SelectScriptAssetFile(m_fileDialog, GetMainWindowHandle(), projectRootDirectory);
                if (false == scriptAssetPath.empty())
                {
                    const std::optional<Core::AssetId> scriptTargetSpriteAssetId =
                        EnsureSelectedSpriteAssetFileForScript(
                            inspectorResult.scriptTargetSpriteAssetId,
                            scriptActionChangedScene);
                    scriptActionSucceeded = scriptTargetSpriteAssetId.has_value()
                        && m_sceneDocument.AssignScriptAssetToSpriteAsset(
                            *scriptTargetSpriteAssetId,
                            scriptAssetPath);
                    SetWindowTextW(
                        m_editorShell.GetSceneViewPlanLabel(),
                        scriptActionSucceeded
                            ? L"Sprite Asset に Script Asset を割り当てました。"
                            : L"Script Asset の割り当てに失敗しました。");
                }
                break;
            }
            case InspectorScriptAction::AssignDropped:
            {
                const std::optional<Core::AssetId> scriptTargetSpriteAssetId =
                    EnsureSelectedSpriteAssetFileForScript(
                        inspectorResult.scriptTargetSpriteAssetId,
                        scriptActionChangedScene);
                scriptActionSucceeded = scriptTargetSpriteAssetId.has_value()
                    && m_sceneDocument.AssignScriptAssetToSpriteAsset(
                        *scriptTargetSpriteAssetId,
                        inspectorResult.droppedScriptAssetPath);
                SetWindowTextW(
                    m_editorShell.GetSceneViewPlanLabel(),
                    scriptActionSucceeded
                        ? L"Sprite Asset に Script Asset を割り当てました。"
                        : L"Script Asset の割り当てに失敗しました。");
                break;
            }
            case InspectorScriptAction::Clear:
            {
                const std::optional<Core::AssetId> scriptTargetSpriteAssetId =
                    EnsureSelectedSpriteAssetFileForScript(
                        inspectorResult.scriptTargetSpriteAssetId,
                        scriptActionChangedScene);
                scriptActionSucceeded = scriptTargetSpriteAssetId.has_value()
                    && m_sceneDocument.ClearScriptAssetFromSpriteAsset(*scriptTargetSpriteAssetId);
                SetWindowTextW(
                    m_editorShell.GetSceneViewPlanLabel(),
                    scriptActionSucceeded
                        ? L"Sprite Asset の Script Asset 割り当てを解除しました。"
                        : L"Script Asset 割り当て解除に失敗しました。");
                break;
            }
            case InspectorScriptAction::None:
            default:
                break;
            }

            if (scriptActionSucceeded)
            {
                if (scriptActionChangedScene)
                {
                    (void)PersistSceneChanges(
                        L"Sprite の Script 設定を Scene へ保存しました。",
                        L"Script 設定は反映されましたが、Scene の保存に失敗しました。",
                        true);
                }

                m_assetsPanelController.Refresh(m_sceneDocument.GetProjectInfo());
                RefreshEditorPanels(canAddSpriteComponent, false);
            }
        }

        if (true == m_assetsPanelController.WasDragReleasedThisFrame()
            && false == m_assetsPanelController.GetDraggingImagePath().empty())
        {
            (void)m_sceneDocument.RegisterImageAsset(m_assetsPanelController.GetDraggingImagePath());
        }

        const InspectorApplyResult textureDropResult = m_inspectorPanelController.ApplyTextureDrop(
            m_sceneDocument.GetScene(),
            m_hierarchyPanelController.GetSelectedEntityId(),
            m_assetsPanelController,
            m_sceneDocument.GetSpriteAssetRegistry());
        if (true == textureDropResult.changed)
        {
            PersistSceneChanges(
                L"Texture を Sprite に設定しました。",
                L"Texture 設定は反映されましたが、Scene の保存に失敗しました。",
                true);
            RefreshEditorPanels(canAddSpriteComponent, false);
            RefreshSceneViewSelectionStatus();
        }

        const InspectorApplyResult scriptDropResult = m_inspectorPanelController.ApplyScriptDrop(
            m_sceneDocument.GetScene(),
            m_hierarchyPanelController.GetSelectedEntityId(),
            m_assetsPanelController);
        if (InspectorScriptAction::AssignDropped == scriptDropResult.scriptAction)
        {
            bool scriptDropChangedScene = false;
            const std::optional<Core::AssetId> scriptDropTargetSpriteAssetId =
                EnsureSelectedSpriteAssetFileForScript(
                    scriptDropResult.scriptTargetSpriteAssetId,
                    scriptDropChangedScene);
            const bool scriptDropSucceeded = scriptDropTargetSpriteAssetId.has_value()
                && m_sceneDocument.AssignScriptAssetToSpriteAsset(
                    *scriptDropTargetSpriteAssetId,
                    scriptDropResult.droppedScriptAssetPath);
            if (true == scriptDropSucceeded)
            {
                if (scriptDropChangedScene)
                {
                    (void)PersistSceneChanges(
                        L"Sprite の Script 設定を Scene へ保存しました。",
                        L"Script 設定は反映されましたが、Scene の保存に失敗しました。",
                        true);
                }

                m_assetsPanelController.Refresh(m_sceneDocument.GetProjectInfo());
                RefreshEditorPanels(canAddSpriteComponent, false);
                SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Sprite Asset に Script Asset を割り当てました。");
                AppendEditorLog(L"Sprite Asset に Script Asset を割り当てました。");
            }
            else
            {
                SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"Script Asset の割り当てに失敗しました。");
                AppendEditorLog(L"Script Asset の割り当てに失敗しました。");
            }
        }

        const SceneViewInteractionResult interactionResult = m_sceneViewController.UpdateInteraction(
            m_sceneDocument.GetScene(),
            m_sceneDocument.GetSpriteAssetRegistry(),
            m_sceneDocument.GetTextureAssetRegistry(),
            m_assetsPanelController,
            m_hierarchyPanelController.GetSelectedEntityId(),
            inputSnapshot);
        if (true == interactionResult.selectionChanged)
        {
            ApplySelectionChange(interactionResult.selectedEntityId, canAddSpriteComponent, true, false);
        }

        if (true == interactionResult.sceneChanged)
        {
            MarkProjectDirty();
            RefreshEditorPanels(canAddSpriteComponent, false);
            RefreshSceneViewSelectionStatus();
        }

        if (true == interactionResult.shouldPersistScene)
        {
            PersistSceneChanges(
                L"SceneView で Sprite の編集内容を保存しました。",
                L"SceneView で Sprite は編集されましたが、Scene の保存に失敗しました。",
                interactionResult.shouldPushHistory);
        }

        if (true == m_assetsPanelController.WasDragReleasedThisFrame())
        {
            m_assetsPanelController.CompleteReleasedDrag();
        }

        const SceneViewDropResult dropResult = m_sceneViewController.ProcessPendingSceneDrop(m_sceneDocument);
        if (true == dropResult.selectionChanged)
        {
            m_hierarchyPanelController.SetSelectedEntityId(dropResult.selectedEntityId);
        }

        if (true == dropResult.sceneChanged || true == dropResult.selectionChanged)
        {
            RefreshEditorPanels(canAddSpriteComponent, true);
        }

        if (true == dropResult.shouldPushHistory)
        {
            m_editorCommandController.PushSnapshot(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
        }

        const EditorCommandUpdateResult commandResult = m_editorCommandController.Update(
            m_sceneDocument,
            m_hierarchyPanelController.GetSelectedEntityId(),
            m_editorShell.GetSceneViewPlanLabel(),
            inputSnapshot);
        if (true == commandResult.changed)
        {
            MarkProjectDirty();
            ApplySelectionChange(commandResult.selectedEntityId, canAddSpriteComponent, true, true);
        }
    }

    void Application::Render()
    {
        if (false == m_editorInitialized)
        {
            return;
        }

        m_sceneViewRenderer.RenderFrame(
            m_sceneDocument.GetScene(),
            m_sceneDocument.GetSpriteAssetRegistry(),
            m_sceneDocument.GetTextureAssetRegistry(),
            m_sceneViewController.GetCamera(),
            m_hierarchyPanelController.GetSelectedEntityId(),
            m_sceneViewController.GetEditMode(),
            m_sceneViewController.GetDragPreviewState());
    }

    HWND Application::GetMainWindowHandle() const
    {
        return reinterpret_cast<HWND>(m_window.GetHwnd());
    }

    void Application::RefreshEditorPanels(bool canAddSpriteComponent, bool resetTrackedEntity)
    {
        if (true == resetTrackedEntity)
        {
            m_inspectorPanelController.ResetTrackedEntity();
        }

        m_hierarchyPanelController.Refresh(m_sceneDocument.GetScene());
        m_inspectorPanelController.Refresh(
            m_sceneDocument.GetScene(),
            m_hierarchyPanelController.GetSelectedEntityId(),
            canAddSpriteComponent,
            m_sceneDocument.GetSpriteAssetRegistry());
    }

    void Application::RefreshSceneViewSelectionStatus()
    {
        m_sceneViewController.RefreshSelectionStatus(
            m_sceneDocument.GetScene(),
            m_hierarchyPanelController.GetSelectedEntityId());
    }

    void Application::ApplySelectionChange(
        std::optional<Game::EntityId> selectedEntityId,
        bool canAddSpriteComponent,
        bool resetTrackedEntity,
        bool refreshSceneViewSelectionStatus)
    {
        m_hierarchyPanelController.SetSelectedEntityId(selectedEntityId);
        RefreshEditorPanels(canAddSpriteComponent, resetTrackedEntity);
        if (true == refreshSceneViewSelectionStatus)
        {
            RefreshSceneViewSelectionStatus();
        }
    }

    bool Application::PersistSceneChanges(
        const wchar_t* successMessage,
        const wchar_t* failureMessage,
        bool pushHistory)
    {
        if (m_sceneDocument.Save())
        {
            if (true == pushHistory)
            {
                m_editorCommandController.PushSnapshot(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
            }

            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), successMessage);
            AppendEditorLog(successMessage);
            m_projectPanelController.Refresh(m_sceneDocument);
            ClearProjectDirty();
            return true;
        }

        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), failureMessage);
        AppendEditorLog(failureMessage);
        MarkProjectDirty();
        return false;
    }

    std::optional<Core::AssetId> Application::EnsureSelectedSpriteAssetFileForScript(
        const Core::AssetId& requestedSpriteAssetId,
        bool& sceneChanged)
    {
        sceneChanged = false;
        if (requestedSpriteAssetId.IsEmpty())
        {
            return std::nullopt;
        }

        if (m_sceneDocument.ResolveSpriteAssetPath(requestedSpriteAssetId).has_value())
        {
            return requestedSpriteAssetId;
        }

        Game::Scene* scene = m_sceneDocument.GetScene();
        const std::optional<Game::EntityId> selectedEntityId = m_hierarchyPanelController.GetSelectedEntityId();
        if (nullptr == scene || false == selectedEntityId.has_value())
        {
            return std::nullopt;
        }

        const auto entity = scene->FindEntity(*selectedEntityId);
        if (false == entity.has_value())
        {
            return std::nullopt;
        }

        const auto spriteComponent = entity->get().GetSpriteComponent();
        if (false == spriteComponent.has_value()
            || spriteComponent->get().spriteAssetRef != requestedSpriteAssetId)
        {
            return std::nullopt;
        }

        const std::optional<Core::AssetId> ensuredSpriteAssetId =
            m_sceneDocument.EnsureSpriteAssetFileForEntity(entity->get(), {});
        if (false == ensuredSpriteAssetId.has_value())
        {
            return std::nullopt;
        }

        sceneChanged = *ensuredSpriteAssetId != requestedSpriteAssetId;
        return ensuredSpriteAssetId;
    }

    void Application::RecordCurrentProject()
    {
        if (m_sceneDocument.GetProjectInfo().has_value())
        {
            (void)m_recentProjectsStore.Record(*m_sceneDocument.GetProjectInfo());
        }
    }

    void Application::AppendEditorLog(const wchar_t* message)
    {
        if (nullptr == message)
        {
            return;
        }

        m_logOutputPanelController.Append(LogOutputCategory::Editor, message);
    }

    void Application::AppendGameLog(const std::wstring& message)
    {
        m_logOutputPanelController.Append(LogOutputCategory::Game, message);
    }

    void Application::AppendBuildLog(const std::wstring& message, bool isError)
    {
        m_logOutputPanelController.Append(LogOutputCategory::Build, message, isError);
    }
}
