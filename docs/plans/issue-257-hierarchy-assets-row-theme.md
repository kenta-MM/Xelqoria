# ExecPlan: issue-257 Hierarchy / Assets の選択状態・hover 表示をテーマ対応する

## 目的

Hierarchy / Assets の行表示、選択状態、hover 表示を `Xelqoria Dark` の共通テーマに沿って統一する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/EditorShell.cpp`, `Editor/Source/EditorShell.h`
- 変更対象クラス: `Xelqoria::Editor::EditorShell`
- 影響する機能: Hierarchy ListBox、Assets ListView の行描画

## 前提

- parent issue: #253
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-253`
- この child issue のブランチ: `issue-257`
- PR の向き: `issue-257` -> `issue-253`

## 実装方針

Win32 標準コントロールの既存操作は維持し、描画色のみ `EditorTheme` に寄せる。

Hierarchy は ListBox を owner draw 化し、selected は `selection`、hover は `hover`、通常行は `panelBackground` で描画する。

Assets は ListView の `NM_CUSTOMDRAW` を親ウィンドウ側で処理し、selected / hover / 通常行の背景色とテキスト色をテーマに合わせる。

## 作業手順

1. Hierarchy ListBox と Assets ListView の生成・通知経路を確認する
2. Hierarchy ListBox を owner draw fixed に変更する
3. 行表示コントロールの hover 再描画用 subclass を追加する
4. Assets ListView の custom draw で行色を設定する
5. ビルドとレイヤー依存チェックを実行する
6. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `Xelqoria.Editor.vcxproj` Debug x64
- 実行するテスト: `LayerDependencyChecker`
- 手動確認項目: Hierarchy / Assets の selected と hover がテーマ色で区別できる

## 完了条件

- Hierarchy の Entity 行の hover / selected 表示がテーマに沿っている
- Assets のアセット行の hover / selected 表示がテーマに沿っている
- hover と selected が視覚的に区別できる
- アイコン描画や Entity / Asset 操作仕様を変更していない
- ビルドが通る
- レイヤー責務に違反していない
- `issue-257` から `issue-253` への PR が作成されている
