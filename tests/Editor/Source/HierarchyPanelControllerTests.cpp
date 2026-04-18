#include <gtest/gtest.h>

#include "HierarchyPanelController.h"

namespace
{
    class ScopedTestWindowClass
    {
    public:
        explicit ScopedTestWindowClass(const wchar_t* className)
            : m_className(className)
        {
            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = DefWindowProcW;
            windowClass.hInstance = GetModuleHandleW(nullptr);
            windowClass.lpszClassName = m_className;
            m_registered = RegisterClassW(&windowClass) != 0;
        }

        ~ScopedTestWindowClass()
        {
            if (m_registered)
            {
                UnregisterClassW(m_className, GetModuleHandleW(nullptr));
            }
        }

        [[nodiscard]] bool IsRegistered() const
        {
            return m_registered;
        }

    private:
        const wchar_t* m_className = nullptr;
        bool m_registered = false;
    };

    HWND CreateTestButtonWindow(const wchar_t* className, int left)
    {
        return CreateWindowExW(
            0,
            className,
            L"Button",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            left,
            100,
            80,
            32,
            nullptr,
            nullptr,
            GetModuleHandleW(nullptr),
            nullptr);
    }
}

TEST(HierarchyPanelControllerTests, PressingAndReleasingInsideSameButtonProducesClick)
{
    constexpr const wchar_t* kWindowClassName = L"XelqoriaHierarchyButtonTestWindow";
    ScopedTestWindowClass windowClass(kWindowClassName);
    ASSERT_TRUE(windowClass.IsRegistered());

    HWND buttonHandle = CreateTestButtonWindow(kWindowClassName, 100);
    ASSERT_NE(nullptr, buttonHandle);

    Xelqoria::Editor::HierarchyButtonInputState inputState{};

    Xelqoria::Editor::HierarchyButtonFrameInput pressFrame{
        true,
        POINT{ 110, 110 }
    };
    EXPECT_FALSE(Xelqoria::Editor::TryConsumeHierarchyButtonClick(buttonHandle, pressFrame, inputState));
    inputState.wasLeftMouseButtonDown = pressFrame.isLeftMouseButtonDown;

    Xelqoria::Editor::HierarchyButtonFrameInput releaseFrame{
        false,
        POINT{ 110, 110 }
    };
    EXPECT_TRUE(Xelqoria::Editor::TryConsumeHierarchyButtonClick(buttonHandle, releaseFrame, inputState));

    DestroyWindow(buttonHandle);
}

TEST(HierarchyPanelControllerTests, ReleasingOutsideButtonDoesNotProduceClick)
{
    constexpr const wchar_t* kWindowClassName = L"XelqoriaHierarchyButtonTestWindowOutside";
    ScopedTestWindowClass windowClass(kWindowClassName);
    ASSERT_TRUE(windowClass.IsRegistered());

    HWND buttonHandle = CreateTestButtonWindow(kWindowClassName, 100);
    ASSERT_NE(nullptr, buttonHandle);

    Xelqoria::Editor::HierarchyButtonInputState inputState{};

    Xelqoria::Editor::HierarchyButtonFrameInput pressFrame{
        true,
        POINT{ 110, 110 }
    };
    EXPECT_FALSE(Xelqoria::Editor::TryConsumeHierarchyButtonClick(buttonHandle, pressFrame, inputState));
    inputState.wasLeftMouseButtonDown = pressFrame.isLeftMouseButtonDown;

    Xelqoria::Editor::HierarchyButtonFrameInput releaseFrame{
        false,
        POINT{ 10, 10 }
    };
    EXPECT_FALSE(Xelqoria::Editor::TryConsumeHierarchyButtonClick(buttonHandle, releaseFrame, inputState));

    DestroyWindow(buttonHandle);
}

TEST(HierarchyPanelControllerTests, AnotherButtonCanStillConsumeReleaseAfterCreateButtonWasCheckedFirst)
{
    constexpr const wchar_t* kWindowClassName = L"XelqoriaHierarchyButtonTestWindowSequential";
    ScopedTestWindowClass windowClass(kWindowClassName);
    ASSERT_TRUE(windowClass.IsRegistered());

    HWND createButtonHandle = CreateTestButtonWindow(kWindowClassName, 100);
    HWND duplicateButtonHandle = CreateTestButtonWindow(kWindowClassName, 220);
    ASSERT_NE(nullptr, createButtonHandle);
    ASSERT_NE(nullptr, duplicateButtonHandle);

    Xelqoria::Editor::HierarchyButtonInputState inputState{};

    Xelqoria::Editor::HierarchyButtonFrameInput pressFrame{
        true,
        POINT{ 230, 110 }
    };
    EXPECT_FALSE(Xelqoria::Editor::TryConsumeHierarchyButtonClick(createButtonHandle, pressFrame, inputState));
    EXPECT_FALSE(Xelqoria::Editor::TryConsumeHierarchyButtonClick(duplicateButtonHandle, pressFrame, inputState));
    inputState.wasLeftMouseButtonDown = pressFrame.isLeftMouseButtonDown;

    Xelqoria::Editor::HierarchyButtonFrameInput releaseFrame{
        false,
        POINT{ 230, 110 }
    };
    EXPECT_FALSE(Xelqoria::Editor::TryConsumeHierarchyButtonClick(createButtonHandle, releaseFrame, inputState));
    EXPECT_TRUE(Xelqoria::Editor::TryConsumeHierarchyButtonClick(duplicateButtonHandle, releaseFrame, inputState));

    DestroyWindow(duplicateButtonHandle);
    DestroyWindow(createButtonHandle);
}
