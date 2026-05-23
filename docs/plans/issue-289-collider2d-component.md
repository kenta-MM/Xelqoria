# ExecPlan: issue-289 2D Collider Component を追加する

## 目的

Entity に 2D ゲーム制作向けの Collider 情報を付与し、Game 層の Component として Scene 保存・読み込み、Editor 表示、SceneView 補助表示まで段階的に対応する。

## child issue 一覧

- issue-290: Game 層に Collider2DComponent と AABB 変換を追加する
- issue-291: Scene 保存・読み込みを Collider2DComponent に対応する
- issue-292: Editor の Inspector と Hierarchy を Collider2DComponent に対応する
- issue-293: SceneView に Collider2D 枠表示を追加する

## child issue 間の依存関係

- issue-290 は Collider2DComponent と Game 層 API の土台であり、issue-291、issue-292、issue-293 の前提とする。
- issue-291、issue-292、issue-293 は issue-290 の変更を取り込んだ上で個別対応する。
- issue-292 と issue-293 は Editor 側の近い領域を触るため、同時編集時は競合に注意する。

## ブランチ戦略

- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-289`
- child issue ブランチ: `issue-290`, `issue-291`, `issue-292`, `issue-293`
- child issue の PR の向き: `issue-289`
- parent issue ブランチの最終 PR の向き: `master`

## parent issue での作業範囲

parent issue では原則として実装作業を行わない。

すべての child issue が parent issue ブランチにマージされた後、統合確認・動作確認・最終的な差分確認のみ行う。

## 最終確認項目

- すべての child issue が parent issue ブランチにマージされている
- child issue 間の変更が競合していない
- Collider2DComponent が Game 層に閉じており、Graphics / RHI / Backends に依存していない
- Scene 保存・読み込みの version=1 互換が維持されている
- Inspector / Hierarchy / SceneView の Editor 対応が Game 層の責務を侵していない
- ビルドが通る
- 必要なテストが通る
- 不要な一時ファイルや作業メモが含まれていない

## 完了条件

- issue-290 から issue-293 までの PR が作成され、parent issue ブランチへマージされている
- 統合後の確認が完了している
- parent issue ブランチから `master` への PR が作成されている
