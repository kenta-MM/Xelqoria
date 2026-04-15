#pragma once

#include <Windows.h>
#include <optional>
#include <vector>

#include "EditorShell.h"
#include "Scene.h"
#include <Entity.h>

namespace Xelqoria::Editor
{
    /// <summary>
    /// Hierarchy パネルの表示一覧と選択状態を管理する。
    /// </summary>
    class HierarchyPanelController
    {
    public:
        /// <summary>
        /// EditorShell の HWND 群へ接続する。
        /// </summary>
        /// <param name="shell">接続先の EditorShell。</param>
        void Bind(const EditorShell& shell);

        /// <summary>
        /// 現在の Scene に合わせて Hierarchy 表示を更新する。
        /// </summary>
        /// <param name="scene">表示対象の Scene。</param>
        void Refresh(const Game::Scene* scene);

        /// <summary>
        /// ListBox 選択状態を内部選択状態へ同期する。
        /// </summary>
        /// <returns>選択が変化した場合は true。</returns>
        bool SyncSelection();

        /// <summary>
        /// 現在選択中の EntityId を設定する。
        /// </summary>
        /// <param name="selectedEntityId">設定する EntityId。</param>
        void SetSelectedEntityId(std::optional<Game::EntityId> selectedEntityId);

        /// <summary>
        /// 現在選択中の EntityId を取得する。
        /// </summary>
        /// <returns>現在選択中の EntityId。</returns>
        [[nodiscard]] std::optional<Game::EntityId> GetSelectedEntityId() const;

    private:
        HWND m_hierarchyListBox = nullptr;
        HWND m_hierarchySummaryLabel = nullptr;
        std::vector<Game::EntityId> m_visibleEntityIds{};
        std::optional<Game::EntityId> m_selectedEntityId{};
    };
}
