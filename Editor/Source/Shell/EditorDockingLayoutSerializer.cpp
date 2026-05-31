#include "Shell/EditorDockingLayoutSerializer.h"

#include "Shell/EditorDockingController.h"

namespace Xelqoria::Editor
{
    EditorDockingLayoutSerializer::EditorDockingLayoutSerializer(EditorDockingController& controller)
        : m_controller(controller)
    {
    }

    bool EditorDockingLayoutSerializer::Save(const std::filesystem::path& layoutPath) const
    {
        return m_controller.SaveLayoutCore(layoutPath);
    }

    bool EditorDockingLayoutSerializer::Load(const std::filesystem::path& layoutPath)
    {
        return m_controller.LoadLayoutCore(layoutPath);
    }
}
