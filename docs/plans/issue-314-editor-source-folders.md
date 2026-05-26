# ExecPlan: issue-314 Editor Source フォルダ再編と Visual Studio project/filter 更新

## 目的

`Editor/Source` 配下の物理フォルダを責務別に整理し、Visual Studio project/filter と include を新しい構成に合わせる。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source` 配下の既存 `.h` / `.cpp`、`Editor/Xelqoria.Editor.vcxproj`、`Editor/Xelqoria.Editor.vcxproj.filters`
- 変更対象クラス: Editor の既存 Shell / Controller / SceneView / Project / Assets / Script / Rendering / DragDrop / Utils / App 関連クラス
- 影響する機能: Editor UI のビルド時 include 解決と Visual Studio 上の表示構成

## 前提

- parent issue: issue-313
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-313`
- この child issue のブランチ: `issue-314`
- PR の向き: `issue-313`

## 実装方針

PanelView 追加や責務変更は行わず、既存ファイルを責務別フォルダへ機械的に移動する。include は `Editor/Source` を基準にした責務別パスへ更新し、追加 include path は増やさない。

## 作業手順

1. 既存 `Editor/Source` 配下のファイルを確認する
2. 責務別フォルダを作成する
3. 既存 `.h` / `.cpp` を責務別フォルダへ移動する
4. include を `Source` 基準の責務別パスへ更新する
5. `.vcxproj` と `.vcxproj.filters` のファイルパスと filter を更新する
6. Editor project のビルドまたは可能な静的確認を実行する
7. `issue-313` 向け PR を作成する

## 検証方法

- 実行するビルド: `msbuild Xelqoria.sln /t:Xelqoria.Editor /p:Configuration=Debug /p:Platform=x64`
- 実行するテスト: LayerDependencyChecker は Editor project build の BeforeTargets で実行される
- 手動確認項目: `rg --files Editor/Source` で責務別配置を確認する

## 完了条件

- `Editor/Source` 配下が責務別フォルダに整理されている
- `.vcxproj` が新しい物理パスを参照している
- `.vcxproj.filters` が責務別 filter 構成になっている
- include 修正後に Editor project がビルドできる
- レイヤー責務に違反していない
- 不要な変更が含まれていない
- `issue-313` に向けた PR が作成されている
