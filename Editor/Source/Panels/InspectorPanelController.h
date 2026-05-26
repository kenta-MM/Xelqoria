#pragma once

#include <Windows.h>
#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "AssetId.h"
#include "Assets/ISpriteAssetResolver.h"
#include "Assets/IMaterialAssetResolver.h"
#include "Assets/SpriteAsset.h"
#include "Assets/SpriteMaterialAsset.h"
#include "Utils/EditorStringUtils.h"
#include "Panels/HierarchyPanelController.h"
#include "ICursor.h"
#include "InputSystem.h"
#include "Commands/SceneEditingOperations.h"
#include "Scene.h"

namespace Xelqoria::Editor
{
    class AssetsPanelController;
    class InspectorPanelView;

    /// <summary>
    /// Inspector から要求された Script 操作種別を表す。
    /// </summary>
    enum class InspectorScriptAction
    {
        None = 0,
        Create,
        Assign,
        AssignDropped,
        Clear
    };

    /// <summary>
    /// Inspector で行われた編集適用結果を表す。
    /// </summary>
    struct InspectorApplyResult
    {
        /// <summary>
        /// Scene に変更が加わったかを表す。
        /// </summary>
        bool changed = false;

        /// <summary>
        /// Script Asset に対する操作要求を表す。
        /// </summary>
        InspectorScriptAction scriptAction = InspectorScriptAction::None;

        /// <summary>
        /// Script 操作対象の SpriteAssetId を表す。
        /// </summary>
        Core::AssetId scriptTargetSpriteAssetId{};

        /// <summary>
        /// ドロップされた Script Asset ファイルパスを表す。
        /// </summary>
        std::filesystem::path droppedScriptAssetPath{};

        /// <summary>
        /// 履歴へ記録する操作名を表す。
        /// </summary>
        std::string operationName{};

        /// <summary>
        /// Material タブで開く要求があるかを表す。
        /// </summary>
        bool openMaterialRequested = false;

        /// <summary>
        /// Material タブで開く MaterialAssetId を表す。
        /// </summary>
        Core::AssetId openMaterialAssetId{};

        /// <summary>
        /// Collider2D タブを開く要求があるかを表す。
        /// </summary>
        bool openCollider2DRequested = false;

        /// <summary>
        /// Collider2DComponent が追加されたかを表す。
        /// </summary>
        bool collider2DComponentAdded = false;

        /// <summary>
        /// Material Asset の保存が必要かを表す。
        /// </summary>
        bool materialAssetChanged = false;

        /// <summary>
        /// 保存対象の MaterialAssetId を表す。
        /// </summary>
        Core::AssetId materialTargetAssetId{};

        /// <summary>
        /// 保存する Material 内容を表す。
        /// </summary>
        Game::Assets::SpriteMaterialAsset materialAsset{};
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
    /// Collider2DComponent 操作 UI の表示状態を表す。
    /// </summary>
    struct InspectorCollider2DComponentActionState
    {
        /// <summary>
        /// Collider2D 編集欄を表示するかを表す。
        /// </summary>
        bool showColliderControls = false;

        /// <summary>
        /// Collider2DComponent セクション見出しを表す。
        /// </summary>
        const wchar_t* sectionLabel = L"Collider2DComponent";

        /// <summary>
        /// Collider2DComponent 操作ボタン文言を表す。
        /// </summary>
        const wchar_t* buttonLabel = L"Add Collider2DComponent";
    };

    /// <summary>
    /// Script 操作 UI の表示状態を表す。
    /// </summary>
    struct InspectorScriptActionState
    {
        /// <summary>
        /// Script 操作 UI を表示するかを表す。
        /// </summary>
        bool showScriptControls = false;

        /// <summary>
        /// Script 作成ボタンを有効化するかを表す。
        /// </summary>
        bool enableCreateButton = false;

        /// <summary>
        /// Script 割り当てボタンを有効化するかを表す。
        /// </summary>
        bool enableAssignButton = false;

        /// <summary>
        /// Script 解除ボタンを有効化するかを表す。
        /// </summary>
        bool enableClearButton = false;
    };

    /// <summary>
    /// Inspector パネルの表示同期と入力反映を管理する。
    /// </summary>
    class InspectorPanelController
    {
    public:
        /// <summary>
        /// Inspector Panel View の HWND 群へ接続する。
        /// </summary>
        /// <param name="view">接続先の Panel View。</param>
        /// <param name="cursor">カーソル位置を取得する Platform 実装。</param>
        void Bind(const InspectorPanelView& view, Platform::ICursor& cursor);

        /// <summary>
        /// 現在選択中 Entity を Inspector 表示へ反映する。
        /// </summary>
        /// <param name="scene">参照対象の Scene。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        /// <param name="canAddSpriteComponent">SpriteComponent を追加できる SpriteAsset があるか。</param>
        void Refresh(
            const Game::Scene* scene,
            std::optional<Game::EntityId> selectedEntityId,
            bool canAddSpriteComponent,
            const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
            const Game::Assets::IMaterialAssetResolver& materialAssetResolver,
            HierarchyPanelController::VisibleItemKind selectedItemKind,
            const std::filesystem::path& selectedAssetPath = {});

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
            const Core::InputSnapshot& inputSnapshot,
            const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
            const Game::Assets::IMaterialAssetResolver& materialAssetResolver,
            HierarchyPanelController::VisibleItemKind selectedItemKind);

        /// <summary>
        /// Assets から Material 欄へドロップされた Material を SpriteComponent へ反映する。
        /// </summary>
        /// <param name="scene">更新対象の Scene。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        /// <param name="assetsPanelController">Assets パネルのドラッグ状態。</param>
        /// <param name="spriteAssetResolver">SpriteAsset 参照の表示更新に使う Resolver。</param>
        /// <param name="materialAssetResolver">MaterialAsset 参照の表示更新に使う Resolver。</param>
        /// <returns>適用結果。</returns>
        InspectorApplyResult ApplyMaterialDrop(
            Game::Scene* scene,
            std::optional<Game::EntityId> selectedEntityId,
            const AssetsPanelController& assetsPanelController,
            const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
            const Game::Assets::IMaterialAssetResolver& materialAssetResolver);

        /// <summary>
        /// Assets から Script 欄へドロップされた Script Asset を SpriteAsset へ割り当てる要求に変換する。
        /// </summary>
        /// <param name="scene">参照対象の Scene。</param>
        /// <param name="selectedEntityId">現在選択中の EntityId。</param>
        /// <param name="assetsPanelController">Assets パネルのドラッグ状態。</param>
        /// <returns>適用結果。</returns>
        [[nodiscard]] InspectorApplyResult ApplyScriptDrop(
            const Game::Scene* scene,
            std::optional<Game::EntityId> selectedEntityId,
            const AssetsPanelController& assetsPanelController) const;

        /// <summary>
        /// Material 欄と Script 欄のドロップ先ハイライトを現在のドラッグ状態へ同期する。
        /// </summary>
        /// <param name="assetsPanelController">Assets パネルのドラッグ状態。</param>
        void UpdateDropHighlight(const AssetsPanelController& assetsPanelController);

        /// <summary>
        /// Assets から Material Details の Texture 欄へドロップされた Texture を Material Asset へ反映する。
        /// </summary>
        [[nodiscard]] InspectorApplyResult ApplyMaterialTextureDrop(
            const Game::Scene* scene,
            std::optional<Game::EntityId> selectedEntityId,
            const AssetsPanelController& assetsPanelController,
            const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
            const Game::Assets::IMaterialAssetResolver& materialAssetResolver) const;

        /// <summary>
        /// 現在のカーソル位置が Script ドロップ対象内かを取得する。
        /// </summary>
        /// <param name="assetsPanelController">Assets パネルのドラッグ状態。</param>
        /// <returns>ドロップ対象内の場合は true。</returns>
        [[nodiscard]] bool IsScriptDropTargetHovered(const AssetsPanelController& assetsPanelController) const;

        /// <summary>
        /// 直前反映 Entity の追跡状態を破棄する。
        /// </summary>
        void ResetTrackedEntity();

        /// <summary>
        /// Inspector 折りたたみ状態をローカル UI 状態ファイルから復元する。
        /// </summary>
        void LoadCollapseState();

        /// <summary>
        /// Inspector 折りたたみ状態をローカル UI 状態ファイルへ保存する。
        /// </summary>
        void SaveCollapseState() const;

        /// <summary>
        /// TextureAssetId から Texture 欄に表示するファイル名を取得する。
        /// </summary>
        /// <param name="textureAssetId">表示対象の TextureAssetId。</param>
        /// <returns>Texture 欄表示文字列。</returns>
        [[nodiscard]] static std::wstring FormatTextureDisplayText(const Core::AssetId& textureAssetId)
        {
            std::string textureRefValue = textureAssetId.GetValue();
            constexpr std::string_view textureAssetPrefix = "textures/";
            if (textureRefValue.starts_with(textureAssetPrefix))
            {
                textureRefValue = textureRefValue.substr(textureAssetPrefix.size());
            }

            const std::wstring relativePath = ToWideString(textureRefValue);
            const std::filesystem::path displayPath(relativePath);
            const std::wstring fileName = displayPath.filename().wstring();
            if (false == fileName.empty())
            {
                return fileName;
            }

            return relativePath;
        }

        /// <summary>
        /// ScriptAssetId から Script 欄に表示するファイル名を取得する。
        /// </summary>
        /// <param name="scriptAssetId">表示対象の ScriptAssetId。</param>
        /// <returns>Script 欄表示文字列。</returns>
        [[nodiscard]] static std::wstring FormatScriptDisplayText(const Core::AssetId& scriptAssetId)
        {
            if (scriptAssetId.IsEmpty())
            {
                return L"None";
            }

            std::string scriptRefValue = scriptAssetId.GetValue();
            constexpr std::string_view scriptAssetPrefix = "scripts/";
            if (scriptRefValue.starts_with(scriptAssetPrefix))
            {
                scriptRefValue = scriptRefValue.substr(scriptAssetPrefix.size());
            }

            const std::wstring relativePath = ToWideString(scriptRefValue);
            const std::filesystem::path displayPath(relativePath);
            const std::wstring fileName = displayPath.filename().wstring();
            if (false == fileName.empty())
            {
                return fileName;
            }

            return relativePath;
        }

        /// <summary>
        /// MaterialAssetId から Material 欄に表示するファイル名を取得する。
        /// </summary>
        /// <param name="materialAssetId">表示対象の MaterialAssetId。</param>
        /// <returns>Material 欄表示文字列。</returns>
        [[nodiscard]] static std::wstring FormatMaterialDisplayText(const Core::AssetId& materialAssetId)
        {
            if (materialAssetId.IsEmpty())
            {
                return L"None";
            }

            std::string materialRefValue = materialAssetId.GetValue();
            constexpr std::string_view materialAssetPrefix = "materials/";
            if (materialRefValue.starts_with(materialAssetPrefix))
            {
                materialRefValue = materialRefValue.substr(materialAssetPrefix.size());
            }

            const std::wstring relativePath = ToWideString(materialRefValue);
            const std::filesystem::path displayPath(relativePath);
            const std::wstring fileName = displayPath.filename().wstring();
            if (false == fileName.empty())
            {
                return fileName;
            }

            return relativePath;
        }

        /// <summary>
        /// Script 操作 UI の表示状態を計算する。
        /// </summary>
        /// <param name="hasSpriteComponent">Entity が SpriteComponent を保持しているか。</param>
        /// <param name="spriteAssetRef">SpriteComponent が参照する SpriteAssetId。</param>
        /// <param name="spriteAssetResolved">SpriteAsset を解決できた場合は true。</param>
        /// <param name="scriptAssetId">現在割り当てられている ScriptAssetId。</param>
        /// <returns>Script 操作 UI 表示状態。</returns>
        [[nodiscard]] static InspectorScriptActionState ComputeScriptActionState(
            bool hasSpriteComponent,
            const Core::AssetId& spriteAssetRef,
            bool spriteAssetResolved,
            const Core::AssetId& scriptAssetId)
        {
            const bool canEditScript = hasSpriteComponent
                && false == spriteAssetRef.IsEmpty()
                && true == spriteAssetResolved;
            return InspectorScriptActionState{
                hasSpriteComponent,
                canEditScript,
                canEditScript,
                canEditScript && false == scriptAssetId.IsEmpty()
            };
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

        /// <summary>
        /// Collider2DComponent 操作 UI の表示状態を計算する。
        /// </summary>
        /// <param name="hasCollider2DComponent">Entity が Collider2DComponent を保持しているか。</param>
        /// <returns>UI 表示状態。</returns>
        [[nodiscard]] static InspectorCollider2DComponentActionState ComputeCollider2DComponentActionState(
            bool hasCollider2DComponent)
        {
            if (hasCollider2DComponent)
            {
                return InspectorCollider2DComponentActionState{
                    true,
                    L"Collider2DComponent",
                    L"Remove Collider2DComponent"
                };
            }

            return InspectorCollider2DComponentActionState{
                false,
                L"Collider2DComponent (not attached)",
                L"Add Collider2DComponent"
            };
        }

        /// <summary>
        /// Collider2DComponent 操作ボタン押下時の追加・削除処理を適用する。
        /// </summary>
        /// <param name="entity">更新対象 Entity。</param>
        /// <returns>Entity が更新された場合は true。</returns>
        [[nodiscard]] static bool ApplyCollider2DComponentAction(Game::Entity& entity)
        {
            if (entity.HasCollider2DComponent())
            {
                return SceneEditingOperations::RemoveCollider2DComponent(entity);
            }

            return SceneEditingOperations::AddCollider2DComponent(entity);
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
        /// ボタン押下追跡を現在フレームの入力状態へ進める。
        /// </summary>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        void FinishButtonClickTracking(const Core::InputSnapshot& inputSnapshot);

        /// <summary>
        /// 現在のカーソル位置が Material ドロップ対象内かを取得する。
        /// </summary>
        /// <param name="assetsPanelController">Assets パネルのドラッグ状態。</param>
        /// <returns>ドロップ対象内の場合は true。</returns>
        [[nodiscard]] bool IsMaterialDropTargetHovered(const AssetsPanelController& assetsPanelController) const;

        [[nodiscard]] bool IsMaterialTextureDropTargetHovered(const AssetsPanelController& assetsPanelController) const;

        void SetMaterialDetailsVisible(bool visible) const;

        void SetMaterialDetailsEnabled(bool enabled) const;

        void ApplyCardVisibility(bool hasSpriteComponent, bool hasMaterialDetails, bool hasCollider2DComponent) const;

        void UpdateSectionLabels(HierarchyPanelController::VisibleItemKind selectedItemKind);

        bool ToggleSectionCollapseFromInput(const Core::InputSnapshot& inputSnapshot);

        HWND m_inspectorSummaryLabel = nullptr;
        HWND m_transformSectionLabel = nullptr;
        std::array<HWND, 9> m_transformEditControls{};
        HWND m_spriteComponentSectionLabel = nullptr;
        HWND m_spriteRefLabel = nullptr;
        HWND m_spriteRefDropHighlight = nullptr;
        HWND m_spriteRefEdit = nullptr;
        HWND m_scriptAssetLabel = nullptr;
        HWND m_scriptAssetEdit = nullptr;
        HWND m_scriptCreateButton = nullptr;
        HWND m_scriptAssignButton = nullptr;
        HWND m_scriptClearButton = nullptr;
        HWND m_spriteComponentActionButton = nullptr;
        HWND m_collider2DComponentSectionLabel = nullptr;
        HWND m_collider2DSummaryLabel = nullptr;
        HWND m_collider2DEnabledCheckBox = nullptr;
        HWND m_collider2DTriggerCheckBox = nullptr;
        HWND m_collider2DShapeTypeLabel = nullptr;
        HWND m_collider2DShapeTypeEdit = nullptr;
        HWND m_collider2DOffsetLabel = nullptr;
        HWND m_collider2DSizeLabel = nullptr;
        HWND m_collider2DRotationLabel = nullptr;
        HWND m_collider2DRotationEdit = nullptr;
        std::array<HWND, 4> m_collider2DEditControls{};
        HWND m_collider2DEditButton = nullptr;
        HWND m_collider2DComponentActionButton = nullptr;
        HWND m_addComponentButton = nullptr;
        HWND m_materialOpenButton = nullptr;
        HWND m_materialSharedNoticeLabel = nullptr;
        HWND m_materialDetailsSectionLabel = nullptr;
        std::array<HWND, 5> m_materialDetailLabels{};
        std::array<HWND, 5> m_materialDetailEditControls{};
        HWND m_materialTextureDropHighlight = nullptr;
        HWND m_materialTextureBrowseButton = nullptr;
        HWND m_materialTintColorButton = nullptr;
        HWND m_materialOutlineEnabledCheckBox = nullptr;
        HWND m_materialOutlineColorButton = nullptr;
        Platform::ICursor* m_cursor = nullptr;
        std::optional<Game::EntityId> m_lastInspectorEntityId{};
        Core::AssetId m_lastSpriteRefAssetId{};
        std::wstring m_lastSpriteRefDisplayText{};
        Core::AssetId m_lastScriptAssetId{};
        std::wstring m_lastScriptDisplayText{};
        bool m_wasLeftMouseButtonDown = false;
        HWND m_pressedButtonHandle = nullptr;
        bool m_isDropHighlightVisible = false;
        HWND m_dropHighlightTargetEdit = nullptr;
        bool m_isTransformCollapsed = false;
        bool m_isSpriteComponentCollapsed = false;
        bool m_isMaterialCollapsed = false;
        bool m_isCollider2DCollapsed = false;
    };
}
