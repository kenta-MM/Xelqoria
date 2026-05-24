# Issue 289 Sprite Collider Asset Follow-up

## Goal

Assets 右クリックから Collider2D Asset を作成できるようにし、Sprite 専用ビューで Sprite Asset に Material / Script / Collider2D を集約する。Entity Inspector から Sprite Asset 固有の編集項目を外し、Entity 側は Transform と Component の有無に集中させる。

## Scope

- `.collider2d` Asset の保存データ、読み込み、Registry を Game 層に追加する。
- `.sprite` Asset に `materialAssetId` と `collider2DAssetId` を追加し、既存ファイル互換を維持する。
- Assets 空白右クリックメニューへ `Collider2Dを作成` を追加する。
- Inspector の `Add Collider2DComponent` から Collider2D Asset を作成し、選択 Entity の Sprite Asset へ割り当てる。
- Sprite 専用タブを追加し、Sprite Asset の Texture / Material / Script / Collider2D を表示する。
- Sprite Asset 固有項目を Entity Inspector から除外する。

## Validation

- LayerDependencyChecker
- Game tests
- Editor tests
- Editor app build
