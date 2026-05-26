#pragma once

#include <Windows.h>
#include <optional>

#include "Scene.h"
#include "SceneView/SceneViewInteractionTypes.h"
#include <cstdint>
#include <Entity.h>

namespace Xelqoria::Editor
{
    /// <summary>
    /// SceneView の状態ラベルを更新する。
    /// </summary>
    class SceneViewStatusPresenter
    {
    public:
        /// <summary>
        /// 現在の入力状態に応じて SceneView ラベルを更新する。
        /// </summary>
        void PresentInteractionStatus(
            HWND sceneViewSizeLabel,
            HWND sceneViewPlanLabel,
            std::uint32_t sceneViewWidth,
            std::uint32_t sceneViewHeight,
            const Game::Scene* scene,
            std::optional<Game::EntityId> selectedEntityId,
            const ScenePendingDropState& pendingDropState,
            const SceneDragPreviewState& dragPreviewState,
            bool hasSceneClick,
            float lastSceneClickX,
            float lastSceneClickY) const;

        /// <summary>
        /// 現在選択状態に応じて SceneView サイズラベルを更新する。
        /// </summary>
        void PresentSelectionStatus(
            HWND sceneViewSizeLabel,
            std::uint32_t sceneViewWidth,
            std::uint32_t sceneViewHeight,
            const Game::Scene* scene,
            std::optional<Game::EntityId> selectedEntityId) const;

        /// <summary>
        /// ドロップ処理結果に応じて SceneView ラベルを更新する。
        /// </summary>
        void PresentDropStatus(
            HWND sceneViewSizeLabel,
            HWND sceneViewPlanLabel,
            std::uint32_t sceneViewWidth,
            std::uint32_t sceneViewHeight,
            const SceneViewDropResult& dropResult) const;
    };
}
