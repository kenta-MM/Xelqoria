# ExecPlan: issue-304 全体の見た目統一と受け入れ確認

## 目的

issue-300 から issue-303 の変更を統合し、完成図に近い一貫した見た目へ整える。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/EditorShell.cpp`
- 変更対象クラス: `EditorShell`
- 影響する機能: Editor 共通ボタン表示、Inspector / Hierarchy / Log の統合表示

## 前提

- parent issue: issue-299
- 先行 child issue: issue-300, issue-301, issue-302, issue-303
- parent issue ブランチ: `issue-299`
- この child issue のブランチ: `issue-304`
- PR の向き: `issue-304` -> `issue-299`

## 実装方針

各パネルの個別機能は先行 issue に置き、ここでは全体統一の見た目調整と統合確認に限定する。

## 作業手順

1. 先行 child issue ブランチを取り込む
2. 主要アクションと削除系アクションのボタン表示を区別する
3. Editor ビルドと Editor テストを実行する
4. 差分が親 issue の目的を満たすことを確認する

## 検証方法

- `Editor/Xelqoria.Editor.vcxproj` Debug x64 ビルド
- `tests/Editor/Xelqoria.Tests.Editor.vcxproj` Debug x64 ビルド
- `artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`

## 完了条件

- Inspector / Hierarchy / Log の変更が同時に成立する
- 削除系操作が通常操作と見分けやすい
- レイヤー責務に違反していない
- Editor ビルドと Editor テストが通る
