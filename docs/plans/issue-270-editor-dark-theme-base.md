# ExecPlan: issue-270 Editor 全体のダークテーマ基盤と共通 UI 描画を実装する

## 目的

Xelqoria Editor の標準 Win32 コントロールを独自ダークテーマで描画するための共通テーマ定義と共通描画基盤を整える。

## 対象範囲

- 変更対象プロジェクト: `Editor.UI`, `Editor`
- 変更対象ファイル: `Editor.UI/Source/EditorTheme.h`, `Editor/Source/EditorShell.cpp`
- 変更対象クラス: `EditorTheme`, `EditorShell`
- 影響する機能: Editor パネル、タブ、リスト、入力欄、テーマ色

## 前提

- parent issue: #269
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-269`
- この child issue のブランチ: `issue-270`
- PR の向き: `issue-270` -> `issue-269`

## 実装方針

OS 非依存のテーマ値は `Editor.UI` に置き、Win32 の描画処理は `Editor` の `EditorShell` に閉じる。既存の Owner Draw / Custom Draw / Subclass 化を活かし、テーマ値に角丸と境界線の共通寸法を追加する。

## 作業手順

1. 既存テーマ定義と EditorShell の描画経路を確認する
2. `EditorTheme` に角丸と境界線の共通寸法を追加する
3. パネル描画をテーマ定義に基づく角丸描画へ寄せる
4. ビルドまたはレイヤー依存チェックを実行する
5. PR を作成する

## 検証方法

- 実行するビルド: `dotnet build tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug`
- 実行するテスト: LayerDependencyChecker
- 手動確認項目: パネル背景、ヘッダー、境界線、選択、hover、入力欄が暗色テーマとして表示される

## 完了条件

- 共通 UI テーマの色、境界線、角丸、アクセントが定義されている
- 対象標準コントロールが独自ダークテーマで描画される
- hover / 押下 / 無効 / 選択状態が視覚的に区別できる
- Graphics に Direct3D 型を漏らしていない
- RHI に描画概念を持ち込んでいない
- `issue-270` から `issue-269` への PR が作成されている
