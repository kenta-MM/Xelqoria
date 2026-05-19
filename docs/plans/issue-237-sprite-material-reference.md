# ExecPlan: issue-237 Sprite に SpriteMaterial 参照を導入する

## 目的

Sprite に SpriteMaterial 参照を導入し、生成時点で default SpriteMaterial を持つ設計にする。

## 対象範囲

- 変更対象プロジェクト: Graphics
- 変更対象ファイル: `Graphics/Source/Sprite.h`, `Graphics/Source/Sprite.cpp`
- 変更対象クラス: `Xelqoria::Graphics::Sprite`
- 影響する機能: Sprite の Material 参照管理

## 前提

- parent issue: issue-235
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-235`
- この child issue のブランチ: `issue-237`
- PR の向き: `issue-237` から `issue-235`

## 実装方針

Sprite は std::shared_ptr<SpriteMaterial> を持ち、コンストラクタで default SpriteMaterial を生成する。SetMaterial(nullptr) は Material 未設定状態を作らないように default SpriteMaterial へ戻す。既存の Texture / Color / Outline API の委譲は issue-238 で扱う。

## 作業手順

1. 関連コードを確認する
2. Sprite にコンストラクタと SetMaterial / GetMaterial を追加する
3. Sprite が Material 未設定状態にならないようにする
4. ビルドまたはテストを実行する
5. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `Graphics/Xelqoria.Graphics.vcxproj` Debug x64
- 実行するテスト: 可能なら Graphics テスト
- 手動確認項目: Sprite / SpriteMaterial 自身に描画処理が追加されていないこと

## 完了条件

- Sprite が SpriteMaterial 参照を持つ
- Sprite が生成時点で default SpriteMaterial を持つ
- Sprite::SetMaterial() / Sprite::GetMaterial() が追加されている
- SetMaterial(nullptr) により Sprite が Material を持たない通常状態にならない
- レイヤー責務に違反していない
- `branch-rules.md` に従った PR が作成されている
