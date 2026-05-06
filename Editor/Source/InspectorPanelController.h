#pragma once

#include <Windows.h>
#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "AssetId.h"
#include "EditorStringUtils.h"
#include "EditorShell.h"
#include "InputSystem.h"
#include "SceneEditingOperations.h"
#include "Scene.h"

namespace Xelqoria::Editor
{
    class AssetsPanelController;

    /// <summary>
    /// Inspector で行われた編集適用結果を表す。
    /// </summary>
    struct InspectorApplyResult
    {
        /// <summary>
        /// Scene に変更が加わったかを表す。
        /// </summary>
        bool changed = false;
    };

    /// <summary>
    /// SpriteComponent 操作 UI の表示状態を表す。
    /// </summary>
    struct InspectorSpriteComponentActionState
    {
        /// <summary>
        /// SpriteRef 入力欄を表示するかを表す。
        /// </summary>
        bool showSpriteRefControls = false;

        /// <summary>
        /// SpriteComponent セクション見出しを表す。
        /// </summary>
        const wchar_t* sectionLabel = L"SpriteComponent";

        /// <summary>
        /// SpriteComponent 操作ボタン文言を表す。
        /// </summary>
        const wchar_t* buttonLabel = L"Add SpriteComponent";

        /// <summary>
        /// SpriteComponent 操作ボタンを有効化するかを表す。
        /// </summary>
        bool enableActionButton = false;
    };

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
        /// <param name="canAddSpriteComponent">SpriteComponent を追加できる SpriteAsset があるか。</param>
        void Refresh(
            const Game::Scene* scene,
            std::optional<Game::EntityId> selectedEntityId,
            bool canAddSpriteComponent);

        /// <summary>
        /// Inspector 入力値を現在選択中 Entity へ反映する。
        /// </summary>
        /// <param name="scene">更新対象の Scene。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        /// <param name="canAddSpriteComponent">SpriteComponent を追加できる SpriteAsset があるか。</param>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        /// <returns>適用結果。</returns>
        InspectorApplyResult ApplyEdits(
            Game::Scene* scene,
            std::optional<Game::EntityId> selectedEntityId,
            bool canAddSpriteComponent,
            const Core::InputSnapshot& inputSnapshot);

        /// <summary>
        /// Assets から Texture 欄へドロップされた画像を SpriteComponent へ反映する。
        /// </summary>
        /// <param name="scene">更新対象の Scene。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        /// <param name="assetsPanelController">Assets パネルのドラッグ状態。</param>
        /// <returns>適用結果。</returns>
        InspectorApplyResult ApplyTextureDrop(
            Game::Scene* scene,
            std::optional<Game::EntityId> selectedEntityId,
            const AssetsPanelController& assetsPanelController);

        /// <summary>
        /// Texture 欄のドロップ先ハイライトを現在のドラッグ状態へ同期する。
        /// </summary>
        /// <param name="assetsPanelController">Assets パネルのドラッグ状態。</param>
        void UpdateTextureDropHighlight(const AssetsPanelController& assetsPanelController);

        /// <summary>
        /// 直前反映 Entity の追跡状態を破棄する。
        /// </summary>
        void ResetTrackedEntity();

        /// <summary>
        /// SpriteAssetId から Texture 欄に表示するファイル名を取得する。
        /// </summary>
        /// <param name="spriteAssetId">表示対象の SpriteAssetId。</param>
        /// <returns>Texture 欄表示文字列。</returns>
        [[nodiscard]] static std::wstring FormatTextureDisplayText(const Core::AssetId& spriteAssetId)
        {
            std::string spriteRefValue = spriteAssetId.GetValue();
            constexpr std::string_view spriteAssetPrefix = "sprites/";
            if (spriteRefValue.starts_with(spriteAssetPrefix))
            {
                spriteRefValue = spriteRefValue.substr(spriteAssetPrefix.size());
            }

            const std::wstring relativePath = ToWideString(spriteRefValue);
            const std::filesystem::path displayPath(relativePath);
            const std::wstring fileName = displayPath.filename().wstring();
            if (false == fileName.empty())
            {
                return fileName;
            }

            return relativePath;
        }

        /// <summary>
        /// SpriteComponent 操作 UI の表示状態を計算する。
        /// </summary>
        /// <param name="hasSpriteComponent">Entity が SpriteComponent を保持しているか。</param>
        /// <param name="canAddSpriteComponent">SpriteComponent を追加できる SpriteAsset があるか。</param>
        /// <returns>UI 表示状態。</returns>
        [[nodiscard]] static InspectorSpriteComponentActionState ComputeSpriteComponentActionState(
            bool hasSpriteComponent,
            bool canAddSpriteComponent)
        {
            if (hasSpriteComponent)
            {
                return InspectorSpriteComponentActionState{
                    true,
                    L"SpriteComponent",
                    L"Remove SpriteComponent",
                    true
                };
            }

            return InspectorSpriteComponentActionState{
                false,
                L"SpriteComponent (not attached)",
                L"Add SpriteComponent",
                canAddSpriteComponent
            };
        }

        /// <summary>
        /// SpriteComponent 操作ボタン押下時の追加・削除処理を適用する。
        /// </summary>
        /// <param name="entity">更新対象 Entity。</param>
        /// <param name="canAddSpriteComponent">SpriteComponent を追加できる SpriteAsset があるか。</param>
        /// <returns>Entity が更新された場合は true。</returns>
        [[nodiscard]] static bool ApplySpriteComponentAction(Game::Entity& entity, bool canAddSpriteComponent)
        {
            if (entity.HasSpriteComponent())
            {
                return SceneEditingOperations::RemoveSpriteComponent(entity);
            }

            if (false == canAddSpriteComponent)
            {
                return false;
            }

            return SceneEditingOperations::AddSpriteComponent(entity);
        }

    private:
        /// <summary>
        /// ボタン押下の完了を検出してクリックとして消費する。
        /// </summary>
        /// <param name="buttonHandle">監視対象のボタン HWND。</param>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        /// <returns>今回のフレームでクリックが成立した場合は true。</returns>
        bool ConsumeButtonClick(HWND buttonHandle, const Core::InputSnapshot& inputSnapshot);

        /// <summary>
        /// 現在のカーソル位置が Texture ドロップ対象内かを取得する。
        /// </summary>
        /// <param name="assetsPanelController">Assets パネルのドラッグ状態。</param>
        /// <returns>ドロップ対象内の場合は true。</returns>
        [[nodiscard]] bool IsTextureDropTargetHovered(const AssetsPanelController& assetsPanelController) const;

        HWND m_inspectorSummaryLabel = nullptr;
        HWND m_transformSectionLabel = nullptr;
        std::array<HWND, 9> m_transformEditControls{};
        HWND m_spriteComponentSectionLabel = nullptr;
        HWND m_spriteRefLabel = nullptr;
        HWND m_spriteRefDropHighlight = nullptr;
        HWND m_spriteRefEdit = nullptr;
        HWND m_spriteComponentActionButton = nullptr;
        std::optional<Game::EntityId> m_lastInspectorEntityId{};
        Core::AssetId m_lastSpriteRefAssetId{};
        std::wstring m_lastSpriteRefDisplayText{};
        bool m_wasLeftMouseButtonDown = false;
        HWND m_pressedButtonHandle = nullptr;
        bool m_isTextureDropHighlightVisible = false;
    };
}
