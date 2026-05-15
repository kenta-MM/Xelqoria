#pragma once

#include <Windows.h>
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "InputSystem.h"

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
        /// EditorShell が所有する UI リソースを破棄する。
        /// </summary>
        ~EditorShell();

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
        /// Assets パネルの ListView を取得する。
        /// </summary>
        /// <returns>Assets ListView の HWND。</returns>
        [[nodiscard]] HWND GetAssetsListView() const;

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
        /// SpriteRef ドロップ先ハイライトを取得する。
        /// </summary>
        /// <returns>SpriteRef ドロップ先ハイライトの HWND。</returns>
        [[nodiscard]] HWND GetSpriteRefDropHighlight() const;

        /// <summary>
        /// Script Asset ラベルを取得する。
        /// </summary>
        /// <returns>Script Asset ラベルの HWND。</returns>
        [[nodiscard]] HWND GetScriptAssetLabel() const;

        /// <summary>
        /// Script Asset 表示欄を取得する。
        /// </summary>
        /// <returns>Script Asset 表示欄の HWND。</returns>
        [[nodiscard]] HWND GetScriptAssetEdit() const;

        /// <summary>
        /// Script Asset 作成ボタンを取得する。
        /// </summary>
        /// <returns>Script Asset 作成ボタンの HWND。</returns>
        [[nodiscard]] HWND GetScriptCreateButton() const;

        /// <summary>
        /// Script Asset 割り当てボタンを取得する。
        /// </summary>
        /// <returns>Script Asset 割り当てボタンの HWND。</returns>
        [[nodiscard]] HWND GetScriptAssignButton() const;

        /// <summary>
        /// Script Asset 割り当て解除ボタンを取得する。
        /// </summary>
        /// <returns>Script Asset 割り当て解除ボタンの HWND。</returns>
        [[nodiscard]] HWND GetScriptClearButton() const;

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
        /// プロジェクト概要ラベルを取得する。
        /// </summary>
        /// <returns>プロジェクト概要ラベルの HWND。</returns>
        [[nodiscard]] HWND GetProjectSummaryLabel() const;

        /// <summary>
        /// プロジェクト内 Scene 一覧 ListBox を取得する。
        /// </summary>
        /// <returns>Scene 一覧 ListBox の HWND。</returns>
        [[nodiscard]] HWND GetProjectSceneListBox() const;

        /// <summary>
        /// 選択 Scene 詳細ラベルを取得する。
        /// </summary>
        /// <returns>選択 Scene 詳細ラベルの HWND。</returns>
        [[nodiscard]] HWND GetProjectSceneDetailLabel() const;

        /// <summary>
        /// SceneView 描画先 child window を取得する。
        /// </summary>
        /// <returns>SceneView host の HWND。</returns>
        [[nodiscard]] HWND GetSceneViewHost() const;

        /// <summary>
        /// LogOutput タブを取得する。
        /// </summary>
        /// <returns>LogOutput TabControl の HWND。</returns>
        [[nodiscard]] HWND GetLogOutputTabControl() const;

        /// <summary>
        /// LogOutput クリアボタンを取得する。
        /// </summary>
        /// <returns>LogOutput クリアボタンの HWND。</returns>
        [[nodiscard]] HWND GetLogClearButton() const;

        /// <summary>
        /// LogOutput コピー ボタンを取得する。
        /// </summary>
        /// <returns>LogOutput コピー ボタンの HWND。</returns>
        [[nodiscard]] HWND GetLogCopyButton() const;

        /// <summary>
        /// LogOutput フィルタ入力欄を取得する。
        /// </summary>
        /// <returns>LogOutput フィルタ入力欄の HWND。</returns>
        [[nodiscard]] HWND GetLogFilterEdit() const;

        /// <summary>
        /// LogOutput 一覧を取得する。
        /// </summary>
        /// <returns>LogOutput ListBox の HWND。</returns>
        [[nodiscard]] HWND GetLogListBox() const;

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

        /// <summary>
        /// Dock UI のフレーム入力を処理する。
        /// </summary>
        /// <param name="parentWindow">親となる Editor メインウィンドウ。</param>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        /// <returns>Dock レイアウトが変化した場合は true。</returns>
        bool UpdateDocking(HWND parentWindow, const Core::InputSnapshot& inputSnapshot);

        /// <summary>
        /// Dock 用 TabControl 通知を処理する。
        /// </summary>
        /// <param name="notifyParameter">WM_NOTIFY の LPARAM。</param>
        /// <returns>Dock UI が通知を消費した場合は true。</returns>
        bool HandleNotify(LPARAM notifyParameter);

        /// <summary>
        /// 現在開いているプロジェクト画面レイアウトを初期状態へ戻す。
        /// </summary>
        void ResetDockLayout();

    private:
        enum class EditorPanelId
        {
            Hierarchy,
            Assets,
            SceneView,
            Inspector,
            LogOutput
        };

        enum class DockAreaId
        {
            LeftTop,
            LeftBottom,
            Center,
            Right,
            Floating
        };

        enum class DockNodeKind
        {
            Leaf,
            Split
        };

        enum class DockSplitOrientation
        {
            Horizontal,
            Vertical
        };

        enum class DockGuideTargetKind
        {
            None,
            Tab,
            SplitLeft,
            SplitRight,
            SplitTop,
            SplitBottom,
            Float
        };

        enum class DockDragKind
        {
            None,
            Panel,
            VerticalSplitter,
            HorizontalSplitter
        };

        using DockNodeId = int;

        struct DockNode;
        struct DockGuideTarget;

        /// <summary>
        /// レイアウト計算結果を保持する。
        /// </summary>
        struct LayoutMetrics;

        /// <summary>
        /// Hierarchy パネル用 child window 群を生成する。
        /// </summary>
        bool InitializeHierarchyPanel(HWND parentWindow, HINSTANCE hInstance);

        /// <summary>
        /// Assets パネル用 child window 群を生成する。
        /// </summary>
        bool InitializeAssetsPanel(HWND parentWindow, HINSTANCE hInstance);

        /// <summary>
        /// Inspector パネル用 child window 群を生成する。
        /// </summary>
        bool InitializeInspectorPanel(HWND parentWindow, HINSTANCE hInstance);

        /// <summary>
        /// SceneView パネル用 child window 群を生成する。
        /// </summary>
        bool InitializeSceneViewPanel(HWND parentWindow, HINSTANCE hInstance);

        /// <summary>
        /// LogOutput パネル用 child window 群を生成する。
        /// </summary>
        bool InitializeLogOutputPanel(HWND parentWindow, HINSTANCE hInstance);

        /// <summary>
        /// Hierarchy パネルをレイアウトする。
        /// </summary>
        void LayoutHierarchyPanel(const LayoutMetrics& metrics);

        /// <summary>
        /// Assets パネルをレイアウトする。
        /// </summary>
        void LayoutAssetsPanel(const LayoutMetrics& metrics);

        /// <summary>
        /// Inspector パネルをレイアウトする。
        /// </summary>
        void LayoutInspectorPanel(const LayoutMetrics& metrics);

        /// <summary>
        /// SceneView パネルをレイアウトする。
        /// </summary>
        void LayoutSceneViewPanel(const LayoutMetrics& metrics);

        /// <summary>
        /// 指定矩形へ Hierarchy パネルをレイアウトする。
        /// </summary>
        void LayoutHierarchyPanelInRect(const RECT& panelRect);

        /// <summary>
        /// 指定矩形へ Assets パネルをレイアウトする。
        /// </summary>
        void LayoutAssetsPanelInRect(const RECT& panelRect);

        /// <summary>
        /// 指定矩形へ Inspector パネルをレイアウトする。
        /// </summary>
        void LayoutInspectorPanelInRect(const RECT& panelRect);

        /// <summary>
        /// 指定矩形へ SceneView パネルをレイアウトする。
        /// </summary>
        void LayoutSceneViewPanelInRect(const RECT& panelRect);

        /// <summary>
        /// 指定矩形へ LogOutput パネルをレイアウトする。
        /// </summary>
        void LayoutLogOutputPanelInRect(const RECT& panelRect);

        /// <summary>
        /// DockArea に割り当てられたパネルをタブ付き領域として配置する。
        /// </summary>
        void LayoutDockArea(DockAreaId dockAreaId, const RECT& areaRect);

        /// <summary>
        /// Dock ツリーの初期状態を構築する。
        /// </summary>
        void BuildInitialDockTree();

        /// <summary>
        /// Dock ツリーの node を指定矩形へレイアウトする。
        /// </summary>
        void LayoutDockNode(DockNodeId dockNodeId, const RECT& nodeRect);

        /// <summary>
        /// Dock leaf に割り当てられたパネルをタブ付き領域として配置する。
        /// </summary>
        void LayoutDockLeaf(DockNodeId dockNodeId, const RECT& areaRect);

        /// <summary>
        /// Dock node を追加して ID を返す。
        /// </summary>
        [[nodiscard]] DockNodeId AddDockNode(DockNode node);

        /// <summary>
        /// カーソル位置にある Dock leaf を返す。
        /// </summary>
        [[nodiscard]] DockNodeId HitTestDockLeaf(POINT cursorClientPoint) const;

        /// <summary>
        /// カーソル位置にある Dock 分割境界を返す。
        /// </summary>
        [[nodiscard]] DockNodeId HitTestDockSplitter(POINT cursorScreenPoint) const;

        /// <summary>
        /// root から到達可能な Dock leaf 一覧を返す。
        /// </summary>
        void CollectReachableDockLeaves(DockNodeId dockNodeId, std::vector<DockNodeId>& dockLeafNodeIds) const;

        /// <summary>
        /// root から到達可能な Dock split 一覧を返す。
        /// </summary>
        void CollectReachableDockSplits(DockNodeId dockNodeId, std::vector<DockNodeId>& dockSplitNodeIds) const;

        /// <summary>
        /// Dock 分割境界のドラッグ量を splitRatio へ反映する。
        /// </summary>
        [[nodiscard]] bool UpdateDockSplitterDrag(HWND parentWindow, POINT cursorScreenPoint);

        /// <summary>
        /// 指定 Dock split の比率を子領域の最小サイズに収める。
        /// </summary>
        [[nodiscard]] float ClampDockSplitRatio(DockNodeId dockNodeId, float ratio) const;

        /// <summary>
        /// 指定パネルを含む Dock leaf を返す。
        /// </summary>
        [[nodiscard]] DockNodeId FindPanelDockLeaf(EditorPanelId panelId) const;

        /// <summary>
        /// Dock ツリーから指定パネルを取り除く。
        /// </summary>
        void RemovePanelFromDockTree(EditorPanelId panelId, bool collapseEmptyLeaves = true);

        /// <summary>
        /// 空になった Dock leaf を親 node へ畳み込む。
        /// </summary>
        [[nodiscard]] bool CollapseEmptyDockLeaves(DockNodeId dockNodeId);

        /// <summary>
        /// Dock ガイドへのドロップ結果を適用する。
        /// </summary>
        void ApplyDockGuideTarget(EditorPanelId panelId, const DockGuideTarget& guideTarget, HWND parentWindow);

        /// <summary>
        /// Dock ガイドに対するカーソル位置を判定する。
        /// </summary>
        [[nodiscard]] DockGuideTarget HitTestDockGuideTarget(HWND parentWindow, POINT cursorScreenPoint) const;

        /// <summary>
        /// Dock ガイド表示を現在のドラッグ状態へ同期する。
        /// </summary>
        void UpdateDockGuideWindows(HWND parentWindow, POINT cursorScreenPoint);

        /// <summary>
        /// Dock ガイドを非表示にする。
        /// </summary>
        void HideDockGuideWindows();

        /// <summary>
        /// Dock ガイド window を指定矩形へ表示する。
        /// </summary>
        void ShowDockGuideWindow(HWND guideWindow, const RECT& guideRect);

        /// <summary>
        /// 指定パネルの HWND 群を表示または非表示にする。
        /// </summary>
        void ShowPanelControls(EditorPanelId panelId, bool visible) const;

        /// <summary>
        /// 指定パネルの HWND 群を指定親へ付け替える。
        /// </summary>
        void SetPanelParent(EditorPanelId panelId, HWND parentWindow) const;

        /// <summary>
        /// 指定パネルの Dock 見出し領域を返す。
        /// </summary>
        [[nodiscard]] RECT GetPanelCaptionRect(EditorPanelId panelId) const;

        /// <summary>
        /// カーソル位置にある Dock 操作対象パネルを返す。
        /// </summary>
        [[nodiscard]] std::optional<EditorPanelId> HitTestPanelCaption(POINT cursorScreenPoint) const;

        /// <summary>
        /// カーソル位置から Dock 先領域を判定する。
        /// </summary>
        [[nodiscard]] DockAreaId HitTestDockArea(HWND parentWindow, POINT cursorScreenPoint) const;

        /// <summary>
        /// Panel を指定 DockArea へ移動する。
        /// </summary>
        void MovePanelToDockArea(EditorPanelId panelId, DockAreaId dockAreaId, HWND parentWindow);

        /// <summary>
        /// 指定パネルをフローティングウィンドウへ移動する。
        /// </summary>
        void FloatPanel(EditorPanelId panelId, POINT screenPoint, HWND parentWindow);

        /// <summary>
        /// 指定パネルをフローティングウィンドウの現在サイズへ合わせて配置する。
        /// </summary>
        void LayoutFloatingPanel(EditorPanelId panelId, HWND floatingWindow);

        /// <summary>
        /// フローティングウィンドウの移動による Dock 操作を開始する。
        /// </summary>
        void BeginFloatingWindowDockDrag(EditorPanelId panelId);

        /// <summary>
        /// フローティングウィンドウの移動中に Dock ガイドを更新する。
        /// </summary>
        void UpdateFloatingWindowDockDrag();

        /// <summary>
        /// フローティングウィンドウの移動終了時に Dock 操作を確定する。
        /// </summary>
        void CompleteFloatingWindowDockDrag(EditorPanelId panelId);

        /// <summary>
        /// フローティングウィンドウの閉じる操作を処理する。
        /// </summary>
        void HandleFloatingWindowClose(EditorPanelId panelId, HWND floatingWindow);

        /// <summary>
        /// 指定パネルのフローティングウィンドウを閉じる。
        /// </summary>
        void DestroyFloatingWindow(EditorPanelId panelId);

        /// <summary>
        /// 指定パネルに対応するフローティングウィンドウ格納先を返す。
        /// </summary>
        [[nodiscard]] HWND& GetFloatingWindowRef(EditorPanelId panelId);

        /// <summary>
        /// Dock TabControl の表示を現在状態へ同期する。
        /// </summary>
        void SyncDockTabs();

        /// <summary>
        /// split Dock leaf 用の TabControl を追加生成する。
        /// </summary>
        [[nodiscard]] HWND CreateAdditionalDockTabControl(HWND parentWindow);

        /// <summary>
        /// 指定 TabControl に DockArea のタブを設定する。
        /// </summary>
        void SyncDockAreaTabs(DockAreaId dockAreaId);

        /// <summary>
        /// 指定 DockArea に紐付く TabControl を返す。
        /// </summary>
        [[nodiscard]] HWND GetDockAreaTabControl(DockAreaId dockAreaId) const;

        /// <summary>
        /// 指定 Dock leaf に紐付く TabControl を返す。
        /// </summary>
        [[nodiscard]] HWND GetDockLeafTabControl(DockNodeId dockNodeId) const;

        /// <summary>
        /// DockArea に紐付くパネル一覧を返す。
        /// </summary>
        [[nodiscard]] std::vector<EditorPanelId>& GetDockAreaPanels(DockAreaId dockAreaId);

        /// <summary>
        /// DockArea に紐付くパネル一覧を返す。
        /// </summary>
        [[nodiscard]] const std::vector<EditorPanelId>& GetDockAreaPanels(DockAreaId dockAreaId) const;

        /// <summary>
        /// Panel 名を返す。
        /// </summary>
        [[nodiscard]] static const wchar_t* GetPanelTitle(EditorPanelId panelId);

        /// <summary>
        /// DockArea の有効なアクティブタブ index を返す。
        /// </summary>
        [[nodiscard]] int ClampActiveTabIndex(DockAreaId dockAreaId) const;

        /// <summary>
        /// DockArea のプレビュー矩形を返す。
        /// </summary>
        [[nodiscard]] RECT GetDockAreaPreviewRect(HWND parentWindow, DockAreaId dockAreaId) const;

        /// <summary>
        /// Dock 先プレビュー表示を現在状態へ同期する。
        /// </summary>
        void UpdateDockPreviewWindow(HWND parentWindow);

        /// <summary>
        /// GroupBox を背面へ配置し、同じ親を持つ中身コントロールを前面に保つ。
        /// </summary>
        void SendGroupBoxesToBack();

        /// <summary>
        /// 中間再描画を抑制して child window を配置する。
        /// </summary>
        /// <param name="window">配置対象の child window。</param>
        /// <param name="x">親クライアント座標 X。</param>
        /// <param name="y">親クライアント座標 Y。</param>
        /// <param name="width">幅。</param>
        /// <param name="height">高さ。</param>
        void MoveChildWindowNoRedraw(HWND window, int x, int y, int width, int height) const;

        /// <summary>
        /// 配置更新後に親と child window 群を同期再描画する。
        /// </summary>
        /// <param name="parentWindow">親となる Editor メインウィンドウ。</param>
        void RedrawLayout(HWND parentWindow) const;

        /// <summary>
        /// SceneView host の現在サイズを反映する。
        /// </summary>
        /// <returns>サイズが変化した場合は true。</returns>
        bool UpdateSceneViewHostSize();

        /// <summary>
        /// DPI に合わせた UI フォントを作成して各 child window へ適用する。
        /// </summary>
        /// <param name="parentWindow">DPI の基準にする親ウィンドウ。</param>
        /// <returns>DPI リソースを更新した場合は true。</returns>
        bool RefreshDpiResources(HWND parentWindow);

        /// <summary>
        /// EditorShell が管理する child window 群を列挙する。
        /// </summary>
        /// <returns>管理対象 child window の一覧。</returns>
        [[nodiscard]] std::array<HWND, 62> CollectControls() const;

        /// <summary>
        /// 指定値を現在 DPI に合わせて拡大縮小する。
        /// </summary>
        /// <param name="value">96 DPI 基準の値。</param>
        /// <returns>DPI 適用後の値。</returns>
        [[nodiscard]] int ScaleMetric(int value) const;

        /// <summary>
        /// フローティングビュー用 window procedure。
        /// </summary>
        static LRESULT CALLBACK FloatingPanelWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

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
        struct DockNode
        {
            DockNodeKind kind = DockNodeKind::Leaf;
            DockSplitOrientation splitOrientation = DockSplitOrientation::Horizontal;
            float splitRatio = 0.5f;
            DockNodeId firstChild = -1;
            DockNodeId secondChild = -1;
            std::vector<EditorPanelId> panels{};
            int activeTabIndex = 0;
            RECT rect{};
            HWND tabControl = nullptr;
        };

        struct DockGuideTarget
        {
            DockGuideTargetKind kind = DockGuideTargetKind::None;
            DockNodeId dockNodeId = -1;
            RECT previewRect{};
        };

        struct FloatingPanelCreateParams
        {
            EditorShell* shell = nullptr;
            EditorPanelId panelId = EditorPanelId::SceneView;
        };

        struct FloatingPanelWindowData
        {
            EditorShell* shell = nullptr;
            EditorPanelId panelId = EditorPanelId::SceneView;
        };

        HFONT m_defaultFont = nullptr;
        UINT m_currentDpi = 96;
        bool m_ownsDefaultFont = false;
        HWND m_parentWindow = nullptr;
        HWND m_leftTopDockTab = nullptr;
        HWND m_leftBottomDockTab = nullptr;
        HWND m_centerDockTab = nullptr;
        HWND m_rightDockTab = nullptr;
        std::vector<HWND> m_dynamicDockTabs{};
        HWND m_dockPreviewWindow = nullptr;
        std::array<HWND, 9> m_dockGuideWindows{};
        HWND m_hierarchyFloatingWindow = nullptr;
        HWND m_assetsFloatingWindow = nullptr;
        HWND m_sceneViewFloatingWindow = nullptr;
        HWND m_inspectorFloatingWindow = nullptr;
        HWND m_logOutputFloatingWindow = nullptr;
        HWND m_hierarchyPanel = nullptr;
        HWND m_assetsPanel = nullptr;
        HWND m_inspectorPanel = nullptr;
        HWND m_sceneViewPanel = nullptr;
        HWND m_sceneViewPlanLabel = nullptr;
        HWND m_projectSummaryLabel = nullptr;
        HWND m_projectSceneListBox = nullptr;
        HWND m_projectSceneDetailLabel = nullptr;
        HWND m_sceneViewHost = nullptr;
        HWND m_sceneViewSizeLabel = nullptr;
        HWND m_logOutputPanel = nullptr;
        HWND m_logOutputTabControl = nullptr;
        HWND m_logClearButton = nullptr;
        HWND m_logCopyButton = nullptr;
        HWND m_logFilterEdit = nullptr;
        HWND m_logListBox = nullptr;
        HWND m_assetsListView = nullptr;
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
        HWND m_spriteRefDropHighlight = nullptr;
        HWND m_spriteRefEdit = nullptr;
        HWND m_scriptAssetLabel = nullptr;
        HWND m_scriptAssetEdit = nullptr;
        HWND m_scriptCreateButton = nullptr;
        HWND m_scriptAssignButton = nullptr;
        HWND m_scriptClearButton = nullptr;
        HWND m_spriteComponentActionButton = nullptr;
        std::uint32_t m_sceneViewWidth = 0;
        std::uint32_t m_sceneViewHeight = 0;
        std::vector<EditorPanelId> m_leftTopDockPanels{};
        std::vector<EditorPanelId> m_leftBottomDockPanels{};
        std::vector<EditorPanelId> m_centerDockPanels{};
        std::vector<EditorPanelId> m_rightDockPanels{};
        std::vector<DockNode> m_dockNodes{};
        DockNodeId m_rootDockNodeId = -1;
        DockGuideTarget m_currentGuideTarget{};
        int m_leftTopActiveTabIndex = 0;
        int m_leftBottomActiveTabIndex = 0;
        int m_centerActiveTabIndex = 0;
        int m_rightActiveTabIndex = 0;
        int m_leftPaneWidth = 260;
        int m_rightPaneWidth = 300;
        int m_leftTopHeight = 280;
        DockDragKind m_dragKind = DockDragKind::None;
        std::optional<EditorPanelId> m_dragPanelId{};
        bool m_hasDockPreview = false;
        RECT m_dockPreviewRect{};
        POINT m_dragStartScreenPoint{};
        DockNodeId m_dragSplitNodeId = -1;
        float m_dragStartSplitRatio = 0.5f;
        int m_dragStartLeftPaneWidth = 0;
        int m_dragStartRightPaneWidth = 0;
        int m_dragStartLeftTopHeight = 0;
        bool m_layoutInitialized = false;
        int m_lastLayoutClientWidth = 0;
        int m_lastLayoutClientHeight = 0;
        UINT m_lastLayoutDpi = 0;
    };
}
