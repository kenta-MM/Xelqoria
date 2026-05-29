#include "Shell/EditorDockingLayoutSerializer.h"

#include "Shell/EditorShell.h"

namespace Xelqoria::Editor
{
    EditorDockingLayoutSerializer::EditorDockingLayoutSerializer(EditorShell& shell)
        : m_shell(shell)
    {
    }

    bool EditorDockingLayoutSerializer::Save(const std::filesystem::path& layoutPath) const
    {
        return m_shell.SaveLayoutCore(layoutPath);
    }

    bool EditorDockingLayoutSerializer::Load(const std::filesystem::path& layoutPath)
    {
        return m_shell.LoadLayoutCore(layoutPath);
    }
}
