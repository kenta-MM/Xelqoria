# エージェント向けアーキテクチャ

## 依存関係

App / Editor → Platform / Core / Game → Graphics → RHI → Backends

Runtime の描画依存順序は `Game → Graphics → RHI → Backends` を絶対に崩してはならない。
OS 依存は `Platform` 抽象を経由し、OS 固有実装は `Platform.Win32` / `Platform.Mac` へ閉じ込める。

## レイヤー責務

- Game = ゲームロジックおよび永続的なゲームデータ
- Graphics = APIに依存しない描画ロジックおよび描画概念
- RHI = 低レベルなGPU抽象化
- Backends = RHIのプラットフォーム固有実装
- Platform = Window / EventLoop / Input / Dialog / Clipboard / Cursor などの OS 抽象
- Platform.Win32 = Windows 向け Platform 実装
- Platform.Mac = macOS 向け Platform スタブまたは実装
- Editor.UI = Editor 専用の OS 非依存 UI 入力補助、SceneView 表示境界

## コアルール

- GPU API 固有コードは Backends プロジェクトでのみ使用可能
- Window / EventLoop / Input / Dialog / Clipboard / Cursor の OS ネイティブ API は Platform.* プロジェクトでのみ使用可能
- Editor の OS ネイティブ UI シェルは Editor に閉じ、Editor.UI へ持ち込まない
- Graphics は Direct3D 型を使用してはならない
- RHI は Sprite、Camera、Material、Renderer などの描画概念を含めてはならない
- Game は Backends や Direct3D を参照してはならない
- Game は Editor.UI や Platform.Win32 を参照してはならない
- Core は Editor.UI や Platform.Win32 を参照してはならない
- Editor.UI は Backends / Direct3D を参照してはならない
- Editor.UI は Win32 / Mac 固有実装へ直接依存してはならない
- Runtime は Editor.UI を参照してはならない

## 描画ルール

- 描画は Renderer クラスが担当する
- Sprite などのデータオブジェクトは自ら描画してはならない
- SceneView / Game Preview は Editor.UI の描画先境界を通じて Editor 側の描画処理へ接続する
