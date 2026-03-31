# Project Map For Agents

この文書は、Xelqoria ソリューションの各プロジェクトの責務、依存先、編集時の注意点を素早く確認するための地図として使う。

## 依存方向

```text
App / Editor
↓
Game
↓
Graphics
↓
RHI
↓
Backends
```

`Core` は基盤ライブラリとして `Graphics`、`Game`、`App`、`Editor` から参照される。

## Core

- プロジェクト: `Core/Xelqoria.Core.vcxproj`
- 主なヘッダ:
  - `Core/Source/AssetId.h`
  - `Core/Source/Window.h`
- 責務:
  - `AssetId` によるアセット識別
  - `Window` による Win32 ウィンドウ管理
- 参照先:
  - なし
- 編集時の注意:
  - 描画概念を入れない
  - Game や Editor の都合に寄せすぎない

## RHI

- プロジェクト: `RHI/Xelqoria.RHI.vcxproj`
- 主なヘッダ:
  - `RHI/Source/GraphicsAPI.h`
  - `RHI/Source/IGraphicsContext.h`
  - `RHI/Source/ITexture.h`
  - `RHI/Source/IVertexBuffer.h`
  - `RHI/Source/IIndexBuffer.h`
- 責務:
  - GPU 操作とリソースの抽象化
  - API 非依存の描画インターフェース提供
- 参照先:
  - なし
- 編集時の注意:
  - `Sprite`、`Camera`、`Scene` などの描画概念を入れない
  - Direct3D 固有型を公開インターフェースへ漏らさない

## Graphics

- プロジェクト: `Graphics/Xelqoria.Graphics.vcxproj`
- 参照先:
  - `Core`
  - `RHI`
- 主なヘッダ:
  - `Graphics/Source/Texture2D.h`
  - `Graphics/Source/Sprite.h`
  - `Graphics/Source/SpriteRenderer.h`
  - `Graphics/Source/ITextureAssetResolver.h`
  - `Graphics/Source/TextureAssetRegistry.h`
- 責務:
  - `Texture2D`、`Sprite`、`SpriteRenderer` などの描画概念
  - AssetId から Texture2D を解決する補助
- 編集時の注意:
  - Direct3D 型を入れない
  - GPU 操作は `RHI::IGraphicsContext` 経由に限定する
  - `Sprite` はデータ保持、描画は `SpriteRenderer` が担当する

## Game

- プロジェクト: `Game/Xelqoria.Game.vcxproj`
- 参照先:
  - `Core`
  - `Graphics`
- 主なヘッダ:
  - `Game/Source/Transform.h`
  - `Game/Source/SpriteComponent.h`
  - `Game/Source/Entity.h`
  - `Game/Source/Scene.h`
  - `Game/Source/SceneSerializer.h`
  - `Game/Source/Assets/SpriteAsset.h`
  - `Game/Source/Assets/SpriteAssetRegistry.h`
- 責務:
  - Scene、Entity、Component、保存データ
  - `SpriteAsset` など実行時再構築用の永続データ
- 編集時の注意:
  - `Backends` や Direct3D API を参照しない
  - 保存データから Graphics / RHI 実体を直接持たない

## Backends.D3D11

- プロジェクト: `Backends.D3D11/Xelqoria.Backends.D3D11.vcxproj`
- 参照先:
  - `RHI`
- 主なヘッダ:
  - `Backends.D3D11/Source/D3D11GraphicsContext.h`
  - `Backends.D3D11/Source/D3D11Texture.h`
  - `Backends.D3D11/Source/D3D11VertexBuffer.h`
- 責務:
  - `RHI` インターフェースの D3D11 実装
- 編集時の注意:
  - D3D11 固有型はこの層に閉じ込める
  - 上位レイヤーへ依存を逆流させない

## Backends.D3D12

- プロジェクト: `Backends.D3D12/Xelqoria.Backends.D3D12.vcxproj`
- 参照先:
  - `RHI`
- 主なヘッダ:
  - `Backends.D3D12/Source/D3D12GraphicsContext.h`
  - `Backends.D3D12/Source/D3D12Texture.h`
  - `Backends.D3D12/Source/D3D12VertexBuffer.h`
- 責務:
  - `RHI` インターフェースの D3D12 実装
- 編集時の注意:
  - D3D12 固有型はこの層に閉じ込める
  - Runtime / Editor の都合をここへ持ち込まない

## App

- プロジェクト: `App/Xelqoria.App.vcxproj`
- 参照先:
  - `Core`
  - `RHI`
  - `Graphics`
  - `Game`
  - `Backends.D3D11`
  - `Backends.D3D12`
- 主なヘッダ:
  - `App/Source/Application.h`
  - `App/Source/RenderBackendBootstrap.h`
- 責務:
  - Runtime エントリーポイント
  - Window、GraphicsContext、Scene、Renderer の組み立て
- 編集時の注意:
  - Editor 専用コードを混入させない
  - Runtime 単体で起動可能な構成を保つ

## Editor

- プロジェクト: `Editor/Xelqoria.Editor.vcxproj`
- 参照先:
  - `Core`
  - `RHI`
  - `Graphics`
  - `Game`
  - `Backends.D3D11`
  - `Backends.D3D12`
- 主なヘッダ:
  - `Editor/Source/Application.h`
  - `Editor/Source/EditorCamera2D.h`
  - `Editor/Source/SceneCommandHistory.h`
  - `Editor/Source/SceneEditingOperations.h`
  - `Editor/Source/RenderBackendBootstrap.h`
- 責務:
  - 制作支援 UI
  - Scene 編集補助
  - Editor 専用カメラ、履歴、編集操作
- 編集時の注意:
  - Editor 専用の状態や UI 実装を Runtime 共通層へ押し込まない
  - Runtime と共有すべきものだけを下位レイヤーへ降ろす

## 迷った時の優先確認

1. `AGENTS.md`
2. `docs/agents/architecture.md`
3. この `project-map.md`
4. 対応する `docs/class_diagram/*.md`
