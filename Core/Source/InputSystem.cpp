#include "InputSystem.h"

#include <utility>

namespace Xelqoria::Core
{
    namespace
    {
        constexpr std::uint32_t LeftMouseButtonCode = 0x01;

        [[nodiscard]] bool IsValidInputCode(int keyCode)
        {
            return 0 <= keyCode && keyCode < static_cast<int>(InputSnapshot::KeyStateCount);
        }
    }

    InputSnapshot::InputSnapshot(
        const std::array<bool, KeyStateCount>& keyDownStates,
        const std::array<bool, KeyStateCount>& previousKeyDownStates,
        bool isLeftMouseButtonDown,
        bool wasLeftMouseButtonDown,
        Platform::Point cursorScreenPoint,
        int mouseWheelDelta)
        : m_keyDownStates(keyDownStates)
        , m_previousKeyDownStates(previousKeyDownStates)
        , m_isLeftMouseButtonDown(isLeftMouseButtonDown)
        , m_wasLeftMouseButtonDown(wasLeftMouseButtonDown)
        , m_cursorScreenPoint(cursorScreenPoint)
        , m_mouseWheelDelta(mouseWheelDelta)
    {
    }

    bool InputSnapshot::IsKeyDown(int keyCode) const
    {
        if (false == IsValidKeyCode(keyCode))
        {
            return false;
        }

        return m_keyDownStates[static_cast<std::size_t>(keyCode)];
    }

    bool InputSnapshot::WasKeyPressed(int keyCode) const
    {
        if (false == IsValidKeyCode(keyCode))
        {
            return false;
        }

        const std::size_t stateIndex = static_cast<std::size_t>(keyCode);
        return m_keyDownStates[stateIndex] && false == m_previousKeyDownStates[stateIndex];
    }

    bool InputSnapshot::WasKeyReleased(int keyCode) const
    {
        if (false == IsValidKeyCode(keyCode))
        {
            return false;
        }

        const std::size_t stateIndex = static_cast<std::size_t>(keyCode);
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

    Platform::Point InputSnapshot::GetCursorScreenPoint() const
    {
        return m_cursorScreenPoint;
    }

    int InputSnapshot::GetMouseWheelDelta() const
    {
        return m_mouseWheelDelta;
    }

    bool InputSnapshot::IsValidKeyCode(int keyCode)
    {
        return 0 <= keyCode && keyCode < static_cast<int>(KeyStateCount);
    }

    InputSystem::InputSystem()
        : InputSystem(
            [](int)
            {
                return false;
            },
            []
            {
                return Platform::Point{};
            },
            []
            {
                return 0;
            })
    {
    }

    InputSystem::InputSystem(std::unique_ptr<Platform::IInput> input)
        : InputSystem()
    {
        SetPlatformInput(std::move(input));
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

    void InputSystem::SetPlatformInput(std::unique_ptr<Platform::IInput> input)
    {
        m_input = std::move(input);
        m_keyStateReader =
            [this](int keyCode)
            {
                if (nullptr == m_input || false == IsValidInputCode(keyCode))
                {
                    return false;
                }

                return m_input->IsKeyDown(static_cast<std::uint32_t>(keyCode));
            };
        m_cursorPositionReader =
            [this]()
            {
                if (nullptr == m_input)
                {
                    return Platform::Point{};
                }

                return m_input->GetCursorScreenPosition();
            };
        m_mouseWheelDeltaReader =
            [this]()
            {
                if (nullptr == m_input)
                {
                    return 0;
                }

                return m_input->GetMouseWheelDelta();
            };
    }

    void InputSystem::SetMouseWheelDeltaReader(MouseWheelDeltaReader mouseWheelDeltaReader)
    {
        m_mouseWheelDeltaReader = std::move(mouseWheelDeltaReader);
    }

    void InputSystem::Update()
    {
        if (m_input)
        {
            m_input->Update();
        }

        std::array<bool, InputSnapshot::KeyStateCount> keyDownStates{};
        std::array<bool, InputSnapshot::KeyStateCount> previousKeyDownStates{};
        for (std::size_t index = 0; index < keyDownStates.size(); ++index)
        {
            previousKeyDownStates[index] = m_snapshot.IsKeyDown(static_cast<int>(index));
            keyDownStates[index] = m_keyStateReader(static_cast<int>(index));
        }

        const bool wasLeftMouseButtonDown = m_snapshot.IsMouseButtonDown(MouseButton::Left);
        const bool isLeftMouseButtonDown = m_keyStateReader(static_cast<int>(LeftMouseButtonCode));
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
