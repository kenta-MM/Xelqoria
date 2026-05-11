# エージェント向けプロジェクト構成

## 依存関係

App / Editor  
↓  
Game  
↓  
Graphics  
↓  
RHI  
↓  
Backends  

Core は共通基盤レイヤー

## Core

- 役割: アプリ基盤、ウィンドウ、アセット識別子
- 描画ロジックを持たない

## RHI

- 役割: GPU抽象化
- 描画概念を持たない
- 公開インターフェースで Direct3D 型を露出しない

## Graphics

- 役割: 描画概念と描画システム
- 利用可能: Core, RHI
- Direct3D 型を使用しない
- GPU操作は RHI を使用する

## Game

- 役割: ゲームロジック、シーン、エンティティ、コンポーネント、永続データ
- 利用可能: Core, Graphics
- Backends および Direct3D を使用しない

## Backends

- 役割: RHI のプラットフォーム固有実装
- Direct3D を使用できる唯一のレイヤー

## App

- 役割: ランタイム構成と起動処理
- 利用可能: Core, Game, Graphics, RHI, Backends
- Editor専用ロジックを含めない

## Editor

- 役割: エディタ専用ツールおよび編集支援
- 利用可能: Core, Editor.UI, Game, Graphics, RHI, Backends
- エディタ専用概念を共有ランタイム層に持ち込まない

## Editor.UI

- 役割: Editor 専用の Win32 UI シェル、パネル配置、Dock 管理
- 利用可能: Core
- Runtime から参照しない
