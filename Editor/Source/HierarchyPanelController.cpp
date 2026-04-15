#include "HierarchyPanelController.h"

#include <string>

#include "EditorStringUtils.h"
#include <Windows.h>
#include <cstdio>
#include <iterator>
#include <optional>
#include "EditorShell.h"
#include <Entity.h>
#include <Scene.h>

namespace Xelqoria::Editor
{
    void HierarchyPanelController::Bind(const EditorShell& shell)
    {
        m_hierarchyListBox = shell.GetHierarchyListBox();
        m_hierarchySummaryLabel = shell.GetHierarchySummaryLabel();
    }

    void HierarchyPanelController::Refresh(const Game::Scene* scene)
    {
        m_visibleEntityIds.clear();

        SendMessageW(m_hierarchyListBox, LB_RESETCONTENT, 0, 0);
        if (nullptr != scene)
        {
            for (const Game::Entity& entity : scene->GetEntities())
            {
                m_visibleEntityIds.push_back(entity.GetId());

                std::wstring label = L"Entity ";
                label += std::to_wstring(entity.GetId());

                if (const auto spriteComponent = entity.GetSpriteComponent(); spriteComponent.has_value())
                {
                    label += L" (";
                    label += ToWideString(spriteComponent->get().spriteAssetRef.GetValue());
                    label += L")";
                }

                SendMessageW(m_hierarchyListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            }
        }

        if (false == m_selectedEntityId.has_value() && false == m_visibleEntityIds.empty())
        {
            m_selectedEntityId = m_visibleEntityIds.front();
        }

        int selectedIndex = LB_ERR;
        for (std::size_t index = 0; index < m_visibleEntityIds.size(); ++index)
        {
            if (m_selectedEntityId.has_value() && m_visibleEntityIds[index] == *m_selectedEntityId)
            {
                selectedIndex = static_cast<int>(index);
                break;
            }
        }

        if (selectedIndex != LB_ERR)
        {
            SendMessageW(m_hierarchyListBox, LB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
        }

        wchar_t summaryText[128]{};
        std::swprintf(
            summaryText,
            std::size(summaryText),
            L"Entities: %u / selected: %u",
            static_cast<unsigned>(m_visibleEntityIds.size()),
            true == m_selectedEntityId.has_value() ? static_cast<unsigned>(*m_selectedEntityId) : 0u);
        SetWindowTextW(m_hierarchySummaryLabel, summaryText);
    }

    bool HierarchyPanelController::SyncSelection()
    {
        if (nullptr == m_hierarchyListBox)
        {
            return false;
        }

        const LRESULT selectedIndex = SendMessageW(m_hierarchyListBox, LB_GETCURSEL, 0, 0);
        if (selectedIndex == LB_ERR)
        {
            return false;
        }

        const auto index = static_cast<std::size_t>(selectedIndex);
        if (index >= m_visibleEntityIds.size())
        {
            return false;
        }

        const Game::EntityId entityId = m_visibleEntityIds[index];
        if (false == m_selectedEntityId.has_value() || *m_selectedEntityId != entityId)
        {
            m_selectedEntityId = entityId;
            return true;
        }

        return false;
    }

    void HierarchyPanelController::SetSelectedEntityId(std::optional<Game::EntityId> selectedEntityId)
    {
        m_selectedEntityId = selectedEntityId;
    }

    std::optional<Game::EntityId> HierarchyPanelController::GetSelectedEntityId() const
    {
        return m_selectedEntityId;
    }
}
