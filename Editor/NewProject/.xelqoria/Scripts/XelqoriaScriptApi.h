#pragma once

struct XelqoriaScriptSpriteApi
{
    void* context;
    void (*ReportError)(void* context, const char* message);
    void (*SetPosition)(void* context, float x, float y, float z);
    void (*SetRotation)(void* context, float x, float y, float z);
    void (*SetScale)(void* context, float x, float y, float z);
    void (*SetVisible)(void* context, int visible);
    void (*SetColor)(void* context, float red, float green, float blue, float alpha);
    int (*IsKeyDown)(void* context, int keyCode);
};

constexpr int XelqoriaKeyLeft = 0x25;
constexpr int XelqoriaKeyUp = 0x26;
constexpr int XelqoriaKeyRight = 0x27;
constexpr int XelqoriaKeyDown = 0x28;

inline XelqoriaScriptSpriteApi* g_xelqoriaScriptSpriteApi = nullptr;

extern "C" __declspec(dllexport) void Xelqoria_SetSpriteApi(XelqoriaScriptSpriteApi* api)
{
    g_xelqoriaScriptSpriteApi = api;
}

inline void ReportScriptError(const char* message)
{
    if (nullptr != g_xelqoriaScriptSpriteApi && nullptr != g_xelqoriaScriptSpriteApi->ReportError)
    {
        g_xelqoriaScriptSpriteApi->ReportError(g_xelqoriaScriptSpriteApi->context, message);
    }
}

inline void SetSpritePosition(float x, float y, float z)
{
    if (nullptr != g_xelqoriaScriptSpriteApi && nullptr != g_xelqoriaScriptSpriteApi->SetPosition)
    {
        g_xelqoriaScriptSpriteApi->SetPosition(g_xelqoriaScriptSpriteApi->context, x, y, z);
    }
}

inline void SetSpriteRotation(float x, float y, float z)
{
    if (nullptr != g_xelqoriaScriptSpriteApi && nullptr != g_xelqoriaScriptSpriteApi->SetRotation)
    {
        g_xelqoriaScriptSpriteApi->SetRotation(g_xelqoriaScriptSpriteApi->context, x, y, z);
    }
}

inline void SetSpriteScale(float x, float y, float z)
{
    if (nullptr != g_xelqoriaScriptSpriteApi && nullptr != g_xelqoriaScriptSpriteApi->SetScale)
    {
        g_xelqoriaScriptSpriteApi->SetScale(g_xelqoriaScriptSpriteApi->context, x, y, z);
    }
}

inline void SetSpriteVisible(bool visible)
{
    if (nullptr != g_xelqoriaScriptSpriteApi && nullptr != g_xelqoriaScriptSpriteApi->SetVisible)
    {
        g_xelqoriaScriptSpriteApi->SetVisible(g_xelqoriaScriptSpriteApi->context, visible ? 1 : 0);
    }
}

inline void SetSpriteColor(float red, float green, float blue, float alpha)
{
    if (nullptr != g_xelqoriaScriptSpriteApi && nullptr != g_xelqoriaScriptSpriteApi->SetColor)
    {
        g_xelqoriaScriptSpriteApi->SetColor(g_xelqoriaScriptSpriteApi->context, red, green, blue, alpha);
    }
}

inline bool IsKeyDown(int keyCode)
{
    if (nullptr == g_xelqoriaScriptSpriteApi || nullptr == g_xelqoriaScriptSpriteApi->IsKeyDown)
    {
        return false;
    }

    return 0 != g_xelqoriaScriptSpriteApi->IsKeyDown(g_xelqoriaScriptSpriteApi->context, keyCode);
}
