---
name: xelqoria-issue-implementation
description: Use when implementing a GitHub issue in the Xelqoria repository. Supports both a single issue and a parent issue with child issues. Follow the repo architecture rules, inspect the affected layer before editing, implement with minimal changes, create PRs according to the branch rules, and verify with appropriate build or tests before reporting.
allowed-tools: Bash(git fetch:*) Bash(git checkout:*) Bash(git branch:*) Bash(git merge:*) Bash(git status:*) Bash(git diff:*) Bash(git add:*) Bash(git commit:*) Bash(git push:*) Bash(rg:*) Bash(sed:*) Bash(find:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" issue view:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" issue comment:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" issue edit:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" repo view:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" pr create:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" auth status:*) Bash("/mnt/c/Program Files/GitHub CLI/gh.exe" auth setup-git:*) Read Edit
---

# Xelqoria Issue Implementation

Xelqoria で GitHub Issue を実装する時に使う。  
単体 Issue と、親 Issue に子 Issue が列挙されているケースの両方をこの skill で扱う。

`XELQORIA_ROOT` には Xelqoria リポジトリのルートパスが入っている前提で扱う。

## 使いどころ

- ユーザーが「この issue を実装して」など Xelqoria の Issue 実装を依頼した時
- ユーザーが親 Issue を指定し、配下の子 Issue を実装するよう依頼した時

## 最初にやること

1. まず [AGENTS.md]($XELQORIA_ROOT/AGENTS.md) を確認する
2. 必要に応じて次を読む
   - アーキテクチャ: [docs/agents/architecture.md]($XELQORIA_ROOT/docs/agents/architecture.md)
   - 実装フロー: [docs/agents/workflows.md]($XELQORIA_ROOT/docs/agents/workflows.md)
   - コーディング規約: [docs/agents/coding-rules.md]($XELQORIA_ROOT/docs/agents/coding-rules.md)
   - ブランチ運用: [docs/agents/branch-rules.md]($XELQORIA_ROOT/docs/agents/branch-rules.md)
   - ExecPlan: [docs/agents/plans.md]($XELQORIA_ROOT/docs/agents/plans.md)

## Issue の特定

1. 現在の作業ブランチ名から Issue 番号を取得する
   - ブランチ名は `docs/agents/branch-rules.md` に従い、`issue-<番号>` を前提とする
   - `issue-26` なら Issue `26` を参照する
2. ブランチ名から番号を取得できない場合のみ、ユーザーが貼った本文や URL を使う
3. 番号が取得できたら `gh issue view` などで内容取得を試みる
4. 取得できない場合は止まらず、ローカル文脈で補えるか確認し、それでも無理なら短く不足情報を聞く

## 実行モードの判定

取得した Issue を確認し、次のどちらで処理するか決める。

### 単体 Issue モード

次の場合は単体 Issue として扱う。

- Issue 本文に実装対象の子 Issue が列挙されていない
- ユーザーが特定の Issue 1 件の実装を依頼している
- 親 Issue か判断できないが、本文だけで実装作業を完結できる

### 親 Issue モード

次の場合は親 Issue として扱う。

- Issue 本文に `#番号` 形式で子 Issue が列挙されている
- ユーザーが「親 Issue」「子 Issue」「配下の Issue」などの実装を依頼している
- Issue 本文が子 Issue の実行管理を目的としている

親 Issue モードでは、親 Issue 自体で実装作業を行わず、子 Issue 単位で実装する。  
親 Issue レベルの作業は、子 Issue の分類、PR 作成状況の確認、結果報告に限定する。

## 単体 Issue モードの標準ワークフロー

1. 変更対象の責務を判定する
   - 判断基準は [AGENTS.md]($XELQORIA_ROOT/AGENTS.md) と `docs/agents` に従う
2. 影響範囲を調べる
   - 関連ファイル、既存実装、テスト、ビルド対象を確認する
   - 既存の設計パターンに寄せる
   - 不要な横展開や大規模リファクタは避ける
3. 必要に応じて作業 Issue 単位の ExecPlan を作成または更新する
4. 実装する
   - 変更は最小限に留める
   - 重要な宣言には日本語の XML ドキュメントコメントを付ける
5. 検証する
   - 可能なら関連テスト、ビルド、静的チェックを実行する
   - 失敗時は原因を切り分け、修正可能なら修正する
   - 環境制約で実行できない場合は明示する
6. `git add` / `git commit` / `git push` を実行する
7. PR を作成する
   - base / head は `docs/agents/branch-rules.md` に従う
   - Issue 番号を PR 本文に含める

## 親 Issue モードの標準ワークフロー

### 1. 全体最新化

1. `git fetch` を実行する
2. 未コミット変更がある場合は中断する
3. 親ブランチは `docs/agents/branch-rules.md` に従い、`issue-<親番号>` とする
4. 親ブランチが存在しない場合は中断する

### 2. 子 Issue 取得

1. 親 Issue 本文から `#番号` を抽出する
2. 各子 Issue を取得する
3. 子 Issue 番号の昇順で整列する
4. 順序を確定できないものは最後に回す

### 3. 依存関係の整理

各子 Issue の内容を確認し、次の条件をすべて満たす場合は独立しているとみなす。

- 他の子 Issue の完了を前提にしていない
- 同じファイルや同じ公開インターフェースを競合する形で変更しない
- 単独で実装と検証が可能

独立した子 Issue は並列実行対象、依存がある子 Issue は直列実行対象に分類する。  
競合の可能性がある場合は並列実行しない。

### 4. 子 Issue 単位で実装する

各子 Issue は、単体 Issue モードと同じ流れで処理する。  
ただし、次を追加で守る。

- 子 Issue ごとに個別のブランチで完結させる
- 子ブランチは `issue-<子番号>` とする
- 必要に応じて親ブランチ `issue-<親番号>` から子ブランチを作成する
- 子 Issue は必ず 1 PR で完結させる
- PR の base は `docs/agents/branch-rules.md` に従う
- 親 Issue 番号と子 Issue 番号を PR 本文に含める
- コンフリクトが発生する可能性がある場合でも PR は作成し、解消は本フローでは行わない

### 5. 先行 Issue ブランチの取り込み

後続 Issue が先行 Issue の変更を前提としており、先行 Issue の PR がまだ親ブランチへマージされていないため後続 Issue の実装を進められない場合は、中断しない。  
ローカル上で後続 Issue の作業ブランチへ先行 Issue の作業ブランチをマージし、実装を継続する。

手順は次の通り。

1. `git fetch` で最新状態を取得する
2. `git status` で未コミット変更がないことを確認する
3. 後続 Issue の作業ブランチ `issue-<後続番号>` にチェックアウトする
4. 先行 Issue の作業ブランチ `issue-<先行番号>` を `git merge issue-<先行番号>` で取り込む
5. コンフリクトが発生した場合は、解消可能な範囲で解消して続行する
6. コンフリクト解消が困難、または要件判断が必要な場合のみ中断し、状況を報告する
7. マージ後、後続 Issue の実装・検証・commit・push・PR 作成を続行する

この取り込みは、後続 Issue の実装を進めるためのローカル作業ブランチ上の統合であり、先行 Issue の PR が正式に親ブランチへマージされたことを意味しない。  
報告時には、どの後続 Issue ブランチにどの先行 Issue ブランチを取り込んだかを明記する。

### 6. 結果統合

1. 全 PR を確認する
2. base が正しいことを確認する
3. 未処理 Issue がないことを確認する
4. コンフリクトがある場合は記録のみ行う

## 判断ルール

- 単体 Issue と親 Issue の使い分けは、Issue 本文とユーザー依頼から判断する
- 親 Issue に子 Issue がある場合、実装は子 Issue 単位で行う
- 子 Issue がある親 Issue では、親 Issue ブランチ上で直接実装しない
- 先行 Issue の変更がないと後続 Issue を実装できない場合は、後続 Issue ブランチへ先行 Issue ブランチをマージして継続する
- 要件にない変更は禁止
- 不要なリファクタは禁止
- 変更対象の責務は `AGENTS.md` と `docs/agents` を優先する
- データオブジェクト自身に描画処理を持たせない
- Graphics 層に Direct3D 型を漏らさない
- RHI 層に Sprite、Camera、Material、Renderer などの高レベル概念を入れない
- Game 層から Backends 層へ直接依存させない

## 禁止事項

- 未コミット状態でブランチを切り替えない
- PR base を子ブランチにしない
- 担当外の Issue まで変更しない
- 親 Issue モードで親 Issue 自体に実装変更を入れない
- 先行 Issue の取り込みが必要なだけで後続 Issue の実装を中断しない
- 不明点を推測で埋めて大きな設計変更をしない

## 完了条件

### 単体 Issue モード

- Issue 要件に対応した実装が完了している
- 必要な検証が実行済み、または実行不可理由が明示されている
- commit / push / PR 作成が完了している

### 親 Issue モード

- 全子 Issue で PR 作成が完了している
- 各 PR の base が正しい
- 未処理 Issue が存在しない
- 並列 / 直列の分類と処理結果が説明できる
- 先行 Issue ブランチを取り込んだ後続 Issue がある場合、その組み合わせを説明できる

## 出力

単体 Issue モードでは次を報告する。

- Issue 番号
- 変更内容
- commit
- PR URL
- 実行した検証
- 残リスクや未確認点

親 Issue モードでは次を報告する。

- 親 Issue 番号
- 子 Issue 分類（並列 / 直列）
- 処理順
- 先行 Issue ブランチを取り込んだ後続 Issue ブランチ（ある場合）
- 各子 Issue の実装結果、commit、PR URL
- 中断理由（ある場合）

## 出力スタイル

- まず実装と検証を進め、可能な限り最後までやる
- 回答は簡潔にする
- レビュー依頼なら、所見を重大度順に並べる
