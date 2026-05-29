# ExecPlan: issue-315 EditorPanelId と IEditorPanelView の導入

## 目的

`EditorPanelId` を `EditorShell` から独立させ、PanelView 共通インターフェース `IEditorPanelView` を導入する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/Shell/EditorPanelId.h`, `Editor/Source/Panels/IEditorPanelView.h`, `EditorShell` と `EditorPanelId` 参照箇所、project/filter
- 変更対象クラス: `EditorShell`
- 影響する機能: Dock / Floating / Tab の panel 識別子参照

## 前提

- parent issue: issue-313
- 先行 issue: issue-314
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-313`
- この child issue のブランチ: `issue-315`
- PR の向き: `issue-313`

## 実装方針

PanelView 本体への HWND 移管や Controller Bind 変更は行わず、独立した `EditorPanelId` と `IEditorPanelView` の契約だけを追加する。既存 `EditorShell::EditorPanelId` 参照は `Xelqoria::Editor::EditorPanelId` へ置き換える。

## 作業手順

1. `EditorPanelId.h` を追加する
2. `IEditorPanelView.h` を最小操作のみで追加する
3. `EditorShell` 内の enum を削除し、独立型を include する
4. `EditorShell::EditorPanelId` 参照を `EditorPanelId` へ更新する
5. project/filter を更新する
6. 依存チェックと可能な静的確認を実行する
7. `issue-313` 向け PR を作成する

## 検証方法

- 実行するビルド: `msbuild Xelqoria.sln /t:Xelqoria.Editor /p:Configuration=Debug /p:Platform=x64`
- 実行するテスト: LayerDependencyChecker
- 手動確認項目: `EditorShell::EditorPanelId` が残っていないこと、`IEditorPanelController` が作成されていないこと

## 完了条件

- `EditorPanelId.h` が追加されている
- `EditorShell::EditorPanelId` が廃止されている
- Dock / Floating / Tab 関連処理が独立した `EditorPanelId` を参照している
- `IEditorPanelView.h` が追加されている
- `IEditorPanelView` に Controller 固有操作が含まれていない
- `IEditorPanelController` が作成されていない
- レイヤー責務に違反していない
- `issue-313` に向けた PR が作成されている
