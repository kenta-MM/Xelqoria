# ブランチ運用ルール

このドキュメントは、Xelqoria で作業ブランチを作成する場合の共通ルールを定義する。

ExecPlan を作成しない小さな対応でも、ブランチを作成する場合はこのルールに従う。

## 基本方針

- ブランチ名は対応する GitHub Issue 番号を使い、`issue-xxx` に統一する
- `xxx` には対応する issue 番号を入れる
- parent issue / child issue のどちらも同じ命名形式を使う
- 親子関係はブランチ名では表現しない
- 親子関係は GitHub Issue の親子関係、PR の向き、必要に応じて ExecPlan の記載で管理する

## child issue が存在しない場合

child issue が存在しない issue では、作業ブランチを `main` から作成する。

```text
main
  └─ issue-xxx
```

PR は `main` に向ける。

```text
issue-xxx → main
```

## child issue が存在する場合

child issue が存在する parent issue では、parent issue ブランチを `main` から作成する。

```text
main
  └─ issue-123
```

child issue ブランチは parent issue ブランチから作成する。

```text
main
  └─ issue-123
       ├─ issue-124
       ├─ issue-125
       └─ issue-126
```

child issue の PR は parent issue ブランチに向ける。

```text
issue-124 → issue-123
issue-125 → issue-123
issue-126 → issue-123
```

すべての child issue が parent issue ブランチにマージされた後、parent issue ブランチから `main` に PR を作成する。

```text
issue-123 → main
```

## parent issue ブランチでの作業範囲

child issue が存在する parent issue ブランチでは、原則として実装作業を行わない。

parent issue ブランチでは、主に以下を行う。

- child issue の統合
- 統合後の動作確認
- 最終的な差分確認
- parent issue 全体の完了確認

実装作業は原則として child issue ブランチで行う。

## 並列作業時のルール

child issue を並列で対応する場合、各 child issue ブランチでは対応する child issue の作業だけを行う。

原則として、child issue ブランチでは以下を編集しない。

- parent issue 用 ExecPlan
- 他の child issue 用 ExecPlan
- 関係のない計画ファイル
- 他の child issue の実装範囲

parent issue 用 ExecPlan の更新が必要な場合は、統合作業または parent issue 側の専用 PR でまとめて行う。

## PR 作成時の確認

PR を作成する前に、以下を確認する。

- ブランチ名が `issue-xxx` 形式になっている
- child issue が存在しない issue の PR は `main` に向いている
- child issue の PR は parent issue ブランチに向いている
- parent issue の最終 PR は `main` に向いている
- 親子関係をブランチ名で表現していない
- 余計な変更が含まれていない
