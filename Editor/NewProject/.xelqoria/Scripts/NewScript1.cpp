#include "XelqoriaScriptApi.h"

#include <Windows.h>

#pragma comment(lib, "user32.lib")

namespace
{
    constexpr float MoveSpeed = 240.0f;

    float g_spriteX = 219.0f;
    float g_spriteY = 30.0f;
    float g_spriteZ = 0.0f;
}

extern "C" __declspec(dllexport) void Start()
{
    SetSpritePosition(g_spriteX, g_spriteY, g_spriteZ);
}

extern "C" __declspec(dllexport) void Update(float deltaTime)
{
    float directionY = 0.0f;
    if (0 != (GetAsyncKeyState('W') & 0x8000))
    {
        directionY += 1.0f;
    }

    if (0 != (GetAsyncKeyState('S') & 0x8000))
    {
        directionY -= 1.0f;
    }

    if (0.0f == directionY)
    {
        return;
    }

    g_spriteY += directionY * MoveSpeed * deltaTime;
    SetSpritePosition(g_spriteX, g_spriteY, g_spriteZ);
}
