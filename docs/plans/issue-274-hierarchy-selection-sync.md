# ExecPlan: issue-274 Hierarchy ビューで Entity 一覧表示と選択同期を実装する

## 目的

現在のシーン内 Entity を Hierarchy ビューに一覧表示し、検索、選択、Scene / Inspector との選択同期を行う。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/EditorShell.h`, `Editor/Source/EditorShell.cpp`, `Editor/Source/HierarchyPanelController.h`, `Editor/Source/HierarchyPanelController.cpp`
- 変更対象クラス: `EditorShell`, `HierarchyPanelController`
- 影響する機能: Hierarchy パネルの表示、検索、選択同期

## 前提

- parent issue: #269
- 先行 issue: #270, #271, #272
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-269`
- この child issue のブランチ: `issue-274`
- PR の向き: `issue-274` -> `issue-269`

## 実装方針

`issue-272` の固定レイアウトを前提に、Hierarchy パネルへ検索入力欄を追加する。現時点の Game::Entity には親子関係 API がないため、現在の Entity 一覧をルートノードとしてツリー風に表示する。

## 作業手順

1. `issue-272` をこのブランチへローカルマージする
2. Hierarchy パネルへ検索入力欄を追加する
3. Entity 名で ListBox 表示をフィルターする
4. Entity 行をルートノードとしてツリー風に表示する
5. 既存のクリック選択と Scene / Inspector 同期を維持する
6. ビルドまたはレイヤー依存チェックを実行する
7. PR を作成する

## 検証方法

- 実行するビルド: `dotnet build tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug`
- 実行するテスト: LayerDependencyChecker
- 手動確認項目: Entity 一覧、検索、クリック選択、Scene / Inspector 同期が動作する

## 完了条件

- 現在のシーン内 Entity が Hierarchy に表示される
- Entity の親子関係を表示できる構造に近いツリー風表示になる
- Entity 名で検索できる
- Entity をクリックして選択できる
- 選択状態が Scene / Inspector と同期する
- 既存のレイヤー責務を破壊していない
- `issue-274` から `issue-269` への PR が作成されている
