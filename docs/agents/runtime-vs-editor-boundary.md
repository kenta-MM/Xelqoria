# Runtime Vs Editor Boundary

この文書は、Runtime 成果物と Editor 成果物の境界判断を、実装時に迷いやすい観点から補足する。

## 基本原則

- Runtime は `App` を起点に単独で起動、描画、アセット解決できること
- Editor は Runtime 共有層の上に成立してよい
- Runtime から Editor を参照してはいけない

## Runtime に入れてよいもの

- `Core`、`RHI`、`Graphics`、`Game` の実行時に必要なコード
- `Backends.D3D11`、`Backends.D3D12` のような描画 API 実装
- Runtime 起動に必要な設定と初期化コード
- 実行時ロードする共通アセット

## Runtime に入れてはいけないもの

- Inspector、Hierarchy、Assets パネルなどの Editor UI
- 選択状態、ドラッグ状態、編集セッション状態
- Undo/Redo 履歴
- Editor のみで意味を持つカメラ補助やパネル配置ロジック
- `Xelqoria::Editor` 名前空間を前提にした参照

## Editor に置いてよいもの

- `EditorCamera2D`
- `SceneCommandHistory`
- `SceneEditingOperations`
- SceneView の入力補助
- 永続保存のための Editor 側 UI シェル

## 共有層へ降ろしてよいもの

- Runtime と Editor の両方で意味を持つ保存データ
- Runtime と Editor の両方で使う純粋な描画概念
- API 非依存の抽象

共有層へ降ろす前に、次の説明ができるか確認する。

- Runtime 単体でも必要か
- Editor の概念名を持ち込まずに説明できるか
- UI 状態や一時編集状態ではないか

## 迷いやすい例

### `SceneSerializer`

- 置き場所: `Game`
- 理由:
  - 保存対象は Runtime / Editor の両方で扱う
  - Editor 専用 UI とは独立している

### `EditorCamera2D`

- 置き場所: `Editor`
- 理由:
  - SceneView の pan / zoom に特化している
  - Runtime のカメラ概念とは別責務

### `SpriteRenderer`

- 置き場所: `Graphics`
- 理由:
  - Runtime と Editor の両方が使える描画責務
  - Direct3D 型を含まない

### `RenderBackendBootstrap`

- 置き場所: `App` または `Editor`
- 理由:
  - 成果物ごとの起動構成に属する
  - 共通ライブラリへ置くと成果物境界が曖昧になりやすい

## レビュー観点

- Runtime ターゲットが `Editor` プロジェクトを参照していないか
- Runtime バイナリに `Editor` 由来の依存が入っていないか
- `Editor` 専用の型名や名前空間が Runtime 側へ露出していないか
- 「便利だから共有層へ移した」だけになっていないか
