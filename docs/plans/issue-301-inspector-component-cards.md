# ExecPlan: issue-301 Inspector の Component カードUI改善

## 目的

Inspector を Component カード単位に整理し、Hierarchy の Component 選択時に対応カードを強調表示する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/Application.cpp`, `Editor/Source/EditorShell.cpp`, `Editor/Source/InspectorPanelController.*`
- 変更対象クラス: `Application`, `EditorShell`, `InspectorPanelController`
- 影響する機能: Inspector 表示、Component カード折りたたみ、選択 Component 強調

## 前提

- parent issue: issue-299
- 先行 child issue: issue-300, issue-302
- parent issue ブランチ: `issue-299`
- この child issue のブランチ: `issue-301`
- PR の向き: `issue-301` -> `issue-299`

## 実装方針

既存 Inspector コントロールを維持しつつ、カード見出しのラベル、折りたたみ、表示制御、選択中 Component の強調表示を Controller へ追加する。

## 作業手順

1. Hierarchy の選択行種別を Inspector Refresh/ApplyEdits へ渡す
2. Inspector の各 Component カード見出しに折りたたみ状態と選択状態を反映する
3. 見出しクリックで折りたたみ状態を変更し、保存する
4. 選択中カードの見出しを紫系アクセントで強調する
5. Editor ビルドと Editor テストを実行する

## 検証方法

- `Editor/Xelqoria.Editor.vcxproj` Debug x64 ビルド
- `tests/Editor/Xelqoria.Tests.Editor.vcxproj` Debug x64 ビルド
- `artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`

## 完了条件

- Inspector 上部に選択中オブジェクト名が表示される
- Component カードを折りたためる
- Component 選択時に該当カードが強調表示される
- Component 選択時も他 Component のカードが非表示にならない
