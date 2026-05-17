---
name: xelqoria-parent-issue-implementation
description: 親Issue配下の子Issueを整理し、独立したものはサブエージェントで並列実装して各PRを作成する。
allowed-tools: Bash(git fetch:*) Bash(git checkout:*) Bash(git branch:*) Bash(git status:*) Bash(git diff:*) Bash(git add:*) Bash(git commit:*) Bash(git push:*) Bash(rg:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" issue view:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" pr create:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" auth status:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" auth setup-git:*) Read Edit
---

# 目的
親Issueに列挙された子Issueを整理し、独立した子Issueはサブエージェントで並列実装する。  
各子Issueは個別のブランチで完結させ、子IssueごとにPRを作成する。

# 前提
- `XELQORIA_ROOT` はリポジトリルート
- GitHub CLI が使用可能
- 親Issueは1件のみ扱う
- 子Issueごとに作業ブランチが存在する、または必要に応じて作成できる
- サブエージェントは担当する子Issue以外を変更しない

# 初期確認
1. 必要なら `docs/agents` 配下を参照
2. ブランチの作成元、命名、PR の向きは `docs/agents/branch-rules.md` に従う

# 親Issue取得
1. ユーザー指定のIssue番号を使用
   1. 未指定の場合のみブランチ名 `issue-<番号>` から取得
2. `gh issue view` で取得
3. 取得できなければ不足情報を1回だけ聞く

# 子Issue取得
1. 親Issue本文から `#番号` を抽出
2. 各子Issueを取得
3. 子Issue番号の昇順で整列
4. 順序を確定できないものは最後に回す

# 依存関係の整理
1. 各子Issueの内容を確認し、依存関係の有無を判定する
2. 次の条件をすべて満たす場合は独立しているとみなす
   - 他の子Issueの完了を前提にしていない
   - 同じファイルや同じ公開インターフェースを競合する形で変更しない
   - 単独で実装と検証が可能
3. 上記を満たさない場合は依存ありとして扱う
4. 独立した子Issueは並列実行対象、依存がある子Issueは直列実行対象に分類する

# 実行フロー
## 1. 全体最新化
1. `git fetch` を実行する
2. 未コミット変更がある場合は中断する

## 2. 親ブランチ確認
1. 親ブランチは `docs/agents/branch-rules.md` に従い、`issue-<親番号>` とする
2. 親ブランチが存在しない場合は中断する

## 3. 並列実行対象の処理
1. 独立した子Issueごとにサブエージェントを1つ割り当てる
2. 各サブエージェントは担当する子Issueのみを処理する
3. 各サブエージェントは次を実施する
   - 対象子Issueを確認する
   - `git status` で未コミット変更がないことを確認する
   - 子ブランチ `issue-<子番号>` に checkout する
   - 必要に応じて `docs/agents/branch-rules.md` に従い、親ブランチ `issue-<親番号>` から作成する
   - `AGENTS.md` に従い最小限の変更を実装する
   - ビルドまたはテストを実行する
   - 実行不可の場合は理由を記録する
   - `git add` / `git commit` / `git push` を実行する
   - PRを作成する（下記PR作成ルールに従う）
4. 各サブエージェントは結果のみを返す
5. 親エージェントは結果を収集し整理する

## 4. 直列実行対象の処理
1. 依存関係がある子Issueは順に処理する
2. 各Issueごとに次を実施する
   - `git status` を確認する
   - 子ブランチ `issue-<子番号>` に checkout する
   - 必要な依存関係を確認する
   - `AGENTS.md` に従い実装する
   - ビルドまたはテストを実行する
   - 実行不可の場合は理由を記録する
   - `git add` / `git commit` / `git push`
   - PRを作成する

## 5. PR作成
- base / head は `docs/agents/branch-rules.md` に従う
- 親Issue番号と子Issue番号を本文に含める
- コンフリクトの有無に関係なく必ず作成する

## 6. 結果統合
1. 全PRを確認する
2. baseが正しいことを確認する
3. 未処理Issueがないことを確認する
4. コンフリクトがある場合は記録のみ行う

# 判断ルール
- 独立した子Issueはサブエージェントで並列実行する
- 依存関係がある子Issueのみ直列実行する
- 子Issueは必ず1PRで完結させる
- 子Issueは単独で検証可能にする
- PRのbaseは `docs/agents/branch-rules.md` に従う
- コンフリクトが発生する可能性がある場合でもPRは必ず作成する
- コンフリクトの解消は本フローでは行わない
- 要件にない変更は禁止
- 競合の可能性がある場合は並列実行しない

# 禁止事項
- 親Issue以外を起点にしない
- PR base を子ブランチにしない
- 未コミット状態でブランチを切り替えない
- サブエージェントが複数Issueを処理しない
- 担当外のブランチを変更しない
- 不要なリファクタを行わない

# 完了条件
- 全子IssueでPR作成完了
- 各PRのbaseが正しい
- 未処理Issueが存在しない

# 出力
- 親Issue番号
- 子Issue分類（並列 / 直列）
- 処理順
- 各子Issue:
  - 実装結果
  - commit
  - PR URL
- 中断理由（ある場合）
