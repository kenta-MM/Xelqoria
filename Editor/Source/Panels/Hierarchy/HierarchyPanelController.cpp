#include "Panels/Hierarchy/HierarchyPanelController.h"

#include <string>

#include "PlatformAdapters/ButtonClickWin32Adapter.h"
#include "Panels/Hierarchy/HierarchyPanelView.h"
#include "Utils/EditorStringUtils.h"
#include <Windows.h>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <utility>
#include <Entity.h>
#include <Scene.h>

namespace Xelqoria::Editor
{
    namespace
    {
        [[nodiscard]] std::filesystem::path GetHierarchyStatePath()
        {
            std::filesystem::path statePath = std::filesystem::temp_directory_path();
            statePath /= L"XelqoriaEditorHierarchyState.txt";
            return statePath;
        }

        [[nodiscard]] const wchar_t* GetComponentRowLabel(HierarchyPanelController::VisibleItemKind kind)
        {
            switch (kind)
            {
            case HierarchyPanelController::VisibleItemKind::SpriteComponent:
                return L"  |-- SpriteComponent";
            case HierarchyPanelController::VisibleItemKind::Material:
                return L"  |-- Material";
            case HierarchyPanelController::VisibleItemKind::Collider2DComponent:
                return L"  |-- Collider2D";
            default:
                return L"";
            }
        }
    }

    void HierarchyPanelController::Bind(const HierarchyPanelView& view)
    {
        m_hierarchyListBox = view.GetListBox();
        m_hierarchySummaryLabel = view.GetSummaryLabel();
        m_hierarchySearchEdit = view.GetSearchEdit();
        m_hierarchyNameEdit = view.GetNameEdit();
        m_hierarchyCreateButton = view.GetCreateButton();
        m_hierarchyDuplicateButton = view.GetDuplicateButton();
        m_hierarchyDeleteButton = view.GetDeleteButton();
        LoadExpansionState();
    }

    void HierarchyPanelController::Refresh(const Game::Scene* scene)
    {
        m_visibleEntityIds.clear();
        m_visibleItemKinds.clear();

        SendMessageW(m_hierarchyListBox, LB_RESETCONTENT, 0, 0);
        wchar_t searchBuffer[128]{};
        if (nullptr != m_hierarchySearchEdit)
        {
            GetWindowTextW(m_hierarchySearchEdit, searchBuffer, static_cast<int>(std::size(searchBuffer)));
        }
        const std::wstring searchText = searchBuffer;
        if (nullptr != scene)
        {
            for (const Game::Entity& entity : scene->GetEntities())
            {
                std::wstring label = ToWideString(entity.GetName());
                if (false == searchText.empty() && std::wstring::npos == label.find(searchText))
                {
                    continue;
                }

                const bool hasSpriteComponent = entity.HasSpriteComponent();
                const bool hasMaterialComponent = hasSpriteComponent;
                const bool hasCollider2DComponent = entity.HasCollider2DComponent();
                const bool hasChildComponent = hasSpriteComponent || hasMaterialComponent || hasCollider2DComponent;
                const bool isExpanded = m_collapsedEntityIds.find(entity.GetId()) == m_collapsedEntityIds.end();
                m_visibleEntityIds.push_back(entity.GetId());
                m_visibleItemKinds.push_back(VisibleItemKind::Entity);

                label = FormatEntityRowLabel(label, hasChildComponent, isExpanded);
                SendMessageW(m_hierarchyListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));

                const std::array<std::pair<bool, VisibleItemKind>, 3> componentRows{
                    std::pair<bool, VisibleItemKind>{ hasSpriteComponent, VisibleItemKind::SpriteComponent },
                    std::pair<bool, VisibleItemKind>{ hasMaterialComponent, VisibleItemKind::Material },
                    std::pair<bool, VisibleItemKind>{ hasCollider2DComponent, VisibleItemKind::Collider2DComponent }
                };
                for (const auto& componentRow : componentRows)
                {
                    if (false == ShouldShowComponentChildRow(componentRow.first, isExpanded))
                    {
                        continue;
                    }

                    m_visibleEntityIds.push_back(entity.GetId());
                    m_visibleItemKinds.push_back(componentRow.second);
                    SendMessageW(
                        m_hierarchyListBox,
                        LB_ADDSTRING,
                        0,
                        reinterpret_cast<LPARAM>(GetComponentRowLabel(componentRow.second)));
                }
            }
        }

        int selectedIndex = LB_ERR;
        for (std::size_t index = 0; index < m_visibleEntityIds.size(); ++index)
        {
            if (m_selectedEntityId.has_value()
                && m_visibleEntityIds[index] == *m_selectedEntityId
                && index < m_visibleItemKinds.size()
                && m_visibleItemKinds[index] == m_selectedItemKind)
            {
                selectedIndex = static_cast<int>(index);
                break;
            }
        }
        if (selectedIndex == LB_ERR)
        {
            for (std::size_t index = 0; index < m_visibleEntityIds.size(); ++index)
            {
                if (m_selectedEntityId.has_value() && m_visibleEntityIds[index] == *m_selectedEntityId)
                {
                    selectedIndex = static_cast<int>(index);
                    m_selectedItemKind = index < m_visibleItemKinds.size()
                        ? m_visibleItemKinds[index]
                        : VisibleItemKind::Entity;
                    break;
                }
            }
        }

        if (selectedIndex == LB_ERR)
        {
            m_selectedEntityId.reset();
            m_selectedItemKind = VisibleItemKind::Entity;
            if (false == m_preserveNoSelection && false == m_visibleEntityIds.empty())
            {
                m_selectedEntityId = m_visibleEntityIds.front();
                m_selectedItemKind = m_visibleItemKinds.empty()
                    ? VisibleItemKind::Entity
                    : m_visibleItemKinds.front();
                selectedIndex = 0;
            }
        }

        if (selectedIndex != LB_ERR)
        {
            SendMessageW(m_hierarchyListBox, LB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
        }
        else
        {
            SendMessageW(m_hierarchyListBox, LB_SETCURSEL, static_cast<WPARAM>(-1), 0);
        }

        const bool hasSelection = selectedIndex != LB_ERR;
        EnableWindow(m_hierarchyDuplicateButton, hasSelection ? TRUE : FALSE);
        EnableWindow(m_hierarchyDeleteButton, hasSelection ? TRUE : FALSE);
        EnableWindow(m_hierarchyNameEdit, hasSelection ? TRUE : FALSE);

        if (true == hasSelection && nullptr != scene)
        {
            const auto entity = scene->FindEntity(*m_selectedEntityId);
            const bool shouldOverwriteNameEdit = m_lastEditedEntityId != m_selectedEntityId || GetFocus() != m_hierarchyNameEdit;
            if (true == shouldOverwriteNameEdit && true == entity.has_value())
            {
                const std::wstring entityName = ToWideString(entity->get().GetName());
                SetWindowTextW(m_hierarchyNameEdit, entityName.c_str());
                m_lastEditedEntityId = m_selectedEntityId;
            }
        }
        else
        {
            SetWindowTextW(m_hierarchyNameEdit, L"");
            m_lastEditedEntityId.reset();
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
        const VisibleItemKind itemKind = index < m_visibleItemKinds.size()
            ? m_visibleItemKinds[index]
            : VisibleItemKind::Entity;
        if (false == m_selectedEntityId.has_value()
            || *m_selectedEntityId != entityId
            || m_selectedItemKind != itemKind)
        {
            m_selectedEntityId = entityId;
            m_selectedItemKind = itemKind;
            m_preserveNoSelection = false;
            return true;
        }

        return false;
    }

    SceneEditResult HierarchyPanelController::ApplyEdits(
        Game::Scene* scene,
        const Core::InputSnapshot& inputSnapshot)
    {
        SceneEditResult result{};
        if (nullptr == scene)
        {
            return result;
        }

        wchar_t searchBuffer[128]{};
        if (nullptr != m_hierarchySearchEdit)
        {
            GetWindowTextW(m_hierarchySearchEdit, searchBuffer, static_cast<int>(std::size(searchBuffer)));
        }
        const std::wstring currentSearchText = searchBuffer;
        if (currentSearchText != m_lastSearchText)
        {
            m_lastSearchText = currentSearchText;
            Refresh(scene);
        }

        const auto selectedEntity = m_selectedEntityId.has_value()
            ? scene->FindEntity(*m_selectedEntityId)
            : std::optional<std::reference_wrapper<Game::Entity>>{};
        const bool isNameEditFocused = GetFocus() == m_hierarchyNameEdit;
        const bool isHierarchyListFocused = GetFocus() == m_hierarchyListBox;
        const bool isEnterPressed = inputSnapshot.WasKeyPressed(VK_RETURN);
        const bool isSpacePressed = inputSnapshot.WasKeyPressed(VK_SPACE);
        const ButtonClickFrameInput frameInput{
            inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left),
            inputSnapshot.GetCursorScreenPoint()
        };

        if (true == isHierarchyListFocused
            && (true == isEnterPressed || true == isSpacePressed)
            && true == ToggleSelectedEntityExpansion(*scene))
        {
            Refresh(scene);
        }

        if (selectedEntity.has_value())
        {
            wchar_t nameBuffer[256]{};
            GetWindowTextW(m_hierarchyNameEdit, nameBuffer, static_cast<int>(std::size(nameBuffer)));

            if ((false == isNameEditFocused || true == isEnterPressed)
                && true == SceneEditingOperations::RenameEntity(selectedEntity->get(), ToNarrowString(nameBuffer)))
            {
                result.changed = true;
                result.selectedEntityId = selectedEntity->get().GetId();
            }
        }

        if (true == TryConsumeButtonClick(BuildButtonClickTarget(m_hierarchyCreateButton), frameInput, m_buttonInputState))
        {
            result = SceneEditingOperations::CreateUntexturedSprite(*scene, 0.0f, 0.0f);
            m_buttonInputState.pressedButtonId = 0;
        }
        else if (true == TryConsumeButtonClick(BuildButtonClickTarget(m_hierarchyDuplicateButton), frameInput, m_buttonInputState))
        {
            result = SceneEditingOperations::DuplicateSelectedEntity(*scene, m_selectedEntityId);
            m_buttonInputState.pressedButtonId = 0;
        }
        else if (true == TryConsumeButtonClick(BuildButtonClickTarget(m_hierarchyDeleteButton), frameInput, m_buttonInputState))
        {
            result = SceneEditingOperations::DeleteSelectedEntity(*scene, m_selectedEntityId);
            m_buttonInputState.pressedButtonId = 0;
        }

        if (false == frameInput.isLeftMouseButtonDown && true == m_buttonInputState.wasLeftMouseButtonDown)
        {
            m_buttonInputState.pressedButtonId = 0;
        }

        m_buttonInputState.wasLeftMouseButtonDown = frameInput.isLeftMouseButtonDown;
        return result;
    }

    void HierarchyPanelController::SetSelectedEntityId(std::optional<Game::EntityId> selectedEntityId)
    {
        m_selectedEntityId = selectedEntityId;
        m_selectedItemKind = VisibleItemKind::Entity;
        m_preserveNoSelection = false == selectedEntityId.has_value();
    }

    std::optional<Game::EntityId> HierarchyPanelController::GetSelectedEntityId() const
    {
        return m_selectedEntityId;
    }

    HierarchyPanelController::VisibleItemKind HierarchyPanelController::GetSelectedItemKind() const
    {
        return m_selectedItemKind;
    }

    bool HierarchyPanelController::ToggleSelectedEntityExpansion(const Game::Scene& scene)
    {
        if (false == m_selectedEntityId.has_value()
            || VisibleItemKind::Entity != m_selectedItemKind)
        {
            return false;
        }

        const auto selectedEntity = scene.FindEntity(*m_selectedEntityId);
        if (false == selectedEntity.has_value()
            || (false == selectedEntity->get().HasSpriteComponent()
                && false == selectedEntity->get().HasCollider2DComponent()))
        {
            return false;
        }

        const Game::EntityId entityId = selectedEntity->get().GetId();
        const auto collapsedIt = m_collapsedEntityIds.find(entityId);
        if (collapsedIt == m_collapsedEntityIds.end())
        {
            m_collapsedEntityIds.insert(entityId);
        }
        else
        {
            m_collapsedEntityIds.erase(collapsedIt);
        }

        SaveExpansionState();
        return true;
    }

    void HierarchyPanelController::LoadExpansionState()
    {
        m_collapsedEntityIds.clear();
        std::ifstream input(GetHierarchyStatePath());
        if (false == input.is_open())
        {
            return;
        }

        std::string token{};
        Game::EntityId entityId = 0;
        while (input >> token >> entityId)
        {
            if ("collapsed" == token)
            {
                m_collapsedEntityIds.insert(entityId);
            }
        }
    }

    void HierarchyPanelController::SaveExpansionState() const
    {
        std::ofstream output(GetHierarchyStatePath(), std::ios::trunc);
        if (false == output.is_open())
        {
            return;
        }

        for (Game::EntityId entityId : m_collapsedEntityIds)
        {
            output << "collapsed " << entityId << '\n';
        }
    }
}
