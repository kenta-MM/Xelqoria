#pragma once

#include <Windows.h>
#include <array>
#include <cstdint>

namespace Xelqoria::Editor
{
    /// <summary>
    /// Editor 固定 UI の Win32 child window 群とレイアウトを管理する。
    /// </summary>
    class EditorShell
    {
    public:
        /// <summary>
        /// EditorShell 用 child window 群を生成する。
        /// </summary>
        /// <param name="parentWindow">親となる Editor メインウィンドウ。</param>
        /// <param name="hInstance">Windows アプリケーションのインスタンスハンドル。</param>
        /// <returns>生成に成功した場合は true。</returns>
        bool Initialize(HWND parentWindow, HINSTANCE hInstance);

        /// <summary>
        /// 現在の親ウィンドウサイズに合わせてレイアウトを更新する。
        /// </summary>
        /// <param name="parentWindow">親となる Editor メインウィンドウ。</param>
        /// <returns>SceneView サイズが変化した場合は true。</returns>
        bool UpdateLayout(HWND parentWindow);

        /// <summary>
        /// Hierarchy パネルの ListBox を取得する。
        /// </summary>
        /// <returns>Hierarchy ListBox の HWND。</returns>
        [[nodiscard]] HWND GetHierarchyListBox() const;

        /// <summary>
        /// Hierarchy パネルの要約ラベルを取得する。
        /// </summary>
        /// <returns>Hierarchy 要約ラベルの HWND。</returns>
        [[nodiscard]] HWND GetHierarchySummaryLabel() const;

        /// <summary>
        /// Hierarchy パネルの Entity 名入力欄を取得する。
        /// </summary>
        /// <returns>Entity 名入力欄の HWND。</returns>
        [[nodiscard]] HWND GetHierarchyNameEdit() const;

        /// <summary>
        /// Hierarchy パネルの新規作成ボタンを取得する。
        /// </summary>
        /// <returns>新規作成ボタンの HWND。</returns>
        [[nodiscard]] HWND GetHierarchyCreateButton() const;

        /// <summary>
        /// Hierarchy パネルの複製ボタンを取得する。
        /// </summary>
        /// <returns>複製ボタンの HWND。</returns>
        [[nodiscard]] HWND GetHierarchyDuplicateButton() const;

        /// <summary>
        /// Hierarchy パネルの削除ボタンを取得する。
        /// </summary>
        /// <returns>削除ボタンの HWND。</returns>
        [[nodiscard]] HWND GetHierarchyDeleteButton() const;

        /// <summary>
        /// Assets パネルの ListBox を取得する。
        /// </summary>
        /// <returns>Assets ListBox の HWND。</returns>
        [[nodiscard]] HWND GetAssetsListBox() const;

        /// <summary>
        /// Assets パネルの要約ラベルを取得する。
        /// </summary>
        /// <returns>Assets 要約ラベルの HWND。</returns>
        [[nodiscard]] HWND GetAssetsSummaryLabel() const;

        /// <summary>
        /// Inspector パネルの要約ラベルを取得する。
        /// </summary>
        /// <returns>Inspector 要約ラベルの HWND。</returns>
        [[nodiscard]] HWND GetInspectorSummaryLabel() const;

        /// <summary>
        /// Transform セクション見出しラベルを取得する。
        /// </summary>
        /// <returns>Transform セクション見出しの HWND。</returns>
        [[nodiscard]] HWND GetTransformSectionLabel() const;

        /// <summary>
        /// Transform 項目ラベル群を取得する。
        /// </summary>
        /// <returns>Transform ラベル配列。</returns>
        [[nodiscard]] const std::array<HWND, 3>& GetTransformLabels() const;

        /// <summary>
        /// Transform 入力欄群を取得する。
        /// </summary>
        /// <returns>Transform 入力欄配列。</returns>
        [[nodiscard]] const std::array<HWND, 9>& GetTransformEditControls() const;

        /// <summary>
        /// SpriteComponent セクション見出しラベルを取得する。
        /// </summary>
        /// <returns>SpriteComponent セクション見出しの HWND。</returns>
        [[nodiscard]] HWND GetSpriteComponentSectionLabel() const;

        /// <summary>
        /// SpriteRef ラベルを取得する。
        /// </summary>
        /// <returns>SpriteRef ラベルの HWND。</returns>
        [[nodiscard]] HWND GetSpriteRefLabel() const;

        /// <summary>
        /// SpriteRef 入力欄を取得する。
        /// </summary>
        /// <returns>SpriteRef 入力欄の HWND。</returns>
        [[nodiscard]] HWND GetSpriteRefEdit() const;

        /// <summary>
        /// SpriteComponent 操作ボタンを取得する。
        /// </summary>
        /// <returns>SpriteComponent 操作用ボタンの HWND。</returns>
        [[nodiscard]] HWND GetSpriteComponentActionButton() const;

        /// <summary>
        /// SceneView の状態ラベルを取得する。
        /// </summary>
        /// <returns>SceneView サイズラベルの HWND。</returns>
        [[nodiscard]] HWND GetSceneViewSizeLabel() const;

        /// <summary>
        /// SceneView 説明ラベルを取得する。
        /// </summary>
        /// <returns>SceneView 説明ラベルの HWND。</returns>
        [[nodiscard]] HWND GetSceneViewPlanLabel() const;

        /// <summary>
        /// SceneView 描画先 child window を取得する。
        /// </summary>
        /// <returns>SceneView host の HWND。</returns>
        [[nodiscard]] HWND GetSceneViewHost() const;

        /// <summary>
        /// 現在の SceneView 幅を取得する。
        /// </summary>
        /// <returns>SceneView 幅。</returns>
        [[nodiscard]] std::uint32_t GetSceneViewWidth() const;

        /// <summary>
        /// 現在の SceneView 高さを取得する。
        /// </summary>
        /// <returns>SceneView 高さ。</returns>
        [[nodiscard]] std::uint32_t GetSceneViewHeight() const;

    private:
        /// <summary>
        /// 共通設定を適用した子ウィンドウを生成する。
        /// </summary>
        /// <param name="parentWindow">親ウィンドウ。</param>
        /// <param name="hInstance">Windows アプリケーションインスタンス。</param>
        /// <param name="className">生成する Win32 クラス名。</param>
        /// <param name="text">初期表示文字列。</param>
        /// <param name="style">適用するウィンドウスタイル。</param>
        /// <param name="exStyle">適用する拡張ウィンドウスタイル。</param>
        /// <returns>生成した child window の HWND。失敗時は nullptr。</returns>
        HWND CreateChildWindow(
            HWND parentWindow,
            HINSTANCE hInstance,
            const wchar_t* className,
            const wchar_t* text,
            DWORD style,
            DWORD exStyle = 0) const;

    private:
        HFONT m_defaultFont = nullptr;
        HWND m_hierarchyPanel = nullptr;
        HWND m_assetsPanel = nullptr;
        HWND m_inspectorPanel = nullptr;
        HWND m_sceneViewPanel = nullptr;
        HWND m_sceneViewPlanLabel = nullptr;
        HWND m_sceneViewHost = nullptr;
        HWND m_sceneViewSizeLabel = nullptr;
        HWND m_assetsListBox = nullptr;
        HWND m_assetsSummaryLabel = nullptr;
        HWND m_hierarchySummaryLabel = nullptr;
        HWND m_hierarchyListBox = nullptr;
        HWND m_hierarchyNameEdit = nullptr;
        HWND m_hierarchyCreateButton = nullptr;
        HWND m_hierarchyDuplicateButton = nullptr;
        HWND m_hierarchyDeleteButton = nullptr;
        HWND m_inspectorSummaryLabel = nullptr;
        HWND m_transformSectionLabel = nullptr;
        std::array<HWND, 3> m_transformLabels{};
        std::array<HWND, 9> m_transformEditControls{};
        HWND m_spriteComponentSectionLabel = nullptr;
        HWND m_spriteRefLabel = nullptr;
        HWND m_spriteRefEdit = nullptr;
        HWND m_spriteComponentActionButton = nullptr;
        std::uint32_t m_sceneViewWidth = 0;
        std::uint32_t m_sceneViewHeight = 0;
    };
}
