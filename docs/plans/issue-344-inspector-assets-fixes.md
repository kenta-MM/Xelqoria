# ExecPlan: issue-344 Inspector and Assets fixes

## 目的

Inspector パネルの Component 表示整理に伴って発生した Assets 右クリック、Script D&D、Sprite 作成、初期 Scene、Inspector スクロールの不具合を修正する。

## 対象範囲

- 変更対象プロジェクト: Editor
- 変更対象ファイル: AssetsPanelController, InspectorPanelView, InspectorPanelController, Application, NewProject Main.scene
- 変更対象クラス: AssetsPanelController, InspectorPanelView, InspectorPanelController, Application
- 影響する機能: Assets 作成メニュー、Inspector Script 割り当て、Scene 生成、Hierarchy 表示、Inspector レイアウト
- 追加調査: Assets で作成した Script Asset の SceneView ドロップ割り当て
- 追加調査: Texture 未設定 Sprite Asset の SceneView ドロップ生成

## 前提

- parent issue: なし
- ブランチ運用ルール: `branch-rules.md`
- この issue のブランチ: `issue-344`
- PR の向き: `issue-344` -> `main`
- Game / Graphics / RHI / Backends の依存方向は変更しない。
- Asset 作成と Scene 生成は分離する。

## 実装方針

Editor 内の Win32 UI 制御に限定して修正し、Runtime レイヤーや描画レイヤーの依存関係は変更しない。Assets 右クリックは ListView の `NM_RCLICK` を主経路にし、空白領域の補助だけ `WM_CONTEXTMENU` で扱う。Sprite Asset 作成はファイル作成と Assets 更新だけにし、Scene への Entity 生成は Assets から Scene View への D&D 経路に限定する。

Inspector は Script セクション全体をドロップ対象に戻し、既存どおり `SpriteAsset::scriptAssetId` を更新する。縦スクロールは View が `scrollOffsetY` と `contentHeight` を持ち、Layout 時にスクロール分を差し引いて子コントロールを配置する。

Script Asset を SceneView へドロップした場合は Entity を生成せず、ドロップ位置にある描画中 Sprite Entity の `SpriteAsset::scriptAssetId` へ割り当てる。`ScriptComponent` は追加しない。

Texture 未設定の Sprite Asset を SceneView へドロップした場合も Entity を生成する。SceneView では Texture 解決済みでない SpriteComponent をプレースホルダー表示とヒット判定の対象にし、Hierarchy だけに Entity が見える状態を避ける。

## 作業手順

1. 関連コードと初期 Scene を確認する。
2. Assets 右クリック処理を一本化する。
3. Script D&D 対象範囲を Script セクション全体へ広げる。
4. Sprite Asset 作成時の Scene Entity 生成を削除する。
5. 新規プロジェクト用 `Main.scene` を空 Scene に戻す。
6. Inspector に縦スクロールと重なり回避を実装する。
7. Script Asset の SceneView ドロップを Sprite Asset 割り当てとして実装する。
8. Editor Debug x64 ビルドと既存 Editor テストを実行する。
9. Texture 未設定 Sprite Asset の SceneView ドロップで Entity が生成されることを確認する。

## 検証方法

- 実行するビルド: Editor Debug x64
- 実行するテスト: 既存 Editor テスト
- 手動確認項目:
  - Assets 右クリックから Sprite / Script を 1 回の選択で作成できる。
  - Script を Assets から Inspector の Script 欄へ D&D すると割り当てられる。
  - Assets で Sprite を作成しても Scene / Hierarchy に Entity が追加されない。
  - Assets から Scene へ Sprite を D&D したときだけ Entity が生成される。
  - Texture 未設定 Sprite Asset を SceneView へ D&D しても Entity が生成され、SceneView にプレースホルダー表示される。
  - Assets から SceneView 上の Sprite Entity へ Script を D&D すると、その Entity の Sprite Asset に Script が割り当てられる。
  - 新規起動時、空 Scene なら Hierarchy も空である。
  - Inspector を縦にスクロールでき、UI が重ならない。

## 完了条件

- issue-344 の修正内容を満たしている。
- ビルドが通る。
- 関連テストが通る。
- レイヤー責務に違反していない。
- 不要な変更が含まれていない。
