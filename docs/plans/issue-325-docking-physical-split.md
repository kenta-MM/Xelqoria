# ExecPlan: issue-325 Docking 関連実装を物理的に別ファイルへ分離する

## 目的

Dock/Floating の挙動を変更せず、`EditorShell` 内の Docking 関連実装を物理的に別 `.cpp` ファイルへ分離する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/Shell/EditorShell.cpp`, `Editor/Source/Shell/EditorShell.Docking.cpp`
- 変更対象クラス: `Xelqoria::Editor::EditorShell`
- 影響する機能: Dock/Floating 操作、Dock レイアウト保存復元、Dock TabControl 表示

## 前提

- parent issue: `#324`
- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-324`
- この child issue のブランチ: `issue-325`
- PR の向き: `issue-325` -> `issue-324`

## 実装方針

`EditorShell` のクラス構造、メンバ状態、公開 API、保存ファイル形式は変更しない。
Dock/Floating 関連メソッドの実装だけを `EditorShell.Docking.cpp` へ移し、既存の匿名 namespace ヘルパーを共有するため `EditorShell.cpp` から include する。

## 作業手順

1. 関連コードを確認する
2. Dock/Floating 関連実装範囲を特定する
3. 実装を `EditorShell.Docking.cpp` へ移す
4. 既存の include とクラス状態を維持する
5. ビルドを実行する
6. 差分に余計な挙動変更がないことを確認する
7. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `msbuild Xelqoria.sln /p:Configuration=Debug /p:Platform=x64`
- 実行するテスト: この変更では専用テスト追加なし
- 手動確認項目: Dock/Floating 操作、既存 Docking レイアウト読み込み、保存後の復元

## 完了条件

- Dock/Floating 関連実装が `EditorShell.Docking.cpp` へ物理分離されている
- `EditorShell` の状態構造と保存ファイル形式が変更されていない
- ビルドが通る
- レイヤー責務に違反していない
- PR が `issue-324` 向けに作成されている
