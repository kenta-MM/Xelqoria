# ExecPlan: issue-219 ドラッグ中のビュー本体追従を安定化する

## 目的

ビュータブをドラッグしている間、ビュー本体を元の配置場所から一時的に取り外し、マウスに追従させる。

## 対象範囲

- 変更対象プロジェクト: Editor
- 変更対象ファイル: `Editor/Source/EditorShell.cpp`, `Editor/Source/EditorShell.h`
- 変更対象クラス: `Xelqoria::Editor::EditorShell`
- 影響する機能: Dock タブドラッグ、ドラッグ中のパネル表示、ドロップ時の配置確定

## 前提

- parent issue: issue-214
- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-214`
- この child issue のブランチ: `issue-219`
- PR の向き: `issue-219` から `issue-214`

## 実装方針

Editor 専用の Win32 UI シェルである `EditorShell` 内に責務を閉じる。

既存の floating panel window と Dock ガイド経路を使い、Dock タブからのドラッグ開始時に対象パネルを Dock ツリーから一時的に取り外す。ドラッグ中は floating window をカーソル位置へ追従させ、ドロップ時は既存の Dock ガイド適用処理で配置を確定する。

## 作業手順

1. 関連コードを確認する
2. Dock タブドラッグ開始時にパネル本体を元 Dock leaf から取り外す
3. ドラッグ中に floating window をカーソルへ追従させる
4. ドラッグ終了時に既存の Dock ガイド処理へ接続する
5. ビルドまたは関連テストを実行する
6. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `tests/Editor/Xelqoria.Tests.Editor.vcxproj` Debug x64
- 実行するテスト: `artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`
- 手動確認項目: タブドラッグ中に対象ビュー本体が元位置に残らず、カーソルへ追従し、ドロップ後に配置が確定すること

## 完了条件

- ビュータブをドラッグ中、ビュー本体がマウスに追従する
- ドラッグ中、元の配置場所にビュー本体が残り続けない
- ドラッグ終了時、ドロップ結果に応じてビューが配置される
- レイヤー責務に違反していない
- 不要な変更が含まれていない
- `branch-rules.md` に従った PR が作成されている
