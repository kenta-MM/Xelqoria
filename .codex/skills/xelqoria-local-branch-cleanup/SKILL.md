---
name: xelqoria-local-branch-cleanup
description: Use when cleaning up local branches in the Xelqoria repository, especially when deleting local branches that do not exist on the remote yet. Fetch remote refs first, identify local-only branches, avoid deleting the current branch, and prefer safe deletion before any force delete.
---

# Xelqoria Local Branch Cleanup

Xelqoria で、リモートブランチに存在しないローカルブランチを整理する時に使う。

`XELQORIA_ROOT` には Xelqoria リポジトリのルートパスが入っている前提で扱う。

## 使いどころ

- ユーザーが「リモートにないローカルブランチを削除したい」と依頼した時
- リモートブランチを先に作るため、不要なローカルブランチを整理したい時
- `git branch` の一覧を整理したい時

## 最初にやること

1. まず [AGENTS.md]($XELQORIA_ROOT/AGENTS.md) を確認する
2. Git の状態を確認する
- 現在ブランチ
- ワークツリーの変更有無
- 削除候補の一覧

## 標準ワークフロー

1. リモート参照を最新化する
- まず `git fetch --prune` を実行して、リモート追跡参照を最新化する

2. 削除候補を特定する
- ローカルブランチ一覧と `origin/*` を比較する
- 「対応する `origin/<branch>` が存在しないローカルブランチ」を候補とする
- 現在チェックアウト中のブランチは候補から除外する
- `main`、`master`、`develop` など明らかな保護対象があれば除外を検討する

3. 削除前に安全確認する
- 候補ブランチに未マージ変更がありそうなら注意する
- まずは通常削除の可否を優先する
- 強制削除が必要な候補は分けて扱う

4. 削除する
- まず `git branch -d <branch>` を使う
- 通常削除できない場合のみ、理由を確認してから `git branch -D <branch>` を検討する
- 一括削除でも、現在ブランチは絶対に削除しない

5. 結果を確認する
- 削除後に `git branch` などで一覧を再確認する
- 削除できなかったブランチがあれば理由を整理する

6. 報告する
- 削除したブランチ
- 削除対象から除外したブランチ
- 強制削除が必要だったかどうか
- 未確認点や、ユーザー判断が必要な点

## 実務上の判断基準

- 破壊的操作なので、いきなり `-D` を使わない
- 現在ブランチは削除しない
- 未マージの可能性がある時は、通常削除の失敗を根拠に扱いを分ける
- ユーザーが明示的に希望しない限り、リモートブランチの削除はしない

## コマンド例

```bash
git fetch --prune
git branch --format='%(refname:short)'
git branch -r --format='%(refname:short)'
git branch -d <branch>
git branch -D <branch>
```

ローカル専用ブランチの抽出は、環境に応じて `git for-each-ref` や `comm` を使ってもよい。  
ただし、コマンドを複雑にしすぎず、結果を人が確認しやすい形で進める。

## 出力スタイル

- まず候補を確認し、必要なら削除前に短く共有する
- 実行したコマンドと判断理由は簡潔に示す
- 強制削除が絡む場合は、その必要性を明確にする
