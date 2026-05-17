# Runtime Vs Editor Boundary

Xelqoria における Runtime 成果物と Editor 成果物の境界を定義する。

## 基本

- Runtime は `App` を起点に単独で起動、描画、アセット解決できる
- Editor は Runtime 共有層の上に成立してよい
- Runtime から Editor を参照しない

## Runtime

### 含めてよいもの

- `Core` `RHI` `Graphics` `Game` の実行時コード
- `Backends.D3D11` `Backends.D3D12` などの描画 API 実装
- Runtime 起動に必要な設定と初期化
- 実行時ロードする共通アセット

### 含めてはいけないもの

- Inspector `Hierarchy` `Assets` などの Editor UI
- 選択、ドラッグ、編集セッション
- Undo / Redo 履歴
- Editor 専用カメラ補助やパネル配置
- `Xelqoria::Editor` 前提の参照

## Editor

### 置いてよいもの

- `EditorCamera2D`
- `Editor.UI` の Win32 UI シェル、Dock、パネル配置
- `SceneCommandHistory`
- `SceneEditingOperations`
- SceneView 入力補助
- 永続保存のための Editor UI シェル

## 共有層

- Runtime と Editor の両方で意味を持つ保存データ
- Runtime と Editor の両方で使う描画概念
- API 非依存の抽象

## 共有化前の確認

- Runtime 単体でも必要か
- Editor 用語なしで説明できるか
- UI や一時編集ではないか

## 例

- `SceneSerializer`: `Game`
  - Runtime / Editor の両方で使う
  - Editor UI に依存しない

- `EditorCamera2D`: `Editor`
  - SceneView の pan / zoom 用
  - Runtime カメラとは別責務

- `SpriteRenderer`: `Graphics`
  - Runtime / Editor の両方で使う
  - Direct3D 型を含まない

- `RenderBackendBootstrap`: `App` または `Editor`
  - 成果物ごとの起動構成
  - 共通層に置くと境界が曖昧になる

## 確認

- Runtime ターゲットが `Editor` を参照していないか
- Runtime バイナリに `Editor` 依存が入っていないか
- `Editor` 専用型や名前空間が Runtime に露出していないか
- 便利さだけで共有層に移していないか
