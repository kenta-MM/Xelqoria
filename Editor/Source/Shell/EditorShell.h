#pragma once

#include <Windows.h>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ICursor.h"
#include "InputSystem.h"
#include "Panels/EditorPanelHostContext.h"
#include "Shell/EditorPanelId.h"
#include "SceneViewSurface.h"

namespace Xelqoria::Editor
{
    class AssetsPanelView;
    class HierarchyPanelView;
    class InspectorPanelView;
    class IEditorPanelView;
    class LogOutputPanelView;
    class SceneViewPanelView;
    class EditorDockingController;

    /// <summary>
    /// Editor 固定 UI の Win32 child window 群とレイアウトを管理する。
    /// </summary>
    class EditorShell
    {
        friend class EditorDockingController;

    public:
        /// <summary>
        /// EditorShell を生成する。
        /// </summary>
        EditorShell();

        /// <summary>
        /// EditorShell 用 child window 群を生成する。
        /// </summary>
        /// <param name="parentWindow">親となる Editor メインウィンドウ。</param>
        /// <param name="hInstance">Windows アプリケーションのインスタンスハンドル。</param>
        /// <param name="cursor">カーソル形状を切り替える Platform 実装。</param>
        /// <returns>生成に成功した場合は true。</returns>
        bool Initialize(HWND parentWindow, HINSTANCE hInstance, Platform::ICursor& cursor);

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
        /// 現在アクティブでない Dock / Floating パネルの child window を非表示に同期する。
        /// </summary>
        void HideInactivePanelControls();

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
        /// Hierarchy パネルの検索入力欄を取得する。
        /// </summary>
        /// <returns>Hierarchy 検索入力欄の HWND。</returns>
        [[nodiscard]] HWND GetHierarchySearchEdit() const;

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
        /// Assets ListView の Header コントロールへテーマ描画を適用する。
        /// </summary>
        void ConfigureAssetsListHeaderTheme() const;

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
        /// SpriteComponent の Material 参照を Material タブで開くボタンを取得する。
        /// </summary>
        /// <returns>Material Open ボタンの HWND。</returns>
        [[nodiscard]] HWND GetMaterialOpenButton() const;

        /// <summary>
        /// SpriteRef ドロップ先ハイライトを取得する。
        /// </summary>
        /// <returns>SpriteRef ドロップ先ハイライトの HWND。</returns>
        [[nodiscard]] HWND GetSpriteRefDropHighlight() const;

        /// <summary>
        /// Material パネルの共有編集注意ラベルを取得する。
        /// </summary>
        /// <returns>共有編集注意ラベルの HWND。</returns>
        [[nodiscard]] HWND GetMaterialSharedNoticeLabel() const;

        /// <summary>
        /// Material 詳細セクション見出しラベルを取得する。
        /// </summary>
        /// <returns>Material 詳細セクション見出しの HWND。</returns>
        [[nodiscard]] HWND GetMaterialDetailsSectionLabel() const;

        /// <summary>
        /// Material 詳細項目ラベル群を取得する。
        /// </summary>
        /// <returns>Material 詳細ラベル配列。</returns>
        [[nodiscard]] const std::array<HWND, 5>& GetMaterialDetailLabels() const;

        /// <summary>
        /// Material 詳細入力欄群を取得する。
        /// </summary>
        /// <returns>Material 詳細入力欄配列。</returns>
        [[nodiscard]] const std::array<HWND, 5>& GetMaterialDetailEditControls() const;

        /// <summary>
        /// Material Texture 欄のドロップ先ハイライトを取得する。
        /// </summary>
        /// <returns>Material Texture ドロップ先ハイライトの HWND。</returns>
        [[nodiscard]] HWND GetMaterialTextureDropHighlight() const;

        /// <summary>
        /// Material Texture 参照ボタンを取得する。
        /// </summary>
        /// <returns>Material Texture 参照ボタンの HWND。</returns>
        [[nodiscard]] HWND GetMaterialTextureBrowseButton() const;

        /// <summary>
        /// Material Tint 色ボタンを取得する。
        /// </summary>
        /// <returns>Material Tint 色ボタンの HWND。</returns>
        [[nodiscard]] HWND GetMaterialTintColorButton() const;

        /// <summary>
        /// Material OutlineEnabled チェックボックスを取得する。
        /// </summary>
        /// <returns>Material OutlineEnabled チェックボックスの HWND。</returns>
        [[nodiscard]] HWND GetMaterialOutlineEnabledCheckBox() const;

        /// <summary>
        /// Material OutlineColor 色ボタンを取得する。
        /// </summary>
        /// <returns>Material OutlineColor 色ボタンの HWND。</returns>
        [[nodiscard]] HWND GetMaterialOutlineColorButton() const;

        /// <summary>
        /// Collider2D パネルの要約ラベルを取得する。
        /// </summary>
        /// <returns>Collider2D 要約ラベルの HWND。</returns>
        [[nodiscard]] HWND GetCollider2DSummaryLabel() const;

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
        /// Collider2DComponent セクション見出しラベルを取得する。
        /// </summary>
        /// <returns>Collider2DComponent セクション見出しの HWND。</returns>
        [[nodiscard]] HWND GetCollider2DComponentSectionLabel() const;

        /// <summary>
        /// Collider2DComponent enabled チェックボックスを取得する。
        /// </summary>
        /// <returns>enabled チェックボックスの HWND。</returns>
        [[nodiscard]] HWND GetCollider2DEnabledCheckBox() const;

        /// <summary>
        /// Collider2DComponent isTrigger チェックボックスを取得する。
        /// </summary>
        /// <returns>isTrigger チェックボックスの HWND。</returns>
        [[nodiscard]] HWND GetCollider2DTriggerCheckBox() const;

        /// <summary>
        /// Collider2D shapeType ラベルを取得する。
        /// </summary>
        /// <returns>shapeType ラベルの HWND。</returns>
        [[nodiscard]] HWND GetCollider2DShapeTypeLabel() const;

        /// <summary>
        /// Collider2D shapeType 表示欄を取得する。
        /// </summary>
        /// <returns>shapeType 表示欄の HWND。</returns>
        [[nodiscard]] HWND GetCollider2DShapeTypeEdit() const;

        /// <summary>
        /// Collider2D offset ラベルを取得する。
        /// </summary>
        /// <returns>offset ラベルの HWND。</returns>
        [[nodiscard]] HWND GetCollider2DOffsetLabel() const;

        /// <summary>
        /// Collider2D size ラベルを取得する。
        /// </summary>
        /// <returns>size ラベルの HWND。</returns>
        [[nodiscard]] HWND GetCollider2DSizeLabel() const;

        /// <summary>
        /// Collider2D rotation ラベルを取得する。
        /// </summary>
        /// <returns>rotation ラベルの HWND。</returns>
        [[nodiscard]] HWND GetCollider2DRotationLabel() const;

        /// <summary>
        /// Collider2D rotation 表示欄を取得する。
        /// </summary>
        /// <returns>rotation 表示欄の HWND。</returns>
        [[nodiscard]] HWND GetCollider2DRotationEdit() const;

        /// <summary>
        /// Collider2D 数値入力欄群を取得する。
        /// </summary>
        /// <returns>offset.x, offset.y, size.x, size.y の入力欄配列。</returns>
        [[nodiscard]] const std::array<HWND, 4>& GetCollider2DEditControls() const;

        /// <summary>
        /// Collider2D Scene 編集ボタンを取得する。
        /// </summary>
        /// <returns>Collider2D Scene 編集ボタンの HWND。</returns>
        [[nodiscard]] HWND GetCollider2DEditButton() const;

        /// <summary>
        /// Collider2DComponent 操作ボタンを取得する。
        /// </summary>
        /// <returns>Collider2DComponent 操作用ボタンの HWND。</returns>
        [[nodiscard]] HWND GetCollider2DComponentActionButton() const;

        /// <summary>
        /// Add Component ボタンを取得する。
        /// </summary>
        /// <returns>Add Component ボタンの HWND。</returns>
        [[nodiscard]] HWND GetAddComponentButton() const;

        /// <summary>
        /// SceneView の状態ラベルを取得する。
        /// </summary>
        /// <returns>SceneView サイズラベルの HWND。</returns>
        [[nodiscard]] HWND GetSceneViewSizeLabel() const;

        /// <summary>
        /// Script ビルド後に再生を開始するボタンを取得する。
        /// </summary>
        /// <returns>ビルドして開始ボタンの HWND。</returns>
        [[nodiscard]] HWND GetBuildAndPlayButton() const;

        /// <summary>
        /// Script 再生を停止または再開するボタンを取得する。
        /// </summary>
        /// <returns>停止/再開ボタンの HWND。</returns>
        [[nodiscard]] HWND GetPauseResumePlayButton() const;

        /// <summary>
        /// Script 再生を終了するボタンを取得する。
        /// </summary>
        /// <returns>終了ボタンの HWND。</returns>
        [[nodiscard]] HWND GetEndPlayButton() const;

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
        /// SceneView / Game Preview の描画先境界を取得する。
        /// </summary>
        /// <returns>描画先 surface。</returns>
        [[nodiscard]] SceneViewSurface GetSceneViewSurface() const;

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
        /// Hierarchy Panel View を取得する。
        /// </summary>
        /// <returns>Hierarchy Panel View。</returns>
        [[nodiscard]] HierarchyPanelView& GetHierarchyPanelView() const;

        /// <summary>
        /// Assets Panel View を取得する。
        /// </summary>
        /// <returns>Assets Panel View。</returns>
        [[nodiscard]] AssetsPanelView& GetAssetsPanelView() const;

        /// <summary>
        /// Inspector Panel View を取得する。
        /// </summary>
        /// <returns>Inspector Panel View。</returns>
        [[nodiscard]] InspectorPanelView& GetInspectorPanelView() const;

        /// <summary>
        /// SceneView Panel View を取得する。
        /// </summary>
        /// <returns>SceneView Panel View。</returns>
        [[nodiscard]] SceneViewPanelView& GetSceneViewPanelView() const;

        /// <summary>
        /// LogOutput Panel View を取得する。
        /// </summary>
        /// <returns>LogOutput Panel View。</returns>
        [[nodiscard]] LogOutputPanelView& GetLogOutputPanelView() const;

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
        /// EditorShell が所有する owner draw control の描画を処理する。
        /// </summary>
        /// <param name="drawItemParameter">WM_DRAWITEM の LPARAM。</param>
        /// <returns>描画を処理した場合は true。</returns>
        bool HandleDrawItem(LPARAM drawItemParameter) const;

        /// <summary>
        /// 現在開いているプロジェクト画面レイアウトを初期状態へ戻す。
        /// </summary>
        void ResetDockLayout();

        /// <summary>
        /// 指定ビューを既定の Dock 位置へ表示する。
        /// </summary>
        /// <param name="panelId">再表示するビュー。</param>
        void ShowPanelAtDefaultDock(EditorPanelId panelId);

        /// <summary>
        /// 指定ビューがある Dock または Floating のタブをアクティブにする。
        /// </summary>
        /// <param name="panelId">アクティブ化するビュー。</param>
        void ActivatePanel(EditorPanelId panelId);

        /// <summary>
        /// 現在の Dock / Floating レイアウトを保存する。
        /// </summary>
        /// <param name="layoutPath">保存先ファイル。</param>
        /// <returns>保存に成功した場合は true。</returns>
        [[nodiscard]] bool SaveLayout(const std::filesystem::path& layoutPath) const;

        /// <summary>
        /// 保存済み Dock / Floating レイアウトを復元する。
        /// </summary>
        /// <param name="layoutPath">復元元ファイル。</param>
        /// <returns>復元に成功した場合は true。</returns>
        [[nodiscard]] bool LoadLayout(const std::filesystem::path& layoutPath);

    private:
        /// <summary>
        /// レイアウト計算結果を保持する。
        /// </summary>
        struct LayoutMetrics;

        /// <summary>
        /// 指定パネルの HWND 群を表示または非表示にする。
        /// </summary>
        void ShowPanelControls(EditorPanelId panelId, bool visible) const;

        /// <summary>
        /// 指定パネルの HWND 群を指定親へ付け替える。
        /// </summary>
        void SetPanelParent(EditorPanelId panelId, HWND parentWindow) const;

        /// <summary>
        /// 指定 Panel の View 境界を返す。
        /// </summary>
        [[nodiscard]] IEditorPanelView& GetPanelView(EditorPanelId panelId) const;

        /// <summary>
        /// GroupBox を背面へ配置し、同じ親を持つ中身コントロールを前面に保つ。
        /// </summary>
        void SendGroupBoxesToBack();

        /// <summary>
        /// 中間再描画を抑制して child window を配置する。
        /// </summary>
        void MoveChildWindowNoRedraw(HWND window, int x, int y, int width, int height) const;

        /// <summary>
        /// レイアウト中に蓄積した child window 配置をまとめて反映する。
        /// </summary>
        void FlushLayoutMoves(HWND parentWindow) const;

        /// <summary>
        /// 配置更新後に親と child window 群を同期再描画する。
        /// </summary>
        void RedrawLayout(HWND parentWindow) const;

        /// <summary>
        /// SceneView host の現在サイズを反映する。
        /// </summary>
        [[nodiscard]] bool UpdateSceneViewHostSize();

        /// <summary>
        /// Editor 標準ボタンの owner draw 描画を行う。
        /// </summary>
        /// <param name="drawItem">描画情報。</param>
        /// <returns>描画を処理した場合は true。</returns>
        bool DrawEditorButton(const DRAWITEMSTRUCT& drawItem) const;

        /// <summary>
        /// Hierarchy ListBox の owner draw 描画を行う。
        /// </summary>
        /// <param name="drawItem">描画情報。</param>
        /// <returns>描画を処理した場合は true。</returns>
        bool DrawHierarchyListBoxItem(const DRAWITEMSTRUCT& drawItem) const;

        /// <summary>
        /// Inspector セクション見出しの owner draw 描画を行う。
        /// </summary>
        /// <param name="drawItem">描画情報。</param>
        /// <returns>描画を処理した場合は true。</returns>
        bool DrawInspectorSectionLabel(const DRAWITEMSTRUCT& drawItem) const;

        /// <summary>
        /// Assets ListView の custom draw 色をテーマに沿って設定する。
        /// </summary>
        /// <param name="customDrawParameter">NMLVCUSTOMDRAW の LPARAM。</param>
        /// <returns>custom draw の戻り値。対象外の場合は空。</returns>
        [[nodiscard]] std::optional<LRESULT> DrawAssetsListViewItem(LPARAM customDrawParameter) const;

        /// <summary>
        /// Assets ListView のヘッダー custom draw をテーマに沿って描画する。
        /// </summary>
        /// <param name="customDrawParameter">NMCUSTOMDRAW の LPARAM。</param>
        /// <returns>custom draw の戻り値。対象外の場合は空。</returns>
        [[nodiscard]] std::optional<LRESULT> DrawAssetsListViewHeader(LPARAM customDrawParameter) const;

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
        [[nodiscard]] std::vector<HWND> CollectControls() const;

        /// <summary>
        /// 指定値を現在 DPI に合わせて拡大縮小する。
        /// </summary>
        /// <param name="value">96 DPI 基準の値。</param>
        /// <returns>DPI 適用後の値。</returns>
        [[nodiscard]] int ScaleMetric(int value) const;

        /// <summary>
        /// Editor 用 TabControl の hover 再描画を処理する。
        /// </summary>
        static LRESULT CALLBACK EditorTabControlSubclassProc(
            HWND window,
            UINT message,
            WPARAM wParam,
            LPARAM lParam,
            UINT_PTR subclassId,
            DWORD_PTR referenceData);

        /// <summary>
        /// Editor 親ウィンドウのテーマ背景と標準コントロール色を処理する。
        /// </summary>
        static LRESULT CALLBACK ParentWindowSubclassProc(
            HWND window,
            UINT message,
            WPARAM wParam,
            LPARAM lParam,
            UINT_PTR subclassId,
            DWORD_PTR referenceData);

        /// <summary>
        /// 親ウィンドウへ届くテーマ関連メッセージを処理する。
        /// </summary>
        /// <param name="message">Win32 メッセージ。</param>
        /// <param name="wParam">メッセージ WPARAM。</param>
        /// <param name="lParam">メッセージ LPARAM。</param>
        /// <returns>処理結果。未処理の場合は空。</returns>
        [[nodiscard]] std::optional<LRESULT> HandleThemeMessage(UINT message, WPARAM wParam, LPARAM lParam) const;

        /// <summary>
        /// Inspector 入力欄かを判定する。
        /// </summary>
        /// <param name="window">判定対象 HWND。</param>
        /// <returns>Inspector 入力欄の場合は true。</returns>
        [[nodiscard]] bool IsInspectorInputControl(HWND window) const;

        /// <summary>
        /// Inspector 補助ラベルかを判定する。
        /// </summary>
        /// <param name="window">判定対象 HWND。</param>
        /// <returns>Inspector 補助ラベルの場合は true。</returns>
        [[nodiscard]] bool IsInspectorSecondaryLabel(HWND window) const;

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

        HFONT m_defaultFont = nullptr;
        HBRUSH m_windowBackgroundBrush = nullptr;
        HBRUSH m_panelBackgroundBrush = nullptr;
        HBRUSH m_inputBackgroundBrush = nullptr;
        UINT m_currentDpi = 96;
        bool m_ownsDefaultFont = false;
        HWND m_parentWindow = nullptr;
        std::unique_ptr<EditorDockingController> m_dockingController{};
        HWND m_workspaceBackground = nullptr;
        HWND m_topBar = nullptr;
        HWND m_topBarProjectButton = nullptr;
        HWND m_topBarPlayButton = nullptr;
        HWND m_topBarLayoutButton = nullptr;
        HWND m_statusBar = nullptr;
        EditorPanelHostContext m_panelHostContext{};
        std::unique_ptr<HierarchyPanelView> m_hierarchyPanelView{};
        std::unique_ptr<AssetsPanelView> m_assetsPanelView{};
        std::unique_ptr<InspectorPanelView> m_inspectorPanelView{};
        std::unique_ptr<SceneViewPanelView> m_sceneViewPanelView{};
        std::unique_ptr<LogOutputPanelView> m_logOutputPanelView{};
        bool m_panelViewsInitialized = false;
        bool m_layoutInitialized = false;
        int m_lastLayoutClientWidth = 0;
        int m_lastLayoutClientHeight = 0;
        UINT m_lastLayoutDpi = 0;
        mutable std::vector<EditorPanelLayoutMove> m_pendingLayoutMoves{};
        Platform::ICursor* m_cursor = nullptr;
    };
}
