#pragma once

#include <Windows.h>
#include <array>
#include <optional>

#include "EditorShell.h"
#include "Scene.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Inspector パネルの表示同期と入力反映を管理する。
    /// </summary>
    class InspectorPanelController
    {
    public:
        /// <summary>
        /// EditorShell の HWND 群へ接続する。
        /// </summary>
        /// <param name="shell">接続先の EditorShell。</param>
        void Bind(const EditorShell& shell);

        /// <summary>
        /// 現在選択中 Entity を Inspector 表示へ反映する。
        /// </summary>
        /// <param name="scene">参照対象の Scene。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        void Refresh(const Game::Scene* scene, std::optional<Game::EntityId> selectedEntityId);

        /// <summary>
        /// Inspector 入力値を現在選択中 Entity へ反映する。
        /// </summary>
        /// <param name="scene">更新対象の Scene。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        void ApplyEdits(Game::Scene* scene, std::optional<Game::EntityId> selectedEntityId);

        /// <summary>
        /// 直前反映 Entity の追跡状態を破棄する。
        /// </summary>
        void ResetTrackedEntity();

    private:
        HWND m_inspectorSummaryLabel = nullptr;
        std::array<HWND, 9> m_transformEditControls{};
        HWND m_spriteRefEdit = nullptr;
        std::optional<Game::EntityId> m_lastInspectorEntityId{};
    };
}
