#include "HierarchyPanelController.h"

#include <string>

#include "ButtonClickWin32Adapter.h"
#include "EditorStringUtils.h"
#include <Windows.h>
#include <cstdio>
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
        m_hierarchySearchEdit = shell.GetHierarchySearchEdit();
        m_hierarchyNameEdit = shell.GetHierarchyNameEdit();
        m_hierarchyCreateButton = shell.GetHierarchyCreateButton();
        m_hierarchyDuplicateButton = shell.GetHierarchyDuplicateButton();
        m_hierarchyDeleteButton = shell.GetHierarchyDeleteButton();
    }

    void HierarchyPanelController::Refresh(const Game::Scene* scene)
    {
        m_visibleEntityIds.clear();

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

                m_visibleEntityIds.push_back(entity.GetId());

                label.insert(0, L"+ ");
                SendMessageW(m_hierarchyListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            }
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

        if (selectedIndex == LB_ERR)
        {
            m_selectedEntityId.reset();
            if (false == m_preserveNoSelection && false == m_visibleEntityIds.empty())
            {
                m_selectedEntityId = m_visibleEntityIds.front();
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
        if (false == m_selectedEntityId.has_value() || *m_selectedEntityId != entityId)
        {
            m_selectedEntityId = entityId;
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
        const bool isEnterPressed = inputSnapshot.WasKeyPressed(VK_RETURN);
        const ButtonClickFrameInput frameInput{
            inputSnapshot.IsMouseButtonDown(Core::MouseButton::Left),
            inputSnapshot.GetCursorScreenPoint()
        };

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
        m_preserveNoSelection = false == selectedEntityId.has_value();
    }

    std::optional<Game::EntityId> HierarchyPanelController::GetSelectedEntityId() const
    {
        return m_selectedEntityId;
    }
}
