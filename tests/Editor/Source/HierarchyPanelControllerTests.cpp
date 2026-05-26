#include <gtest/gtest.h>

#include <cstdint>

#include "Panels/HierarchyPanelController.h"

namespace
{
    [[nodiscard]] Xelqoria::Editor::ButtonClickTarget CreateButtonClickTarget(std::uintptr_t id, int left)
    {
        return Xelqoria::Editor::ButtonClickTarget{
            id,
            true,
            true,
            Xelqoria::Editor::ButtonClickRect{ left, 100, left + 80, 132 }
        };
    }
}

TEST(HierarchyPanelControllerTests, PressingAndReleasingInsideSameButtonProducesClick)
{
    const Xelqoria::Editor::ButtonClickTarget buttonTarget = CreateButtonClickTarget(1, 100);

    Xelqoria::Editor::ButtonClickInputState inputState{};

    Xelqoria::Editor::ButtonClickFrameInput pressFrame{
        true,
        Xelqoria::Platform::Point{ 110, 110 }
    };
    EXPECT_FALSE(Xelqoria::Editor::TryConsumeButtonClick(buttonTarget, pressFrame, inputState));
    inputState.wasLeftMouseButtonDown = pressFrame.isLeftMouseButtonDown;

    Xelqoria::Editor::ButtonClickFrameInput releaseFrame{
        false,
        Xelqoria::Platform::Point{ 110, 110 }
    };
    EXPECT_TRUE(Xelqoria::Editor::TryConsumeButtonClick(buttonTarget, releaseFrame, inputState));
}

TEST(HierarchyPanelControllerTests, ReleasingOutsideButtonDoesNotProduceClick)
{
    const Xelqoria::Editor::ButtonClickTarget buttonTarget = CreateButtonClickTarget(1, 100);

    Xelqoria::Editor::ButtonClickInputState inputState{};

    Xelqoria::Editor::ButtonClickFrameInput pressFrame{
        true,
        Xelqoria::Platform::Point{ 110, 110 }
    };
    EXPECT_FALSE(Xelqoria::Editor::TryConsumeButtonClick(buttonTarget, pressFrame, inputState));
    inputState.wasLeftMouseButtonDown = pressFrame.isLeftMouseButtonDown;

    Xelqoria::Editor::ButtonClickFrameInput releaseFrame{
        false,
        Xelqoria::Platform::Point{ 10, 10 }
    };
    EXPECT_FALSE(Xelqoria::Editor::TryConsumeButtonClick(buttonTarget, releaseFrame, inputState));
}

TEST(HierarchyPanelControllerTests, AnotherButtonCanStillConsumeReleaseAfterCreateButtonWasCheckedFirst)
{
    const Xelqoria::Editor::ButtonClickTarget createButtonTarget = CreateButtonClickTarget(1, 100);
    const Xelqoria::Editor::ButtonClickTarget duplicateButtonTarget = CreateButtonClickTarget(2, 220);

    Xelqoria::Editor::ButtonClickInputState inputState{};

    Xelqoria::Editor::ButtonClickFrameInput pressFrame{
        true,
        Xelqoria::Platform::Point{ 230, 110 }
    };
    EXPECT_FALSE(Xelqoria::Editor::TryConsumeButtonClick(createButtonTarget, pressFrame, inputState));
    EXPECT_FALSE(Xelqoria::Editor::TryConsumeButtonClick(duplicateButtonTarget, pressFrame, inputState));
    inputState.wasLeftMouseButtonDown = pressFrame.isLeftMouseButtonDown;

    Xelqoria::Editor::ButtonClickFrameInput releaseFrame{
        false,
        Xelqoria::Platform::Point{ 230, 110 }
    };
    EXPECT_FALSE(Xelqoria::Editor::TryConsumeButtonClick(createButtonTarget, releaseFrame, inputState));
    EXPECT_TRUE(Xelqoria::Editor::TryConsumeButtonClick(duplicateButtonTarget, releaseFrame, inputState));
}

TEST(HierarchyPanelControllerTests, FormatEntityRowLabelShowsCollider2DExpansionState)
{
    EXPECT_EQ(
        L"- Player",
        Xelqoria::Editor::HierarchyPanelController::FormatEntityRowLabel(L"Player", true, true));
    EXPECT_EQ(
        L"+ Player",
        Xelqoria::Editor::HierarchyPanelController::FormatEntityRowLabel(L"Player", true, false));
    EXPECT_EQ(
        L"  Player",
        Xelqoria::Editor::HierarchyPanelController::FormatEntityRowLabel(L"Player", false, false));
}

TEST(HierarchyPanelControllerTests, Collider2DChildRowVisibilityFollowsExpansionState)
{
    EXPECT_TRUE(Xelqoria::Editor::HierarchyPanelController::ShouldShowCollider2DChildRow(true, true));
    EXPECT_FALSE(Xelqoria::Editor::HierarchyPanelController::ShouldShowCollider2DChildRow(true, false));
    EXPECT_FALSE(Xelqoria::Editor::HierarchyPanelController::ShouldShowCollider2DChildRow(false, true));
}
