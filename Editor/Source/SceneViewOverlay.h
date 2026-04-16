#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include "Entity.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// SceneView の選択ヒット判定に使用する矩形ターゲットを表す。
    /// </summary>
    struct SceneViewHitTarget
    {
        /// <summary>
        /// 対応する Entity ID を表す。
        /// </summary>
        Game::EntityId entityId = 0;

        /// <summary>
        /// ワールド座標系での矩形中心 X 座標を表す。
        /// </summary>
        float centerX = 0.0f;

        /// <summary>
        /// ワールド座標系での矩形中心 Y 座標を表す。
        /// </summary>
        float centerY = 0.0f;

        /// <summary>
        /// ワールド座標系での矩形幅を表す。
        /// </summary>
        float width = 0.0f;

        /// <summary>
        /// ワールド座標系での矩形高さを表す。
        /// </summary>
        float height = 0.0f;

    };

    /// <summary>
    /// 軸平行矩形の内側に点が含まれるかを判定する。
    /// </summary>
    /// <param name="target">判定対象の矩形。</param>
    /// <param name="worldX">判定するワールド X 座標。</param>
    /// <param name="worldY">判定するワールド Y 座標。</param>
    /// <returns>点が矩形内に含まれる場合は true。</returns>
    inline bool ContainsPoint(const SceneViewHitTarget& target, float worldX, float worldY)
    {
        const float halfWidth = target.width * 0.5f;
        const float halfHeight = target.height * 0.5f;
        return worldX >= target.centerX - halfWidth
            && worldX <= target.centerX + halfWidth
            && worldY >= target.centerY - halfHeight
            && worldY <= target.centerY + halfHeight;
    }

    /// <summary>
    /// 指定座標に重なる候補から最前面相当の Entity を選択する。
    /// </summary>
    /// <param name="targets">判定対象の候補一覧。配列末尾ほど前面に描画された候補を表す。</param>
    /// <param name="worldX">判定するワールド X 座標。</param>
    /// <param name="worldY">判定するワールド Y 座標。</param>
    /// <returns>選択された Entity ID。候補が無い場合は空。</returns>
    inline std::optional<Game::EntityId> PickTopmostEntityAtWorldPoint(
        std::span<const SceneViewHitTarget> targets,
        float worldX,
        float worldY)
    {
        for (auto iterator = targets.rbegin(); iterator != targets.rend(); ++iterator)
        {
            const SceneViewHitTarget& target = *iterator;
            if (false == ContainsPoint(target, worldX, worldY))
            {
                continue;
            }

            return target.entityId;
        }

        return std::nullopt;
    }
}
