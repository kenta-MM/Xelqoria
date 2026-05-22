# ExecPlan: issue-272 固定レイアウトで Assets / Hierarchy / Scene / Inspector / Console を配置する

## 目的

Editor メイン画面に Assets / Hierarchy / Scene / Inspector / Console / StatusBar を固定レイアウトで配置する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/EditorShell.cpp`
- 変更対象クラス: `EditorShell`
- 影響する機能: Editor メイン画面の初期配置、Dock 操作、Scene / Game タブ表示

## 前提

- parent issue: #269
- 先行 issue: #270, #271
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-269`
- この child issue のブランチ: `issue-272`
- PR の向き: `issue-272` -> `issue-269`

## 実装方針

`issue-271` までの TopBar / StatusBar を前提に、既存 Dock tree の初期状態を固定レイアウトとして扱う。Assets を左上、Hierarchy を左下、Scene / Game を中央、Inspector を右、Console を下に配置し、今回スコープ外のパネルドラッグ操作は無効化する。

## 作業手順

1. `issue-271` をこのブランチへローカルマージする
2. 初期 Dock tree を固定レイアウト順へ変更する
3. SceneView のタブ表示を Scene / Game に見えるよう調整する
4. パネルドラッグによる Dock 操作を無効化する
5. ビルドまたはレイヤー依存チェックを実行する
6. PR を作成する

## 検証方法

- 実行するビルド: `dotnet build tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug`
- 実行するテスト: LayerDependencyChecker
- 手動確認項目: Assets 左上、Hierarchy 左下、Scene / Game 中央、Inspector 右、Console 下、StatusBar 最下部に表示される

## 完了条件

- Assets が左上に表示される
- Hierarchy が左下に表示される
- Scene / Game が中央に表示される
- Inspector が右に表示される
- Console が下に表示される
- StatusBar が最下部に表示される
- 各パネルにヘッダーと薄い境界線がある
- 既存のレイヤー責務を破壊していない
- `issue-272` から `issue-269` への PR が作成されている
