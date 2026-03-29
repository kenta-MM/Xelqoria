---
name: xelqoria-asset-pipeline-check
description: Use when Xelqoria changes touch AssetId, SpriteAsset, Scene serialization, asset registries, or sprite resolution flow. Verify the separation between saved data and runtime objects, and confirm that AssetId-based resolution remains consistent from Scene to rendering.
---

# Xelqoria Asset Pipeline Check

Xelqoria のアセット解決経路や保存データまわりを確認する時に使う。

## 使いどころ

- `AssetId`、`SpriteAsset`、`SpriteComponent`、`SceneSerializer` を触る時
- `TextureAssetRegistry` や `SpriteAssetRegistry` を変える時
- Scene から描画までの解決経路が崩れていないか確認したい時

## 最初にやること

1. [AGENTS.md]($XELQORIA_ROOT/AGENTS.md) を確認する
2. [docs/agents/asset-flow.md]($XELQORIA_ROOT/docs/agents/asset-flow.md) を読む
3. 関連ヘッダと serializer 実装を確認する

## チェック項目

1. 保存データ
- 保存対象が識別子と値オブジェクトに留まっているか
- GPU リソースや API 実体が保存データへ入っていないか

2. 解決経路
- `SpriteComponent.spriteAssetRef` から `SpriteAsset` を引けるか
- `SpriteAsset.textureAssetId` から `Texture2D` を引けるか
- `Scene::ResolveSprites(...)` の責務が保たれているか

3. 描画責務
- `Graphics::Sprite` が描画を持たず、データ保持に留まっているか
- 描画は `SpriteRenderer` に集約されているか

4. 互換性
- 既存保存データを壊す変更が入っていないか
- serializer の key や必須項目を変えたなら、影響を明示しているか

## 出力スタイル

- アセット解決が壊れる可能性のある点を先に示す
- 保存互換、Runtime 再構築、描画責務の順で確認結果をまとめる
