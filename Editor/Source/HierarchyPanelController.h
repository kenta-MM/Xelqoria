#pragma once

#include <Windows.h>
#include <optional>
#include <vector>

#include "SceneEditingOperations.h"
#include "EditorShell.h"
#include "Scene.h"
#include <Entity.h>

namespace Xelqoria::Editor
{
    /// <summary>
    /// Hierarchy ボタン入力を 1 フレーム分だけ表す。
    /// </summary>
    struct HierarchyButtonFrameInput
    {
        /// <summary>
        /// 現在フレームで左マウスボタンが押下中かを表す。
        /// </summary>
        bool isLeftMouseButtonDown = false;

        /// <summary>
        /// 現在フレームのカーソル位置をスクリーン座標で表す。
        /// </summary>
        POINT cursorScreenPoint{};
    };

    /// <summary>
    /// Hierarchy ボタン入力の継続状態を表す。
    /// </summary>
    struct HierarchyButtonInputState
    {
        /// <summary>
        /// 前フレームで左マウスボタンが押下中だったかを表す。
        /// </summary>
        bool wasLeftMouseButtonDown = false;

        /// <summary>
        /// 押下開始を検出したボタン HWND を表す。
        /// </summary>
        HWND pressedButtonHandle = nullptr;
    };

    /// <summary>
    /// 共有入力スナップショットから対象ボタンのクリック成立を判定する。
    /// </summary>
    /// <param name="buttonHandle">判定対象のボタン HWND。</param>
    /// <param name="frameInput">現在フレームの入力状態。</param>
    /// <param name="inputState">前フレームから持ち越す継続状態。</param>
    /// <returns>今回のフレームでクリックが成立した場合は true。</returns>
    inline bool TryConsumeHierarchyButtonClick(
        HWND buttonHandle,
        const HierarchyButtonFrameInput& frameInput,
        HierarchyButtonInputState& inputState)
    {
        if (nullptr == buttonHandle || false == IsWindowVisible(buttonHandle) || false == IsWindowEnabled(buttonHandle))
        {
            return false;
        }

        RECT buttonRect{};
        GetWindowRect(buttonHandle, &buttonRect);
        const bool isCursorInsideButton = PtInRect(&buttonRect, frameInput.cursorScreenPoint) != FALSE;

        if (frameInput.isLeftMouseButtonDown
            && false == inputState.wasLeftMouseButtonDown
            && true == isCursorInsideButton)
        {
            inputState.pressedButtonHandle = buttonHandle;
        }

        return false == frameInput.isLeftMouseButtonDown
            && true == inputState.wasLeftMouseButtonDown
            && inputState.pressedButtonHandle == buttonHandle
            && true == isCursorInsideButton;
    }

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
        /// <returns>適用結果。</returns>
        SceneEditResult ApplyEdits(Game::Scene* scene);

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
        bool m_wasEnterKeyDown = false;
        HierarchyButtonInputState m_buttonInputState{};
    };
}
