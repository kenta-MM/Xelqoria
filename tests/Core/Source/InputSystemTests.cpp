#include <gtest/gtest.h>

#include "InputSystem.h"

TEST(InputSystemTests, KeyPressedIsTrueOnlyOnTransitionToDown)
{
    bool isAKeyDown = false;
    Xelqoria::Core::InputSystem inputSystem(
        [&isAKeyDown](int virtualKeyCode)
        {
            return virtualKeyCode == 'A' && isAKeyDown;
        },
        []
        {
            return POINT{ 10, 20 };
        });

    inputSystem.Update();
    EXPECT_FALSE(inputSystem.GetSnapshot().IsKeyDown('A'));
    EXPECT_FALSE(inputSystem.GetSnapshot().WasKeyPressed('A'));

    isAKeyDown = true;
    inputSystem.Update();
    EXPECT_TRUE(inputSystem.GetSnapshot().IsKeyDown('A'));
    EXPECT_TRUE(inputSystem.GetSnapshot().WasKeyPressed('A'));
    EXPECT_FALSE(inputSystem.GetSnapshot().WasKeyReleased('A'));

    inputSystem.Update();
    EXPECT_TRUE(inputSystem.GetSnapshot().IsKeyDown('A'));
    EXPECT_FALSE(inputSystem.GetSnapshot().WasKeyPressed('A'));
}

TEST(InputSystemTests, KeyReleasedIsTrueOnlyOnTransitionToUp)
{
    bool isAKeyDown = true;
    Xelqoria::Core::InputSystem inputSystem(
        [&isAKeyDown](int virtualKeyCode)
        {
            return virtualKeyCode == 'A' && isAKeyDown;
        },
        []
        {
            return POINT{};
        });

    inputSystem.Update();
    EXPECT_TRUE(inputSystem.GetSnapshot().IsKeyDown('A'));
    EXPECT_TRUE(inputSystem.GetSnapshot().WasKeyPressed('A'));

    isAKeyDown = false;
    inputSystem.Update();
    EXPECT_FALSE(inputSystem.GetSnapshot().IsKeyDown('A'));
    EXPECT_FALSE(inputSystem.GetSnapshot().WasKeyPressed('A'));
    EXPECT_TRUE(inputSystem.GetSnapshot().WasKeyReleased('A'));

    inputSystem.Update();
    EXPECT_FALSE(inputSystem.GetSnapshot().WasKeyReleased('A'));
}

TEST(InputSystemTests, MouseLeftButtonTracksTransitionsAndCursorPosition)
{
    bool isLeftButtonDown = false;
    POINT cursorPoint{ 30, 40 };
    Xelqoria::Core::InputSystem inputSystem(
        [&isLeftButtonDown](int virtualKeyCode)
        {
            return virtualKeyCode == VK_LBUTTON && isLeftButtonDown;
        },
        [&cursorPoint]
        {
            return cursorPoint;
        });

    inputSystem.Update();
    EXPECT_FALSE(inputSystem.GetSnapshot().IsMouseButtonDown(Xelqoria::Core::MouseButton::Left));
    EXPECT_EQ(30, inputSystem.GetSnapshot().GetCursorScreenPoint().x);
    EXPECT_EQ(40, inputSystem.GetSnapshot().GetCursorScreenPoint().y);

    isLeftButtonDown = true;
    cursorPoint = POINT{ 50, 60 };
    inputSystem.Update();
    EXPECT_TRUE(inputSystem.GetSnapshot().IsMouseButtonDown(Xelqoria::Core::MouseButton::Left));
    EXPECT_TRUE(inputSystem.GetSnapshot().WasMouseButtonPressed(Xelqoria::Core::MouseButton::Left));
    EXPECT_EQ(50, inputSystem.GetSnapshot().GetCursorScreenPoint().x);
    EXPECT_EQ(60, inputSystem.GetSnapshot().GetCursorScreenPoint().y);

    isLeftButtonDown = false;
    inputSystem.Update();
    EXPECT_FALSE(inputSystem.GetSnapshot().IsMouseButtonDown(Xelqoria::Core::MouseButton::Left));
    EXPECT_TRUE(inputSystem.GetSnapshot().WasMouseButtonReleased(Xelqoria::Core::MouseButton::Left));
}

TEST(InputSystemTests, MouseWheelDeltaIsCapturedPerUpdate)
{
    int mouseWheelDelta = 0;
    Xelqoria::Core::InputSystem inputSystem(
        [](int)
        {
            return false;
        },
        []
        {
            return POINT{};
        },
        [&mouseWheelDelta]
        {
            const int currentDelta = mouseWheelDelta;
            mouseWheelDelta = 0;
            return currentDelta;
        });

    mouseWheelDelta = 120;
    inputSystem.Update();
    EXPECT_EQ(120, inputSystem.GetSnapshot().GetMouseWheelDelta());

    inputSystem.Update();
    EXPECT_EQ(0, inputSystem.GetSnapshot().GetMouseWheelDelta());
}
