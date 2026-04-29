#pragma once

#include <Windows.h>
#include <optional>

#include "EditorSceneDocument.h"
#include "InputSystem.h"
#include "SceneCommandHistory.h"
#include <Entity.h>

namespace Xelqoria::Editor
{
    /// <summary>
    /// Editor コマンドの更新結果を表す。
    /// </summary>
    struct EditorCommandUpdateResult
    {
        /// <summary>
        /// Scene に変更が加わったかを表す。
        /// </summary>
        bool changed = false;

        /// <summary>
        /// 更新後に選択すべき EntityId を表す。
        /// </summary>
        std::optional<Game::EntityId> selectedEntityId{};
    };

    /// <summary>
    /// Undo/Redo と Scene 編集ショートカットを管理する。
    /// </summary>
    class EditorCommandController
    {
    public:
        /// <summary>
        /// 現在の Scene スナップショットで履歴を再初期化する。
        /// </summary>
        /// <param name="document">対象ドキュメント。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        void Reset(const EditorSceneDocument& document, std::optional<Game::EntityId> selectedEntityId);

        /// <summary>
        /// 現在の Scene スナップショットを履歴へ積む。
        /// </summary>
        /// <param name="document">対象ドキュメント。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        void PushSnapshot(const EditorSceneDocument& document, std::optional<Game::EntityId> selectedEntityId);

        /// <summary>
        /// ショートカット入力を評価して Scene へ反映する。
        /// </summary>
        /// <param name="document">更新対象ドキュメント。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        /// <param name="sceneViewPlanLabel">状態表示に使うラベル。</param>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        /// <returns>コマンド適用結果。</returns>
        EditorCommandUpdateResult Update(
            EditorSceneDocument& document,
            std::optional<Game::EntityId> selectedEntityId,
            HWND sceneViewPlanLabel,
            const Core::InputSnapshot& inputSnapshot);

    private:
        /// <summary>
        /// Scene と選択状態から履歴スナップショットを作成する。
        /// </summary>
        /// <param name="document">対象ドキュメント。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        /// <returns>履歴スナップショット。</returns>
        [[nodiscard]] SceneCommandHistoryEntry CaptureSceneHistoryEntry(
            const EditorSceneDocument& document,
            std::optional<Game::EntityId> selectedEntityId) const;

        /// <summary>
        /// 履歴スナップショットを Scene へ復元する。
        /// </summary>
        /// <param name="entry">復元対象の履歴スナップショット。</param>
        /// <param name="document">復元先ドキュメント。</param>
        /// <param name="sceneViewPlanLabel">状態表示に使うラベル。</param>
        /// <returns>復元結果。</returns>
        [[nodiscard]] EditorCommandUpdateResult RestoreSceneHistoryEntry(
            const SceneCommandHistoryEntry& entry,
            EditorSceneDocument& document,
            HWND sceneViewPlanLabel) const;

    private:
        SceneCommandHistory m_sceneCommandHistory{};
    };
}
