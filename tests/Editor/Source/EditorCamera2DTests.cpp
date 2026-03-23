#include <gtest/gtest.h>

#include "EditorCamera2D.h"

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
    EXPECT_FLOAT_EQ(worldPoint.y, 0.0f);
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
