# Architecture For Agents

この文書は、Xelqoria におけるレイヤー責務と依存方向の原則を定義する。

## エンジン概要

Xelqoria は **C++ ベースのレンダリングエンジン**。

Unity のようにゲーム制作を支えられるエンジンを志向するが、Unity の構造や体験をそのまま模倣することは目的ではない。
Xelqoria は Xelqoria 自身の設計原則と責務分離を優先し、必要な機能を独自の思想で積み上げる。

設計目標:

- Direct3D11 / Direct3D12 の抽象化
- 安全なレンダリング拡張
- レイヤー責務の明確化
- 将来の 3D 対応
- 既存エンジンの模倣ではなく、Xelqoria 独自の設計と言語化を重視する

## 依存方向

```text
Game
↓
Graphics
↓
RHI
↓
Backends
```

この依存方向を崩してはならない。

## ディレクトリ構成

```text
Engine
├ Core
├ RHI
├ Backends
│ ├ D3D11
│ └ D3D12
├ Graphics
└ Game
```

## Core

アプリケーション基盤。

責務:

- Application
- Window
- メインループ
- エンジン初期化順序管理
- エンジン起動処理

非責務:

- 描画内容

## RHI (Rendering Hardware Interface)

GPU 操作の抽象レイヤー。

責務:

- GPU 初期化
- SwapChain
- Frame Begin / End
- Draw / DrawIndexed
- GPU リソース抽象

代表例:

- GraphicsAPI
- IGraphicsContext
- ITexture
- IBuffer
- IShader
- IPipelineState

非責務:

- Sprite
- Mesh
- Camera
- Material
- SpriteRenderer

## Backends

Graphics API 実装層。

対象:

- Backends/D3D11
- Backends/D3D12

責務:

- RHI インターフェース実装
- API 固有コード管理

重要:

- Game / Graphics から Backends を直接参照しない

## Graphics

エンジンの描画機能層。

責務:

- エンジン描画概念
- 描画管理
- Sprite 描画
- カメラ

代表例:

- Texture2D
- Sprite
- SpriteRenderer
- Camera2D
- RenderSystem

重要:

- Graphics は Direct3D 型を直接扱わない
- GPU 操作は必ず RHI 経由で行う

## Game

ユーザーゲームコード。

責務:

- シーン
- エンティティ
- ゲームロジック

使用可能レイヤー:

- Core
- Graphics

禁止:

- Backends
- Direct3D API

## 保存データと実行時オブジェクトの分離

Scene 保存や Asset 永続化では、保存対象と実行時だけ存在するオブジェクトを分離して扱う。

判断基準:

- 保存対象は識別子、値オブジェクト、設定値、シリアライズ可能な構造に限定する
- 実行時オブジェクトは GPU ハンドル、描画キャッシュ、実行中のみ有効な参照を保持してよい
- 保存対象から Graphics / RHI の実体を直接参照しない
- ロード時は保存データから Runtime オブジェクトを再構築する

保存対象としてよい例:

- EntityId
- Transform
- Core::AssetId
- SpriteComponent
- SpriteRenderSettings

保存対象にしてはいけない例:

- Graphics::Sprite
- Graphics::Texture2D
- RHI::ITexture
- 実行中の Window / IGraphicsContext

### SpriteComponent と Graphics::Sprite の役割差

SpriteComponent は Game 層に置く保存向きデータであり、Scene 保存対象として扱う。

保持してよい情報:

- Sprite アセット識別子
- 可視状態
- sortOrder
- opacity
- 欠損時の補助情報

Graphics::Sprite は描画時に参照する Runtime オブジェクトであり、保存データへ直接含めない。

保持してよい情報:

- Texture2D 参照
- 描画位置
- 拡大率
- 回転角度

設計原則:

- SceneSerializer は SpriteComponent などの保存向きデータだけを読む
- Asset 解決後に Graphics::Sprite や Texture2D を Runtime 側で組み立てる
- 保存フォーマットは Runtime キャッシュの有無に依存させない
- 保存データは Editor と Runtime のどちらからも同じ意味で扱える構造にする

## 主要設計要素

### Sprite

Sprite は描画データであり、自身は描画しない。

保持する情報:

- Texture2D
- position
- rotation
- scale
- color
- uv

### SpriteRenderer

SpriteRenderer が描画を担当する。

責務:

- Sprite のデータ取得
- Texture2D から RHI Texture の取得
- Draw 命令発行

依存:

```text
SpriteRenderer
├ Sprite
└ IGraphicsContext
```

### Texture2D

エンジン側テクスチャ抽象。

保持:

- width
- height
- RHI texture

依存:

```text
Texture2D
↓
RHI::ITexture
```

### IGraphicsContext

GPU 低レベル描画インターフェース。

責務:

- Initialize
- Shutdown
- BeginFrame
- EndFrame
- Resize
- Draw
- DrawIndexed

禁止:

- DrawSprite
- DrawMesh

上記は Graphics 層で実装する。

## 描画フロー

```text
Application::Run
↓
IGraphicsContext::BeginFrame
↓
SpriteRenderer / RenderSystem
↓
IGraphicsContext::EndFrame
```

## Sprite 描画フロー

```text
SpriteRenderer::Draw
↓
Sprite 情報取得
↓
Texture2D 取得
↓
RHI Texture 取得
↓
Draw 命令
```

## 最終目標

- API 差異吸収
- 安全なレンダリング拡張
- 2D / 3D 共存
- AI フレンドリー設計
