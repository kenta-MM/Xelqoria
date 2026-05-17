# Coding Rules For Agents

Xelqoria で AI エージェントがコードを追加・変更する際の共通ルール。

## 基本

- 依存方向は App / Editor -> Platform / Core / Game -> Graphics -> RHI -> Backends
- OS 依存は Platform 抽象を経由し、OS 固有実装は Platform.Win32 / Platform.Mac へ閉じ込める
- 既存レイヤーの責務を崩さない
- 変更は最小限にする

## 役割

- Game: ゲームロジック
- Graphics: 高レベル描画
- RHI: 低レベル GPU 抽象
- Backends: GPU API 実装
- Core: 共通基盤
- Platform: Window / EventLoop / Input / Dialog / Clipboard / Cursor などの OS 抽象
- Platform.Win32: Windows 向け Platform 実装
- Platform.Mac: macOS 向け Platform スタブまたは実装
- Editor.UI: Editor 専用 UI シェル、Dock / パネル配置、UI 入力補助、SceneView 表示境界

## 必須

- Sprite は描画しない
- 描画は Renderer が行う
- RHI は Sprite Camera Material Renderer を持たない
- Game は Direct3D と Backends を知らない
- Core と Game は Editor.UI と Platform.Win32 を知らない
- Editor.UI は Backends / Direct3D / Platform.Win32 を知らない
- Runtime は Editor.UI を知らない

## 禁止

- Graphics に ID3D11Device ID3D11Texture2D ID3D12Device などを入れない
- RHI に Sprite Camera Material SpriteRenderer を入れない
- Game から Backends を直接参照しない
- Game から Editor.UI や Platform.Win32 を直接参照しない
- Core から Editor.UI や Platform.Win32 を直接参照しない
- Editor.UI から Backends や Direct3D を直接参照しない
- Editor.UI から Platform.Win32 や Platform.Mac を直接参照しない
- Platform.Win32 / Platform.Mac 以外で OS ネイティブ API を直接使用しない
- 依存逆転の #include を作らない
- using で名前空間を省略しない
- グローバル変数を定義しない

## 配置

- 描画機能は Graphics
- GPU 抽象は RHI
- GPU API 直接操作は Backends
- エンジンシステムは Graphics または Core
- ゲームロジックは Game
- OS 抽象は Platform
- OS 固有 API は Platform.Win32 / Platform.Mac
- Editor UI シェル / Dock / UI 入力補助 / SceneView 表示境界は Editor.UI
- Runtime 共有層に Editor 専用概念を置かない

## 実装

- 付けられる const は付ける
- 否定条件は if (false == value) を使う
- 真条件は if (value) を使う
- 早期 return を優先する
- 型名は完全修飾名、または既存方針に従う
- ローカル変数は使用直前で定義する
- 再代入や使い回しを避ける
- 暗黙変換を避け、必要なら明示的に変換する
- class のアクセス順は public -> protected -> private

## コメント

- コメントは日本語
- DocFX 互換 XML コメントを使う
- 宣言直前に書く
- /// <summary> を基本とする
- 引数は /// <param name="...">
- 戻り値は /// <returns>
- 「何をするか」「入出力」「前提条件」を簡潔に書く
- 識別子は英語のままでよい

## 確認

- レイヤー責務は明確か
- API 抽象は破綻していないか
- Direct3D 固有コードは Backends に閉じているか
- OS 固有 API は Platform.Win32 / Platform.Mac に閉じているか
- Editor.UI が Backends / Direct3D / Platform.Win32 / Platform.Mac を参照していないか
- Core と Game が Editor.UI / Platform.Win32 を参照していないか
- 2D / 3D 共存を阻害しないか

## 優先順

1. 依存方向維持
2. OS 抽象は Platform
3. OS 固有実装は Platform.Win32 / Platform.Mac
4. 低レベル GPU 抽象は RHI
5. 高レベル描画は Graphics
6. 迷ったら architecture.md / project-map.md / workflows.md で責務を再確認する
