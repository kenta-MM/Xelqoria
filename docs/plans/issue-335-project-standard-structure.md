# ExecPlan: issue-335 新規プロジェクト標準構成とプロジェクトファイル保存内容を更新する

## 目的

新規プロジェクト作成時に、ユーザー管理対象の Assets とプロジェクト管理ファイルを分離した標準構成を生成する。

## 対象範囲

- 変更対象プロジェクト: `Editor`, `tests/Editor`
- 変更対象ファイル: `EditorProject.h`, `EditorProject.cpp`, `EditorProjectTests.cpp`
- 変更対象クラス: `Xelqoria::Editor::EditorProject`
- 影響する機能: 新規プロジェクト作成、プロジェクトファイル保存、既存プロジェクト読込

## 前提

- parent issue: issue-334
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-334`
- この child issue のブランチ: `issue-335`
- PR の向き: `issue-335` -> `issue-334`

## 実装方針

新規プロジェクト作成時の標準構成を `EditorProject` に集約する。

新しいプロジェクトファイルは `<ProjectName>.xelqoria` とし、`projectStructureVersion`、`assetRoot`、`projectSettings`、`startupScene` をプロジェクトルートからの相対パスで保存する。既存 `.proj` 形式は Open 側で引き続き読み込めるようにする。

## 作業手順

1. `EditorProject` のプロジェクト情報に Assets、ProjectSettings、内部ディレクトリのパスを追加する
2. 新規作成時に `Assets/Scenes`、`Assets/Textures`、`Assets/Materials`、`Assets/Scripts`、`ProjectSettings`、`.xelqoria/cache` を作成する
3. 初期 Scene を `Assets/Scenes/Main.scene` に保存する
4. `<ProjectName>.xelqoria` に新しいメタデータを保存する
5. 既存 `.proj` 読み込み互換を維持する
6. EditorProject のテストを新構成に更新する
7. Editor テストまたはビルドを実行する

## 検証方法

- 実行するビルド: `dotnet build tools/LayerDependencyChecker/LayerDependencyChecker.csproj` または利用可能な Visual Studio ビルド
- 実行するテスト: `Xelqoria.Tests.Editor`
- 手動確認項目: 生成されたプロジェクトファイルの相対パス、標準ディレクトリ構成

## 完了条件

- 新規プロジェクト作成時に標準構成のディレクトリとファイルが作成される
- `.xelqoria` に `projectStructureVersion`、`assetRoot`、`projectSettings`、`startupScene` が保存される
- `.xelqoria` 内のパスがプロジェクトルートからの相対パスで保存される
- 初期 Scene が `Assets/Scenes/Main.scene` に配置される
- 関連テストが通る
- `issue-335` から `issue-334` への PR が作成されている
