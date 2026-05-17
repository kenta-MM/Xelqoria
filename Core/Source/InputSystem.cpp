#include "InputSystem.h"

#include <utility>
#include <Windows.h>
#include <array>

namespace Xelqoria::Core
{
    InputSnapshot::InputSnapshot(
        const std::array<bool, KeyStateCount>& keyDownStates,
        const std::array<bool, KeyStateCount>& previousKeyDownStates,
        bool isLeftMouseButtonDown,
        bool wasLeftMouseButtonDown,
        POINT cursorScreenPoint,
        int mouseWheelDelta)
        : m_keyDownStates(keyDownStates)
        , m_previousKeyDownStates(previousKeyDownStates)
        , m_isLeftMouseButtonDown(isLeftMouseButtonDown)
        , m_wasLeftMouseButtonDown(wasLeftMouseButtonDown)
        , m_cursorScreenPoint(cursorScreenPoint)
        , m_mouseWheelDelta(mouseWheelDelta)
    {
    }

    bool InputSnapshot::IsKeyDown(int virtualKeyCode) const
    {
        if (false == IsValidVirtualKeyCode(virtualKeyCode))
        {
            return false;
        }

        return m_keyDownStates[static_cast<std::size_t>(virtualKeyCode)];
    }

    bool InputSnapshot::WasKeyPressed(int virtualKeyCode) const
    {
        if (false == IsValidVirtualKeyCode(virtualKeyCode))
        {
            return false;
        }

        const std::size_t stateIndex = static_cast<std::size_t>(virtualKeyCode);
        return m_keyDownStates[stateIndex] && false == m_previousKeyDownStates[stateIndex];
    }

    bool InputSnapshot::WasKeyReleased(int virtualKeyCode) const
    {
        if (false == IsValidVirtualKeyCode(virtualKeyCode))
        {
            return false;
        }

        const std::size_t stateIndex = static_cast<std::size_t>(virtualKeyCode);
        return false == m_keyDownStates[stateIndex] && true == m_previousKeyDownStates[stateIndex];
    }

    bool InputSnapshot::IsMouseButtonDown(MouseButton button) const
    {
        if (MouseButton::Left == button)
        {
            return m_isLeftMouseButtonDown;
        }

        return false;
    }

    bool InputSnapshot::WasMouseButtonPressed(MouseButton button) const
    {
        if (MouseButton::Left == button)
        {
            return m_isLeftMouseButtonDown && false == m_wasLeftMouseButtonDown;
        }

        return false;
    }

    bool InputSnapshot::WasMouseButtonReleased(MouseButton button) const
    {
        if (MouseButton::Left == button)
        {
            return false == m_isLeftMouseButtonDown && true == m_wasLeftMouseButtonDown;
        }

        return false;
    }

    POINT InputSnapshot::GetCursorScreenPoint() const
    {
        return m_cursorScreenPoint;
    }

    int InputSnapshot::GetMouseWheelDelta() const
    {
        return m_mouseWheelDelta;
    }

    bool InputSnapshot::IsValidVirtualKeyCode(int virtualKeyCode)
    {
        return 0 <= virtualKeyCode && virtualKeyCode < static_cast<int>(KeyStateCount);
    }

    InputSystem::InputSystem()
        : InputSystem(
            [](int virtualKeyCode)
            {
                return 0 != (::GetAsyncKeyState(virtualKeyCode) & 0x8000);
            },
            []
            {
                POINT cursorScreenPoint{};
                ::GetCursorPos(&cursorScreenPoint);
                return cursorScreenPoint;
            },
            []
            {
                return 0;
            })
    {
    }

    InputSystem::InputSystem(KeyStateReader keyStateReader, CursorPositionReader cursorPositionReader)
        : InputSystem(std::move(keyStateReader), std::move(cursorPositionReader), [] { return 0; })
    {
    }

    InputSystem::InputSystem(
        KeyStateReader keyStateReader,
        CursorPositionReader cursorPositionReader,
        MouseWheelDeltaReader mouseWheelDeltaReader)
        : m_keyStateReader(std::move(keyStateReader))
        , m_cursorPositionReader(std::move(cursorPositionReader))
        , m_mouseWheelDeltaReader(std::move(mouseWheelDeltaReader))
    {
    }

    void InputSystem::SetMouseWheelDeltaReader(MouseWheelDeltaReader mouseWheelDeltaReader)
    {
        m_mouseWheelDeltaReader = std::move(mouseWheelDeltaReader);
    }

    void InputSystem::Update()
    {
        std::array<bool, InputSnapshot::KeyStateCount> keyDownStates{};
        std::array<bool, InputSnapshot::KeyStateCount> previousKeyDownStates{};
        for (std::size_t index = 0; index < keyDownStates.size(); ++index)
        {
            previousKeyDownStates[index] = m_snapshot.IsKeyDown(static_cast<int>(index));
            keyDownStates[index] = m_keyStateReader(static_cast<int>(index));
        }

        const bool wasLeftMouseButtonDown = m_snapshot.IsMouseButtonDown(MouseButton::Left);
        const bool isLeftMouseButtonDown = m_keyStateReader(VK_LBUTTON);
        m_snapshot = InputSnapshot(
            keyDownStates,
            previousKeyDownStates,
            isLeftMouseButtonDown,
            wasLeftMouseButtonDown,
            m_cursorPositionReader(),
            m_mouseWheelDeltaReader());
    }

    const InputSnapshot& InputSystem::GetSnapshot() const
    {
        return m_snapshot;
    }
}
