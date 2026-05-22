# ExecPlan: issue-276 Inspector で Transform / Sprite / Material / Script を編集できるようにする

## 目的

Inspector で選択中 Entity / Asset の情報を表示し、Entity の主要 Component を編集できるようにする。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/AssetsPanelController.h`, `Editor/Source/AssetsPanelController.cpp`, `Editor/Source/InspectorPanelController.h`, `Editor/Source/InspectorPanelController.cpp`, `Editor/Source/Application.cpp`
- 変更対象クラス: `AssetsPanelController`, `InspectorPanelController`, `Application`
- 影響する機能: Inspector 表示、Assets 選択表示、Transform / Sprite / Material / Script 編集

## 前提

- parent issue: #269
- 先行 issue: #270, #271, #272, #273
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-269`
- この child issue のブランチ: `issue-276`
- PR の向き: `issue-276` -> `issue-269`

## 実装方針

既存の Transform 9項目編集、Sprite Material 参照、Script 表示 / 作成 / 割当 / 解除、Material パネルの編集経路を維持する。不足している Asset 選択時の Inspector 表示を追加するため、Assets 側から選択中ファイルパスを渡す。

## 作業手順

1. `issue-273` をこのブランチへローカルマージする
2. AssetsPanelController から選択中ファイルパスを取得できるようにする
3. Inspector Refresh に選択中 Asset パスを渡す
4. Entity 未選択かつ Asset 選択時に Asset 情報を Inspector summary に表示する
5. 既存 Entity 編集経路の挙動を維持する
6. ビルドまたはレイヤー依存チェックを実行する
7. PR を作成する

## 検証方法

- 実行するビルド: `dotnet build tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug`
- 実行するテスト: LayerDependencyChecker
- 手動確認項目: Entity 選択時の Transform / Sprite / Script 表示と、Asset 選択時の Inspector summary 表示

## 完了条件

- Entity 選択時に Transform が表示される
- Position / Rotation / Scale を編集できる
- Sprite / Material / Script 参照を表示・編集できる
- Script 未設定時に None と表示される
- Script 追加ボタンが表示される
- Asset 選択時に選択アセットの情報が表示される
- 既存のレイヤー責務を破壊していない
- `issue-276` から `issue-269` への PR が作成されている
