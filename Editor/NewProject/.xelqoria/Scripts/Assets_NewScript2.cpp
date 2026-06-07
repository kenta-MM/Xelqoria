#include "XelqoriaScriptApi.h"

extern "C" __declspec(dllexport) void Start()
{
}

extern "C" __declspec(dllexport) void Update(float deltaTime)
{
    static float x = 0.0f;
    static float y = 0.0f;
    constexpr float speed = 120.0f;

    if (IsKeyDown(XelqoriaKeyLeft))
    {
        x -= speed * deltaTime;
    }

    if (IsKeyDown(XelqoriaKeyRight))
    {
        x += speed * deltaTime;
    }

    if (IsKeyDown(XelqoriaKeyUp))
    {
        y += speed * deltaTime;
    }

    if (IsKeyDown(XelqoriaKeyDown))
    {
        y -= speed * deltaTime;
    }

    SetSpritePosition(x, y, 0.0f);
}
