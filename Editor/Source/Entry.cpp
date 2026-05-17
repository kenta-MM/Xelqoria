#include <Windows.h>

#include "Application.h"

namespace
{
    void EnablePerMonitorDpiAwareness()
    {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (nullptr == user32)
        {
            return;
        }

        using SetProcessDpiAwarenessContextFunction = BOOL(WINAPI*)(HANDLE);
        auto setProcessDpiAwarenessContext =
            reinterpret_cast<SetProcessDpiAwarenessContextFunction>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (nullptr == setProcessDpiAwarenessContext)
        {
            return;
        }

        constexpr LONG_PTR PerMonitorAwareV2 = -4;
        setProcessDpiAwarenessContext(reinterpret_cast<HANDLE>(PerMonitorAwareV2));
    }
}

/// <summary>
/// エディタアプリケーションのエントリーポイントを提供する。
/// </summary>
/// <param name="hInstance">現在のアプリケーションインスタンスハンドル。</param>
/// <param name="hPrevInstance">未使用の旧インスタンスハンドル。</param>
/// <param name="pCmdLine">コマンドライン引数文字列。</param>
/// <param name="nCmdShow">初回表示時のウィンドウ表示方法。</param>
/// <returns>Application の実行結果コード。</returns>
int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR pCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    EnablePerMonitorDpiAwareness();

    Xelqoria::Editor::Application app(hInstance);
    return app.Run();
}
