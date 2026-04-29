#include "EditorCommandController.h"

#include "SceneEditingOperations.h"
#include "SceneSerializer.h"
#include <Windows.h>
#include <memory>
#include <optional>
#include "EditorSceneDocument.h"
#include "SceneCommandHistory.h"
#include <Entity.h>
#include <Scene.h>

namespace Xelqoria::Editor
{
    void EditorCommandController::Reset(const EditorSceneDocument& document, std::optional<Game::EntityId> selectedEntityId)
    {
        m_sceneCommandHistory.Reset(CaptureSceneHistoryEntry(document, selectedEntityId));
    }

    void EditorCommandController::PushSnapshot(const EditorSceneDocument& document, std::optional<Game::EntityId> selectedEntityId)
    {
        m_sceneCommandHistory.Push(CaptureSceneHistoryEntry(document, selectedEntityId));
    }

    EditorCommandUpdateResult EditorCommandController::Update(
        EditorSceneDocument& document,
        std::optional<Game::EntityId> selectedEntityId,
        HWND sceneViewPlanLabel,
        const Core::InputSnapshot& inputSnapshot)
    {
        EditorCommandUpdateResult result{};

        const bool isControlDown = inputSnapshot.IsKeyDown(VK_CONTROL);
        const bool isUndoPressed = isControlDown && inputSnapshot.WasKeyPressed('Z');
        const bool isRedoPressed = isControlDown && inputSnapshot.WasKeyPressed('Y');
        const bool isDuplicatePressed = isControlDown && inputSnapshot.WasKeyPressed('D');
        const bool isDeletePressed = inputSnapshot.WasKeyPressed(VK_DELETE);

        if (isUndoPressed)
        {
            const auto entry = m_sceneCommandHistory.Undo();
            if (entry.has_value())
            {
                result = RestoreSceneHistoryEntry(*entry, document, sceneViewPlanLabel);
                if (result.changed)
                {
                    SetWindowTextW(sceneViewPlanLabel, L"Ctrl+Z で直前の Scene スナップショットへ戻しました。");
                }
            }
        }

        if (isRedoPressed)
        {
            const auto entry = m_sceneCommandHistory.Redo();
            if (entry.has_value())
            {
                result = RestoreSceneHistoryEntry(*entry, document, sceneViewPlanLabel);
                if (result.changed)
                {
                    SetWindowTextW(sceneViewPlanLabel, L"Ctrl+Y で Scene スナップショットを再適用しました。");
                }
            }
        }

        Game::Scene* scene = document.GetScene();
        if (scene && isDuplicatePressed)
        {
            const SceneEditResult duplicateResult =
                SceneEditingOperations::DuplicateSelectedEntity(*scene, selectedEntityId);
            if (duplicateResult.changed)
            {
                selectedEntityId = duplicateResult.selectedEntityId;
                result.changed = true;
                result.selectedEntityId = selectedEntityId;
                if (document.Save())
                {
                    PushSnapshot(document, selectedEntityId);
                    SetWindowTextW(sceneViewPlanLabel, L"Ctrl+D で選択 Entity を複製しました。");
                }
                else
                {
                    SetWindowTextW(sceneViewPlanLabel, L"Ctrl+D で Entity は複製されましたが、Scene の保存に失敗しました。");
                }
                
            }
        }

        if (scene && isDeletePressed)
        {
            const SceneEditResult deleteResult =
                SceneEditingOperations::DeleteSelectedEntity(*scene, selectedEntityId);
            if (deleteResult.changed)
            {
                selectedEntityId = deleteResult.selectedEntityId;
                result.changed = true;
                result.selectedEntityId = selectedEntityId;
                if (document.Save())
                {
                    PushSnapshot(document, selectedEntityId);
                    SetWindowTextW(sceneViewPlanLabel, L"Delete で選択 Entity を削除しました。");
                }
                else
                {
                    SetWindowTextW(sceneViewPlanLabel, L"Delete で Entity は削除されましたが、Scene の保存に失敗しました。");
                }
                
            }
        }

        return result;
    }

    SceneCommandHistoryEntry EditorCommandController::CaptureSceneHistoryEntry(
        const EditorSceneDocument& document,
        std::optional<Game::EntityId> selectedEntityId) const
    {
        const Game::Scene* scene = document.GetScene();
        if (nullptr == scene)
        {
            return SceneCommandHistoryEntry{};
        }

        return SceneCommandHistoryEntry{
            Game::SceneSerializer::SaveToText(*scene),
            selectedEntityId
        };
    }

    EditorCommandUpdateResult EditorCommandController::RestoreSceneHistoryEntry(
        const SceneCommandHistoryEntry& entry,
        EditorSceneDocument& document,
        HWND sceneViewPlanLabel) const
    {
        EditorCommandUpdateResult result{};

        const auto loadResult = Game::SceneSerializer::LoadFromText(entry.serializedScene);
        if (false == loadResult.IsSuccess() || false == loadResult.scene.has_value())
        {
            ::OutputDebugStringA("Editor::EditorCommandController failed to restore Scene history entry.\n");
            SetWindowTextW(sceneViewPlanLabel, L"履歴スナップショットの再読込に失敗しました。");
            return result;
        }

        document.ReplaceScene(std::make_unique<Game::Scene>(*loadResult.scene));
        result.changed = true;
        result.selectedEntityId = entry.selectedEntityId;

        Game::Scene* scene = document.GetScene();
        if (result.selectedEntityId.has_value()
            && (nullptr == scene || false == scene->FindEntity(*result.selectedEntityId).has_value()))
        {
            result.selectedEntityId.reset();
        }

        return result;
    }
}
