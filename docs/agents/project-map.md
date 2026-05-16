# エージェント向けプロジェクト構成

## 依存関係

App / Editor  
↓  
Platform / Core  
↓  
Game  
↓  
Graphics  
↓  
RHI  
↓  
Backends  

Core は OS 非依存の共通基盤レイヤー

## Core

- 役割: OS 非依存のアプリ基盤、アセット識別子
- 描画ロジックを持たない
- OS ネイティブ UI 部品を持たない
- Editor.UI / Platform.Win32 を参照しない

## Platform

- 役割: Window, EventLoop, Input, FileDialog, Clipboard, Cursor などの OS 抽象
- 利用可能: なし、または OS 非依存の標準ライブラリ型
- Win32 / Mac 固有型を公開インターフェースへ露出しない

## Platform.Win32

- 役割: Windows 向け Platform 実装
- 利用可能: Platform
- Win32 API を使用できる

## Platform.Mac

- 役割: macOS 向け Platform スタブまたは実装
- 利用可能: Platform
- Windows ビルド対象へ入れない

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
- 利用可能: Core, Platform, Platform.Win32, Game, Graphics, RHI, Backends
- Editor専用ロジックを含めない

## Editor

- 役割: エディタ専用ツールおよび編集支援
- 利用可能: Core, Platform, Platform.Win32, Editor.UI, Game, Graphics, RHI, Backends
- エディタ専用概念を共有ランタイム層に持ち込まない

## Editor.UI

- 役割: Editor 専用の UI シェル、パネル配置、Dock 管理、UI 入力補助、SceneView 表示境界
- 利用可能: Core, Platform
- Runtime から参照しない
- Backends / Direct3D / Platform.Win32 を参照しない
