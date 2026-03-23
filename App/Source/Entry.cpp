#include <Windows.h>
#include "Application.h"

/// <summary>
/// アプリケーションのエントリーポイントを提供する。
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

    Xelqoria::App::Application app(hInstance);
    return app.Run();
}
