# ExecPlan: issue-230 Editor view flicker reduction

## 目的

Inspector / LogOutput / Dock レイアウト更新時の不要な Win32 再描画を減らし、ビューのちらつきを抑制する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/Application.cpp`, `Editor/Source/EditorShell.cpp`, `Editor/Source/InspectorPanelController.cpp`, `Editor/Source/LogOutputPanelController.cpp`
- 変更対象クラス: `Xelqoria::Editor::Application`, `Xelqoria::Editor::EditorShell`, `Xelqoria::Editor::InspectorPanelController`, `Xelqoria::Editor::LogOutputPanelController`
- 影響する機能: Inspector 表示同期、LogOutput 表示更新、Dock レイアウト再描画

## 前提

- ブランチ運用ルール: `docs/agents/branch-rules.md`
- この作業ブランチ: `issue-230`
- UI の見た目、文言、機能、保存形式、公開 API は変更しない
- 変更は Editor の Win32 UI 更新最適化に閉じる

## 実装方針

Inspector は同一値の `SetWindowTextW` / `ShowWindow` / checkbox 更新を避け、編集確定時の二重 Refresh を解消する。

LogOutput は ListBox の全行再構築中だけ `WM_SETREDRAW` を止め、空状態の瞬間描画を抑える。

Dock レイアウトは全 child を erase する再描画を避け、Editor 親 window に `WS_CLIPCHILDREN` を付けて child 領域の背景塗りを抑える。

## 作業手順

1. Inspector / LogOutput / Dock レイアウトの更新経路を確認する
2. Inspector の重複 Refresh と同一値更新を抑制する
3. LogOutput ListBox 更新中の再描画を抑制する
4. EditorShell / Application の全 child erase 経路を弱める
5. ビルドとテストを実行する

## 実施メモ

- Dock splitter ドラッグ中の child window 再配置で `SWP_NOREDRAW` / `SWP_NOCOPYBITS` を使い、中間の背景消去と旧ビットコピーを抑制する
- 配置反映後は親 window から `RDW_ALLCHILDREN` / `RDW_UPDATENOW` / `RDW_NOERASE` で child を含めて同期再描画する

## 検証方法

- 実行するビルド: `tools/wsl/build.sh Editor/Xelqoria.Editor.vcxproj`
- 実行するテスト: `tools/wsl/test.sh --no-build`
- 手動確認項目: Inspector 編集、LogOutput 連続追加、Dock splitter / タブ切替 / Floating / 再ドッキング / View menu 復帰

## 完了条件

- Inspector / LogOutput / Dock レイアウト更新時の不要な再描画が抑制されている
- 既存の見た目、文言、操作、保存形式が維持されている
- ビルドが通る
- 関連テストが通る
- レイヤー責務に違反していない
- 不要な変更が含まれていない
