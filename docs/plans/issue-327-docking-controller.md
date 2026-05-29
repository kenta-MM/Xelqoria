# ExecPlan: issue-327 Docking 操作を専用クラスへ分離する

## 目的

Dock/Floating の挙動を変更せず、Docking 操作を `EditorDockingController` へ分離する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/Shell/EditorDockingController.*`, `Editor/Source/Shell/EditorShell.*`, `Editor/Xelqoria.Editor.vcxproj`
- 変更対象クラス: `EditorShell`, `EditorDockingController`
- 影響する機能: Docking 入力処理、Dock/Floating 操作、TabControl 通知、レイアウト保存復元

## 前提

- parent issue: `#324`
- 先行 issue: `#325`, `#326`
- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-324`
- この child issue のブランチ: `issue-327`
- PR の向き: `issue-327` -> `issue-324`

## 実装方針

`EditorShell` の public Docking API は維持しつつ、呼び出し先を `EditorDockingController` へ委譲する。
既存の Docking 実装本体は `EditorShell` の内部 core 操作として残し、状態構造や保存形式を変更しない。

## 作業手順

1. #325/#326 の変更を作業ブランチへ取り込む
2. `EditorDockingController` を追加する
3. `EditorShell` の Docking public API を controller へ委譲する
4. 既存 Docking 実装を core 操作用メソッドとして整理する
5. ビルドを実行する
6. 差分に余計な挙動変更がないことを確認する
7. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `MSBuild.exe Xelqoria.slnx /p:Configuration=Debug /p:Platform=x64 /m`
- 実行するテスト: この変更では専用テスト追加なし
- 手動確認項目: Dock/Floating 操作、TabControl 切り替え、既存 Docking レイアウト読み込み、保存後の復元

## 完了条件

- Docking 操作の呼び出し口が `EditorDockingController` に分離されている
- `EditorShell` の公開 API と保存ファイル形式が維持されている
- ビルドが通る
- レイヤー責務に違反していない
- PR が `issue-324` 向けに作成されている
