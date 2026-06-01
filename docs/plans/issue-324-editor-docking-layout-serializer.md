# ExecPlan: issue-324 EditorDockingLayoutSerializer responsibility split

## 目的

`EditorDockingLayoutSerializer` が Dock/Floating レイアウトの保存形式、ファイル I/O、読み込みパースを担当し、`EditorDockingController` は UI 状態管理と Snapshot 変換に集中する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル:
  - `Editor/Source/Shell/EditorDockingController.h`
  - `Editor/Source/Shell/EditorDockingController.cpp`
  - `Editor/Source/Shell/EditorDockingLayoutSerializer.cpp`
  - `Editor/Source/Shell/EditorDockingLayoutSnapshot.h`
  - `Editor/Source/Shell/EditorShell.cpp`
  - `Editor/Xelqoria.Editor.vcxproj`
  - `Editor/Xelqoria.Editor.vcxproj.filters`
- 変更対象クラス:
  - `EditorDockingController`
  - `EditorDockingLayoutSerializer`
  - `EditorDockingLayoutSnapshot`
- 影響する機能: Editor Dock/Floating レイアウトの保存・復元

## 前提

- parent issue: なし
- ブランチ運用ルール: `branch-rules.md`
- この issue のブランチ: `issue-324`
- PR の向き: `issue-324 -> main`

## 実装方針

Editor レイヤー内に保存用 DTO として `EditorDockingLayoutSnapshot` を追加する。Serializer は Controller の private 型に触れず、public API の `CreateLayoutSnapshot` / `ApplyLayoutSnapshot` 経由で保存・復元を行う。既存の layout signature/version と読み書き形式は維持する。

## 作業手順

1. 関連コードと既存レイアウト保存形式を確認する
2. Snapshot 型と Controller API を追加する
3. 保存・読み込み・panel 名変換を Serializer へ移す
4. Controller から `friend`、`SaveLayoutCore`、`LoadLayoutCore`、保存専用構造と `<fstream>` を削除する
5. project/filter に新規ヘッダを追加する
6. ビルドまたは可能な静的検証を実行する

## 検証方法

- 実行するビルド: `MSBuild.exe Editor/Xelqoria.Editor.vcxproj /p:Configuration=Debug /p:Platform=x64`
- 実行するテスト: `MSBuild.exe tests/Editor/Xelqoria.Tests.Editor.vcxproj /p:Configuration=Debug /p:Platform=x64`、`artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`
- 手動確認項目: 保存済み layout file の互換性、Dock tree と Floating group の復元

## 完了条件

- `friend class EditorDockingLayoutSerializer;` が削除されている
- `SaveLayoutCore` / `LoadLayoutCore` が削除されている
- Serializer が保存・読み込みを実装している
- Controller が Snapshot 作成・適用のみを担当している
- 既存 layout file 形式が維持されている
- ビルドが通る、または実行不可理由が明確になっている
