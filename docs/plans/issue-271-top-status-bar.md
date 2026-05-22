# ExecPlan: issue-271 TopBar / StatusBar を画像ベースで実装する

## 目的

Editor メイン画面の上部と最下部に、Xelqoria Dark テーマに合わせた TopBar / StatusBar を固定表示する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/EditorShell.h`, `Editor/Source/EditorShell.cpp`
- 変更対象クラス: `EditorShell`
- 影響する機能: Editor メイン画面の上部操作領域、最下部状態表示、固定レイアウトの使用可能領域

## 前提

- parent issue: #269
- 先行 issue: #270
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-269`
- この child issue のブランチ: `issue-271`
- PR の向き: `issue-271` -> `issue-269`

## 実装方針

`issue-270` のテーマ基盤をローカルマージして利用する。TopBar / StatusBar は Editor の OS ネイティブ UI シェルに閉じ、`Editor.UI` や Runtime 側へ Win32 実装を持ち込まない。

## 作業手順

1. `issue-270` をこのブランチへローカルマージする
2. TopBar / StatusBar 用の独自描画 child window を追加する
3. TopBar に主要操作ボタンを配置する
4. StatusBar を最下部に固定し、既存パネル領域から除外する
5. ビルドまたはレイヤー依存チェックを実行する
6. PR を作成する

## 検証方法

- 実行するビルド: `dotnet build tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug`
- 実行するテスト: LayerDependencyChecker
- 手動確認項目: TopBar が上部、StatusBar が最下部に固定表示され、各パネルと重ならない

## 完了条件

- TopBar がメイン画面上部に固定表示される
- StatusBar がメイン画面最下部に固定表示される
- ボタン、メニュー、状態表示がダークテーマで描画される
- hover / 押下 / 無効状態が視覚的に区別できる
- 既存のレイヤー責務を破壊していない
- `issue-271` から `issue-269` への PR が作成されている
