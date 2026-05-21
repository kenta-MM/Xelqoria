# ExecPlan: issue-236 SpriteMaterial クラスを Graphics 層に追加する

## 目的

Graphics 層に 2D Sprite 専用の SpriteMaterial クラスを追加し、Sprite の描画方法に関する Texture / TextureAssetId / Color / Outline 情報を保持できるようにする。

## 対象範囲

- 変更対象プロジェクト: Graphics
- 変更対象ファイル: `Graphics/Source/SpriteMaterial.h`, `Graphics/Source/SpriteMaterial.cpp`, `Graphics/Xelqoria.Graphics.vcxproj`, `Graphics/Xelqoria.Graphics.vcxproj.filters`
- 変更対象クラス: `Xelqoria::Graphics::SpriteMaterial`
- 影響する機能: SpriteMaterial の保持データ定義

## 前提

- parent issue: issue-235
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-235`
- この child issue のブランチ: `issue-236`
- PR の向き: `issue-236` から `issue-235`

## 実装方針

SpriteMaterial は Graphics 層の描画概念として追加し、Texture2D 参照、Core::AssetId、RGBA 色、OutlineEnabled、OutlineThickness、OutlineColor を保持する。RHI / Backends 固有型や描画処理は含めない。

## 作業手順

1. 関連コードを確認する
2. SpriteMaterial の公開 API と保持メンバーを追加する
3. Graphics プロジェクトファイルへ追加する
4. ビルドまたはテストを実行する
5. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: 可能なら Graphics またはソリューションの Debug x64 ビルド
- 実行するテスト: 可能なら Graphics テスト
- 手動確認項目: SpriteMaterial に Direct3D 型が含まれず、RHI へ Material 概念が追加されていないこと

## 完了条件

- Graphics 層に SpriteMaterial が追加されている
- SpriteMaterial が Texture / TextureAssetId / Color / OutlineEnabled / OutlineThickness / OutlineColor を保持できる
- SpriteMaterial 自身に描画処理がない
- レイヤー責務に違反していない
- `branch-rules.md` に従った PR が作成されている
