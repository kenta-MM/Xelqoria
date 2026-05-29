# ExecPlan: panel source folder split

## 目的

Editor/Source/Panels 配下の Panel 関連ソースを、Panel 種別ごとのサブフォルダへ整理する。

## 対象範囲

- 変更対象プロジェクト: Editor, tests/Editor
- 変更対象ファイル: Editor/Source/Panels 配下の Panel controller/view、Editor/Source/SceneView/SceneViewController、Editor project files、関連 include
- 変更対象クラス: AssetsPanelController/View, HierarchyPanelController/View, InspectorPanelController/View, SceneViewController, SceneViewPanelView, LogOutputPanelController/View, MaterialPanelController
- 影響する機能: Editor UI ビルド、Editor テストビルド

## 前提

- ブランチ運用ルール: `branch-rules.md`
- PR の向き: 指定なし

## 実装方針

Panel 共通基盤は Editor/Source/Panels 直下に残す。Panel 種別に紐づく controller/view は Assets, Hierarchy, Inspector, SceneView, LogOutput の各サブフォルダへ移す。MaterialPanelController は専用 MaterialPanelView 削除後も Inspector 内 Material 編集ヘルパとして使われているため、Inspector 配下へ移す。

## 作業手順

1. 現在の include と project 登録を確認する
2. 対象ファイルをサブフォルダへ移動する
3. include パスと project/filter 登録を更新する
4. 残参照を確認する
5. Editor 本体と Editor テストをビルドする
6. Editor テストを実行する

## 検証方法

- MSBuild: `Editor/Xelqoria.Editor.vcxproj` Debug x64
- MSBuild: `tests/Editor/Xelqoria.Tests.Editor.vcxproj` Debug x64
- Test: `artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`

## 完了条件

- 指定のフォルダ構成へ移動されている
- include と project 登録に古いパスが残っていない
- Editor ビルドが通る
- Editor テストが通る

## 実施結果

- Panel 種別ごとのサブフォルダへソースを移動した
- `Editor/Xelqoria.Editor.vcxproj` と `.filters` を新しい配置へ更新した
- `Editor/Xelqoria.Editor.vcxproj` Debug x64 ビルド成功
- `tests/Editor/Xelqoria.Tests.Editor.vcxproj` Debug x64 ビルド成功
- `Xelqoria.Tests.Editor.exe` 実行成功: 77 tests passed
