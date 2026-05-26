#pragma once

#include <Windows.h>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "ButtonClickInput.h"
#include "Commands/SceneEditingOperations.h"
#include "Shell/EditorShell.h"
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
        /// Hierarchy の表示行種別を表す。
        /// </summary>
        enum class VisibleItemKind
        {
            Entity,
            SpriteComponent,
            Material,
            Collider2DComponent
        };

        /// <summary>
        /// Hierarchy の Entity 行ラベルを生成する。
        /// </summary>
        /// <param name="entityName">Entity 名。</param>
        /// <param name="hasChildComponent">子 Component 行を持つか。</param>
        /// <param name="isExpanded">Entity 行が展開中か。</param>
        /// <returns>Hierarchy 表示用の Entity 行ラベル。</returns>
        [[nodiscard]] static std::wstring FormatEntityRowLabel(
            std::wstring_view entityName,
            bool hasChildComponent,
            bool isExpanded)
        {
            std::wstring label{ entityName };
            if (hasChildComponent)
            {
                label.insert(0, isExpanded ? L"- " : L"+ ");
            }
            else
            {
                label.insert(0, L"  ");
            }

            return label;
        }

        /// <summary>
        /// Component 子行を表示するかを判定する。
        /// </summary>
        /// <param name="hasComponent">Entity が対象 Component を保持しているか。</param>
        /// <param name="isExpanded">Entity 行が展開中か。</param>
        /// <returns>子行を表示する場合は true。</returns>
        [[nodiscard]] static bool ShouldShowComponentChildRow(
            bool hasComponent,
            bool isExpanded)
        {
            return hasComponent && isExpanded;
        }

        /// <summary>
        /// Collider2DComponent 子行を表示するかを判定する。
        /// </summary>
        /// <param name="hasCollider2DComponent">Entity が Collider2DComponent を保持しているか。</param>
        /// <param name="isExpanded">Entity 行が展開中か。</param>
        /// <returns>子行を表示する場合は true。</returns>
        [[nodiscard]] static bool ShouldShowCollider2DChildRow(
            bool hasCollider2DComponent,
            bool isExpanded)
        {
            return ShouldShowComponentChildRow(hasCollider2DComponent, isExpanded);
        }

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

        /// <summary>
        /// 現在選択中の Hierarchy 行種別を取得する。
        /// </summary>
        /// <returns>現在選択中の行種別。</returns>
        [[nodiscard]] VisibleItemKind GetSelectedItemKind() const;

    private:
        HWND m_hierarchyListBox = nullptr;
        HWND m_hierarchySummaryLabel = nullptr;
        HWND m_hierarchySearchEdit = nullptr;
        HWND m_hierarchyNameEdit = nullptr;
        HWND m_hierarchyCreateButton = nullptr;
        HWND m_hierarchyDuplicateButton = nullptr;
        HWND m_hierarchyDeleteButton = nullptr;
        std::vector<Game::EntityId> m_visibleEntityIds{};
        std::vector<VisibleItemKind> m_visibleItemKinds{};
        std::set<Game::EntityId> m_collapsedEntityIds{};
        std::optional<Game::EntityId> m_selectedEntityId{};
        VisibleItemKind m_selectedItemKind = VisibleItemKind::Entity;
        std::optional<Game::EntityId> m_lastEditedEntityId{};
        std::wstring m_lastSearchText{};
        ButtonClickInputState m_buttonInputState{};
        bool m_preserveNoSelection = false;

        /// <summary>
        /// 現在選択中の Entity 行の展開状態を切り替える。
        /// </summary>
        /// <param name="scene">表示対象の Scene。</param>
        /// <returns>展開状態を変更した場合は true。</returns>
        bool ToggleSelectedEntityExpansion(const Game::Scene& scene);

        /// <summary>
        /// Hierarchy 展開状態をローカル UI 状態ファイルから復元する。
        /// </summary>
        void LoadExpansionState();

        /// <summary>
        /// Hierarchy 展開状態をローカル UI 状態ファイルへ保存する。
        /// </summary>
        void SaveExpansionState() const;
    };
}
