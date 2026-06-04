# ExecPlan: issue-334 プロジェクト階層を整理し Assets ビューを Assets 配下に限定する

## 目的

新規プロジェクトと既存プロジェクトの構成を整理し、Assets ビューにはプロジェクトルート全体ではなく `<ProjectRoot>/Assets` 配下のみを表示する。

## child issue 一覧

- issue-335: 新規プロジェクト標準構成とプロジェクトファイル保存内容を更新する
- issue-336: Assets ビューとファイル監視対象を Assets 配下に限定する
- issue-337: 既存プロジェクトの Assets 配下移行処理を追加する
- issue-338: Project Settings をメニューから開く導線を追加する

## child issue 間の依存関係

- issue-335 は `Assets/`、`ProjectSettings/`、`.xelqoria/` とプロジェクトファイル内の相対パス定義を作るため、他 child issue の前提として先に対応する。
- issue-336 は issue-335 の `assetRoot` を前提に Assets ビューと監視対象を限定する。
- issue-337 は issue-335 の `projectStructureVersion` と標準ディレクトリ構成を前提に既存プロジェクトを移行する。
- issue-338 は issue-335 の `projectSettings` パスを前提に Project Settings を開く導線を追加する。
- issue-336、issue-337、issue-338 は issue-335 完了後に並列対応可能。ただし同じ Editor 周辺のファイルを触る可能性があるため、競合が見えた場合は直列で扱う。

## ブランチ戦略

- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-334`
- child issue ブランチ: `issue-335`、`issue-336`、`issue-337`、`issue-338`
- child issue の PR の向き: `issue-334`
- parent issue ブランチの最終 PR の向き: `master`

## parent issue での作業範囲

parent issue では原則として実装作業を行わない。

すべての child issue が parent issue ブランチにマージされた後、統合確認、動作確認、最終的な差分確認のみ行う。

## 最終確認項目

- すべての child issue が parent issue ブランチにマージされている
- Assets ビューが `<ProjectRoot>/Assets` 配下のみを表示する
- ファイル監視対象が `<ProjectRoot>/Assets` 配下のみになっている
- 新規プロジェクト作成時に標準構成が作成される
- 既存プロジェクトを開いたときに必要な移行が行われる
- `Project > Settings...` から `ProjectSettings/project_settings.json` を編集できる
- レイヤー責務に違反していない
- ビルドが通る
- 必要なテストが通る
- 不要な一時ファイルや作業メモが含まれていない

## 完了条件

- issue-335、issue-336、issue-337、issue-338 が完了している
- 統合後の確認が完了している
- parent issue ブランチから `master` への PR が作成されている
