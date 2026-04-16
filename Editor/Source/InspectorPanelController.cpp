#include "InspectorPanelController.h"

#include <string>

#include "EditorStringUtils.h"
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <AssetId.h>
#include "EditorShell.h"
#include <Entity.h>
#include <Scene.h>

namespace Xelqoria::Editor
{
    void InspectorPanelController::Bind(const EditorShell& shell)
    {
        m_inspectorSummaryLabel = shell.GetInspectorSummaryLabel();
        m_transformEditControls = shell.GetTransformEditControls();
        m_spriteRefEdit = shell.GetSpriteRefEdit();
    }

    void InspectorPanelController::Refresh(const Game::Scene* scene, std::optional<Game::EntityId> selectedEntityId)
    {
        if (false == selectedEntityId.has_value() || nullptr == scene)
        {
            SetWindowTextW(m_inspectorSummaryLabel, L"Inspector: no entity selected");
            return;
        }

        const auto entity = scene->FindEntity(*selectedEntityId);
        if (false == entity.has_value())
        {
            SetWindowTextW(m_inspectorSummaryLabel, L"Inspector: selected entity not found");
            return;
        }

        const auto& transform = entity->get().GetTransform();
        const float values[9] = {
            transform.position.x,
            transform.position.y,
            transform.position.z,
            transform.rotation.x,
            transform.rotation.y,
            transform.rotation.z,
            transform.scale.x,
            transform.scale.y,
            transform.scale.z
        };

        for (std::size_t index = 0; index < m_transformEditControls.size(); ++index)
        {
            wchar_t valueText[32]{};
            std::swprintf(valueText, std::size(valueText), L"%.3f", values[index]);
            SetWindowTextW(m_transformEditControls[index], valueText);
        }

        if (const auto spriteComponent = entity->get().GetSpriteComponent(); spriteComponent.has_value())
        {
            const std::wstring spriteRef = ToWideString(spriteComponent->get().spriteAssetRef.GetValue());
            SetWindowTextW(m_spriteRefEdit, spriteRef.c_str());
        }
        else
        {
            SetWindowTextW(m_spriteRefEdit, L"");
        }

        wchar_t summaryText[128]{};
        std::swprintf(
            summaryText,
            std::size(summaryText),
            L"Inspector: Entity %u",
            static_cast<unsigned>(*selectedEntityId));
        SetWindowTextW(m_inspectorSummaryLabel, summaryText);
        m_lastInspectorEntityId = selectedEntityId;
    }

    void InspectorPanelController::ApplyEdits(Game::Scene* scene, std::optional<Game::EntityId> selectedEntityId)
    {
        if (false == selectedEntityId.has_value() || nullptr == scene)
        {
            return;
        }

        if (m_lastInspectorEntityId != selectedEntityId)
        {
            Refresh(scene, selectedEntityId);
        }

        const auto entity = scene->FindEntity(*selectedEntityId);
        if (false == entity.has_value())
        {
            return;
        }

        float* transformValues[9] = {
            &entity->get().GetTransform().position.x,
            &entity->get().GetTransform().position.y,
            &entity->get().GetTransform().position.z,
            &entity->get().GetTransform().rotation.x,
            &entity->get().GetTransform().rotation.y,
            &entity->get().GetTransform().rotation.z,
            &entity->get().GetTransform().scale.x,
            &entity->get().GetTransform().scale.y,
            &entity->get().GetTransform().scale.z
        };

        for (std::size_t index = 0; index < m_transformEditControls.size(); ++index)
        {
            wchar_t buffer[64]{};
            GetWindowTextW(m_transformEditControls[index], buffer, static_cast<int>(std::size(buffer)));

            wchar_t* end = nullptr;
            const float parsed = std::wcstof(buffer, &end);
            if (end != buffer && *end == L'\0')
            {
                *transformValues[index] = parsed;
            }
        }

        wchar_t spriteRefBuffer[256]{};
        GetWindowTextW(m_spriteRefEdit, spriteRefBuffer, static_cast<int>(std::size(spriteRefBuffer)));
        const std::wstring spriteRefValue(spriteRefBuffer);
        const std::string spriteRef = ToNarrowString(spriteRefValue);

        if (auto spriteComponent = entity->get().GetSpriteComponent(); true == spriteComponent.has_value())
        {
            spriteComponent->get().spriteAssetRef = Core::AssetId(spriteRef);
        }
    }

    void InspectorPanelController::ResetTrackedEntity()
    {
        m_lastInspectorEntityId.reset();
    }
}
