#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Scene.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Editor の履歴へ保存する Scene スナップショットを表す。
    /// </summary>
    struct SceneCommandHistoryEntry
    {
        /// <summary>
        /// Scene の保存テキストを保持する。
        /// </summary>
        std::string serializedScene{};

        /// <summary>
        /// スナップショット時点の選択 EntityId を保持する。
        /// </summary>
        std::optional<Game::EntityId> selectedEntityId{};
    };

    /// <summary>
    /// Scene 全体のスナップショットを使って Undo/Redo を管理する。
    /// </summary>
    class SceneCommandHistory
    {
    public:
        /// <summary>
        /// 既存履歴を破棄して初期スナップショットを設定する。
        /// </summary>
        /// <param name="entry">履歴の先頭へ設定するスナップショット。</param>
        void Reset(SceneCommandHistoryEntry entry);

        /// <summary>
        /// 新しい編集結果を履歴へ積む。
        /// </summary>
        /// <param name="entry">追加するスナップショット。</param>
        void Push(SceneCommandHistoryEntry entry);

        /// <summary>
        /// Undo 可能かを取得する。
        /// </summary>
        /// <returns>Undo 可能な場合は true。</returns>
        bool CanUndo() const;

        /// <summary>
        /// Redo 可能かを取得する。
        /// </summary>
        /// <returns>Redo 可能な場合は true。</returns>
        bool CanRedo() const;

        /// <summary>
        /// 1 段階 Undo して復元対象スナップショットを返す。
        /// </summary>
        /// <returns>復元対象スナップショット。Undo 不可の場合は空。</returns>
        std::optional<SceneCommandHistoryEntry> Undo();

        /// <summary>
        /// 1 段階 Redo して復元対象スナップショットを返す。
        /// </summary>
        /// <returns>復元対象スナップショット。Redo 不可の場合は空。</returns>
        std::optional<SceneCommandHistoryEntry> Redo();

        /// <summary>
        /// 現在位置のスナップショットを取得する。
        /// </summary>
        /// <returns>現在スナップショット。未初期化時は空。</returns>
        std::optional<SceneCommandHistoryEntry> GetCurrent() const;

        /// <summary>
        /// 履歴に保存されている件数を取得する。
        /// </summary>
        /// <returns>履歴件数。</returns>
        std::size_t GetCount() const;

    private:
        std::vector<SceneCommandHistoryEntry> m_entries{};
        std::size_t m_currentIndex = 0;
    };
}
