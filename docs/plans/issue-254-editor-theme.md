# ExecPlan: issue-254 EditorTheme と Xelqoria Dark テーマを追加する

## 目的

Editor UI の見た目を一元管理する `EditorTheme` を `Editor.UI` に追加し、デフォルトテーマとして Xelqoria Dark を提供する。

## 対象範囲

- 変更対象プロジェクト: `Editor.UI`
- 変更対象ファイル: `Editor.UI/Source/EditorTheme.h`, `Editor.UI/Xelqoria.Editor.UI.vcxproj`, `Editor.UI/Xelqoria.Editor.UI.vcxproj.filters`
- 変更対象クラス: `EditorColor`, `EditorTheme`
- 影響する機能: 後続 Editor View テーマ適用の共通土台

## 前提

- parent issue: #253
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-253`
- この child issue のブランチ: `issue-254`
- PR の向き: `issue-254` -> `issue-253`

## 実装方針

Editor.UI の責務に収まる OS 非依存のテーマ定義を追加する。Win32、Direct3D、Backends には依存せず、色はコード固定の RGBA 値として表す。

## 作業手順

1. 関連コードと Editor.UI プロジェクト構成を確認する
2. 既存の色表現型の有無を確認する
3. `EditorTheme` と Xelqoria Dark のデフォルト定義を追加する
4. Visual Studio プロジェクトにヘッダーを追加する
5. ビルドまたは関連検証を実行する
6. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `dotnet build tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug`
- 実行するビルド: `msbuild Xelqoria.slnx /t:Xelqoria.Editor.UI /p:Configuration=Debug /p:Platform=x64`（環境に MSBuild がある場合）
- 代替確認: `tools/LayerDependencyChecker/bin/Debug/net8.0/LayerDependencyChecker.exe D:\github\Xelqoria`
- 実行するテスト: なし
- 手動確認項目: `Editor.UI` が Backends / Direct3D / Platform.Win32 に依存していないこと

## 完了条件

- `Editor.UI` に `EditorTheme` 相当のテーマ定義が存在する
- デフォルトテーマとして Xelqoria Dark が存在する
- `EditorTheme` が対象色を一元管理できる
- `Editor.UI` が Backends / Direct3D / Platform.Win32 に依存していない
- Core に描画処理を追加していない
- PR が `issue-253` 向けに作成されている
