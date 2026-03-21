---
name: xelqoria-parent-issue-implementation
description: Use when implementing a parent GitHub issue in the Xelqoria repository by processing the listed child issues in numeric order. Switch to each child issue branch, implement the required changes with minimal edits, commit and push each branch, and create a pull request back to the parent issue branch.
allowed-tools: Bash(git fetch:*) Bash(git branch:*) Bash(git checkout:*) Bash(git status:*) Bash(git diff:*) Bash(git add:*) Bash(git commit:*) Bash(git push:*) Bash(rg:*) Bash(sed:*) Bash(find:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" issue view:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" repo view:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" pr create:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" auth status:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" auth setup-git:*) Read Edit
---

# Xelqoria Parent Issue Implementation

Xelqoria で、親 Issue に列挙された子 Issue を順番に実装する時に使う。

`XELQORIA_ROOT` には Xelqoria リポジトリのルートパスが入っている前提で扱う。

## 使いどころ

- ユーザーが「親 Issue の子 Issue を順に全部実装して」と依頼した時
- 親 Issue に子 Issue 一覧があり、各子 Issue を個別ブランチで実装して親 Issue に対する PR を積みたい時

## 最初にやること

1. まず [AGENTS.md]($XELQORIA_ROOT/AGENTS.md) を確認する
2. 必要に応じて次を読む
- アーキテクチャ: [docs/agents/architecture.md]($XELQORIA_ROOT/docs/agents/architecture.md)
- 実装フロー: [docs/agents/workflows.md]($XELQORIA_ROOT/docs/agents/workflows.md)
- コーディング規約: [docs/agents/coding-rules.md]($XELQORIA_ROOT/docs/agents/coding-rules.md)

## 親 Issue の特定方法

1. まずユーザーが渡した親 Issue 番号または URL を使う
2. 明示指定がない場合だけ現在ブランチ名から親 Issue 番号を推定する
- ブランチ名は `issue-<番号>` を前提とする
- `issue-60` にいるなら親 Issue は `60` とみなす
3. 番号が取れたら `gh issue view` で本文を取得する
4. 取得できない場合は止まらず、ローカル文脈で補えるか確認し、それでも無理なら短く不足情報を聞く
5. 親 Issue はこの 1 件だけを起点として扱う
- 親 Issue の本文にさらに上位 Issue への参照があっても、それは追跡しない
- 親の親、祖先 Issue、epic などへ自動で遡らない

## 子 Issue の抽出ルール

1. 親 Issue 本文の「子 Issue」節またはチェックリストから `#<番号>` を抽出する
2. 各子 Issue を `gh issue view` で開き、タイトルと本文を確認する
3. 子 Issue 名に含まれる数値を実装順として扱う
- 例: `S01`, `S02`, `F03`
4. 順序数が比較できる子 Issue は昇順で処理する
5. 順序数が読めないものは末尾に回し、必要ならユーザーに確認する

## 標準ワークフロー

1. 親ブランチを確認する
- 親 Issue の作業ブランチは `issue-<親番号>` を前提とする
- ローカルになければ `origin/issue-<親番号>` の存在を確認する
- 作業開始前に `git fetch` でリモート参照を更新する

2. 子 Issue を 1 件ずつ処理する
- 対象の子 Issue ブランチ `issue-<子番号>` へ切り替える
- ローカルにない場合は親ブランチから新規作成する
- 既にブランチがある場合は未コミット変更の有無を確認してから切り替える

3. 子 Issue の内容を実装する
- 変更対象の責務は [AGENTS.md]($XELQORIA_ROOT/AGENTS.md) と `docs/agents` に従って判定する
- 既存の設計パターンに寄せる
- 不要な横展開や大規模リファクタは避ける
- 重要な宣言には日本語の XML ドキュメントコメントを付ける

4. 子 Issue 単位で検証する
- 可能なら関連テスト、ビルド、静的チェックを実行する
- 失敗時は原因を切り分け、修正可能なら修正する
- 環境制約で実行できない場合は明示する

5. 子 Issue 単位で反映する
- 差分を確認して `git add`、`git commit`、`git push` を行う
- `gh auth status` で認証状態を確認し、必要なら `gh auth setup-git` を使う
- PR は `issue-<子番号>` から `issue-<親番号>` へ作成する
- PR 本文には親 Issue 番号と対応した子 Issue 番号を明記する

6. 次の子 Issue へ進む
- 親ブランチへ戻る
- リモートの親ブランチを基準に次の子 Issue ブランチへ切り替える
- 子 Issue が残っていれば同じ流れを繰り返す

## 重要な判断基準

- 各 PR は必ず親 Issue ブランチ向けに作る
- 次の子 Issue が前の子 Issue の未マージ差分に依存する場合は、そのまま続行しない
- その場合は「親ブランチへ先に取り込む必要がある」または「stacked PR に切り替える必要がある」と短く報告して止める
- 既存の未コミット変更があるブランチへは、そのまま切り替えない
- 子 Issue の順序は親 Issue の記述順ではなく、子 Issue 名に含まれる順序数を優先する

## 禁止事項

- 親 Issue を探す時に、親の親まで辿らない
- 親 Issue 本文中の上位 Issue、epic、meta issue を新しい親 Issue として扱わない
- 子 Issue の実装順を、親 Issue に記載された順番だけで決めない
- 後続の子 Issue のために、前の子 Issue の未反映差分を勝手に親ブランチへ取り込んだ前提で作業しない
- 既存の未コミット変更がある状態で別の子 Issue ブランチへ切り替えない

## 完了条件

- 親 Issue に列挙された子 Issue を順番に処理している
- 各子 Issue で実装、検証、コミット、push、PR 作成まで完了している
- 各 PR が親 Issue ブランチを base にしている
- 途中で止めた場合は、止めた子 Issue と理由が明確になっている

## 出力スタイル

- まず親 Issue と処理順を短く共有する
- 各子 Issue では、実装、検証、commit、push、PR 作成まで可能な限り最後までやる
- 回答は簡潔にする
- 最後に、処理した子 Issue、作成したコミット、作成した PR、止まった理由があればそれをまとめる
