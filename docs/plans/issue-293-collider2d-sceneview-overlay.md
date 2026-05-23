# ExecPlan: issue-293 SceneView に Collider2D 枠表示を追加する

## 目的

選択中 Entity の Collider2DComponent を SceneView 上の Editor 補助表示として確認できるようにする。

## 対象範囲

- 変更対象プロジェクト: `Editor`, `tests/Editor`
- 変更対象ファイル: `Editor/Source/SceneViewOverlay.h`, `Editor/Source/SceneViewRenderer.cpp`, `tests/Editor/Source/EditorCamera2DTests.cpp`
- 変更対象クラス: `SceneViewRenderer`
- 影響する機能: SceneView 選択 Entity 補助表示

## 前提

- parent issue: issue-289
- 先行 issue: issue-290, issue-291, issue-292
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-289`
- この child issue のブランチ: `issue-293`
- PR の向き: `issue-289`

## 実装方針

Collider2DComponent 自身には描画処理を持たせず、SceneViewRenderer が Editor 補助表示として描画する。

Transform と Collider2DComponent から Game 層の `BuildAabbCollider2D` を使ってワールド AABB を生成し、SceneView カメラでビュー座標へ変換した矩形枠を描画する。`enabled == false` は薄く表示し、`isTrigger == true` は通常 Collider と色とマーカーで区別する。

## 作業手順

1. SceneView 用 Collider2D overlay rect 生成処理を追加する
2. SceneViewRenderer で選択 Entity の Collider2D 枠を描画する
3. Trigger / disabled の表示差を追加する
4. overlay rect 生成テストを追加する
5. Editor テストと Editor 本体ビルドを実行する

## 検証方法

- `MSBuild.exe tests\Editor\Xelqoria.Tests.Editor.vcxproj /m /p:Configuration=Debug /p:Platform=x64 /v:minimal`
- `MSBuild.exe Editor\Xelqoria.Editor.vcxproj /m /p:Configuration=Debug /p:Platform=x64 /v:minimal`
- `artifacts\x64\Debug\Xelqoria.Tests.Editor.exe`

## 完了条件

- 選択 Entity に Collider2DComponent がある場合、SceneView に AABB 枠が表示される
- 枠は Transform position / scale / Collider offset / size を反映する
- `rotation.z` は枠回転に影響しない
- `enabled == false` は薄く表示される
- `isTrigger == true` は通常 Collider と区別できる
- Collider2DComponent 自身に描画処理を追加していない
- 関連テストが通る
- `issue-293` から `issue-289` への PR が作成されている
