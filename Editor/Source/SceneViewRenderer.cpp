#include "SceneViewRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "Assets/ISpriteAssetResolver.h"
#include "ITextureAssetResolver.h"
#include "RenderBackendBootstrap.h"
#include "Sprite.h"
#include <Windows.h>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include "EditorCamera2D.h"
#include "SceneViewController.h"
#include <Assets/SpriteAssetRegistry.h>
#include <Entity.h>
#include <Scene.h>
#include <SolidQuadRenderer.h>
#include <SpriteRenderer.h>
#include <TextureAssetRegistry.h>
#include <GraphicsAPI.h>
#include <IGraphicsContext.h>

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr float SceneGridStepWorldUnits = 64.0f;
        constexpr float SceneGridLineThicknessPixels = 1.0f;
        constexpr float SceneAxisLineThicknessPixels = 2.0f;
        constexpr float SelectionOutlineThicknessPixels = 2.0f;
        constexpr float SelectionPivotSizePixels = 8.0f;
        constexpr float EditModeIndicatorLineThicknessPixels = 5.0f;
        constexpr float EditModeIndicatorHandleSizePixels = 10.0f;
        constexpr float EditModeIndicatorCenterSizePixels = 13.0f;
        constexpr float UntexturedSpriteSizePixels = 64.0f;

        /// <summary>
        /// 未テクスチャ Sprite のプレースホルダー描画対象かを判定する。
        /// </summary>
        /// <param name="spriteComponent">判定対象の SpriteComponent。</param>
        /// <returns>SpriteAsset と MaterialAsset のどちらも参照していない場合は true。</returns>
        bool ShouldDrawUntexturedSpritePlaceholder(const Game::SpriteComponent& spriteComponent)
        {
            return spriteComponent.spriteAssetRef.IsEmpty()
                && spriteComponent.materialAssetRef.IsEmpty();
        }

        /// <summary>
        /// SceneView オーバーレイ描画に使う色定義を返す。
        /// </summary>
        /// <returns>色定義一覧。</returns>
        std::array<float, 4> MakeColor(float red, float green, float blue, float alpha)
        {
            return std::array<float, 4>{ red, green, blue, alpha };
        }

        /// <summary>
        /// ワールド値を指定ステップへ切り下げる。
        /// </summary>
        /// <param name="value">丸め対象の値。</param>
        /// <param name="step">使用するステップ幅。</param>
        /// <returns>step 単位で切り下げた値。</returns>
        float FloorToStep(float value, float step)
        {
            if (step <= 0.0f)
            {
                return value;
            }

            return std::floor(value / step) * step;
        }

        /// <summary>
        /// SceneView 描画順に合わせて Sprite 一覧を並べ替える。
        /// </summary>
        /// <param name="resolvedSprites">描画候補 Sprite 一覧。</param>
        /// <param name="selectedEntityId">選択中 EntityId。</param>
        /// <returns>最終描画順に並んだ Sprite 一覧。</returns>
        std::vector<Game::ResolvedSceneSprite> OrderSceneRenderSpritesForSceneView(
            std::vector<Game::ResolvedSceneSprite> resolvedSprites,
            std::optional<Game::EntityId> selectedEntityId)
        {
            if (false == selectedEntityId.has_value())
            {
                return resolvedSprites;
            }

            std::vector<Game::ResolvedSceneSprite> orderedSprites;
            orderedSprites.reserve(resolvedSprites.size());
            std::optional<Game::ResolvedSceneSprite> selectedSprite{};

            for (Game::ResolvedSceneSprite& renderSprite : resolvedSprites)
            {
                if (renderSprite.entityId == *selectedEntityId)
                {
                    selectedSprite = std::move(renderSprite);
                    continue;
                }

                orderedSprites.push_back(std::move(renderSprite));
            }

            if (true == selectedSprite.has_value())
            {
                orderedSprites.push_back(std::move(*selectedSprite));
            }

            return orderedSprites;
        }

        /// <summary>
        /// 縁取り付きの単色矩形を描画する。
        /// </summary>
        /// <param name="renderer">描画に使用する SolidQuadRenderer。</param>
        /// <param name="centerX">描画中心の X 座標。</param>
        /// <param name="centerY">描画中心の Y 座標。</param>
        /// <param name="width">矩形幅。</param>
        /// <param name="height">矩形高さ。</param>
        /// <param name="fillColor">塗り色。</param>
        /// <param name="outlineColor">縁取り色。</param>
        void DrawOutlinedQuad(
            Graphics::SolidQuadRenderer& renderer,
            float centerX,
            float centerY,
            float width,
            float height,
            const std::array<float, 4>& fillColor,
            const std::array<float, 4>& outlineColor)
        {
            constexpr float OutlinePixels = 2.0f;
            renderer.Draw(Graphics::SolidQuad{
                centerX,
                centerY,
                width + OutlinePixels * 2.0f,
                height + OutlinePixels * 2.0f,
                outlineColor
            });
            renderer.Draw(Graphics::SolidQuad{
                centerX,
                centerY,
                width,
                height,
                fillColor
            });
        }

        /// <summary>
        /// 移動モード用の上下左右ハンドルを描画する。
        /// </summary>
        /// <param name="renderer">描画に使用する SolidQuadRenderer。</param>
        /// <param name="viewX">描画中心の View X 座標。</param>
        /// <param name="viewY">描画中心の View Y 座標。</param>
        void DrawMoveModeIndicator(Graphics::SolidQuadRenderer& renderer, float viewX, float viewY)
        {
            const std::array<float, 4> outlineColor = MakeColor(0.93f, 0.97f, 1.0f, 0.95f);
            const std::array<float, 4> leftColor = MakeColor(0.22f, 0.46f, 0.95f, 0.98f);
            const std::array<float, 4> upColor = MakeColor(0.52f, 0.78f, 0.36f, 0.98f);
            const std::array<float, 4> rightColor = MakeColor(0.93f, 0.68f, 0.18f, 0.98f);
            const std::array<float, 4> downColor = MakeColor(0.88f, 0.25f, 0.22f, 0.98f);
            constexpr float ShaftLengthPixels = 34.0f;
            constexpr float ShaftOffsetPixels = 25.0f;
            constexpr float HeadOffsetPixels = 48.0f;
            constexpr float HeadSizePixels = 14.0f;

            DrawOutlinedQuad(renderer, viewX - ShaftOffsetPixels, viewY, ShaftLengthPixels, EditModeIndicatorLineThicknessPixels, leftColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX - HeadOffsetPixels, viewY, HeadSizePixels, HeadSizePixels, leftColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX + ShaftOffsetPixels, viewY, ShaftLengthPixels, EditModeIndicatorLineThicknessPixels, rightColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX + HeadOffsetPixels, viewY, HeadSizePixels, HeadSizePixels, rightColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX, viewY - ShaftOffsetPixels, EditModeIndicatorLineThicknessPixels, ShaftLengthPixels, upColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX, viewY - HeadOffsetPixels, HeadSizePixels, HeadSizePixels, upColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX, viewY + ShaftOffsetPixels, EditModeIndicatorLineThicknessPixels, ShaftLengthPixels, downColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX, viewY + HeadOffsetPixels, HeadSizePixels, HeadSizePixels, downColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX, viewY, EditModeIndicatorCenterSizePixels, EditModeIndicatorCenterSizePixels, MakeColor(0.26f, 0.48f, 0.95f, 1.0f), outlineColor);
        }

        /// <summary>
        /// 拡大縮小モード用の四隅ハンドルを描画する。
        /// </summary>
        /// <param name="renderer">描画に使用する SolidQuadRenderer。</param>
        /// <param name="viewX">描画中心の View X 座標。</param>
        /// <param name="viewY">描画中心の View Y 座標。</param>
        void DrawScaleModeIndicator(Graphics::SolidQuadRenderer& renderer, float viewX, float viewY)
        {
            const std::array<float, 4> outlineColor = MakeColor(0.94f, 1.0f, 0.90f, 0.95f);
            const std::array<float, 4> lineColor = MakeColor(0.60f, 0.78f, 0.42f, 0.98f);
            const std::array<float, 4> handleColor = MakeColor(0.36f, 0.58f, 0.25f, 1.0f);
            constexpr float BoxSizePixels = 54.0f;
            constexpr float BoxHalfPixels = BoxSizePixels * 0.5f;
            constexpr float ArrowOffsetPixels = 48.0f;
            constexpr float ArrowLengthPixels = 24.0f;

            DrawOutlinedQuad(renderer, viewX, viewY - BoxHalfPixels, BoxSizePixels, EditModeIndicatorLineThicknessPixels, lineColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX, viewY + BoxHalfPixels, BoxSizePixels, EditModeIndicatorLineThicknessPixels, lineColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX - BoxHalfPixels, viewY, EditModeIndicatorLineThicknessPixels, BoxSizePixels, lineColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX + BoxHalfPixels, viewY, EditModeIndicatorLineThicknessPixels, BoxSizePixels, lineColor, outlineColor);

            DrawOutlinedQuad(renderer, viewX - BoxHalfPixels, viewY - BoxHalfPixels, EditModeIndicatorHandleSizePixels, EditModeIndicatorHandleSizePixels, handleColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX + BoxHalfPixels, viewY - BoxHalfPixels, EditModeIndicatorHandleSizePixels, EditModeIndicatorHandleSizePixels, handleColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX - BoxHalfPixels, viewY + BoxHalfPixels, EditModeIndicatorHandleSizePixels, EditModeIndicatorHandleSizePixels, handleColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX + BoxHalfPixels, viewY + BoxHalfPixels, EditModeIndicatorHandleSizePixels, EditModeIndicatorHandleSizePixels, handleColor, outlineColor);

            DrawOutlinedQuad(renderer, viewX - ArrowOffsetPixels, viewY - ArrowOffsetPixels, ArrowLengthPixels, EditModeIndicatorLineThicknessPixels, lineColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX - ArrowOffsetPixels, viewY - ArrowOffsetPixels, EditModeIndicatorLineThicknessPixels, ArrowLengthPixels, lineColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX + ArrowOffsetPixels, viewY - ArrowOffsetPixels, ArrowLengthPixels, EditModeIndicatorLineThicknessPixels, lineColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX + ArrowOffsetPixels, viewY - ArrowOffsetPixels, EditModeIndicatorLineThicknessPixels, ArrowLengthPixels, lineColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX - ArrowOffsetPixels, viewY + ArrowOffsetPixels, ArrowLengthPixels, EditModeIndicatorLineThicknessPixels, lineColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX - ArrowOffsetPixels, viewY + ArrowOffsetPixels, EditModeIndicatorLineThicknessPixels, ArrowLengthPixels, lineColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX + ArrowOffsetPixels, viewY + ArrowOffsetPixels, ArrowLengthPixels, EditModeIndicatorLineThicknessPixels, lineColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX + ArrowOffsetPixels, viewY + ArrowOffsetPixels, EditModeIndicatorLineThicknessPixels, ArrowLengthPixels, lineColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX, viewY, EditModeIndicatorCenterSizePixels, EditModeIndicatorCenterSizePixels, handleColor, outlineColor);
        }

        /// <summary>
        /// 回転モード用のリングハンドルを描画する。
        /// </summary>
        /// <param name="renderer">描画に使用する SolidQuadRenderer。</param>
        /// <param name="viewX">描画中心の View X 座標。</param>
        /// <param name="viewY">描画中心の View Y 座標。</param>
        void DrawRotateModeIndicator(Graphics::SolidQuadRenderer& renderer, float viewX, float viewY)
        {
            const std::array<float, 4> outlineColor = MakeColor(0.98f, 0.94f, 1.0f, 0.95f);
            const std::array<float, 4> ringColor = MakeColor(0.57f, 0.36f, 0.86f, 0.98f);
            constexpr float RadiusPixels = 44.0f;
            constexpr int SegmentCount = 24;
            constexpr float Pi = 3.1415926535f;

            for (int index = 0; index < SegmentCount - 3; ++index)
            {
                const float angle = (static_cast<float>(index) / static_cast<float>(SegmentCount)) * 2.0f * Pi;
                const float segmentX = viewX + std::cos(angle) * RadiusPixels;
                const float segmentY = viewY + std::sin(angle) * RadiusPixels;
                DrawOutlinedQuad(renderer, segmentX, segmentY, 8.0f, 8.0f, ringColor, outlineColor);
            }

            DrawOutlinedQuad(renderer, viewX + 34.0f, viewY - 28.0f, 16.0f, 16.0f, ringColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX + 43.0f, viewY - 15.0f, 10.0f, 20.0f, ringColor, outlineColor);
            DrawOutlinedQuad(renderer, viewX, viewY, EditModeIndicatorCenterSizePixels, EditModeIndicatorCenterSizePixels, ringColor, outlineColor);
        }

        /// <summary>
        /// 選択 Sprite の中心に編集モードを表すインジケーターを描画する。
        /// </summary>
        /// <param name="renderer">描画に使用する SolidQuadRenderer。</param>
        /// <param name="viewX">描画中心の View X 座標。</param>
        /// <param name="viewY">描画中心の View Y 座標。</param>
        /// <param name="editMode">現在の編集モード。</param>
        void DrawEditModeIndicator(
            Graphics::SolidQuadRenderer& renderer,
            float viewX,
            float viewY,
            SceneViewEditMode editMode)
        {
            if (SceneViewEditMode::Move == editMode)
            {
                DrawMoveModeIndicator(renderer, viewX, viewY);
                return;
            }

            if (SceneViewEditMode::Scale == editMode)
            {
                DrawScaleModeIndicator(renderer, viewX, viewY);
                return;
            }

            if (SceneViewEditMode::Rotate == editMode)
            {
                DrawRotateModeIndicator(renderer, viewX, viewY);
            }
        }
    }

    bool SceneViewRenderer::Initialize(
        HINSTANCE hInstance,
        HWND hostWindow,
        std::uint32_t width,
        std::uint32_t height,
        RHI::GraphicsAPI api)
    {
        m_graphics = BootstrapRenderBackend(api);
        if (nullptr == m_graphics)
        {
            return false;
        }

        if (nullptr == hostWindow || 0 == width || 0 == height)
        {
            return false;
        }

        if (false == m_graphics->Initialize(hostWindow, hInstance, width, height))
        {
            return false;
        }

        m_sceneViewWidth = width;
        m_sceneViewHeight = height;
        m_spriteRenderer = std::make_unique<Graphics::SpriteRenderer>(*m_graphics);
        m_solidQuadRenderer = std::make_unique<Graphics::SolidQuadRenderer>(*m_graphics);
        return true;
    }

    void SceneViewRenderer::Shutdown()
    {
        if (m_graphics)
        {
            m_graphics->Shutdown();
            m_graphics.reset();
        }

        m_spriteRenderer.reset();
        m_solidQuadRenderer.reset();
    }

    void SceneViewRenderer::Resize(std::uint32_t width, std::uint32_t height)
    {
        m_sceneViewWidth = width;
        m_sceneViewHeight = height;

        if (m_graphics && m_sceneViewWidth > 0 && m_sceneViewHeight > 0)
        {
            m_graphics->Resize(m_sceneViewWidth, m_sceneViewHeight);
        }
    }

    RHI::IGraphicsContext& SceneViewRenderer::GetGraphicsContext()
    {
        return *m_graphics;
    }

    void SceneViewRenderer::RenderFrame(
        Game::Scene* scene,
        Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
        const Game::Assets::IMaterialAssetResolver& materialAssetResolver,
        Graphics::TextureAssetRegistry& textureAssetRegistry,
        const EditorCamera2D& camera,
        std::optional<Game::EntityId> selectedEntityId,
        SceneViewEditMode editMode,
        const SceneDragPreviewState& dragPreviewState)
    {
        if (nullptr == m_graphics)
        {
            return;
        }

        m_graphics->BeginFrame();
        if (nullptr != m_spriteRenderer && nullptr != scene)
        {
            scene->ValidateSpriteReferences(spriteAssetRegistry);
            std::vector<Game::ResolvedSceneSprite> resolvedSprites =
                scene->ResolveSceneSprites(spriteAssetRegistry, materialAssetResolver, textureAssetRegistry);
            resolvedSprites = OrderSceneRenderSpritesForSceneView(std::move(resolvedSprites), selectedEntityId);

            m_spriteRenderer->Begin();

            if (nullptr != m_solidQuadRenderer && 0 != m_sceneViewWidth && 0 != m_sceneViewHeight)
            {
                const EditorWorldRect visibleWorldRect = camera.GetVisibleWorldRect();
                const float startX = FloorToStep(visibleWorldRect.left, SceneGridStepWorldUnits);
                for (float worldX = startX; worldX <= visibleWorldRect.right; worldX += SceneGridStepWorldUnits)
                {
                    const float viewX = camera.TransformWorldToViewX(worldX);
                    m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                        viewX,
                        0.0f,
                        SceneGridLineThicknessPixels,
                        static_cast<float>(m_sceneViewHeight),
                        MakeColor(0.25f, 0.27f, 0.31f, 0.45f)
                    });
                }

                const float startY = FloorToStep(visibleWorldRect.bottom, SceneGridStepWorldUnits);
                for (float worldY = startY; worldY <= visibleWorldRect.top; worldY += SceneGridStepWorldUnits)
                {
                    const float viewY = camera.TransformWorldToViewY(worldY);
                    m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                        0.0f,
                        viewY,
                        static_cast<float>(m_sceneViewWidth),
                        SceneGridLineThicknessPixels,
                        MakeColor(0.25f, 0.27f, 0.31f, 0.45f)
                    });
                }

                const float originViewX = camera.TransformWorldToViewX(0.0f);
                const float originViewY = camera.TransformWorldToViewY(0.0f);

                m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                    originViewX,
                    0.0f,
                    SceneAxisLineThicknessPixels,
                    static_cast<float>(m_sceneViewHeight),
                    MakeColor(0.91f, 0.35f, 0.31f, 0.8f)
                });
                m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                    0.0f,
                    originViewY,
                    static_cast<float>(m_sceneViewWidth),
                    SceneAxisLineThicknessPixels,
                    MakeColor(0.31f, 0.76f, 0.43f, 0.8f)
                });
            }

            for (Game::ResolvedSceneSprite& renderSprite : resolvedSprites)
            {
                Graphics::Sprite& sprite = renderSprite.sprite;
                const auto position = sprite.GetPosition();
                const auto scale = sprite.GetScale();

                sprite.SetPosition(
                    camera.TransformWorldToViewX(position.x),
                    camera.TransformWorldToViewY(position.y));
                sprite.SetScale(
                    camera.TransformWorldScale(scale.x),
                    camera.TransformWorldScale(scale.y));

                m_spriteRenderer->Draw(sprite);
            }

            if (nullptr != m_solidQuadRenderer && nullptr != scene)
            {
                const auto renderItems = scene->CollectSpriteRenderItems();
                for (const Game::SceneSpriteRenderItem& renderItem : renderItems)
                {
                    if (nullptr == renderItem.transform
                        || nullptr == renderItem.spriteComponent
                        || false == ShouldDrawUntexturedSpritePlaceholder(*renderItem.spriteComponent))
                    {
                        continue;
                    }

                    const float viewX = camera.TransformWorldToViewX(renderItem.transform->position.x);
                    const float viewY = camera.TransformWorldToViewY(renderItem.transform->position.y);
                    const float widthPixels =
                        camera.TransformWorldScale(UntexturedSpriteSizePixels * std::abs(renderItem.transform->scale.x));
                    const float heightPixels =
                        camera.TransformWorldScale(UntexturedSpriteSizePixels * std::abs(renderItem.transform->scale.y));
                    m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                        viewX,
                        viewY,
                        (std::max)(widthPixels, 1.0f),
                        (std::max)(heightPixels, 1.0f),
                        MakeColor(0.48f, 0.50f, 0.54f, 1.0f)
                    });

                    if (true == selectedEntityId.has_value() && renderItem.entityId == *selectedEntityId)
                    {
                        const float horizontalBorderY = (heightPixels * 0.5f) + (SelectionOutlineThicknessPixels * 0.5f);
                        const float verticalBorderX = (widthPixels * 0.5f) + (SelectionOutlineThicknessPixels * 0.5f);
                        const std::array<float, 4> outlineColor = MakeColor(0.98f, 0.86f, 0.18f, 0.95f);

                        m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                            viewX,
                            viewY - horizontalBorderY,
                            widthPixels + SelectionOutlineThicknessPixels * 2.0f,
                            SelectionOutlineThicknessPixels,
                            outlineColor
                        });
                        m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                            viewX,
                            viewY + horizontalBorderY,
                            widthPixels + SelectionOutlineThicknessPixels * 2.0f,
                            SelectionOutlineThicknessPixels,
                            outlineColor
                        });
                        m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                            viewX - verticalBorderX,
                            viewY,
                            SelectionOutlineThicknessPixels,
                            heightPixels + SelectionOutlineThicknessPixels * 2.0f,
                            outlineColor
                        });
                        m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                            viewX + verticalBorderX,
                            viewY,
                            SelectionOutlineThicknessPixels,
                            heightPixels + SelectionOutlineThicknessPixels * 2.0f,
                            outlineColor
                        });
                        m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                            viewX,
                            viewY,
                            SelectionPivotSizePixels,
                            SelectionPivotSizePixels,
                            MakeColor(1.0f, 0.94f, 0.45f, 1.0f)
                        });
                        DrawEditModeIndicator(*m_solidQuadRenderer, viewX, viewY, editMode);
                    }
                }
            }

            if (nullptr != m_solidQuadRenderer && true == selectedEntityId.has_value())
            {
                const auto selectedTarget = std::find_if(
                    resolvedSprites.begin(),
                    resolvedSprites.end(),
                    [selectedEntityId](const Game::ResolvedSceneSprite& renderSprite)
                    {
                        return true == selectedEntityId.has_value() && renderSprite.entityId == *selectedEntityId;
                    });
                if (selectedTarget != resolvedSprites.end())
                {
                    const auto texture = selectedTarget->sprite.GetTexture();
                    if (texture)
                    {
                        const auto position = selectedTarget->sprite.GetPosition();
                        const auto scale = selectedTarget->sprite.GetScale();
                        const float widthPixels = static_cast<float>(texture->GetWidth()) * std::abs(scale.x);
                        const float heightPixels = static_cast<float>(texture->GetHeight()) * std::abs(scale.y);
                        const float horizontalBorderY = (heightPixels * 0.5f) + (SelectionOutlineThicknessPixels * 0.5f);
                        const float verticalBorderX = (widthPixels * 0.5f) + (SelectionOutlineThicknessPixels * 0.5f);
                        const std::array<float, 4> outlineColor = MakeColor(0.98f, 0.86f, 0.18f, 0.95f);

                        m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                            position.x,
                            position.y - horizontalBorderY,
                            widthPixels + SelectionOutlineThicknessPixels * 2.0f,
                            SelectionOutlineThicknessPixels,
                            outlineColor
                        });
                        m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                            position.x,
                            position.y + horizontalBorderY,
                            widthPixels + SelectionOutlineThicknessPixels * 2.0f,
                            SelectionOutlineThicknessPixels,
                            outlineColor
                        });
                        m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                            position.x - verticalBorderX,
                            position.y,
                            SelectionOutlineThicknessPixels,
                            heightPixels + SelectionOutlineThicknessPixels * 2.0f,
                            outlineColor
                        });
                        m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                            position.x + verticalBorderX,
                            position.y,
                            SelectionOutlineThicknessPixels,
                            heightPixels + SelectionOutlineThicknessPixels * 2.0f,
                            outlineColor
                        });
                        m_solidQuadRenderer->Draw(Graphics::SolidQuad{
                            position.x,
                            position.y,
                            SelectionPivotSizePixels,
                            SelectionPivotSizePixels,
                            MakeColor(1.0f, 0.94f, 0.45f, 1.0f)
                        });
                        DrawEditModeIndicator(*m_solidQuadRenderer, position.x, position.y, editMode);
                    }
                }
            }

            if (nullptr != m_spriteRenderer
                && true == dragPreviewState.hasPreview
                && true == dragPreviewState.isCursorInside
                && dragPreviewState.texture)
            {
                Graphics::Sprite previewSprite{};
                previewSprite.SetTexture(dragPreviewState.texture);
                previewSprite.SetPosition(dragPreviewState.viewX, dragPreviewState.viewY);
                previewSprite.SetScale(
                    camera.TransformWorldScale(1.0f),
                    camera.TransformWorldScale(1.0f));
                previewSprite.SetRotationDegrees(0.0f);
                previewSprite.SetOutlineEnabled(true);
                previewSprite.SetOutlineThickness(1.0f);
                previewSprite.SetOutlineColor(1.0f, 0.84f, 0.04f, 1.0f);
                m_spriteRenderer->Draw(previewSprite);
            }

            m_spriteRenderer->End();
        }

        m_graphics->EndFrame();
    }
}
