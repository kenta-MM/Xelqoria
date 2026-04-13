#include <gtest/gtest.h>

#include "EditorCamera2D.h"
#include "SceneViewOverlay.h"

TEST(EditorCamera2DTests, StoresPanZoomAndViewportState)
{
    Xelqoria::Editor::EditorCamera2D camera;

    camera.SetCenter(120.0f, -48.0f);
    camera.SetZoom(2.5f);
    camera.SetViewport(1280, 720);

    const auto viewport = camera.GetViewport();
    EXPECT_FLOAT_EQ(camera.GetCenterX(), 120.0f);
    EXPECT_FLOAT_EQ(camera.GetCenterY(), -48.0f);
    EXPECT_FLOAT_EQ(camera.GetZoom(), 2.5f);
    EXPECT_EQ(viewport.width, 1280u);
    EXPECT_EQ(viewport.height, 720u);
}

TEST(EditorCamera2DTests, AppliesCameraStateToSceneViewRenderCoordinates)
{
    Xelqoria::Editor::EditorCamera2D camera;
    camera.SetCenter(100.0f, -20.0f);
    camera.SetZoom(1.5f);

    EXPECT_FLOAT_EQ(camera.TransformWorldToViewX(140.0f), 60.0f);
    EXPECT_FLOAT_EQ(camera.TransformWorldToViewY(20.0f), 60.0f);
    EXPECT_FLOAT_EQ(camera.TransformWorldScale(2.0f), 3.0f);
}

TEST(EditorCamera2DTests, ClampsZoomToPositiveMinimum)
{
    Xelqoria::Editor::EditorCamera2D camera;

    camera.SetZoom(0.0f);

    EXPECT_GT(camera.GetZoom(), 0.0f);
}

TEST(EditorCamera2DTests, ConvertsScreenPointToWorldPointWithPanAndZoom)
{
    Xelqoria::Editor::EditorCamera2D camera;
    camera.SetCenter(100.0f, -20.0f);
    camera.SetZoom(2.0f);
    camera.SetViewport(800, 600);

    const auto worldPoint = camera.TransformScreenToWorld(Xelqoria::Editor::EditorScreenPoint{
        500.0f,
        260.0f
    });

    EXPECT_FLOAT_EQ(worldPoint.x, 150.0f);
    EXPECT_FLOAT_EQ(worldPoint.y, -40.0f);
}

TEST(EditorCamera2DTests, ConvertsViewportCenterToCameraCenter)
{
    Xelqoria::Editor::EditorCamera2D camera;
    camera.SetCenter(-32.0f, 48.0f);
    camera.SetZoom(3.0f);
    camera.SetViewport(1280, 720);

    const auto worldPoint = camera.TransformScreenToWorld(Xelqoria::Editor::EditorScreenPoint{
        640.0f,
        360.0f
    });

    EXPECT_FLOAT_EQ(worldPoint.x, -32.0f);
    EXPECT_FLOAT_EQ(worldPoint.y, 48.0f);
}

TEST(EditorCamera2DTests, ReportsVisibleWorldRectFromViewportAndZoom)
{
    Xelqoria::Editor::EditorCamera2D camera;
    camera.SetCenter(10.0f, -6.0f);
    camera.SetZoom(2.0f);
    camera.SetViewport(800, 600);

    const auto visibleRect = camera.GetVisibleWorldRect();

    EXPECT_FLOAT_EQ(visibleRect.left, -190.0f);
    EXPECT_FLOAT_EQ(visibleRect.right, 210.0f);
    EXPECT_FLOAT_EQ(visibleRect.top, 144.0f);
    EXPECT_FLOAT_EQ(visibleRect.bottom, -156.0f);
}

TEST(EditorCamera2DTests, PicksLastDrawnHitTargetWhenBoundsOverlap)
{
    const std::array<Xelqoria::Editor::SceneViewHitTarget, 3> targets{
        Xelqoria::Editor::SceneViewHitTarget{ 1, 0.0f, 0.0f, 32.0f, 32.0f },
        Xelqoria::Editor::SceneViewHitTarget{ 2, 0.0f, 0.0f, 24.0f, 24.0f },
        Xelqoria::Editor::SceneViewHitTarget{ 3, 96.0f, 96.0f, 16.0f, 16.0f }
    };

    const auto selectedEntityId = Xelqoria::Editor::PickTopmostEntityAtWorldPoint(targets, 2.0f, -3.0f);

    ASSERT_TRUE(selectedEntityId.has_value());
    EXPECT_EQ(*selectedEntityId, static_cast<Xelqoria::Game::EntityId>(2));
}

TEST(EditorCamera2DTests, IgnoresEarlierTargetsWhenLaterTargetCoversThePoint)
{
    const std::array<Xelqoria::Editor::SceneViewHitTarget, 3> targets{
        Xelqoria::Editor::SceneViewHitTarget{ 5, 0.0f, 0.0f, 64.0f, 64.0f },
        Xelqoria::Editor::SceneViewHitTarget{ 9, 128.0f, 128.0f, 32.0f, 32.0f },
        Xelqoria::Editor::SceneViewHitTarget{ 7, 0.0f, 0.0f, 16.0f, 16.0f }
    };

    const auto selectedEntityId = Xelqoria::Editor::PickTopmostEntityAtWorldPoint(targets, 1.0f, 1.0f);

    ASSERT_TRUE(selectedEntityId.has_value());
    EXPECT_EQ(*selectedEntityId, static_cast<Xelqoria::Game::EntityId>(7));
}

TEST(EditorCamera2DTests, ReturnsNoHitWhenPointIsOutsideTargets)
{
    const std::array<Xelqoria::Editor::SceneViewHitTarget, 1> targets{
        Xelqoria::Editor::SceneViewHitTarget{ 7, 16.0f, 32.0f, 8.0f, 8.0f }
    };

    const auto selectedEntityId = Xelqoria::Editor::PickTopmostEntityAtWorldPoint(targets, 40.0f, 80.0f);

    EXPECT_FALSE(selectedEntityId.has_value());
}
