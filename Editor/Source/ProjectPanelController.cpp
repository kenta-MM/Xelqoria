#include "ProjectPanelController.h"

#include <cstdio>
#include <filesystem>
#include <iterator>
#include <string>

namespace Xelqoria::Editor
{
    namespace
    {
        [[nodiscard]] std::wstring BuildSceneLabel(const std::filesystem::path& scenePath)
        {
            return scenePath.filename().wstring();
        }
    }

    void ProjectPanelController::Bind(const EditorShell& shell)
    {
        m_projectSummaryLabel = shell.GetProjectSummaryLabel();
        m_projectSceneListBox = shell.GetProjectSceneListBox();
        m_projectSceneDetailLabel = shell.GetProjectSceneDetailLabel();
    }

    void ProjectPanelController::Refresh(const EditorSceneDocument& document)
    {
        m_sceneFiles = document.EnumerateProjectSceneFiles();
        SendMessageW(m_projectSceneListBox, LB_RESETCONTENT, 0, 0);

        for (const std::filesystem::path& scenePath : m_sceneFiles)
        {
            const std::wstring label = BuildSceneLabel(scenePath);
            SendMessageW(m_projectSceneListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        }

        const auto& projectInfo = document.GetProjectInfo();
        if (false == projectInfo.has_value())
        {
            m_selectedScenePath.clear();
            SetWindowTextW(m_projectSummaryLabel, L"Project: not opened");
            SetWindowTextW(m_projectSceneDetailLabel, L"Scene: not selected");
            return;
        }

        m_selectedScenePath = projectInfo->activeScenePath;
        int selectedIndex = LB_ERR;
        for (std::size_t index = 0; index < m_sceneFiles.size(); ++index)
        {
            if (m_sceneFiles[index] == m_selectedScenePath)
            {
                selectedIndex = static_cast<int>(index);
                break;
            }
        }

        if (selectedIndex != LB_ERR)
        {
            SendMessageW(m_projectSceneListBox, LB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
        }

        const std::wstring summary =
            L"Project: " + projectInfo->name
            + L" / Scenes: " + std::to_wstring(m_sceneFiles.size())
            + L" / Root: " + projectInfo->rootDirectory.wstring();
        SetWindowTextW(m_projectSummaryLabel, summary.c_str());
        RefreshSelectedSceneDetail(document);
    }

    bool ProjectPanelController::Update(EditorSceneDocument& document)
    {
        const LRESULT selectedIndex = SendMessageW(m_projectSceneListBox, LB_GETCURSEL, 0, 0);
        if (selectedIndex == LB_ERR)
        {
            return false;
        }

        const auto index = static_cast<std::size_t>(selectedIndex);
        if (index >= m_sceneFiles.size() || m_sceneFiles[index] == m_selectedScenePath)
        {
            return false;
        }

        const std::filesystem::path previousScenePath = m_selectedScenePath;
        m_selectedScenePath = m_sceneFiles[index];
        if (false == document.OpenProjectScene(m_selectedScenePath))
        {
            m_selectedScenePath = previousScenePath;
            SetWindowTextW(m_projectSceneDetailLabel, L"Scene: 読み込みに失敗しました。");
            return false;
        }

        Refresh(document);
        return true;
    }

    void ProjectPanelController::RefreshSelectedSceneDetail(const EditorSceneDocument& document)
    {
        const Game::Scene* scene = document.GetScene();
        const unsigned entityCount = nullptr != scene
            ? static_cast<unsigned>(scene->GetEntities().size())
            : 0u;

        const std::wstring sceneName = m_selectedScenePath.empty()
            ? std::wstring(L"not selected")
            : m_selectedScenePath.filename().wstring();

        wchar_t detailText[512]{};
        std::swprintf(
            detailText,
            std::size(detailText),
            L"Scene: %ls / Entities: %u",
            sceneName.c_str(),
            entityCount);
        SetWindowTextW(m_projectSceneDetailLabel, detailText);
    }
}
