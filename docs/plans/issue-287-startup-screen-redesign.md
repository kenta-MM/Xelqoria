# ExecPlan: issue-287 Xelqoria Editor startup screen redesign

## 目的

Editor 起動直後の初期画面とプロジェクト作成ダイアログを、既存の Win32 UI 実装を保ったままダークテーマ、紫/青アクセント、幾何学風装飾の見た目へ刷新する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル:
  - `Editor/Source/StartupScreenController.h`
  - `Editor/Source/StartupScreenController.cpp`
  - `Editor/Source/Application.cpp`
- 変更対象クラス:
  - `StartupScreenController`
  - `Application`
- 影響する機能:
  - 起動画面
  - プロジェクト作成ダイアログ
  - 最近使ったプロジェクト一覧
  - 起動画面の表示タブ

## 前提

- parent issue: なし
- ブランチ運用ルール: `branch-rules.md`
- この issue のブランチ: `issue-287`
- PR の向き: `issue-287` -> `master`
- Issue に記載された `assets/...` パスは固定パスとして扱わず、添付された PNG を見た目の参照として扱う。

## 実装方針

Editor の OS ネイティブ UI シェル内に閉じて、Game / Graphics / RHI / Backends へ依存を追加しない。

StartupScreenController が起動画面専用のサブクラス化、描画、状態、入力処理を担当する。見た目は GDI ベースの背景塗り、枠線、テキスト、簡易アイコンで表現し、標準 Button / ListBox / Edit を owner draw / color hook / layout 調整で拡張する。表示タブは新規永続ストアを追加せず、`RecentProjectsStore` から読み込める既存 `.proj` を `EditorProject::Open` で検証し、ファイル時刻の降順で表示する。

## 作業手順

1. 関連コードとメッセージ経路を確認する。
2. StartupScreenController にタブ、タイトル領域、カスタム描画、表示タブ用一覧を追加する。
3. プロジェクト作成ダイアログを同じテーマに合わせる。
4. 起動画面中はネイティブメニューバーを非表示にし、Editor 遷移時に復元する。
5. ビルドまたは関連テストを実行する。
6. `branch-rules.md` に従って PR を作成する。

## 検証方法

- 実行するビルド: `msbuild Xelqoria.sln /t:Xelqoria.Editor /p:Configuration=Debug /p:Platform=x64`
- 実行するテスト: `msbuild tests/Editor/Xelqoria.Tests.Editor.vcxproj /p:Configuration=Debug /p:Platform=x64`
- 手動確認項目:
  - 起動時にプロジェクトタブが選択される。
  - プロジェクト作成、プロジェクトを開く、最近使った一覧、表示タブが操作できる。
  - ダイアログの作成、参照、キャンセルが既存どおり動く。
  - ウィンドウリサイズで主要要素が崩れない。

## 完了条件

- Issue の受け入れ条件に対応している。
- Editor 層内の変更に閉じている。
- ビルドまたは関連テストの結果を報告できる。
- 不要な変更が含まれていない。
- PR が作成されている。
