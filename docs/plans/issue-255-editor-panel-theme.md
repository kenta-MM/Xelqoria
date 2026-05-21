# ExecPlan: issue-255 Editor View のパネル背景・ヘッダー・境界線を共通テーマに統一する

## 目的

Assets / Inspector / Hierarchy / Scene / Console などの Editor View で、パネル背景、ヘッダー、境界線、Editor Window 背景を Xelqoria Dark テーマへ寄せる。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/EditorShell.h`, `Editor/Source/EditorShell.cpp`
- 変更対象クラス: `EditorShell`
- 影響する機能: Editor Workspace のパネル外観、標準 child control の背景色

## 前提

- parent issue: #253
- 前提 child issue: #254
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-253`
- この child issue のブランチ: `issue-255`
- PR の向き: `issue-255` -> `issue-253`

## 実装方針

`Editor.UI` の `EditorThemes::XelqoriaDark` を Editor 側で Win32 の `COLORREF` / `HBRUSH` へ変換し、既存の `EditorShell` レイアウトと Docking 操作は維持する。

共通 PanelRenderer は新規作成せず、既存 GroupBox 相当の panel window を Editor 専用 custom child window に置き換える。タブの active / inactive / hover、行選択、Inspector 入力欄の詳細テーマ、LogOutput のログ種別色、Scene View 描画内の色は後続 child issue で扱う。

## 作業手順

1. `EditorShell` のパネル生成とレイアウトを確認する
2. Xelqoria Dark を Win32 色へ変換する薄いヘルパーを追加する
3. Workspace 背景と panel window の custom window class を追加する
4. 主要 panel の背景、ヘッダー、境界線をテーマ描画に置き換える
5. 標準 child control の背景と主要テキスト色を可能な範囲でテーマへ合わせる
6. 依存チェックと可能なビルドを実行する

## 検証方法

- 実行するビルド: `dotnet build tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug`
- 実行する依存チェック: `tools/LayerDependencyChecker/bin/Debug/net8.0/LayerDependencyChecker.exe D:\github\Xelqoria`
- 実行するビルド: `msbuild Xelqoria.slnx /t:Xelqoria.Editor /p:Configuration=Debug /p:Platform=x64`（環境に MSBuild がある場合）
- 実行するテスト: なし

## 完了条件

- 主要 Editor View の背景と境界線が共通テーマに沿っている
- パネルヘッダーの見た目が共通テーマに沿っている
- Editor Window 全体の背景色がテーマに沿っている
- 既存の編集操作、Scene 操作、Asset 操作、Docking 操作の挙動を変更していない
- Editor / Graphics / RHI / Backends の依存方向を崩していない
- PR が `issue-253` 向けに作成されている
