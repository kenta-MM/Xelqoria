#pragma once

#include <optional>
#include <string_view>

#include "Scene.h"
#include <Entity.h>

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
        /// 新しい Entity を作成する。
        /// </summary>
        /// <param name="scene">編集対象の Scene。</param>
        /// <returns>作成結果と更新後の選択状態。</returns>
        static SceneEditResult CreateEntity(Game::Scene& scene);

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

        /// <summary>
        /// 指定 Entity の配置位置を更新する。
        /// </summary>
        /// <param name="scene">編集対象の Scene。</param>
        /// <param name="entityId">更新対象の EntityId。</param>
        /// <param name="x">設定する X 座標。</param>
        /// <param name="y">設定する Y 座標。</param>
        /// <returns>位置変更で更新が発生した場合は true。</returns>
        static bool MoveEntity(
            Game::Scene& scene,
            Game::EntityId entityId,
            float x,
            float y);

        /// <summary>
        /// Entity 名を更新する。
        /// </summary>
        /// <param name="entity">更新対象の Entity。</param>
        /// <param name="newName">設定する Entity 名。</param>
        /// <returns>名前変更で更新が発生した場合は true。</returns>
        static bool RenameEntity(Game::Entity& entity, std::string_view newName);

        /// <summary>
        /// Entity に既定の SpriteComponent を追加する。
        /// </summary>
        /// <param name="entity">追加対象の Entity。</param>
        /// <returns>追加で変更が発生した場合は true。</returns>
        static bool AddSpriteComponent(Game::Entity& entity);

        /// <summary>
        /// Entity から SpriteComponent を削除する。
        /// </summary>
        /// <param name="entity">削除対象の Entity。</param>
        /// <returns>削除で変更が発生した場合は true。</returns>
        static bool RemoveSpriteComponent(Game::Entity& entity);
    };
}
