# ExecPlan: issue-303 Log のフィルター、種別表示、Clear 表示制御改善

## 目的

Log パネルの種別フィルターと表示色を整理し、Clear 操作を内部ログ保持と画面表示クリアに分離する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/EditorShell.cpp`, `Editor/Source/LogOutputPanelController.*`
- 変更対象クラス: `EditorShell`, `LogOutputPanelController`
- 影響する機能: Log パネル表示、フィルター、Clear 操作

## 前提

- parent issue: issue-299
- parent issue ブランチ: `issue-299`
- この child issue のブランチ: `issue-303`
- PR の向き: `issue-303` -> `issue-299`

## 実装方針

LogEntry に category を保持し、TabControl のフィルターで `All / Info / Editor / Warn / Error` を切り替える。Clear はログ配列を消さず、表示開始位置だけを進める。

## 作業手順

1. Log タブ名を用途別に変更する
2. LogEntry に category を追加する
3. Log 表示行へ severity/category ラベルと色分けを追加する
4. Clear を表示クリア動作へ変更する
5. Editor ビルドと Editor テストを実行する

## 検証方法

- `Editor/Xelqoria.Editor.vcxproj` Debug x64 ビルド
- `tests/Editor/Xelqoria.Tests.Editor.vcxproj` Debug x64 ビルド
- `artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`

## 完了条件

- All / Info / Editor / Warn / Error のフィルターがある
- ログ行に種別が表示される
- severity ごとの色分けがある
- Clear 後も内部ログデータは保持される
