# ExecPlan: issue-277 Console ビューでダミーログ表示とフィルター UI を実装する

## 目的

Console ビューに実ログ基盤へ接続しないダミーログ表示と、ログ種別ごとのフィルター UI を実装する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/EditorShell.cpp`, `Editor/Source/LogOutputPanelController.h`, `Editor/Source/LogOutputPanelController.cpp`
- 変更対象クラス: `EditorShell`, `LogOutputPanelController`
- 影響する機能: Console パネルの初期表示、ログフィルター、ログ描画

## 前提

- parent issue: #269
- 先行 issue: #270, #271, #272
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-269`
- この child issue のブランチ: `issue-277`
- PR の向き: `issue-277` -> `issue-269`

## 実装方針

`issue-272` の固定レイアウトを前提に、Console パネル内のタブを「すべて / ログ / 警告 / エラー」の severity フィルターとして扱う。実ログ基盤には接続せず、起動時にダミーログを投入する。

## 作業手順

1. `issue-272` をこのブランチへローカルマージする
2. Console タブ表示を severity フィルターへ変更する
3. ダミーログ 5 件を初期投入する
4. タイムスタンプ付きの表示行へ整形する
5. クリア、検索、コピー、色分け描画を維持する
6. ビルドまたはレイヤー依存チェックを実行する
7. PR を作成する

## 検証方法

- 実行するビルド: `dotnet build tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug`
- 実行するテスト: LayerDependencyChecker
- 手動確認項目: ダミーログ、タイムスタンプ、すべて / ログ / 警告 / エラー切替、クリアが動作する

## 完了条件

- Console にダミーログが表示される
- Xelqoria Editor started / Project loaded / Assets scan completed / Scene initialized / Renderer ready が表示される
- すべて / ログ / 警告 / エラー のフィルターを切り替えられる
- クリアボタンで表示ログをクリアできる
- タイムスタンプが表示される
- ログ種別ごとの表示切替ができる
- 既存のレイヤー責務を破壊していない
- `issue-277` から `issue-269` への PR が作成されている
