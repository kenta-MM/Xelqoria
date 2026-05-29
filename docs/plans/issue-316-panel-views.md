# ExecPlan: issue-316 PanelView 実装と EditorShell からの Panel 固有 View 処理分離

## 目的

各 Panel の HWND 群を `XXXPanelView` 境界から扱えるようにし、`EditorShell` の Dock / Floating / Tab 処理から Panel 固有の表示・親変更処理を分離する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/Panels/*PanelView.*`, `Editor/Source/Shell/EditorShell.*`, `Editor/Xelqoria.Editor.vcxproj`, `Editor/Xelqoria.Editor.vcxproj.filters`
- 変更対象クラス: `EditorShell`, 各 `XXXPanelView`
- 影響する機能: Dock 初期配置、Floating、Tab 切り替え、Panel 表示・非表示、Panel 親ウィンドウ変更

## 前提

- parent issue: issue-313
- 先行 issue: issue-314, issue-315
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-313`
- この child issue のブランチ: `issue-316`
- PR の向き: `issue-313`

## 実装方針

既存 UI の見た目と操作を維持するため、まず PanelView を既存 HWND 群へ接続する View 境界として実装する。Dock / Floating / Tab の状態は引き続き `EditorShell` が保持し、PanelView は表示・親変更・control 列挙だけを担当する。

## 作業手順

1. 各 `XXXPanelView.h` / `.cpp` を追加する
2. `EditorShell` に各 PanelView メンバーを追加する
3. 既存 Panel 初期化後に PanelView を生成する
4. `ShowPanelControls` と `SetPanelParent` を PanelView 呼び出しへ変更する
5. project/filter を更新する
6. 依存チェックと静的確認を実行する
7. `issue-313` 向け PR を作成する

## 検証方法

- 実行するビルド: `msbuild Xelqoria.sln /t:Xelqoria.Editor /p:Configuration=Debug /p:Platform=x64`
- 実行するテスト: LayerDependencyChecker
- 手動確認項目: PanelView が Dock / Floating / Tab 状態を持たないこと、`ShowPanelControls` / `SetPanelParent` が PanelView 経由になっていること

## 完了条件

- 対象 Panel すべてに `XXXPanelView.h` / `XXXPanelView.cpp` が追加されている
- 各 `XXXPanelView` が `IEditorPanelView` を実装している
- `EditorShell` が各 PanelView を保持している
- `ShowPanelControls` と `SetPanelParent` 相当の処理が PanelView の `Show` / `SetParent` を呼ぶ形になっている
- Dock / Floating / Tab の仕様を変更していない
- レイヤー責務に違反していない
- `issue-313` に向けた PR が作成されている
