# ExecPlan: issue-238 Sprite の既存描画情報 API を SpriteMaterial へ委譲する

## 目的

Sprite の既存 Texture / Color / Outline 系 API を互換 API として残し、内部実装を SpriteMaterial へ委譲する。

## 対象範囲

- 変更対象プロジェクト: Graphics
- 変更対象ファイル: `Graphics/Source/Sprite.h`, `Graphics/Source/Sprite.cpp`
- 変更対象クラス: `Xelqoria::Graphics::Sprite`
- 影響する機能: Sprite の既存描画情報 API

## 前提

- parent issue: issue-235
- 先行 issue: issue-237
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-235`
- この child issue のブランチ: `issue-238`
- PR の向き: `issue-238` から `issue-235`

## 実装方針

SetTexture / GetTexture / SetTextureAssetId / GetTextureAssetId / SetColor / GetColor / Outline 系 API を SpriteMaterial へ委譲する。Sprite 側の Texture / Color / Outline 直接保持メンバーは削除し、SpriteMaterial を主所有箇所にする。

## 作業手順

1. 関連コードを確認する
2. 既存 API の実装を SpriteMaterial へ委譲する
3. Sprite 側の描画情報保持メンバーを削除する
4. 必要な接続を確認する
5. ビルドまたはテストを実行する
6. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `Graphics/Xelqoria.Graphics.vcxproj` Debug x64
- 実行するテスト: 可能なら Graphics テスト
- 手動確認項目: Sprite / SpriteMaterial 自身に描画処理が追加されていないこと

## 完了条件

- 既存 Texture 系 API が SpriteMaterial へ委譲されている
- 既存 Color 系 API が SpriteMaterial へ委譲されている
- 既存 Outline 系 API が SpriteMaterial へ委譲されている
- Sprite が Texture / Color / Outline を主所有する構造になっていない
- レイヤー責務に違反していない
- `branch-rules.md` に従った PR が作成されている
