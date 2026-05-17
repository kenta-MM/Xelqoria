# ExecPlan: issue-216 独立ウィンドウ化と独立ウィンドウ内タブ化を整備する

## 目的

タブ化されたビューを独立ウィンドウ化し、独立ウィンドウ内でも複数ビューをタブとしてまとめられるようにする。

## 対象範囲

- 変更対象プロジェクト: Editor
- 変更対象ファイル: `Editor/Source/EditorShell.cpp`, `Editor/Source/EditorShell.h`
- 変更対象クラス: `Xelqoria::Editor::EditorShell`
- 影響する機能: 独立ウィンドウ、独立ウィンドウ内タブ、独立ウィンドウ close 時の非表示化

## 前提

- parent issue: issue-214
- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-214`
- この child issue のブランチ: `issue-216`
- PR の向き: `issue-216` から `issue-214`

## 実装方針

Editor 専用の Win32 UI シェルである `EditorShell` 内に責務を閉じる。

既存の Dock ガイドや配置先仕様は変更せず、floating window 側に複数パネルを保持できるタブグループを追加する。独立ウィンドウを閉じた場合は、その window に含まれるビューを Dock へ戻さず非表示にする。

## 作業手順

1. 関連コードを確認する
2. floating window ごとの panel group と TabControl を追加する
3. floating window 内のタブ切り替えで表示 panel を切り替える
4. drag/drop で既存 floating window へ panel をまとめられるようにする
5. floating window close 時に含まれる panel を非表示にする
6. ビルドまたは関連テストを実行する
7. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `tests/Editor/Xelqoria.Tests.Editor.vcxproj` Debug x64
- 実行するテスト: `artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`
- 手動確認項目: 独立ウィンドウ化、メインウィンドウ外移動、独立ウィンドウ内タブ化、close 時の非表示化

## 完了条件

- タブ化されたビューをドラッグして独立ウィンドウ化できる
- 独立ウィンドウ化したビューをメインウィンドウ外へ移動できる
- 独立ウィンドウ内で複数ビューをタブ化できる
- 独立ウィンドウ化したビューを閉じると、そのビューは非表示になる
- レイヤー責務に違反していない
- 不要な変更が含まれていない
- `branch-rules.md` に従った PR が作成されている
