#pragma once

#include <Windows.h>
#include <string>
#include <vector>

#include "Shell/EditorPanelId.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Dock ノードの保存復元用データ。
    /// </summary>
    struct EditorDockingLayoutNodeSnapshot
    {
        bool isLeaf = true;
        bool isHorizontalSplit = true;
        float splitRatio = 0.5f;
        int firstChild = -1;
        int secondChild = -1;
        int activeTabIndex = 0;
        std::wstring tabKey{};
        std::vector<EditorPanelId> panels{};
    };

    /// <summary>
    /// Floating panel group の保存復元用データ。
    /// </summary>
    struct EditorFloatingPanelGroupSnapshot
    {
        RECT rect{};
        int activeTabIndex = 0;
        std::vector<EditorPanelId> panels{};
    };

    /// <summary>
    /// Dock/Floating レイアウト全体の保存復元用データ。
    /// </summary>
    struct EditorDockingLayoutSnapshot
    {
        int rootDockNodeId = -1;
        std::vector<EditorDockingLayoutNodeSnapshot> dockNodes{};
        std::vector<EditorFloatingPanelGroupSnapshot> floatingGroups{};
    };
}
