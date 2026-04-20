# Asset Flow

Xelqoria におけるアセット参照から描画までの最小フローを定義する。

## 目的

- 保存データと実行時オブジェクトを分離する
- AssetId ベースの解決経路を維持する
- Game と Graphics の責務を分離する

## フロー

    Scene
    -> Entity
    -> SpriteComponent(spriteAssetRef: AssetId)
    -> ISpriteAssetResolver
    -> SpriteAsset(textureAssetId: AssetId)
    -> ITextureAssetResolver
    -> Texture2D
    -> Graphics::Sprite
    -> SpriteRenderer
    -> RHI::IGraphicsContext

## 保存データ

- Core::AssetId: 識別子
- Game::SpriteComponent: spriteAssetRef を持つ
- Game::Assets::SpriteAsset: textureAssetId を持つ
- Game::SceneSerializer: Scene 保存の窓口
- Game::SceneTextWriter: Scene を `key=value` テキストへ書き出す
- Game::SceneTextReader: Scene 保存テキストを復元する
- Scene 保存形式は `entity.<index>.extensions.<name>` を将来拡張用に予約する
- 保存データは GPU リソースや Direct3D 型を持たない

## 実行時

1. Scene::CollectSpriteRenderItems() が描画候補を集める
2. Scene::ResolveSceneSprites(...) が ISpriteAssetResolver で SpriteAsset を解決する
3. SpriteAsset.textureAssetId から Graphics::ITextureAssetResolver で Texture2D を解決する
4. Game::ResolvedSceneSprite に EntityId と Graphics::Sprite をまとめる
5. 必要に応じて Scene::ResolveSprites(...) が Graphics::Sprite だけを取り出す
6. Graphics::SpriteRenderer が RHI::IGraphicsContext で描画する

## 責務

- Game: 保存データ、Entity、Scene、解決起点、EntityId 付き描画候補の保持
- Graphics: Texture2D Sprite SpriteRenderer
- RHI: 描画 API 抽象
- Backends: 描画 API 実装

## 禁止

- SpriteComponent が Texture2D や ITexture を直接持つ
- SpriteAsset が GPU リソースや Direct3D 型を持つ
- Game から Backends を直接参照する
- Sprite が描画 API を呼ぶ

## 確認

- 保存対象が実行時オブジェクトに依存していないか
- 解決経路が AssetId ベースか
- 描画責務が SpriteRenderer に集約されているか
