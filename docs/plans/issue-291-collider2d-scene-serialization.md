# ExecPlan: issue-291 Scene 保存・読み込みを Collider2DComponent に対応する

## 目的

Scene 保存形式を Collider2DComponent に対応させ、既存 version=1 Scene の互換読み込みを維持する。

## 対象範囲

- 変更対象プロジェクト: `Game`, `tests/Game`
- 変更対象ファイル: `Game/Source/SceneSaveFormat.h`, `Game/Source/SceneTextWriter.cpp`, `Game/Source/SceneTextReader.cpp`, `tests/Game/Source/TransformTests.cpp`
- 変更対象クラス: `SceneTextWriter`, `SceneTextReader`, `SceneEntitySaveRecord`
- 影響する機能: Scene 保存・読み込み、保存形式バージョン、Collider2DComponent 復元

## 前提

- parent issue: issue-289
- 先行 issue: issue-290
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-289`
- この child issue のブランチ: `issue-291`
- PR の向き: `issue-289`

## 実装方針

Scene 保存時は現在バージョンを `2` とし、Entity ごとに `hasCollider2DComponent` を必ず出力する。

読み込み時は `version=1` を Collider2DComponent なしとして受け入れ、`version=2` では Collider2DComponent の各フィールドを読み込む。bool、shapeType、Vector2、非正の size は読み込みエラーにする。

## 作業手順

1. Scene 保存形式の現在バージョンを 2 に更新する
2. Collider2D 用保存レコードを追加する
3. SceneTextWriter で Collider2DComponent を出力する
4. SceneTextReader で Collider2DComponent フィールドを解析・検証する
5. version=1 互換読み込みと version=2 保存・読み込みのテストを追加する
6. レイヤー依存チェックと Game テストを実行する

## 検証方法

- `dotnet run --project tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug -- "D:\github\Xelqoria"`
- `MSBuild.exe tests\Game\Xelqoria.Tests.Game.vcxproj /m /p:Configuration=Debug /p:Platform=x64 /v:minimal`
- `artifacts\x64\Debug\Xelqoria.Tests.Game.exe`

## 完了条件

- Scene 保存時に Collider2DComponent の有無と各フィールドが出力される
- Collider2DComponent がない Entity は `hasCollider2DComponent=false` のみ出力される
- Scene 読み込み時に Collider2DComponent が復元される
- `version=1` Scene は Collider2DComponent なしとして読み込める
- 不正な bool、shapeType、offset、size、非正の size は読み込みエラーになる
- 関連テストが通る
- `issue-291` から `issue-289` への PR が作成されている
