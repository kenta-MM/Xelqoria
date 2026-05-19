# ExecPlan: issue-241 SpriteMaterial 導入に伴うテストとクラス図を更新する

## 目的

SpriteMaterial 導入後の主要な挙動をテストで確認し、Graphics クラス図を更新する。

## 対象範囲

- 変更対象プロジェクト: tests/Graphics, docs
- 変更対象ファイル: `tests/Graphics/Source/SpriteRenderMathTests.cpp`, `docs/class_diagram/graphics-class-diagram.md`
- 変更対象クラス: `Xelqoria::Graphics::Sprite`, `Xelqoria::Graphics::SpriteMaterial`
- 影響する機能: SpriteMaterial の保持情報、Sprite の default Material、Graphics クラス図

## 前提

- parent issue: issue-235
- 先行 issue: issue-240
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-235`
- この child issue のブランチ: `issue-241`
- PR の向き: `issue-241` から `issue-235`

## 実装方針

既存の Graphics テストへ SpriteMaterial 単体の保持情報、Sprite の default Material、SetMaterial(nullptr) の挙動確認を追加する。Graphics クラス図は SpriteMaterial と Sprite からの参照関係を反映し、Sprite が Texture / Color / Outline を直接主所有する表現を削除する。

## 作業手順

1. 関連コードとクラス図を確認する
2. SpriteMaterial / Sprite の追加テストを実装する
3. Graphics クラス図を更新する
4. ビルドまたはテストを実行する
5. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `tests/Graphics/Xelqoria.Tests.Graphics.vcxproj` Debug x64
- 実行するテスト: `./artifacts/x64/Debug/Xelqoria.Tests.Graphics.exe`
- 手動確認項目: Graphics 層に Direct3D 型が混入していないこと、RHI に高レベル描画概念が追加されていないこと

## 完了条件

- SpriteMaterial 導入に関する主要な挙動がテストで確認されている
- Graphics クラス図が SpriteMaterial 導入後の公開型と依存関係を表している
- 既存テストが通る
- レイヤー責務に違反していない
- `branch-rules.md` に従った PR が作成されている
