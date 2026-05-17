#pragma once

#include <Windows.h>
#include <optional>
#include <vector>

#include "ButtonClickInput.h"
#include "SceneEditingOperations.h"
#include "EditorShell.h"
#include "InputSystem.h"
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
        /// Hierarchy パネル上の編集操作を現在の Scene へ反映する。
        /// </summary>
        /// <param name="scene">更新対象の Scene。</param>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        /// <returns>適用結果。</returns>
        SceneEditResult ApplyEdits(Game::Scene* scene, const Core::InputSnapshot& inputSnapshot);

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
        HWND m_hierarchyNameEdit = nullptr;
        HWND m_hierarchyCreateButton = nullptr;
        HWND m_hierarchyDuplicateButton = nullptr;
        HWND m_hierarchyDeleteButton = nullptr;
        std::vector<Game::EntityId> m_visibleEntityIds{};
        std::optional<Game::EntityId> m_selectedEntityId{};
        std::optional<Game::EntityId> m_lastEditedEntityId{};
        ButtonClickInputState m_buttonInputState{};
        bool m_preserveNoSelection = false;
    };
}
