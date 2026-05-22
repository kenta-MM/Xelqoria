# ExecPlan: issue-260 Scene View の背景色・境界線・既存選択枠をテーマ対応する

## 目的

Scene View の背景色、境界線、既存選択枠を `Xelqoria Dark` に合わせる。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/SceneViewRenderer.cpp`
- 変更対象クラス: `Xelqoria::Editor::SceneViewRenderer`
- 影響する機能: Scene View の描画背景、境界線、選択枠、ドラッグプレビュー輪郭

## 前提

- parent issue: #253
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-253`
- この child issue のブランチ: `issue-260`
- PR の向き: `issue-260` -> `issue-253`

## 実装方針

RHI / Backends へ Editor テーマや描画概念を追加しない。

`SceneViewRenderer` 内で `EditorThemes::XelqoriaDark` を参照し、既存の `SolidQuadRenderer` によるオーバーレイ描画で Scene View 背景、境界線、既存選択枠をテーマ色へ寄せる。グリッド、ギズモ、Transform 操作仕様は追加・変更しない。

## 作業手順

1. Scene View の描画経路と既存選択枠を確認する
2. `EditorTheme` の色を Scene View 描画用 RGBA へ変換するヘルパーを追加する
3. Scene View 背景と境界線をテーマ色で描画する
4. 既存選択枠とドラッグプレビュー輪郭をテーマ色へ変更する
5. ビルドとレイヤー依存チェックを実行する
6. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `Xelqoria.Editor.vcxproj` Debug x64
- 実行するテスト: `LayerDependencyChecker`
- 手動確認項目: Scene View の背景、境界線、選択枠がテーマ色で表示される

## 完了条件

- Scene View の背景色がテーマに沿っている
- Scene View の境界線がテーマに沿っている
- 既存選択枠がテーマ色に沿っている
- 既存ステータス表示がテーマ色に沿っている
- Scene View のグリッド描画を新規追加していない
- 新規ギズモ描画や Transform 操作仕様の変更を含んでいない
- Editor / Graphics / RHI / Backends の依存方向を崩していない
- ビルドが通る
- `issue-260` から `issue-253` への PR が作成されている
