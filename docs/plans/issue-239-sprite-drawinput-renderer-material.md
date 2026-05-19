# ExecPlan: issue-239 SpriteDrawInput と SpriteRenderer を SpriteMaterial 経由に対応させる

## 目的

Sprite::ToDrawInput() が SpriteMaterial の描画情報を反映し、SpriteRenderer::Draw(const Sprite&) が Material 経由の描画情報で描画できることを確認する。

## 対象範囲

- 変更対象プロジェクト: Graphics, tests/Graphics
- 変更対象ファイル: `Graphics/Source/Sprite.cpp`, `tests/Graphics/Source/SpriteRenderMathTests.cpp`
- 変更対象クラス: `Xelqoria::Graphics::Sprite`, `Xelqoria::Graphics::SpriteRenderer`
- 影響する機能: Sprite から SpriteDrawInput への変換、SpriteRenderer の Sprite 描画経路

## 前提

- parent issue: issue-235
- 先行 issue: issue-238
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-235`
- この child issue のブランチ: `issue-239`
- PR の向き: `issue-239` から `issue-235`

## 実装方針

issue-238 で Sprite::ToDrawInput() は SpriteMaterial の値を参照する形へ接続済み。SpriteRenderer::Draw(const Sprite&) は既存どおり ToDrawInput() を通す。ここでは Material 由来の色・外枠・テクスチャが renderer の描画定数へ反映されるテストを追加して経路を確認する。

## 作業手順

1. 関連コードを確認する
2. SpriteRenderer::Draw(const Sprite&) の経路を確認する
3. Material 由来の描画情報が反映されるテストを追加する
4. ビルドまたはテストを実行する
5. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `tests/Graphics/Xelqoria.Tests.Graphics.vcxproj` Debug x64
- 実行するテスト: 可能なら Graphics テスト実行
- 手動確認項目: RHI に SpriteMaterial / Material / Sprite / Renderer などの高レベル概念が追加されていないこと

## 完了条件

- Sprite::ToDrawInput() が SpriteMaterial の Texture / TextureAssetId / Color / Outline 情報を反映している
- Sprite::ToDrawInput() が Sprite の Position / Scale / Rotation を維持している
- SpriteRenderer::Draw(const Sprite&) で SpriteMaterial 経由の描画情報が反映される
- レイヤー責務に違反していない
- `branch-rules.md` に従った PR が作成されている
