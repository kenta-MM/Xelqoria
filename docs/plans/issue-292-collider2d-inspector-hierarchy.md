# ExecPlan: issue-292 Editor の Inspector と Hierarchy を Collider2DComponent に対応する

## 目的

Editor 上で Collider2DComponent を追加、削除、編集できるようにし、Hierarchy 上でも Entity の Component として確認できるようにする。

## 対象範囲

- 変更対象プロジェクト: `Editor`, `tests/Editor`
- 変更対象ファイル: `Editor/Source/EditorShell.*`, `Editor/Source/InspectorPanelController.*`, `Editor/Source/HierarchyPanelController.cpp`, `tests/Editor/Source/InspectorPanelControllerTests.cpp`
- 変更対象クラス: `EditorShell`, `InspectorPanelController`, `HierarchyPanelController`
- 影響する機能: Inspector Component 操作、Hierarchy 表示、Scene dirty 用の編集結果

## 前提

- parent issue: issue-289
- 先行 issue: issue-290, issue-291
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-289`
- この child issue のブランチ: `issue-292`
- PR の向き: `issue-289`

## 実装方針

Collider2DComponent は Entity 直接付与の Component として扱う。Inspector には追加・削除ボタンと、追加済み時の `enabled` / `isTrigger` / `shapeType` / `offset` / `size` 編集欄を追加する。

`size.x` / `size.y` は 0 より大きい値のみ反映し、不正入力では Component 値を更新しない。編集結果は `InspectorApplyResult.changed` と `operationName` で Scene dirty と履歴記録の既存経路に乗せる。

## 作業手順

1. EditorShell に Collider2D 用 UI コントロールを追加する
2. InspectorPanelController で Collider2DComponent の表示・追加・削除・編集を反映する
3. Hierarchy に Collider2DComponent 行を表示する
4. Inspector の純粋ロジックテストを追加する
5. Editor テストのビルドと実行を行う

## 検証方法

- `MSBuild.exe tests\Editor\Xelqoria.Tests.Editor.vcxproj /m /p:Configuration=Debug /p:Platform=x64 /v:minimal`
- `artifacts\x64\Debug\Xelqoria.Tests.Editor.exe`

## 完了条件

- Inspector に Collider2DComponent の Add / Remove 操作がある
- 追加済み Collider2DComponent の enabled、isTrigger、offset、size を編集できる
- 不正な size 入力では Component が更新されない
- 編集後に `InspectorApplyResult.changed` が true になる
- Hierarchy に `Collider2DComponent` が Entity の子要素として表示される
- 関連テストが通る
- `issue-292` から `issue-289` への PR が作成されている
