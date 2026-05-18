# ExecPlan: issue-217 「表示」ボタンとビュー再表示機能を追加する

## 目的

メインウィンドウに「表示」メニューを追加し、非表示になった各ビューを既定のドッキング位置へ再表示できるようにする。

## 対象範囲

- 変更対象プロジェクト: Editor
- 変更対象ファイル: `Editor/Source/Application.cpp`, `Editor/Source/Application.h`, `Editor/Source/EditorShell.cpp`, `Editor/Source/EditorShell.h`
- 変更対象クラス: `Xelqoria::Editor::Application`, `Xelqoria::Editor::EditorShell`
- 影響する機能: メニューバー、ビュー再表示、既定 Dock 位置復帰

## 前提

- parent issue: issue-214
- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-214`
- この child issue のブランチ: `issue-217`
- PR の向き: `issue-217` から `issue-214`

## 実装方針

Editor 専用の Win32 UI シェルである `EditorShell` 内にビュー再表示と既定 Dock 位置復帰の責務を置く。`Application` はメニュー項目とコマンド振り分けのみ担当する。

表示状態の保存は非スコープのため、永続化は行わない。

## 作業手順

1. 関連コードを確認する
2. メニューバーに `表示` メニューを追加する
3. 各ビューの表示コマンドを追加する
4. `EditorShell` に既定 Dock 位置への再表示 API を追加する
5. ビルドまたは関連テストを実行する
6. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `tests/Editor/Xelqoria.Tests.Editor.vcxproj` Debug x64
- 実行するテスト: `artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`
- 手動確認項目: `表示` メニューから各ビューを再表示し、既定 Dock 位置へ戻ること

## 完了条件

- メインウィンドウの `プロジェクト` メニュー右側に `表示` メニューが表示される
- `表示` メニューから各ビューを再表示できる
- 再表示したビューは既定の Dock 位置に表示される
- レイヤー責務に違反していない
- 不要な変更が含まれていない
- `branch-rules.md` に従った PR が作成されている
