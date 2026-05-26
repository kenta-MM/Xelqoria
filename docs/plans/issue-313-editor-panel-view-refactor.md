# ExecPlan: issue-313 Editor Panel View 分離およびフォルダ再編

## 目的

EditorShell が抱えている Panel 固有の HWND 生成・配置・表示制御を PanelView へ分離し、EditorShell の責務を Dock / Floating / Tab / 全体レイアウト管理へ寄せる。

## child issue 一覧

- issue-314: Editor Source フォルダ再編と Visual Studio project/filter 更新
- issue-315: EditorPanelId と IEditorPanelView の導入
- issue-316: PanelView 実装と EditorShell からの Panel 固有 View 処理分離
- issue-317: Controller Bind 変更と CollectControls vector 化

## child issue 間の依存関係

- issue-314 は以降の新規ファイル配置の前提になるため最初に対応する。
- issue-315 は issue-316 の PanelView 実装の前提になる。
- issue-316 は issue-317 の Controller Bind 変更の前提になる。
- すべて直列で対応する。

## ブランチ戦略

- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-313`
- child issue ブランチ: `issue-314`, `issue-315`, `issue-316`, `issue-317`
- child issue の PR の向き: `issue-313`
- parent issue ブランチの最終 PR の向き: `main`

## parent issue での作業範囲

parent issue では原則として実装作業を行わない。

すべての child issue が parent issue ブランチにマージされた後、統合確認・動作確認・最終的な差分確認のみ行う。

## 最終確認項目

- すべての child issue が parent issue ブランチにマージされている
- child issue 間の変更が競合していない
- EditorShell が Dock / Floating / Tab / 全体レイアウト管理中心の責務になっている
- 各 PanelView が既存 UI の見た目と操作感を維持している
- 各 Controller が EditorShell 全体ではなく必要な View に Bind している
- レイヤー責務に違反していない
- Editor project のビルドが通る
- LayerDependencyChecker に違反しない
- 不要な一時ファイルや作業メモが含まれていない

## 完了条件

- すべての child issue が完了している
- 統合後の確認が完了している
- parent issue ブランチから `main` への PR が作成されている
