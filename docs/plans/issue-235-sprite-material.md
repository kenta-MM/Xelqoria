# ExecPlan: issue-235 SpriteMaterial 導入による Sprite 描画責務の整理

## 目的

Sprite が直接保持している Texture / Color / Outline 系の描画方法に関する情報を SpriteMaterial 側へ分離し、Sprite は Position / Scale / Rotation と SpriteMaterial 参照を持つ構造へ整理する。

## child issue 一覧

- issue-236: SpriteMaterial クラスを Graphics 層に追加する
- issue-237: Sprite に SpriteMaterial 参照を導入する
- issue-238: Sprite の既存描画情報 API を SpriteMaterial へ委譲する
- issue-239: SpriteDrawInput と SpriteRenderer を SpriteMaterial 経由に対応させる
- issue-240: SpriteMaterial 共有と個別 Material 運用を確認する
- issue-241: SpriteMaterial 導入に伴うテストとクラス図を更新する

## child issue 間の依存関係

- issue-236 は SpriteMaterial クラス本体を追加するため最初に対応する。
- issue-237 は issue-236 の SpriteMaterial を Sprite から参照するため、issue-236 の後に対応する。
- issue-238 は issue-237 の Material 参照へ既存 API を委譲するため、issue-237 の後に対応する。
- issue-239 は issue-238 の委譲後の状態を ToDrawInput / SpriteRenderer に接続するため、issue-238 の後に対応する。
- issue-240 は共有 Material の挙動確認を行うため、issue-239 の後に対応する。
- issue-241 は導入後のテストとクラス図更新を行うため、issue-240 の後に対応する。
- 同じ Sprite / SpriteDrawInput / SpriteRenderer 周辺の公開 API を段階的に変更するため、並列対応可能な child issue はなし。

## ブランチ戦略

- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-235`
- child issue ブランチ: `issue-236` / `issue-237` / `issue-238` / `issue-239` / `issue-240` / `issue-241`
- child issue の PR の向き: 各 `issue-xxx` から `issue-235`
- parent issue ブランチの最終 PR の向き: `issue-235` から `main`

## parent issue での作業範囲

parent issue では原則として実装作業を行わない。

すべての child issue が parent issue ブランチにマージされた後、統合確認・動作確認・最終的な差分確認のみ行う。

## 最終確認項目

- すべての child issue が parent issue ブランチにマージされている
- child issue 間の変更が競合していない
- SpriteMaterial が Graphics 層にあり、RHI / Backends へ描画概念を持ち込んでいない
- Sprite は生成時点で SpriteMaterial を持ち、Texture / Color / Outline の主所有を SpriteMaterial へ移している
- Sprite::ToDrawInput() と SpriteRenderer::Draw(const Sprite&) が SpriteMaterial 経由の描画情報を反映している
- 複数 Sprite が同じ SpriteMaterial を共有できる
- ビルドが通る
- 必要なテストが通る
- 必要なドキュメントが更新されている
- 不要な一時ファイルや作業メモが含まれていない

## 完了条件

- すべての child issue が完了している
- 統合後の確認が完了している
- parent issue ブランチから `main` への PR が作成されている
