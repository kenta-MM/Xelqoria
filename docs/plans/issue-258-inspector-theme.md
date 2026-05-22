# ExecPlan: issue-258 Inspector のセクション表示と入力欄の見た目をテーマ対応する

## 目的

Inspector のセクション表示と入力欄の見た目を `Xelqoria Dark` に合わせ、白背景の標準コントロール感を減らす。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/EditorShell.cpp`, `Editor/Source/EditorShell.h`
- 変更対象クラス: `Xelqoria::Editor::EditorShell`
- 影響する機能: Inspector の Transform / SpriteComponent セクション、Inspector 入力欄

## 前提

- parent issue: #253
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-253`
- この child issue のブランチ: `issue-258`
- PR の向き: `issue-258` -> `issue-253`

## 実装方針

入力処理や編集対象データ構造は変更しない。

Transform / SpriteComponent のセクション見出しは既存 Static を owner draw 化し、テーマのヘッダー色、境界線、アクセント色で描画する。

Inspector の Edit コントロールは親ウィンドウの `WM_CTLCOLOREDIT` で判定し、テーマの暗色入力背景とテキスト色を適用する。

## 作業手順

1. Inspector の生成・レイアウト・色適用経路を確認する
2. セクション見出しを owner draw 化する
3. Inspector 入力欄だけ暗色入力背景を適用する
4. ラベル幅、行高、余白を調整する
5. ビルドとレイヤー依存チェックを実行する
6. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `Xelqoria.Editor.vcxproj` Debug x64
- 実行するテスト: `LayerDependencyChecker`
- 手動確認項目: Inspector のセクション、ラベル、入力欄がテーマ色で表示され、既存編集操作が維持される

## 完了条件

- Inspector のセクション表示がテーマに沿って整理されている
- 入力欄の背景が白背景の標準 UI のように浮いていない
- ラベルと値の余白が揃っている
- 数値入力欄のサイズが揃っている
- 入力処理や編集対象データ構造を変更していない
- ビルドが通る
- レイヤー責務に違反していない
- `issue-258` から `issue-253` への PR が作成されている
