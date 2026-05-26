#pragma once

namespace Xelqoria::Editor
{
    /// <summary>
    /// Editor の Dock / Floating / Tab で扱う Panel 識別子。
    /// </summary>
    enum class EditorPanelId
    {
        Hierarchy,
        Assets,
        SceneView,
        Inspector,
        Sprite,
        Material,
        Collider2D,
        LogOutput
    };
}
