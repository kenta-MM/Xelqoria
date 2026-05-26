# ExecPlan: issue-300 共通UI定数とUI状態永続化の基盤追加

## 目的

Editor UI 改善の土台として、Inspector 折りたたみ状態と Hierarchy 展開状態を保持し、保存・復元できる基盤を追加する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/HierarchyPanelController.*`, `Editor/Source/InspectorPanelController.*`
- 変更対象クラス: `HierarchyPanelController`, `InspectorPanelController`
- 影響する機能: Hierarchy 展開状態、Inspector 折りたたみ状態

## 前提

- parent issue: issue-299
- parent issue ブランチ: `issue-299`
- この child issue のブランチ: `issue-300`
- PR の向き: `issue-300` -> `issue-299`

## 実装方針

Win32 描画処理とは切り離し、各 Controller が UI 状態を保持する。保存失敗時は通常操作を妨げない。

## 作業手順

1. Hierarchy 展開状態をローカル状態ファイルへ保存・復元する
2. Inspector 折りたたみ状態を保持し、ローカル状態ファイルへ保存・復元する関数を追加する
3. Editor ビルドと Editor テストを実行する

## 検証方法

- `Editor/Xelqoria.Editor.vcxproj` Debug x64 ビルド
- `tests/Editor/Xelqoria.Tests.Editor.vcxproj` Debug x64 ビルド
- `artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`

## 完了条件

- Inspector 折りたたみ状態を Component 種別ごとに保持できる
- Hierarchy 展開状態を GameObject ごとに保持できる
- 保存失敗時に通常操作が妨げられない
