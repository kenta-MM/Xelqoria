# ExecPlan: issue-256 Editor タブの active / inactive / hover 表示をテーマ対応する

## 目的

Editor のタブ表示を Xelqoria Dark テーマに合わせ、active / inactive / hover を視覚的に区別できるようにする。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/Application.cpp`, `Editor/Source/EditorShell.h`, `Editor/Source/EditorShell.cpp`
- 変更対象クラス: `Application`, `EditorShell`
- 影響する機能: Dock / Floating / Console の TabControl 外観

## 前提

- parent issue: #253
- 前提 child issue: #254, #255
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-253`
- この child issue のブランチ: `issue-256`
- PR の向き: `issue-256` -> `issue-253`

## 実装方針

既存の Docking データ構造やタブ切り替え処理は維持する。TabControl を owner draw にし、`EditorThemes::XelqoriaDark` の `PanelBackground` / `PanelHeaderBackground` / `Hover` / `Accent` / `TextPrimary` / `TextSecondary` を使って active / inactive / hover を描き分ける。

hover 表示は TabControl のサブクラスでマウス移動時に再描画を促すだけに留め、ドラッグやタブ切り替えの挙動は変更しない。

## 作業手順

1. TabControl の生成箇所と通知処理を確認する
2. Dock / Floating / Console の TabControl を owner draw にする
3. `EditorShell` に owner draw 描画処理を追加する
4. `Application` から `EditorShell` の draw handler を呼ぶ
5. 依存チェックと可能なビルドを実行する

## 検証方法

- 実行するビルド: `dotnet build tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug`
- 実行する依存チェック: `tools/LayerDependencyChecker/bin/Debug/net8.0/LayerDependencyChecker.exe D:\github\Xelqoria`
- 実行する静的確認: `git diff --check`
- 実行するビルド: `msbuild Xelqoria.slnx /t:Xelqoria.Editor /p:Configuration=Debug /p:Platform=x64`（環境に MSBuild がある場合）
- 実行するテスト: なし

## 完了条件

- タブの active 状態が視覚的に分かる
- タブの inactive 状態が active と区別できる
- タブの hover 状態が active / inactive と区別できる
- タブ表示が `EditorTheme` の色を参照している
- 既存の Docking 操作やタブ切り替え挙動を変更していない
- PR が `issue-253` 向けに作成されている
