# ExecPlan: issue-337 既存プロジェクトの Assets 配下移行処理を追加する

## 目的

既存プロジェクトを開いたとき、旧構成のルート直下アセットや旧ディレクトリを新しい `Assets` 配下の標準構成へ移行する。

## 対象範囲

- 変更対象プロジェクト: `Editor`, `tests/Editor`
- 変更対象ファイル: `EditorProject.h`, `EditorProject.cpp`, `EditorSceneDocument.h`, `EditorSceneDocument.cpp`, `Application.cpp`, `EditorProjectTests.cpp`
- 変更対象クラス: `Xelqoria::Editor::EditorProject`, `Xelqoria::Editor::EditorSceneDocument`, `Xelqoria::Editor::Application`
- 影響する機能: 既存プロジェクト読込、旧プロジェクト移行、Editor ログ出力

## 前提

- parent issue: issue-334
- 先行 child issue: issue-335
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-334`
- この child issue のブランチ: `issue-337`
- PR の向き: `issue-337` -> `issue-334`

## 実装方針

`EditorProject::Open` で `projectStructureVersion` が未定義または現行より古いプロジェクトを検出し、プロジェクトルート配下のユーザーアセットを `Assets` 配下へ移動する。

移行対象は Scene、Texture、Material、Script、ProjectSettings に限定し、`.git`、`.vs`、`.xelqoria`、`Assets`、`ProjectSettings`、ビルド出力系ディレクトリは移行対象から除外する。移動先に同名ファイルがある場合は `_1`、`_2` 形式で一意化する。移行後は新しい `.xelqoria` メタデータを保存し、旧 `.proj` は削除せず残す。

移行結果のメッセージは `EditorProject` から `EditorSceneDocument` 経由で `Application` に渡し、既存プロジェクトを開いた直後に LogOutput へ追記する。

## 作業手順

1. 旧プロジェクト判定用に `projectStructureVersion` を読み取る
2. 旧構成の場合に標準ディレクトリを作成する
3. ルート直下の移行対象ファイルを `Assets/*` または `ProjectSettings` へ移動する
4. 旧 `Scenes`、`Textures`、`Materials`、`Scripts` の内容を `Assets/*` に統合する
5. 衝突時に一意な移動先パスを生成する
6. 新しい `.xelqoria` メタデータを書き出す
7. 移行ログを Editor の LogOutput に表示する
8. 移行あり、移行なし、legacy `.proj` のテストを追加・更新する
9. Editor テストと差分チェックを実行する

## 検証方法

- 実行するビルド: `MSBuild tests/Editor/Xelqoria.Tests.Editor.vcxproj /p:Configuration=Debug /p:Platform=x64 /m`
- 実行するテスト: `artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`
- 実行する静的確認: `git diff --check`
- 手動確認項目: 移行後の `.xelqoria` の相対パス、衝突時のリネーム、現行構成で再移行されないこと

## 完了条件

- 旧プロジェクトを開くと標準構成が作成される
- 旧ルート直下アセットと旧ディレクトリ内容が `Assets` 配下へ移動される
- 同名ファイル衝突時に既存ファイルを上書きしない
- 移行後の `.xelqoria` に現行 `projectStructureVersion` と相対パスが保存される
- 現行構成のプロジェクトを開いても再移行されない
- 移行結果が Editor の LogOutput に表示される
- 関連テストが通る
- `issue-337` から `issue-334` への PR が作成されている
