# ExecPlan: issue-220 ビューサイズ変更時のちらつきを修正する

## 目的

Dock splitter や window resize によるビューサイズ変更中、Editor の各ビューが不自然に消えたり大きくちらついたりしないようにする。

## 対象範囲

- 変更対象プロジェクト: Editor
- 変更対象ファイル: `Editor/Source/EditorShell.cpp`
- 変更対象クラス: `Xelqoria::Editor::EditorShell`
- 影響する機能: Dock レイアウト更新後の Win32 child window 再描画

## 前提

- parent issue: issue-214
- prerequisite branch: `issue-218` を作業ブランチへローカルマージ済み
- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-214`
- この child issue のブランチ: `issue-220`
- PR の向き: `issue-220` から `issue-214`

## 実装方針

リサイズ中の child window 移動は既存の `SWP_NOREDRAW` 経路を維持し、最後の `RedrawLayout` で強制 erase / synchronous repaint を繰り返さないようにする。親 window に対して child を含む invalidation を一度だけ行い、背景消去と即時再描画によるちらつきを抑える。

## 作業手順

1. レイアウト更新と再描画経路を確認する
2. `RedrawLayout` の再描画フラグを調整する
3. ビルドを実行する
4. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `tools/wsl/build.sh Editor/Xelqoria.Editor.vcxproj`
- 実行するテスト: 追加なし
- 手動確認項目: Dock splitter / window resize 中のちらつきが抑制されること

## 完了条件

- リサイズ中の強制 erase / 即時再描画が抑制されている
- ビルドが通る
- レイヤー責務に違反していない
- 不要な変更が含まれていない
- `branch-rules.md` に従った PR が作成されている
