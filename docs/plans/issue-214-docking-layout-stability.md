# ExecPlan: issue-214 各ビューのドッキング・独立化・レイアウト復元を安定化する

## 目的

各ビューについて、タブ移動、ドッキング、独立化、再ドッキング、リサイズ、再表示、レイアウト復元を安定して扱えるようにする。

## child issue 一覧

- issue-215: 現行のドッキング仕様を維持したままドッキング処理を安定化する
- issue-216: 独立ウィンドウ化と独立ウィンドウ内タブ化を整備する
- issue-217: 「表示」ボタンとビュー再表示機能を追加する
- issue-218: レイアウト保存・復元を追加する
- issue-219: ドラッグ中のビュー本体追従を安定化する
- issue-220: ビューサイズ変更時のちらつきを修正する
- issue-221: 全ビューを対象に操作確認と安定化を行う

## child issue 間の依存関係

- issue-215, issue-216, issue-217, issue-218, issue-219, issue-220 はいずれも EditorShell の Dock ツリー、TabControl、パネル表示状態、Win32 child window 管理に触れるため直列対応する。
- issue-219 は issue-215 のドッキング先判定とプレビュー安定化を前提にする。
- issue-216 は issue-215 と issue-219 のドラッグ状態管理を前提にする。
- issue-217 は issue-216 の非表示状態管理と既定位置復帰の扱いを前提にする。
- issue-218 は issue-215 から issue-217 で確定した Dock/Floating/表示状態を保存対象にする。
- issue-220 は最終的なレイアウト更新経路に対してちらつき対策を行う。
- issue-221 はすべての実装系 child issue 完了後に全ビューの操作確認と追加安定化を行う。

## ブランチ戦略

- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-214`
- child issue ブランチ: `issue-215`, `issue-216`, `issue-217`, `issue-218`, `issue-219`, `issue-220`, `issue-221`
- child issue の PR の向き: 各 child issue ブランチから `issue-214`
- parent issue ブランチの最終 PR の向き: `issue-214` から `main`

## parent issue での作業範囲

parent issue では原則として実装作業を行わない。

すべての child issue が parent issue ブランチにマージされた後、統合確認・動作確認・最終的な差分確認のみ行う。

## 処理順

1. issue-215
2. issue-219
3. issue-216
4. issue-217
5. issue-218
6. issue-220
7. issue-221

## 最終確認項目

- すべての child issue が parent issue ブランチにマージされている
- child issue 間の変更が競合していない
- parent issue の目的を満たしている
- レイヤー責務に違反していない
- ビルドが通る
- 必要なテストが通る
- 必要なドキュメントが更新されている
- 不要な一時ファイルや作業メモが含まれていない

## 完了条件

- すべての child issue が完了している
- 統合後の確認が完了している
- parent issue ブランチから `main` への PR が作成されている
