#pragma once

namespace Xelqoria::Math
{
    struct Vector2
    {
        float x = 0.0f;
        float y = 0.0f;

        constexpr Vector2() noexcept = default;
        constexpr Vector2(float x, float y) noexcept : x(x), y(y) {}

        [[nodiscard]] constexpr Vector2 operator+(const Vector2& rhs) const noexcept
        {
            return { x + rhs.x, y + rhs.y };
        }

        [[nodiscard]] constexpr Vector2 operator-(const Vector2& rhs) const noexcept
        {
            return { x - rhs.x, y - rhs.y };
        }

        [[nodiscard]] constexpr Vector2 operator*(const float s) const noexcept
        {
            return { x * s, y * s };
        }
    };
}