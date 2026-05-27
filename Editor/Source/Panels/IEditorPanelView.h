#pragma once

#include <Windows.h>
#include <vector>

#include "Shell/EditorPanelId.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Editor Panel View が共通で提供する最小操作。
    /// </summary>
    class IEditorPanelView
    {
    public:
        /// <summary>
        /// View を破棄する。
        /// </summary>
        virtual ~IEditorPanelView() = default;

        /// <summary>
        /// View が対応する Panel 識別子を取得する。
        /// </summary>
        /// <returns>Panel 識別子。</returns>
        [[nodiscard]] virtual EditorPanelId GetPanelId() const = 0;

        /// <summary>
        /// View の表示タイトルを取得する。
        /// </summary>
        /// <returns>表示タイトル。</returns>
        [[nodiscard]] virtual const wchar_t* GetTitle() const = 0;

        /// <summary>
        /// View の外枠 child window を取得する。
        /// </summary>
        /// <returns>外枠 window の HWND。</returns>
        [[nodiscard]] virtual HWND GetRootWindow() const = 0;

        /// <summary>
        /// View が所有する child window 群を生成する。
        /// </summary>
        /// <param name="parentWindow">親ウィンドウ。</param>
        /// <param name="hInstance">Windows アプリケーションインスタンス。</param>
        /// <returns>生成に成功した場合は true。</returns>
        virtual bool Initialize(HWND parentWindow, HINSTANCE hInstance) = 0;

        /// <summary>
        /// View を指定矩形に配置する。
        /// </summary>
        /// <param name="bounds">親クライアント座標の配置矩形。</param>
        virtual void Layout(const RECT& bounds) = 0;

        /// <summary>
        /// View の表示状態を切り替える。
        /// </summary>
        /// <param name="visible">表示する場合は true。</param>
        virtual void Show(bool visible) = 0;

        /// <summary>
        /// View が所有する child window 群の親を変更する。
        /// </summary>
        /// <param name="parentWindow">新しい親ウィンドウ。</param>
        virtual void SetParent(HWND parentWindow) = 0;

        /// <summary>
        /// View が所有する child window 群を列挙する。
        /// </summary>
        /// <param name="controls">列挙結果を追加する配列。</param>
        virtual void CollectControls(std::vector<HWND>& controls) const = 0;
    };
}
