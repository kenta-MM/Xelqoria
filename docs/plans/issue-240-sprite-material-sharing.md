# ExecPlan: issue-240 SpriteMaterial 共有と個別 Material 運用を確認する

## 目的

複数 Sprite が同一 SpriteMaterial を共有できること、および個別に見た目を変更したい場合は個別 SpriteMaterial を設定できることを確認する。

## 対象範囲

- 変更対象プロジェクト: tests/Graphics
- 変更対象ファイル: `tests/Graphics/Source/SpriteRenderMathTests.cpp`
- 変更対象クラス: `Xelqoria::Graphics::Sprite`, `Xelqoria::Graphics::SpriteMaterial`
- 影響する機能: SpriteMaterial 共有と個別 Material 設定

## 前提

- parent issue: issue-235
- 先行 issue: issue-239
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-235`
- この child issue のブランチ: `issue-240`
- PR の向き: `issue-240` から `issue-235`

## 実装方針

Sprite::SetMaterial() に同じ std::shared_ptr<SpriteMaterial> を設定した複数 Sprite が、共有 Material の変更を ToDrawInput() に反映することをテストする。さらに個別 SpriteMaterial を設定した Sprite は共有 Material の後続変更から独立することを確認する。

## 作業手順

1. 関連コードを確認する
2. Material 共有のテストを追加する
3. 個別 Material 設定のテストを追加する
4. ビルドまたはテストを実行する
5. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `tests/Graphics/Xelqoria.Tests.Graphics.vcxproj` Debug x64
- 実行するテスト: `./artifacts/x64/Debug/Xelqoria.Tests.Graphics.exe`
- 手動確認項目: 共有 SpriteMaterial の暗黙コピーが行われていないこと

## 完了条件

- 複数 Sprite が同じ SpriteMaterial を共有できる
- 共有 SpriteMaterial を変更すると、それを参照している Sprite の描画に反映される
- 個別に見た目を変更したい場合、個別の SpriteMaterial を設定できる
- 共有 SpriteMaterial の暗黙コピーが行われない
- `branch-rules.md` に従った PR が作成されている
