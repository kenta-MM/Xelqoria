# ExecPlan: issue-249 Inspector で SpriteComponent に Material を設定・編集できるようにする

## 目的

Inspector の SpriteComponent 表示を Material 参照中心へ移行し、Material アセットの作成、表示、D&D 設定を可能にする。
Material の Texture / Tint(Color) / Outline 編集は Inspector から分離し、右側ペインの Material タブで行えるようにする。

## 対象範囲

- 変更対象プロジェクト: Game, Editor
- 変更対象ファイル: `Game/Source/SpriteComponent.h`, `Game/Source/Assets/*`, `Game/Source/Scene*.cpp`, `Editor/Source/*`
- 変更対象クラス: `SpriteComponent`, `EditorSceneDocument`, `AssetsPanelController`, `InspectorPanelController`, `MaterialPanelController`, `EditorShell`
- 影響する機能: Scene 保存、Asset 登録、Assets 右クリック作成、Inspector 表示と D&D、Material タブ表示と編集

## 前提

- parent issue: issue-235
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-235`
- この child issue のブランチ: `issue-249`
- PR の向き: `issue-249` から `issue-235`

## 実装方針

Material アセットは Game の永続データとして `SpriteMaterialAsset` を追加し、描画 API 固有型や Backends への依存は持たせない。Editor は `.material` ファイルを Assets 上で作成・登録し、SpriteComponent は `materialAssetRef` を保持する。既存の `spriteAssetRef` は互換用に残し、Material 未設定の既存 Scene では既存 SpriteAsset の Texture をもとに Material を自動作成して参照へ移行する。

SpriteComponent Inspector は Material の参照表示、D&D 差し替え、Open 操作だけを扱う。Texture / Color / Outline など Material の中身は右側ペインの Material タブに集約し、共有 Material を編集している注意表示を出す。暗黙的な Material 複製は行わない。

描画時の Sprite 解決は、SpriteComponent に Material 参照がある場合は Material アセットの Texture / Tint(Color) / Outline を優先して使用する。Material 参照が空の既存データでは従来の SpriteAsset Texture 解決を維持する。

## 作業手順

1. 関連コードを確認する
2. Material アセットのデータ型、Loader、Registry を追加する
3. Scene 保存/読込と Sprite 解決を Material 参照に対応させる
4. EditorSceneDocument に `.material` 作成・登録・移行処理を追加する
5. AssetsPanelController に Material 作成メニューと D&D 状態を追加する
6. InspectorPanelController に Material 参照欄と Open 操作を追加する
7. EditorShell に Inspector と同じ右側ペインの Material タブを追加する
8. MaterialPanelController に Material 詳細編集を追加する
9. 関連テストを追加・更新する
10. ビルドまたはテストを実行する

## 検証方法

- 実行するビルド: 可能なら `tools/wsl/build.sh`
- 実行するテスト: 可能なら `tools/wsl/test.sh`
- 手動確認項目: Material 作成、Assets 表示、Inspector への Material D&D、Assets から `.material` ダブルクリック、Inspector の Open、Material タブでの Texture / Tint / Outline 編集、既存 Texture 参照の Material 移行

## 完了条件

- SpriteComponent の Inspector に Material 欄が表示される
- Material 設定時に Inspector から Material タブを開ける
- Assets 上の `.material` ダブルクリックで Material タブを開ける
- Material タブで Texture / Tint(Color) / OutlineEnabled / OutlineThickness / OutlineColor を編集できる
- SpriteComponent Inspector では Material の中身を直接編集しない
- Material タブに共有 Material 編集の注意表示がある
- Assets 上で Material アセットを作成・表示できる
- Material アセットを SpriteComponent へ D&D 設定できる
- 既存 Texture 設定済み SpriteComponent が Material 参照へ移行される
- Script 欄と Transform 欄の既存挙動が維持されている
- レイヤー責務に違反していない
