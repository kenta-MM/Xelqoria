# ExecPlan: issue-275 Scene ビューでグリッド・カメラ枠・選択枠・軸ギズモを表示する

## 目的

中央 Scene / Game ビューに、Scene 編集用の補助表示と基本操作を実装する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/SceneViewRenderer.cpp`, `Editor/Source/SceneViewController.cpp`, `Editor/Source/SceneViewInputTracker.h`, `Editor/Source/SceneViewInputTracker.cpp`
- 変更対象クラス: `SceneViewRenderer`, `SceneViewController`, `SceneViewInputTracker`
- 影響する機能: SceneView 補助描画、クリック選択、ドラッグ移動、ズーム、パン

## 前提

- parent issue: #269
- 先行 issue: #270, #271, #272
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-269`
- この child issue のブランチ: `issue-275`
- PR の向き: `issue-275` -> `issue-269`

## 実装方針

`issue-272` の固定 Scene / Game 領域を前提に、既存 Renderer の責務内で補助表示を描画する。Game のデータオブジェクトに描画処理を持たせず、RHI / Backends へ Editor 固有概念を追加しない。

## 作業手順

1. `issue-272` をこのブランチへローカルマージする
2. SceneViewRenderer にカメラ枠と軸ギズモを追加する
3. SceneViewInputTracker でホイールズームを追加する
4. Space + 左ドラッグでパンできるようにする
5. SceneView の状態表示にズーム率を表示する
6. ビルドまたはレイヤー依存チェックを実行する
7. PR を作成する

## 検証方法

- 実行するビルド: `dotnet build tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug`
- 実行するテスト: LayerDependencyChecker
- 手動確認項目: グリッド、X/Y 軸、カメラ枠、選択枠、軸ギズモ、ズーム率が表示され、ホイールズームと Space + 左ドラッグパンが動作する

## 完了条件

- Scene / Game タブが表示される
- Scene にグリッド、X 軸 / Y 軸、カメラ枠、選択枠、軸ギズモ、ズーム率が表示される
- Entity / Sprite をクリック選択できる
- 選択中 Entity をドラッグ移動できる
- マウスホイールでズームできる
- 指定操作でパンできる
- 選択状態が Hierarchy / Inspector と同期する
- 既存のレイヤー責務を破壊していない
- `issue-275` から `issue-269` への PR が作成されている
