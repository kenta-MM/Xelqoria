#include "Shell/EditorDockingController.h"

#include "Shell/EditorShell.h"

namespace Xelqoria::Editor
{
    EditorDockingController::EditorDockingController(EditorShell& shell)
        : m_shell(shell)
    {
    }

    bool EditorDockingController::Update(HWND parentWindow, const Core::InputSnapshot& inputSnapshot)
    {
        return m_shell.UpdateDockingCore(parentWindow, inputSnapshot);
    }

    bool EditorDockingController::HandleNotify(LPARAM notifyParameter)
    {
        return m_shell.HandleDockNotifyCore(notifyParameter);
    }

    void EditorDockingController::ResetLayout()
    {
        m_shell.ResetDockLayoutCore();
    }

    void EditorDockingController::ShowPanelAtDefaultDock(EditorPanelId panelId)
    {
        m_shell.ShowPanelAtDefaultDockCore(panelId);
    }

    void EditorDockingController::ActivatePanel(EditorPanelId panelId)
    {
        m_shell.ActivatePanelCore(panelId);
    }

    bool EditorDockingController::SaveLayout(const std::filesystem::path& layoutPath) const
    {
        return m_shell.SaveLayoutCore(layoutPath);
    }

    bool EditorDockingController::LoadLayout(const std::filesystem::path& layoutPath)
    {
        return m_shell.LoadLayoutCore(layoutPath);
    }
}
