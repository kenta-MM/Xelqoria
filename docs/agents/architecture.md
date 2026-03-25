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

## Runtime と Editor 成果物境界

Xelqoria では Runtime 成果物と Editor 成果物を別ターゲットとして扱い、依存の混入を防ぐ。

成果物の基本整理:

- App は Runtime 実行ファイルとして扱う
- Editor は制作支援用の実行ファイルとして扱う
- Core / Game / Graphics / RHI / Backends は両ターゲットから共有してよい
- Editor 専用 UI、編集状態、制作補助コードは Runtime 成果物へ含めない

Runtime 成果物に含めてよいもの:

- ゲーム実行に必要な Core / Game / Graphics / RHI / Backends の依存
- 実行時ロード対象の共通アセット
- Runtime 起動に必要な設定値と起動エントリ

Runtime 成果物に含めてはいけないもの:

- Editor パネル、Inspector、Hierarchy などの UI 実装
- Editor 専用の選択状態、ドラッグ状態、編集セッション状態
- Editor でのみ意味を持つデバッグ補助コード
- Editor 専用の命名やディレクトリ前提に依存したリンク

Editor 成果物で追加してよいもの:

- EditorCamera2D など編集作業用の表示補助
- SceneView、Assets、Inspector のような UI シェル
- Runtime データを読み書きするための制作補助ロジック

成果物境界の判断基準:

- Runtime 単体で起動・描画・アセット解決が完結するか
- Runtime バイナリが Editor 名前空間や Editor ソースを参照していないか
- Editor が Runtime データを利用していても、逆方向参照になっていないか
- 共有ライブラリへ置くコードが「実行時にも必要か」で説明できるか

命名と出力物の方針:

- Runtime のエントリは App 配下に置き、実行用成果物として命名する
- Editor のエントリは Editor 配下に置き、制作ツール成果物として命名する
- 共通アセットは Runtime / Editor で共有できる形式に保つ
- Editor 専用メタデータを追加する場合は、Runtime 配送物と分離した配置を前提にする

Q04 向け確認観点:

- Runtime のリンク対象に Editor プロジェクトが含まれていないこと
- Runtime 出力物に Editor 専用 DLL / ライブラリ / リソースが混入していないこと
- Editor は Runtime 共有層の上に成立し、Runtime は Editor なしで成立すること

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
