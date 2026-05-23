# ExecPlan: issue-290 Game 層に Collider2DComponent と AABB 変換を追加する

## 目的

Entity に 2D Collider 情報を保持する Game 層の最小構成を追加し、既存 Physics2D の `AabbCollider2D` へ変換できるようにする。

## 対象範囲

- 変更対象プロジェクト: `Game`, `Editor`, `tests/Game`, `tests/Editor`
- 変更対象ファイル: `Game/Source/Collider2DComponent.*`, `Game/Source/Entity.*`, `Editor/Source/SceneEditingOperations.*`
- 変更対象クラス: `Entity`, `SceneEditingOperations`
- 影響する機能: Entity の Component 保持、Entity 複製、Scene 編集操作、AABB 変換

## 前提

- parent issue: issue-289
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-289`
- この child issue のブランチ: `issue-290`
- PR の向き: `issue-289`

## 実装方針

Collider2DComponent は Game 層の Component として追加し、Graphics / RHI / Backends には依存させない。

初期形状は `Box` のみにし、Transform の position / scale と Collider の offset / size から Physics2D の `AabbCollider2D` を生成する。`rotation.z` は AABB 変換に反映しない。

## 作業手順

1. `Collider2DShapeType` と `Collider2DComponent` を追加する
2. `BuildAabbCollider2D` を追加する
3. `Entity` に Collider2DComponent の保持、取得、有無判定、削除 API を追加する
4. `SceneEditingOperations` に追加・削除 API と複製時コピーを追加する
5. Game / Editor の関連テストを追加する
6. レイヤー依存チェック、Game / Editor テストのビルドと実行を行う

## 検証方法

- `dotnet run --project tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug -- "D:\github\Xelqoria"`
- `MSBuild.exe tests\Game\Xelqoria.Tests.Game.vcxproj /m /p:Configuration=Debug /p:Platform=x64 /v:minimal`
- `MSBuild.exe tests\Editor\Xelqoria.Tests.Editor.vcxproj /m /p:Configuration=Debug /p:Platform=x64 /v:minimal`
- `artifacts\x64\Debug\Xelqoria.Tests.Game.exe`
- `artifacts\x64\Debug\Xelqoria.Tests.Editor.exe`

## 完了条件

- Entity に Collider2DComponent を追加、取得、有無判定、削除できる
- SceneEditingOperations の追加・削除 API が変更有無を返す
- Entity 複製時に Collider2DComponent がコピーされる
- Transform と Collider2DComponent から AABB を生成できる
- 負の scale は halfSize に絶対値として反映され、rotation.z は影響しない
- レイヤー責務に違反していない
- 関連テストが通る
- `issue-290` から `issue-289` への PR が作成されている
