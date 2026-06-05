# ExecPlan: issue-336 Assets ビューとファイル監視対象を Assets 配下に限定する

## 目的

Assets ビューとファイル監視の対象を `<ProjectRoot>/Assets` 配下に限定し、プロジェクト管理ファイルや内部ディレクトリを Assets ビューから除外する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `AssetsPanelController.h`, `AssetsPanelController.cpp`
- 変更対象クラス: `Xelqoria::Editor::AssetsPanelController`
- 影響する機能: Assets ビュー表示、Assets ビュー更新監視、Assets 内の作成/削除メニュー

## 前提

- parent issue: issue-334
- 先行 child issue: issue-335
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-334`
- この child issue のブランチ: `issue-336`
- PR の向き: `issue-336` -> `issue-334`

## 実装方針

issue-335 で追加された `EditorProjectInfo::assetRootDirectory` を Assets パネルの表示ルートとして使う。

Assets パネル内の表示、選択、作成メニュー、ドラッグ、監視署名は既に `m_assetsRootDirectory` を基準にしているため、Refresh 時に設定する基準ディレクトリを `<ProjectRoot>` から `<ProjectRoot>/Assets` へ切り替える。

## 作業手順

1. `AssetsPanelController::Refresh` のルート解決を `projectInfo->assetRootDirectory` 基準に変更する
2. Assets ルートが未設定の場合はプロジェクトルート配下の `Assets` を fallback とする
3. 監視対象コメントを Assets 配下に合わせる
4. Editor テストまたはビルドを実行する

## 検証方法

- 実行するビルド: `MSBuild tests/Editor/Xelqoria.Tests.Editor.vcxproj /p:Configuration=Debug /p:Platform=x64 /m`
- 実行するテスト: `artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`
- 手動確認項目: Assets パネルが `<ProjectRoot>/Assets` 配下のみを列挙すること

## 完了条件

- Assets ビューの基準が `<ProjectRoot>/Assets` になる
- `ProjectSettings/`、`.xelqoria/`、プロジェクトファイルが Assets ビューに表示されない
- ファイル監視署名の対象が `<ProjectRoot>/Assets` になる
- 関連ビルドとテストが通る
- `issue-336` から `issue-334` への PR が作成されている
