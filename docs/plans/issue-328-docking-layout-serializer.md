# ExecPlan: issue-328 Docking レイアウト保存復元を専用責務へ分離する

## 目的

Dock/Floating の挙動と保存ファイル形式を変更せず、Docking レイアウト保存復元を serializer 相当の専用責務へ分離する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/Shell/EditorDockingLayoutSerializer.*`, `Editor/Source/Shell/EditorDockingController.*`, `Editor/Source/Shell/EditorShell.h`, `Editor/Xelqoria.Editor.vcxproj`
- 変更対象クラス: `EditorDockingLayoutSerializer`, `EditorDockingController`, `EditorShell`
- 影響する機能: Docking レイアウト保存復元

## 前提

- parent issue: `#324`
- 先行 issue: `#325`, `#326`, `#327`
- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-324`
- この child issue のブランチ: `issue-328`
- PR の向き: `issue-328` -> `issue-324`

## 実装方針

`EditorDockingLayoutSerializer` を追加し、`EditorDockingController` の保存復元 API から serializer へ委譲する。
既存の保存復元 core 処理とファイル形式は変更せず、互換性を維持する。

## 作業手順

1. #325/#326/#327 の変更を作業ブランチへ取り込む
2. `EditorDockingLayoutSerializer` を追加する
3. `EditorDockingController` の保存復元処理を serializer へ委譲する
4. 保存ファイル形式を変更していないことを確認する
5. ビルドを実行する
6. 差分に余計な挙動変更がないことを確認する
7. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `MSBuild.exe Xelqoria.slnx /p:Configuration=Debug /p:Platform=x64 /m`
- 実行するテスト: この変更では専用テスト追加なし
- 手動確認項目: 既存 Docking レイアウト読み込み、保存後の再起動復元

## 完了条件

- Docking レイアウト保存復元の呼び出し口が `EditorDockingLayoutSerializer` に分離されている
- 保存ファイル形式が変更されていない
- ビルドが通る
- レイヤー責務に違反していない
- PR が `issue-324` 向けに作成されている
