#include "SceneView/SceneViewStatusPresenter.h"

#include <iterator>

#include "Utils/EditorStringUtils.h"
#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include "SceneView/SceneViewInteractionTypes.h"
#include <Entity.h>
#include <Scene.h>

namespace Xelqoria::Editor
{
    namespace
    {
        /// <summary>
        /// SceneView サイズラベルへ標準文面を書き込む。
        /// </summary>
        void SetWaitingStatus(HWND sceneViewSizeLabel, std::uint32_t sceneViewWidth, std::uint32_t sceneViewHeight)
        {
            wchar_t statusText[160]{};
            std::swprintf(
                statusText,
                std::size(statusText),
                L"SceneView size: %u x %u / click: waiting",
                sceneViewWidth,
                sceneViewHeight);
            SetWindowTextW(sceneViewSizeLabel, statusText);
        }
    }

    void SceneViewStatusPresenter::PresentInteractionStatus(
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
        float lastSceneClickY) const
    {
        if (nullptr == sceneViewSizeLabel || nullptr == sceneViewPlanLabel)
        {
            return;
        }

        wchar_t statusText[160]{};
        if (pendingDropState.hasPendingDrop && false == pendingDropState.spriteAssetId.IsEmpty())
        {
            const std::wstring assetId = ToWideString(pendingDropState.spriteAssetId.GetValue());
            std::swprintf(
                statusText,
                std::size(statusText),
                L"SceneView size: %u x %u / drop: %ls @ (%.1f, %.1f)",
                sceneViewWidth,
                sceneViewHeight,
                assetId.c_str(),
                pendingDropState.worldX,
                pendingDropState.worldY);
        }
        else if (pendingDropState.hasPendingDrop && false == pendingDropState.scriptAssetId.IsEmpty())
        {
            const std::wstring assetId = ToWideString(pendingDropState.scriptAssetId.GetValue());
            std::swprintf(
                statusText,
                std::size(statusText),
                L"SceneView size: %u x %u / script drop: %ls @ (%.1f, %.1f)",
                sceneViewWidth,
                sceneViewHeight,
                assetId.c_str(),
                pendingDropState.worldX,
                pendingDropState.worldY);
        }
        else if (dragPreviewState.hasPreview
            && dragPreviewState.isCursorInside
            && false == dragPreviewState.spriteAssetId.IsEmpty())
        {
            const std::wstring previewAssetId = ToWideString(dragPreviewState.spriteAssetId.GetValue());
            std::swprintf(
                statusText,
                std::size(statusText),
                L"SceneView size: %u x %u / drag preview: %ls @ (%.1f, %.1f)",
                sceneViewWidth,
                sceneViewHeight,
                previewAssetId.c_str(),
                dragPreviewState.worldX,
                dragPreviewState.worldY);
        }
        else if (selectedEntityId.has_value() && nullptr != scene)
        {
            const auto entity = scene->FindEntity(*selectedEntityId);
            if (entity.has_value())
            {
                std::swprintf(
                    statusText,
                    std::size(statusText),
                    L"SceneView size: %u x %u / selected: Entity %u @ (%.1f, %.1f)",
                    sceneViewWidth,
                    sceneViewHeight,
                    static_cast<unsigned>(*selectedEntityId),
                    entity->get().GetTransform().position.x,
                    entity->get().GetTransform().position.y);
            }
            else
            {
                SetWaitingStatus(sceneViewSizeLabel, sceneViewWidth, sceneViewHeight);
                return;
            }
        }
        else if (hasSceneClick)
        {
            std::swprintf(
                statusText,
                std::size(statusText),
                L"SceneView size: %u x %u / click: (%.1f, %.1f)",
                sceneViewWidth,
                sceneViewHeight,
                lastSceneClickX,
                lastSceneClickY);
        }
        else
        {
            SetWaitingStatus(sceneViewSizeLabel, sceneViewWidth, sceneViewHeight);
            SetWindowTextW(
                sceneViewPlanLabel,
                L"SceneView にはグリッド、原点、選択フィードバックを重ねて表示しています。");
            return;
        }

        SetWindowTextW(sceneViewSizeLabel, statusText);
        SetWindowTextW(
            sceneViewPlanLabel,
            pendingDropState.hasPendingDrop
                ? L"SceneView はドロップを受理済みです。次段で配置または割り当てへ入力を引き渡します。"
                : (dragPreviewState.hasPreview && dragPreviewState.isCursorInside)
                    ? L"SceneView 上でドラッグ配置プレビューを表示中です。ドロップ位置と表示サイズを確認できます。"
                    : L"SceneView にはグリッド、原点、選択フィードバックを重ねて表示しています。");
    }

    void SceneViewStatusPresenter::PresentSelectionStatus(
        HWND sceneViewSizeLabel,
        std::uint32_t sceneViewWidth,
        std::uint32_t sceneViewHeight,
        const Game::Scene* scene,
        std::optional<Game::EntityId> selectedEntityId) const
    {
        if (nullptr == sceneViewSizeLabel)
        {
            return;
        }

        if (false == selectedEntityId.has_value() || nullptr == scene)
        {
            return;
        }

        const auto entity = scene->FindEntity(*selectedEntityId);
        if (false == entity.has_value())
        {
            return;
        }

        wchar_t statusText[160]{};
        std::swprintf(
            statusText,
            std::size(statusText),
            L"SceneView size: %u x %u / selected: Entity %u @ (%.1f, %.1f)",
            sceneViewWidth,
            sceneViewHeight,
            static_cast<unsigned>(*selectedEntityId),
            entity->get().GetTransform().position.x,
            entity->get().GetTransform().position.y);
        SetWindowTextW(sceneViewSizeLabel, statusText);
    }

    void SceneViewStatusPresenter::PresentDropStatus(
        HWND sceneViewSizeLabel,
        HWND sceneViewPlanLabel,
        std::uint32_t sceneViewWidth,
        std::uint32_t sceneViewHeight,
        const SceneViewDropResult& dropResult) const
    {
        if (nullptr == sceneViewPlanLabel)
        {
            return;
        }

        switch (dropResult.status)
        {
        case SceneDropPlacementStatus::EmptyAssetId:
            SetWindowTextW(sceneViewPlanLabel, L"SceneView のドロップ入力に AssetId が含まれていないため配置を中止しました。");
            return;
        case SceneDropPlacementStatus::MissingSpriteAsset:
            SetWindowTextW(sceneViewPlanLabel, L"ドロップされた Sprite AssetId を解決できないため配置を中止しました。");
            return;
        case SceneDropPlacementStatus::MissingTexture:
            SetWindowTextW(sceneViewPlanLabel, L"ドロップされた Sprite の Texture を解決できないため配置を中止しました。");
            return;
        case SceneDropPlacementStatus::MissingTargetEntity:
            SetWindowTextW(sceneViewPlanLabel, L"Script Asset のドロップ先に Sprite Entity が見つからないため割り当てを中止しました。");
            return;
        case SceneDropPlacementStatus::MissingSpriteComponent:
            SetWindowTextW(sceneViewPlanLabel, L"ドロップ先 Entity が Sprite Asset を参照していないため Script 割り当てを中止しました。");
            return;
        case SceneDropPlacementStatus::ScriptAssignFailed:
            SetWindowTextW(sceneViewPlanLabel, L"Sprite Asset への Script Asset 割り当てに失敗しました。");
            return;
        case SceneDropPlacementStatus::ReloadFailed:
            SetWindowTextW(sceneViewPlanLabel, L"Entity は生成されましたが、保存/再読込の確認に失敗しました。");
            return;
        case SceneDropPlacementStatus::SaveFailed:
            SetWindowTextW(sceneViewPlanLabel, L"Scene は再読込できましたが、保存ファイルへの書き出しに失敗しました。");
            return;
        case SceneDropPlacementStatus::Success:
            if (nullptr != sceneViewSizeLabel && dropResult.selectedEntityId.has_value())
            {
                wchar_t statusText[160]{};
                std::swprintf(
                    statusText,
                    std::size(statusText),
                    dropResult.assetChanged
                        ? L"SceneView size: %u x %u / assigned Script to Entity %u"
                        : L"SceneView size: %u x %u / reloaded Entity %u",
                    sceneViewWidth,
                    sceneViewHeight,
                    static_cast<unsigned>(*dropResult.selectedEntityId));
                SetWindowTextW(sceneViewSizeLabel, statusText);
            }

            SetWindowTextW(
                sceneViewPlanLabel,
                dropResult.assetChanged
                    ? L"SceneView ドロップで Sprite Asset に Script Asset を割り当てました。"
                    : L"SceneView ドロップで生成した Entity を保存テキストへ反映し、再読込後も選択を維持しました。");
            return;
        case SceneDropPlacementStatus::None:
        default:
            return;
        }
    }
}
