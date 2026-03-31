# Asset Flow

この文書は、Xelqoria におけるアセット参照から描画までの最小フローを整理する。

## 目的

- 保存データと実行時オブジェクトの分離を守る
- `AssetId` ベースの解決経路を明確にする
- `Game` と `Graphics` の責務境界を崩さない

## 全体像

```text
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
```

## 保存データ側

- `Core::AssetId`
  - 文字列パスから切り離した識別子
- `Game::SpriteComponent`
  - `spriteAssetRef` に SpriteAsset の識別子を持つ
- `Game::Assets::SpriteAsset`
  - `textureAssetId` に Texture2D 相当の識別子を持つ
- `Game::SceneSerializer`
  - Scene をテキストへ保存、または復元する

保存データは GPU リソースや Direct3D 実体を持たない。

## 実行時解決側

### 1. Scene が描画候補を集める

- `Scene::CollectSpriteRenderItems()` が Entity と Component を列挙する

### 2. SpriteAsset を解決する

- `Scene::ResolveSprites(...)` が `ISpriteAssetResolver` を通して `SpriteAsset` を取得する

### 3. Texture2D を解決する

- `SpriteAsset.textureAssetId` を使い、`Graphics::ITextureAssetResolver` から `Texture2D` を取得する

### 4. Graphics::Sprite を組み立てる

- `Graphics::Sprite` に Texture2D、位置、拡大率、回転などを設定する
- `Sprite` は描画を行わず、描画用データを保持する

### 5. SpriteRenderer が描画する

- `Graphics::SpriteRenderer` が `RHI::IGraphicsContext` を使って描画を実行する

## レイヤー責務

- `Game`
  - 保存データ、Entity、Scene、参照解決の起点
- `Graphics`
  - Texture2D、Sprite、SpriteRenderer
- `RHI`
  - 描画 API 抽象
- `Backends`
  - 実際の API 実装

## 禁止パターン

- `SpriteComponent` が `Texture2D` や `ITexture` を直接持つ
- `SpriteAsset` が GPU リソースや Direct3D 型を持つ
- `Game` から `Backends` を直接参照する
- `Sprite` に描画 API 呼び出しを持たせる

## 変更時の確認

- 保存対象が実行時オブジェクトへ依存していないか
- 解決経路が `AssetId` ベースのまま保たれているか
- 描画責務が `SpriteRenderer` へ集約されているか
