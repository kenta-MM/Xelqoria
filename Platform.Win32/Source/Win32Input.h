#pragma once

#include "IInput.h"

namespace Xelqoria::Platform::Win32
{
    /// <summary>
    /// Win32 API から現在の入力状態を読み取る。
    /// </summary>
    class Win32Input final : public IInput
    {
    public:
        void Update() override;
        [[nodiscard]] bool IsKeyDown(std::uint32_t keyCode) const override;
        [[nodiscard]] bool IsMouseButtonDown(std::uint32_t buttonCode) const override;
        [[nodiscard]] Point GetCursorScreenPosition() const override;
        [[nodiscard]] int GetMouseWheelDelta() const override;
    };
}
