#pragma once

#include <optional>

#include "Scene.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Scene 編集コマンドの適用結果を表す。
    /// </summary>
    struct SceneEditResult
    {
        /// <summary>
        /// Scene に変更が加わったかを表す。
        /// </summary>
        bool changed = false;

        /// <summary>
        /// 編集後に選択すべき EntityId を表す。
        /// </summary>
        std::optional<Game::EntityId> selectedEntityId{};
    };

    /// <summary>
    /// Editor から利用する基本的な Scene 編集操作を提供する。
    /// </summary>
    class SceneEditingOperations
    {
    public:
        /// <summary>
        /// 現在選択中の Entity を削除する。
        /// </summary>
        /// <param name="scene">編集対象の Scene。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        /// <returns>削除結果と更新後の選択状態。</returns>
        static SceneEditResult DeleteSelectedEntity(
            Game::Scene& scene,
            std::optional<Game::EntityId> selectedEntityId);

        /// <summary>
        /// 現在選択中の Entity を複製する。
        /// </summary>
        /// <param name="scene">編集対象の Scene。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        /// <returns>複製結果と更新後の選択状態。</returns>
        static SceneEditResult DuplicateSelectedEntity(
            Game::Scene& scene,
            std::optional<Game::EntityId> selectedEntityId);
    };
}
