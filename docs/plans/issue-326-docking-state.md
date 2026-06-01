# ExecPlan: issue-326 Docking 関連状態を専用構造へ分離する

## 目的

Dock/Floating の挙動を変更せず、`EditorShell` にある Docking 関連状態を専用構造へ分離する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/Shell/EditorShell.h`, `Editor/Source/Shell/EditorShell.cpp`, `Editor/Source/Shell/EditorShell.Docking.cpp`
- 変更対象クラス: `Xelqoria::Editor::EditorShell`
- 影響する機能: Dock tree、Floating group、Dock guide、Dock drag、Dock レイアウト保存復元

## 前提

- parent issue: `#324`
- 先行 issue: `#325`
- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-324`
- この child issue のブランチ: `issue-326`
- PR の向き: `issue-326` -> `issue-324`

## 実装方針

`EditorShell` の Docking 関連メンバを private な `DockingState` に集約する。
保存ファイル形式、Dock/Floating 操作、`EditorShell` の公開 API は変更しない。

## 作業手順

1. #325 の物理分離変更を作業ブランチへ取り込む
2. Docking 関連状態を `DockingState` に移す
3. 既存実装の参照先を `m_docking` へ更新する
4. 保存復元ロジックの形式互換を維持する
5. ビルドを実行する
6. 差分に余計な挙動変更がないことを確認する
7. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `MSBuild.exe Xelqoria.slnx /p:Configuration=Debug /p:Platform=x64 /m`
- 実行するテスト: この変更では専用テスト追加なし
- 手動確認項目: Dock/Floating 操作、既存 Docking レイアウト読み込み、保存後の復元

## 完了条件

- Docking 関連状態が `DockingState` に集約されている
- 保存ファイル形式が変更されていない
- ビルドが通る
- レイヤー責務に違反していない
- PR が `issue-324` 向けに作成されている
