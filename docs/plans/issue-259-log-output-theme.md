# ExecPlan: issue-259 LogOutput / Console のログ表示色をテーマ対応する

## 目的

LogOutput / Console の背景色と通常 / 警告 / エラーログ表示を `Xelqoria Dark` に合わせる。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/LogOutputPanelController.h`, `Editor/Source/LogOutputPanelController.cpp`
- 変更対象クラス: `Xelqoria::Editor::LogOutputPanelController`
- 影響する機能: LogOutput ListBox の owner draw 表示

## 前提

- parent issue: #253
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-253`
- この child issue のブランチ: `issue-259`
- PR の向き: `issue-259` -> `issue-253`

## 実装方針

既存のログ追加 API とフィルタ、コピー、タブ切り替え動作は維持する。

ListBox の owner draw で `EditorThemes::XelqoriaDark` を参照し、通常ログは `TextPrimary`、警告ログは `Warning`、エラーログは `Error`、選択行は `Selection` を使って描画する。

## 作業手順

1. 既存 LogOutput の owner draw 経路を確認する
2. ログ行に通常 / 警告 / エラーの内部 severity を持たせる
3. ListBox 行描画を `Xelqoria Dark` の背景色・テキスト色へ変更する
4. ビルドとレイヤー依存チェックを実行する
5. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `Xelqoria.Editor.vcxproj` Debug x64
- 実行するテスト: `LayerDependencyChecker`
- 手動確認項目: Console / LogOutput の通常 / 警告 / エラー行と選択行がテーマ色で表示される

## 完了条件

- Console / LogOutput の背景色がテーマに沿っている
- 通常ログのテキスト色がテーマに沿っている
- 警告ログのテキスト色が `Warning` に沿っている
- エラーログのテキスト色が `Error` に沿っている
- 選択行がテーマに沿っている
- 既存のログ出力仕様を変更していない
- ビルドが通る
- レイヤー責務に違反していない
- `issue-259` から `issue-253` への PR が作成されている
