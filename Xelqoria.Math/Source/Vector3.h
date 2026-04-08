#pragma once

namespace Xelqoria::Math
{
    struct Vector3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        constexpr Vector3() noexcept = default;
        constexpr Vector3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}

        [[nodiscard]] constexpr Vector3 operator+(const Vector3& rhs) const noexcept
        {
            return { x + rhs.x, y + rhs.y, z + rhs.z };
        }

        [[nodiscard]] constexpr Vector3 operator-(const Vector3& rhs) const noexcept
        {
            return { x - rhs.x, y - rhs.y, z - rhs.z };
        }

        [[nodiscard]] constexpr Vector3 operator*(const float s) const noexcept
        {
            return { x * s, y * s, z * s };
        }
    };
}