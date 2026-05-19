# ExecPlan: issue-234 Scene 編集の Undo / Redo、Dirty 管理、複合操作履歴を統一する

## 目的

既存の `SceneCommandHistory` ベースの Scene スナップショット履歴を Editor 専用機能として拡張し、Undo / Redo ショートカット、Dirty 管理、複合操作履歴、保存タイミングを Issue 要件へ合わせる。

## 対象範囲

- 変更対象プロジェクト: `Editor`, `tests/Editor`
- 変更対象ファイル:
  - `Editor/Source/SceneCommandHistory.*`
  - `Editor/Source/EditorCommandController.*`
  - `Editor/Source/Application.*`
  - `Editor/Source/SceneViewInteractionTypes.h`
  - `Editor/Source/SceneViewInputTracker.*`
  - `Editor/Source/SceneDropPlacementService.cpp`
  - `Editor/Source/ProjectPanelController.*`
  - `tests/Editor/Source/SceneCommandHistoryTests.cpp`
- 変更対象クラス:
  - `SceneCommandHistory`
  - `EditorCommandController`
  - `Application`
  - `SceneViewInputTracker`
  - `ProjectPanelController`
- 影響する機能:
  - Scene 編集履歴
  - Undo / Redo
  - Scene 保存
  - Dirty 表示
  - SceneView ドラッグ編集

## 前提

- parent issue: #234
- ブランチ運用ルール: `branch-rules.md`
- この issue のブランチ: `issue-234`
- PR の向き: `issue-234` → `master`
- Undo / Redo は Editor 層だけで扱い、Game 層へ履歴概念を追加しない。

## 実装方針

`SceneCommandHistory` に操作名と保存済み履歴位置を追加する。Scene 編集操作は成功時のみスナップショットと操作名を履歴へ追加し、Scene ファイル保存は Project メニューの保存操作に限定する。Dirty 判定は履歴の現在位置と保存済み位置で行い、Project パネルの Scene 表示へ `*` を付与する。

SceneView の Move / Scale / Rotate はドラッグ開始時の状態を保持し、ドラッグ中は履歴へ追加せず、マウス解放時に `Move Entity` / `Scale Entity` / `Rotate Entity` として 1 件だけ追加する。

## 作業手順

1. 関連コードを確認する
2. ExecPlan を作成する
3. `SceneCommandHistory` を操作名・保存済み位置対応にする
4. `EditorCommandController` の Redo ショートカットと即時保存を修正する
5. Application の Scene 編集保存フローを Dirty / 履歴追加へ置き換える
6. SceneView の複合操作結果へ操作名を持たせる
7. Dirty 表示を Project パネルの Scene 表示へ反映する
8. テストを追加・更新する
9. ビルドまたはテストを実行する

## 検証方法

- 実行するビルド: 可能なら Editor / tests のビルド
- 実行するテスト: `tests/Editor` の SceneCommandHistory 関連テスト
- 手動確認項目:
  - `Ctrl + Z` で Undo
  - `Shift + Ctrl + Z` で Redo
  - `Ctrl + Y` で Redo されない
  - Scene 編集後に Scene 表示へ `*` が出る
  - 保存で `*` が消える
  - Undo で保存済み位置へ戻ると `*` が消える

## 完了条件

- Issue #234 の受け入れ条件に対応している
- レイヤー責務に違反していない
- 関連テストが通る
- 不要な変更が含まれていない
