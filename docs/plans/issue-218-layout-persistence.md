# ExecPlan: issue-218 レイアウト保存・復元を追加する

## 目的

Editor の Dock / Floating レイアウトを終了時に保存し、次回起動時に復元できるようにする。

## 対象範囲

- 変更対象プロジェクト: Editor
- 変更対象ファイル: `Editor/Source/EditorShell.h`, `Editor/Source/EditorShell.cpp`, `Editor/Source/Application.cpp`
- 変更対象クラス: `Xelqoria::Editor::EditorShell`, `Xelqoria::Editor::Application`
- 影響する機能: Dock 配置、タブ順、Floating window 位置・サイズ、Dock 分割比率

## 前提

- parent issue: issue-214
- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-214`
- この child issue のブランチ: `issue-218`
- PR の向き: `issue-218` から `issue-214`

## 実装方針

レイアウト保存・復元は Win32 Editor UI を所有する `EditorShell` に閉じる。保存形式は `Saved/EditorLayout.txt` の行指向テキストとし、Dock tree、leaf のタブ順、active tab、split ratio、Floating group の矩形とタブ順を保存する。

表示・非表示状態は保存対象にしない。復元時に保存データから欠けているビューは既定 Dock 位置へ戻し、起動後に全ビューへ到達できる状態にする。

## 作業手順

1. 関連コードを確認する
2. `EditorShell` に保存・復元 API とシリアライズ処理を追加する
3. `Application` の初期化時と終了時に保存・復元を呼び出す
4. ビルドを実行する
5. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `tools/wsl/build.sh Editor/Xelqoria.Editor.vcxproj`
- 実行するテスト: 追加なし
- 手動確認項目: Dock / Floating / タブ順 / 分割位置が再起動後に復元され、閉じたビューは既定位置へ戻る

## 完了条件

- レイアウト保存・復元が実装されている
- ビルドが通る
- レイヤー責務に違反していない
- 不要な変更が含まれていない
- `branch-rules.md` に従った PR が作成されている
