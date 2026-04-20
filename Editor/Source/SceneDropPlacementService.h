#pragma once

#include "EditorSceneDocument.h"
#include "SceneViewInteractionTypes.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// SceneView ドロップ入力を Scene へ適用する。
    /// </summary>
    class SceneDropPlacementService
    {
    public:
        /// <summary>
        /// 保留中ドロップを Scene へ反映する。
        /// </summary>
        /// <param name="document">更新対象の Scene ドキュメント。</param>
        /// <param name="pendingDropState">適用する保留中ドロップ状態。</param>
        /// <returns>ドロップ処理結果。</returns>
        [[nodiscard]] SceneViewDropResult ProcessPendingSceneDrop(
            EditorSceneDocument& document,
            const ScenePendingDropState& pendingDropState) const;
    };
}
