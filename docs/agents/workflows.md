# Workflows For Agents

この文書は、AI エージェントが実装時に迷わないための作業フローを定義する。

## レイヤー判定フロー

新規クラス・機能を作る前に、次の順で配置先を決める。

1. 描画概念か (`Sprite`, `Camera`, `Material`, `RenderSystem` など)
- Yes: Graphics
2. GPU API 直接操作か (Direct3D / Vulkan / Metal)
- Yes: Backends
3. GPU 抽象か (`Buffer`, `Texture`, `Shader`, `Pipeline`)
- Yes: RHI
4. エンジン基盤システムか (`Application`, `Window`, 起動順序管理)
- Yes: Core
5. ゲーム固有ロジックか (`Player`, `Enemy`, `Stage`, `UI`)
- Yes: Game

不明な場合は Graphics を優先し、依存方向を再確認する。

## 実装ワークフロー

1. 変更対象の責務を確認する
2. 依存方向 (`Game -> Graphics -> RHI -> Backends`) に違反しない設計にする
3. 必要最小限のインターフェース変更に留める
4. 既存レイヤーに Direct3D 型漏れがないか確認する
5. ビルド・実行確認後、変更理由を記録する

## Sprite 機能追加ワークフロー

1. `Sprite` はデータ保持のみ
2. 描画処理は `SpriteRenderer` に追加
3. GPU 呼び出しは `IGraphicsContext` 経由に限定
4. Direct3D 固有コードが Graphics に入っていないことを確認

## RHI 機能追加ワークフロー

1. 低レベル抽象 (`ITexture` / `IBuffer` など) を RHI に定義
2. D3D11 / D3D12 の Backends 実装を追加
3. Graphics では抽象インターフェースのみ参照
4. RHI に `Sprite` / `Mesh` 等の高レベル概念を追加しない
5. `tools/check-layer-dependencies.ps1` に通る `#include` 構成にする

## PR 前チェック

- 依存方向を破っていない
- Graphics に Direct3D 型がない
- Game から Backends 参照がない
- RHI に高レベル描画概念がない
- Sprite が自己描画していない
- Renderer が描画責務を持っている

## 最小構成目標

```text
Core
├ Application
└ Window

RHI
├ GraphicsAPI
├ IGraphicsContext
└ ITexture

Backends
├ D3D11GraphicsContext
└ D3D11Texture

Graphics
├ Texture2D
├ Sprite
└ SpriteRenderer
```

目標: 1 枚の Sprite を描画する。
