# ExecPlan: issue-221 全ビューを対象に操作確認と安定化を行う

## 目的

現時点で存在する全ビューについて、タブ移動、独立化、再ドッキング、ウィンドウリサイズ、ビューを閉じる、再表示、レイアウト復元が破綻しないことを確認する。

## 対象範囲

- 変更対象プロジェクト: Editor
- 変更対象ファイル: `Editor/Source/EditorShell.cpp`, `Editor/Source/Application.cpp`
- 変更対象クラス: `Xelqoria::Editor::EditorShell`, `Xelqoria::Editor::Application`
- 影響する機能: Dock / Floating / View menu / Layout persistence / resize redraw

## 前提

- parent issue: issue-214
- prerequisite branch: `issue-220` を作業ブランチへローカルマージ済み
- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-214`
- この child issue のブランチ: `issue-221`
- PR の向き: `issue-221` から `issue-214`

## 実装方針

この issue では先行 issue の統合状態を前提に確認を行う。コード確認と自動検証で破綻が見つかった場合のみ、要件に沿った最小限の安定化を行う。

## 作業手順

1. 先行 issue の変更を取り込んだ状態にする
2. Dock / Floating / View menu / Layout persistence / resize redraw の連携を確認する
3. 必要な安定化があれば最小限で実装する
4. ビルドとテストを実行する
5. `branch-rules.md` に従って PR を作成する

## 検証方法

- 実行するビルド: `tools/wsl/build.sh Editor/Xelqoria.Editor.vcxproj`
- 実行するテスト: `tools/wsl/test.sh --no-build`
- 手動確認項目: GUI 上で各ビューのタブ移動、独立化、再ドッキング、閉じる、再表示、レイアウト復元を確認する

## 確認結果

- `tools/wsl/build.sh Editor/Xelqoria.Editor.vcxproj` は成功した
- `tools/wsl/test.sh --no-build` は成功した
- WSL からの自動実行では Win32 GUI のドラッグ操作を目視確認できないため、手動確認項目は PR 上で残確認とする
- コード確認では追加の安定化が必要な破綻は見つからなかった

## 完了条件

- ビルドが通る
- 関連テストが通る
- レイヤー責務に違反していない
- 不要な変更が含まれていない
- `branch-rules.md` に従った PR が作成されている
