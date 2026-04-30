#include "Application.h"

#include <array>
#include <chrono>
#include <commdlg.h>
#include <filesystem>
#include <shlobj.h>
#include <string>

#include "GraphicsAPI.h"
#include <ShlObj_core.h>
#include <shtypes.h>
#include <Windows.h>
#include <optional>
#include <InputSystem.h>
#include "EditorCommandController.h"
#include "InspectorPanelController.h"
#include "SceneEditingOperations.h"
#include "SceneViewInteractionTypes.h"
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

        [[nodiscard]] std::filesystem::path SelectProjectFile(HWND ownerWindow)
        {
            std::array<wchar_t, MAX_PATH> filePath{};
            OPENFILENAMEW openFileName{};
            openFileName.lStructSize = sizeof(openFileName);
            openFileName.hwndOwner = ownerWindow;
            openFileName.lpstrFilter = L"Xelqoria Project (*.proj)\0*.proj\0All Files (*.*)\0*.*\0";
            openFileName.lpstrFile = filePath.data();
            openFileName.nMaxFile = static_cast<DWORD>(filePath.size());
            openFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            if (GetOpenFileNameW(&openFileName))
            {
                return filePath.data();
            }

            return {};
        }

        [[nodiscard]] std::filesystem::path SelectFolder(HWND ownerWindow, const wchar_t* title)
        {
            BROWSEINFOW browseInfo{};
            browseInfo.hwndOwner = ownerWindow;
            browseInfo.lpszTitle = title;
            browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

            PIDLIST_ABSOLUTE itemList = SHBrowseForFolderW(&browseInfo);
            if (nullptr == itemList)
            {
                return {};
            }

            std::filesystem::path folderPath{};
            std::array<wchar_t, MAX_PATH> selectedPath{};
            if (SHGetPathFromIDListW(itemList, selectedPath.data()))
            {
                folderPath = selectedPath.data();
            }

            CoTaskMemFree(itemList);
            return folderPath;
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
        if (false == m_window.Create(m_hInstance, title.c_str(), clientWidth, clientHeight))
        {
            return false;
        }

        InitializeProjectMenu();
        m_window.SetCommandHandler(
            [this](unsigned commandId)
            {
                HandleProjectMenuCommand(commandId);
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

        if (false == m_startupScreenController.Initialize(m_window.GetHwnd(), m_hInstance))
        {
            return false;
        }

        m_startupScreenController.UpdateLayout(m_window.GetHwnd());
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

        if (false == m_editorShell.Initialize(m_window.GetHwnd(), m_hInstance))
        {
            return false;
        }

        m_assetsPanelController.Bind(m_editorShell);
        m_hierarchyPanelController.Bind(m_editorShell);
        m_inspectorPanelController.Bind(m_editorShell);
        m_projectPanelController.Bind(m_editorShell);
        m_sceneViewController.Bind(m_editorShell);

        const bool sceneViewSizeChanged = m_editorShell.UpdateLayout(m_window.GetHwnd());
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

        m_assetsPanelController.Refresh(
            m_sceneDocument.GetRegisteredSpriteAssetIds(),
            m_sceneDocument.GetSpriteAssetRegistry(),
            m_sceneDocument.GetTextureAssetRegistry());
        const bool canAddSpriteComponent = m_assetsPanelController.HasVisibleSpriteAssets();
        RefreshEditorPanels(canAddSpriteComponent, false);
        RefreshSceneViewSelectionStatus();
        m_editorCommandController.Reset(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
        m_editorInitialized = true;
        RedrawWindow(m_window.GetHwnd(), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
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

        HMENU menuBar = CreateMenu();
        AppendMenuW(menuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(m_projectMenu), L"プロジェクト");
        SetMenu(m_window.GetHwnd(), menuBar);
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
            m_startupScreenController.UpdateLayout(m_window.GetHwnd());
            return;
        }

        const bool sceneViewSizeChanged = m_editorShell.UpdateLayout(m_window.GetHwnd());
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
            return false;
        }

        RecordCurrentProject();
        m_projectPanelController.Refresh(m_sceneDocument);
        ClearProjectDirty();
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
            return false;
        }

        RecordCurrentProject();
        const bool canAddSpriteComponent = m_assetsPanelController.HasVisibleSpriteAssets();
        m_projectPanelController.Refresh(m_sceneDocument);
        ApplySelectionChange(std::nullopt, canAddSpriteComponent, true, true);
        ClearProjectDirty();
        return true;
    }

    void Application::CreateProjectFromMenu()
    {
        if (false == ConfirmSaveIfDirty())
        {
            return;
        }

        const std::filesystem::path parentDirectory = SelectFolder(m_window.GetHwnd(), L"プロジェクトの保存先フォルダを選択");
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
            return;
        }

        RecordCurrentProject();
        m_projectPanelController.Refresh(m_sceneDocument);
        ApplySelectionChange(std::nullopt, m_assetsPanelController.HasVisibleSpriteAssets(), true, true);
        m_editorCommandController.Reset(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
        ClearProjectDirty();
        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクトを作成しました。");
    }

    void Application::OpenProjectFromMenu()
    {
        if (false == ConfirmSaveIfDirty())
        {
            return;
        }

        const std::filesystem::path projectFilePath = SelectProjectFile(m_window.GetHwnd());
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
            return;
        }

        RecordCurrentProject();
        m_projectPanelController.Refresh(m_sceneDocument);
        ApplySelectionChange(std::nullopt, m_assetsPanelController.HasVisibleSpriteAssets(), true, true);
        m_editorCommandController.Reset(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
        ClearProjectDirty();
        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクトを開きました。");
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
            return false;
        }

        m_editorCommandController.PushSnapshot(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
        m_projectPanelController.Refresh(m_sceneDocument);
        ClearProjectDirty();
        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクトを保存しました。");
        return true;
    }

    void Application::SaveProjectAsFromMenu()
    {
        if (false == m_editorInitialized)
        {
            return;
        }

        const std::filesystem::path parentDirectory = SelectFolder(m_window.GetHwnd(), L"別名保存先フォルダを選択");
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
            return;
        }

        RecordCurrentProject();
        m_projectPanelController.Refresh(m_sceneDocument);
        ClearProjectDirty();
        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"プロジェクトを別名で保存しました。");
    }

    bool Application::ConfirmSaveIfDirty()
    {
        if (false == m_projectDirty)
        {
            return true;
        }

        const int result = MessageBoxW(
            m_window.GetHwnd(),
            L"プロジェクトを保存しますか？",
            L"保存確認",
            MB_YESNO | MB_ICONQUESTION);
        if (result == IDYES)
        {
            SaveProjectFromMenu();
        }

        return true;
    }

    void Application::MarkProjectDirty()
    {
        m_projectDirty = true;
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
        (void)deltaTime;

        m_inputSystem.Update();
        const Core::InputSnapshot& inputSnapshot = m_inputSystem.GetSnapshot();

        if (false == m_editorInitialized)
        {
            m_startupScreenController.UpdateLayout(m_window.GetHwnd());
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

        const bool sceneViewSizeChanged = m_editorShell.UpdateLayout(m_window.GetHwnd());
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
        if (true == m_projectPanelController.Update(m_sceneDocument))
        {
            const bool canAddSpriteComponentAfterSceneLoad = m_assetsPanelController.HasVisibleSpriteAssets();
            ApplySelectionChange(std::nullopt, canAddSpriteComponentAfterSceneLoad, true, true);
            m_editorCommandController.Reset(m_sceneDocument, m_hierarchyPanelController.GetSelectedEntityId());
            MarkProjectDirty();
            SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), L"選択した Scene を読み込みました。");
        }

        m_assetsPanelController.UpdateDragState(inputSnapshot);
        const bool canAddSpriteComponent = m_assetsPanelController.HasVisibleSpriteAssets();

        if (true == m_hierarchyPanelController.SyncSelection())
        {
            RefreshEditorPanels(canAddSpriteComponent, true);
        }

        const SceneEditResult hierarchyEditResult =
            m_hierarchyPanelController.ApplyEdits(m_sceneDocument.GetScene(), inputSnapshot);
        if (true == hierarchyEditResult.changed)
        {
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
            inputSnapshot);
        if (true == inspectorResult.changed)
        {
            PersistSceneChanges(
                L"Inspector の編集内容を Scene へ保存しました。",
                L"Inspector の編集内容は反映されましたが、Scene の保存に失敗しました。",
                true);
            RefreshEditorPanels(canAddSpriteComponent, false);
            RefreshSceneViewSelectionStatus();
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
                L"SceneView で Sprite の位置を保存しました。",
                L"SceneView で Sprite は移動しましたが、Scene の保存に失敗しました。",
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
            m_sceneViewController.GetDragPreviewState());
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
            canAddSpriteComponent);
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
            m_projectPanelController.Refresh(m_sceneDocument);
            ClearProjectDirty();
            return true;
        }

        SetWindowTextW(m_editorShell.GetSceneViewPlanLabel(), failureMessage);
        MarkProjectDirty();
        return false;
    }

    void Application::RecordCurrentProject()
    {
        if (m_sceneDocument.GetProjectInfo().has_value())
        {
            m_recentProjectsStore.Record(*m_sceneDocument.GetProjectInfo());
        }
    }
}
