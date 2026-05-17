#pragma once

#include <memory>
#include <optional>

#include "AssetId.h"
#include "Entity.h"
#include "Texture2D.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// SceneView で選択 Sprite に適用する編集モードを表す。
    /// </summary>
    enum class SceneViewEditMode
    {
        None,
        Move,
        Scale,
        Rotate
    };

    /// <summary>
    /// SceneView ドラッグプレビューの現在状態を表す。
    /// </summary>
    struct SceneDragPreviewState
    {
        /// <summary>
        /// プレビュー対象の SpriteAssetId を表す。
        /// </summary>
        Core::AssetId spriteAssetId{};

        /// <summary>
        /// プレビュー描画に使用するテクスチャを表す。
        /// </summary>
        std::shared_ptr<Graphics::Texture2D> texture{};

        /// <summary>
        /// プレビュー中心のワールド座標 X を表す。
        /// </summary>
        float worldX = 0.0f;

        /// <summary>
        /// プレビュー中心のワールド座標 Y を表す。
        /// </summary>
        float worldY = 0.0f;

        /// <summary>
        /// プレビュー描画位置の View X 座標を表す。
        /// </summary>
        float viewX = 0.0f;

        /// <summary>
        /// プレビュー描画位置の View Y 座標を表す。
        /// </summary>
        float viewY = 0.0f;

        /// <summary>
        /// プレビュー表示対象があるかを表す。
        /// </summary>
        bool hasPreview = false;

        /// <summary>
        /// カーソルが SceneView 内にあるかを表す。
        /// </summary>
        bool isCursorInside = false;
    };

    /// <summary>
    /// SceneView へ引き渡す保留中ドロップ入力を表す。
    /// </summary>
    struct ScenePendingDropState
    {
        /// <summary>
        /// ドロップされた SpriteAssetId を表す。
        /// </summary>
        Core::AssetId spriteAssetId{};

        /// <summary>
        /// ドロップ先のワールド座標 X を表す。
        /// </summary>
        float worldX = 0.0f;

        /// <summary>
        /// ドロップ先のワールド座標 Y を表す。
        /// </summary>
        float worldY = 0.0f;

        /// <summary>
        /// 保留中ドロップがあるかを表す。
        /// </summary>
        bool hasPendingDrop = false;
    };

    /// <summary>
    /// SceneView 入力更新による選択変更結果を表す。
    /// </summary>
    struct SceneViewInteractionResult
    {
        /// <summary>
        /// Scene に変更が加わったかを表す。
        /// </summary>
        bool sceneChanged = false;

        /// <summary>
        /// 変更内容を保存すべきタイミングかを表す。
        /// </summary>
        bool shouldPersistScene = false;

        /// <summary>
        /// 保存成功時にコマンド履歴へ積むべきかを表す。
        /// </summary>
        bool shouldPushHistory = false;

        /// <summary>
        /// 選択状態が変化したかを表す。
        /// </summary>
        bool selectionChanged = false;

        /// <summary>
        /// 更新後の選択 EntityId を表す。
        /// </summary>
        std::optional<Game::EntityId> selectedEntityId{};
    };

    /// <summary>
    /// SceneView ドロップ適用結果の状態種別を表す。
    /// </summary>
    enum class SceneDropPlacementStatus
    {
        None,
        EmptyAssetId,
        MissingSpriteAsset,
        MissingTexture,
        ReloadFailed,
        SaveFailed,
        Success
    };

    /// <summary>
    /// SceneView ドロップ処理結果を表す。
    /// </summary>
    struct SceneViewDropResult
    {
        /// <summary>
        /// Scene が変更されたかを表す。
        /// </summary>
        bool sceneChanged = false;

        /// <summary>
        /// コマンド履歴へスナップショット追加すべきかを表す。
        /// </summary>
        bool shouldPushHistory = false;

        /// <summary>
        /// 選択状態が変化したかを表す。
        /// </summary>
        bool selectionChanged = false;

        /// <summary>
        /// 更新後の選択 EntityId を表す。
        /// </summary>
        std::optional<Game::EntityId> selectedEntityId{};

        /// <summary>
        /// ドロップ適用の最終状態を表す。
        /// </summary>
        SceneDropPlacementStatus status = SceneDropPlacementStatus::None;
    };
}
