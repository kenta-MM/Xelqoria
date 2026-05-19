#include "EditorCommandController.h"

#include "SceneEditingOperations.h"
#include "SceneSerializer.h"
#include <Windows.h>
#include <memory>
#include <optional>
#include <utility>
#include "EditorSceneDocument.h"
#include "SceneCommandHistory.h"
#include <Entity.h>
#include <Scene.h>

namespace Xelqoria::Editor
{
    void EditorCommandController::Reset(const EditorSceneDocument& document, std::optional<Game::EntityId> selectedEntityId)
    {
        m_sceneCommandHistory.Reset(CaptureSceneHistoryEntry(document, selectedEntityId, "Initial Scene"));
    }

    bool EditorCommandController::PushSnapshot(
        const EditorSceneDocument& document,
        std::optional<Game::EntityId> selectedEntityId,
        std::string operationName)
    {
        return m_sceneCommandHistory.Push(CaptureSceneHistoryEntry(document, selectedEntityId, std::move(operationName)));
    }

    void EditorCommandController::MarkSaved()
    {
        m_sceneCommandHistory.MarkSaved();
    }

    bool EditorCommandController::IsDirty() const
    {
        return m_sceneCommandHistory.IsDirty();
    }

    EditorCommandUpdateResult EditorCommandController::Update(
        EditorSceneDocument& document,
        std::optional<Game::EntityId> selectedEntityId,
        HWND sceneViewPlanLabel,
        const Core::InputSnapshot& inputSnapshot)
    {
        EditorCommandUpdateResult result{};

        const bool isControlDown = inputSnapshot.IsKeyDown(VK_CONTROL);
        const bool isShiftDown = inputSnapshot.IsKeyDown(VK_SHIFT);
        const bool isRedoPressed = isControlDown && isShiftDown && inputSnapshot.WasKeyPressed('Z');
        const bool isUndoPressed = isControlDown && false == isShiftDown && inputSnapshot.WasKeyPressed('Z');
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
                    SetWindowTextW(sceneViewPlanLabel, L"Ctrl+Z で直前の Scene 状態へ戻しました。");
                }
            }

            return result;
        }

        if (isRedoPressed)
        {
            const auto entry = m_sceneCommandHistory.Redo();
            if (entry.has_value())
            {
                result = RestoreSceneHistoryEntry(*entry, document, sceneViewPlanLabel);
                if (result.changed)
                {
                    SetWindowTextW(sceneViewPlanLabel, L"Shift+Ctrl+Z で Scene 状態を再適用しました。");
                }
            }

            return result;
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
                if (PushSnapshot(document, selectedEntityId, "Duplicate Entity"))
                {
                    SetWindowTextW(sceneViewPlanLabel, L"Ctrl+D で選択 Entity を複製しました。");
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
                if (PushSnapshot(document, selectedEntityId, "Delete Entity"))
                {
                    SetWindowTextW(sceneViewPlanLabel, L"Delete で選択 Entity を削除しました。");
                }
            }
        }

        return result;
    }

    SceneCommandHistoryEntry EditorCommandController::CaptureSceneHistoryEntry(
        const EditorSceneDocument& document,
        std::optional<Game::EntityId> selectedEntityId,
        std::string operationName) const
    {
        const Game::Scene* scene = document.GetScene();
        if (nullptr == scene)
        {
            return SceneCommandHistoryEntry{};
        }

        return SceneCommandHistoryEntry{
            Game::SceneSerializer::SaveToText(*scene),
            selectedEntityId,
            std::move(operationName)
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
