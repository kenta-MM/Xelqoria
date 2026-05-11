# エージェント向けアーキテクチャ

## 依存関係

Game → Graphics → RHI → Backends

この順序を絶対に崩してはならない。

## レイヤー責務

- Game = ゲームロジックおよび永続的なゲームデータ
- Graphics = APIに依存しない描画ロジックおよび描画概念
- RHI = 低レベルなGPU抽象化
- Backends = RHIのプラットフォーム固有実装
- Editor.UI = Editor 専用の UI シェルおよび Dock/パネル配置

## コアルール

- プラットフォーム固有APIは Backends プロジェクトでのみ使用可能
- Graphics は Direct3D 型を使用してはならない
- RHI は Sprite、Camera、Material、Renderer などの描画概念を含めてはならない
- Game は Backends や Direct3D を参照してはならない
- Runtime は Editor.UI を参照してはならない

## 描画ルール

- 描画は Renderer クラスが担当する
- Sprite などのデータオブジェクトは自ら描画してはならない
