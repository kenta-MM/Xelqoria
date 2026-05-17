# ExecPlan: issue-215 現行のドッキング仕様を維持したままドッキング処理を安定化する

## 目的

現行仕様のドッキング可能な配置箇所を維持したまま、既存の全ビューを安定して別のドッキング領域へ移動できるようにする。

## 対象範囲

- 変更対象プロジェクト: Editor
- 変更対象ファイル: `Editor/Source/EditorShell.cpp`, `Editor/Source/EditorShell.h`
- 変更対象クラス: `Xelqoria::Editor::EditorShell`
- 影響する機能: Dock ツリー、Dock ガイド、Dock プレビュー、再ドッキング

## 前提

- parent issue: issue-214
- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-214`
- この child issue のブランチ: `issue-215`
- PR の向き: `issue-215` から `issue-214`

## 実装方針

Editor 専用の Win32 UI シェルである `EditorShell` 内に責務を閉じる。

既存の Dock ガイド種別、配置先、Dock ツリー構造は増減させず、再ドッキング時に同一パネルが Dock ツリー内で重複しないようにする。

## 作業手順

1. 関連コードを確認する
2. 同一 Dock leaf 内で split ドロップする場合の状態遷移を安定化する
3. Dock タブ同期とレイアウト再計算の既存経路に合わせる
4. ビルドまたは関連テストを実行する
5. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: 可能なら Editor またはソリューションの Debug x64 ビルド
- 実行するテスト: 可能なら Editor 関連テスト
- 手動確認項目: 全ビューのタブを別 Dock leaf へドロップして、枠線プレビューと再ドッキング後の操作が破綻しないこと

## 完了条件

- 現行の Dock ガイド配置先が維持されている
- 再ドッキング後にパネルが重複表示されない
- 再ドッキング後も対象ビューを正常に操作できる
- レイヤー責務に違反していない
- 不要な変更が含まれていない
- `branch-rules.md` に従った PR が作成されている
