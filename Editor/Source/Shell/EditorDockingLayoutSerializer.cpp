#include "Shell/EditorDockingLayoutSerializer.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "Shell/EditorDockingController.h"
#include "Shell/EditorDockingLayoutSnapshot.h"

namespace Xelqoria::Editor
{
    namespace
    {
        [[nodiscard]] constexpr std::array<EditorPanelId, 5> GetAllLayoutPanels()
        {
            return {
                EditorPanelId::Hierarchy,
                EditorPanelId::Assets,
                EditorPanelId::SceneView,
                EditorPanelId::Inspector,
                EditorPanelId::LogOutput
            };
        }

        [[nodiscard]] const wchar_t* GetPanelLayoutName(EditorPanelId panelId)
        {
            switch (panelId)
            {
            case EditorPanelId::Hierarchy:
                return L"Hierarchy";
            case EditorPanelId::Assets:
                return L"Assets";
            case EditorPanelId::SceneView:
                return L"SceneView";
            case EditorPanelId::Inspector:
                return L"Inspector";
            case EditorPanelId::Sprite:
                return L"Sprite";
            case EditorPanelId::Material:
                return L"Material";
            case EditorPanelId::Collider2D:
                return L"Collider2D";
            case EditorPanelId::LogOutput:
                return L"LogOutput";
            default:
                return L"SceneView";
            }
        }

        [[nodiscard]] std::optional<EditorPanelId> TryParsePanelLayoutName(const std::wstring& name)
        {
            for (EditorPanelId panelId : GetAllLayoutPanels())
            {
                if (name == GetPanelLayoutName(panelId))
                {
                    return panelId;
                }
            }

            return std::nullopt;
        }
    }

    EditorDockingLayoutSerializer::EditorDockingLayoutSerializer(EditorDockingController& controller)
        : m_controller(controller)
    {
    }

    bool EditorDockingLayoutSerializer::Save(const std::filesystem::path& layoutPath) const
    {
        const EditorDockingLayoutSnapshot snapshot = m_controller.CreateLayoutSnapshot();

        std::error_code errorCode;
        const std::filesystem::path parentPath = layoutPath.parent_path();
        if (false == parentPath.empty())
        {
            std::filesystem::create_directories(parentPath, errorCode);
            if (errorCode)
            {
                return false;
            }
        }

        std::wofstream output(layoutPath, std::ios::binary | std::ios::trunc);
        if (false == output.is_open())
        {
            return false;
        }

        output << L"XelqoriaEditorLayout 1\n";
        output << L"Root " << snapshot.rootDockNodeId << L'\n';
        output << L"Nodes " << snapshot.dockNodes.size() << L'\n';
        for (std::size_t index = 0; index < snapshot.dockNodes.size(); ++index)
        {
            const EditorDockingLayoutNodeSnapshot& dockNode = snapshot.dockNodes[index];
            output << L"Node "
                << index << L' '
                << (dockNode.isLeaf ? L"Leaf" : L"Split") << L' '
                << (dockNode.isHorizontalSplit ? L"Horizontal" : L"Vertical") << L' '
                << dockNode.splitRatio << L' '
                << dockNode.firstChild << L' '
                << dockNode.secondChild << L' '
                << dockNode.activeTabIndex << L' '
                << dockNode.tabKey << L' '
                << dockNode.panels.size();
            for (EditorPanelId panelId : dockNode.panels)
            {
                output << L' ' << GetPanelLayoutName(panelId);
            }
            output << L'\n';
        }

        output << L"Floating " << snapshot.floatingGroups.size() << L'\n';
        for (const EditorFloatingPanelGroupSnapshot& group : snapshot.floatingGroups)
        {
            output << L"FloatingGroup "
                << group.rect.left << L' '
                << group.rect.top << L' '
                << group.rect.right << L' '
                << group.rect.bottom << L' '
                << group.activeTabIndex << L' '
                << group.panels.size();
            for (EditorPanelId panelId : group.panels)
            {
                output << L' ' << GetPanelLayoutName(panelId);
            }
            output << L'\n';
        }

        return output.good();
    }

    bool EditorDockingLayoutSerializer::Load(const std::filesystem::path& layoutPath)
    {
        std::wifstream input(layoutPath, std::ios::binary);
        if (false == input.is_open())
        {
            return false;
        }

        std::wstring signature{};
        int version = 0;
        input >> signature >> version;
        if (input.fail() || L"XelqoriaEditorLayout" != signature || 1 != version)
        {
            return false;
        }

        EditorDockingLayoutSnapshot snapshot{};
        std::wstring section{};
        input >> section >> snapshot.rootDockNodeId;
        if (input.fail() || L"Root" != section)
        {
            return false;
        }

        std::size_t nodeCount = 0;
        input >> section >> nodeCount;
        if (input.fail() || L"Nodes" != section || 0 == nodeCount)
        {
            return false;
        }

        snapshot.dockNodes.resize(nodeCount);
        for (std::size_t index = 0; index < nodeCount; ++index)
        {
            std::wstring nodeToken{};
            std::size_t savedIndex = 0;
            std::wstring kind{};
            std::wstring orientation{};
            std::size_t panelCount = 0;
            input >> nodeToken
                >> savedIndex
                >> kind
                >> orientation
                >> snapshot.dockNodes[index].splitRatio
                >> snapshot.dockNodes[index].firstChild
                >> snapshot.dockNodes[index].secondChild
                >> snapshot.dockNodes[index].activeTabIndex
                >> snapshot.dockNodes[index].tabKey
                >> panelCount;
            if (input.fail() || L"Node" != nodeToken || savedIndex != index)
            {
                return false;
            }

            snapshot.dockNodes[index].isLeaf = L"Leaf" == kind;
            snapshot.dockNodes[index].isHorizontalSplit = L"Horizontal" == orientation;
            for (std::size_t panelIndex = 0; panelIndex < panelCount; ++panelIndex)
            {
                std::wstring panelName{};
                input >> panelName;
                if (input.fail())
                {
                    return false;
                }
                const std::optional<EditorPanelId> panelId = TryParsePanelLayoutName(panelName);
                if (panelId.has_value())
                {
                    snapshot.dockNodes[index].panels.push_back(*panelId);
                }
            }
        }

        std::size_t floatingGroupCount = 0;
        input >> section >> floatingGroupCount;
        if (input.fail() || L"Floating" != section)
        {
            return false;
        }

        snapshot.floatingGroups.reserve(floatingGroupCount);
        for (std::size_t index = 0; index < floatingGroupCount; ++index)
        {
            std::wstring groupToken{};
            EditorFloatingPanelGroupSnapshot group{};
            std::size_t panelCount = 0;
            input >> groupToken
                >> group.rect.left
                >> group.rect.top
                >> group.rect.right
                >> group.rect.bottom
                >> group.activeTabIndex
                >> panelCount;
            if (input.fail() || L"FloatingGroup" != groupToken)
            {
                return false;
            }

            for (std::size_t panelIndex = 0; panelIndex < panelCount; ++panelIndex)
            {
                std::wstring panelName{};
                input >> panelName;
                if (input.fail())
                {
                    return false;
                }
                const std::optional<EditorPanelId> panelId = TryParsePanelLayoutName(panelName);
                if (panelId.has_value())
                {
                    group.panels.push_back(*panelId);
                }
            }
            if (false == group.panels.empty())
            {
                snapshot.floatingGroups.push_back(std::move(group));
            }
        }

        if (input.bad() || input.fail())
        {
            return false;
        }

        return m_controller.ApplyLayoutSnapshot(snapshot);
    }
}
