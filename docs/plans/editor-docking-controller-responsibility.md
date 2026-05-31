# ExecPlan: Editor Docking controller responsibility split

## 目的

`EditorShell` に残っている Docking の状態・型・操作を `EditorDockingController` へ移し、`EditorShell` の責務を PanelView の生成・所有とトップレベルレイアウト調整へ寄せる。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/Shell/EditorShell.*`, `Editor/Source/Shell/EditorDockingController.*`, `Editor/Source/Shell/EditorDockingLayoutSerializer.*`
- 変更対象クラス: `EditorShell`, `EditorDockingController`, `EditorDockingLayoutSerializer`
- 影響する機能: Dock/Floating レイアウト、タブ、ドラッグ、レイアウト保存復元

## 前提

- ブランチ運用ルール: `docs/agents/branch-rules.md`
- PR の向き: 未指定のため、この作業では PR 作成まで行わない
- Editor の OS ネイティブ UI シェルは Editor プロジェクト内に閉じる

## 実装方針

Docking の enum / node / state / drag / hit test / DockTree / Floating 管理は `EditorDockingController` に集約する。`EditorShell` は公開 API を Controller へ委譲し、PanelView の所有、Shell 全体の child window 生成、描画系ヘルパー、トップレベル `UpdateLayout` の入口を保持する。

`EditorDockingLayoutSerializer` は `EditorShell` ではなく `EditorDockingController` を保存復元対象として参照する。

## 作業手順

1. 既存の `EditorShell` / Docking / serializer の依存を確認する
2. Docking 型・状態・操作宣言を `EditorDockingController.h` へ移す
3. Docking 実装を `EditorDockingController` メンバとして動くように変更する
4. `EditorShell` から初期化・レイアウト・破棄・非表示同期を Controller へ委譲する
5. 保存復元 serializer の参照先を Controller へ変更する
6. ビルドで確認する

## 検証方法

- 実行するビルド: `msbuild Xelqoria.slnx /p:Configuration=Debug /p:Platform=x64`
- 実行するテスト: ビルド結果に応じて判断
- 手動確認項目: Dock/Floating のドラッグ、タブ切替、保存復元

## 完了条件

- Docking の状態・型・操作が `EditorShell.h` から外れている
- `EditorShell` は Docking 操作を `EditorDockingController` へ委譲している
- レイヤー責務に違反していない
- ビルド確認結果を報告している
