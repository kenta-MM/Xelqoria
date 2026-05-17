# ExecPlans

ExecPlan は、Codex が大きめの実装や複数 issue にまたがる作業を安全に進めるための実行計画である。

目的は、実装前に作業範囲・手順・検証方法・ブランチ戦略を明確にし、途中で作業が中断されても再開しやすくすることである。

## 基本方針

ExecPlan は、原則として「実際に作業する issue 単位」で作成する。

- child issue が存在しない issue は、その issue 用の ExecPlan を作成する
- child issue が存在する parent issue では、実装作業は原則として child issue で行う
- child issue が存在する場合、各 child issue ごとに作業用 ExecPlan を作成する
- parent issue 用 ExecPlan は、実装計画ではなく、child issue の依存関係・ブランチ戦略・統合後の最終確認項目を管理するために作成する
- 並列実行中の child issue ブランチでは、parent issue 用 ExecPlan を原則編集しない

## ExecPlan を作成するタイミング

次の場合は ExecPlan を作成する。

- 複数ファイル・複数クラスに影響する実装を行うとき
- 複数レイヤーに影響する変更を行うとき
- 設計判断が必要な変更を行うとき
- child issue を持つ parent issue を扱うとき
- child issue を実装するとき
- 作業の中断・再開が想定されるとき
- Codex に実装を依頼する前に、作業手順を明確にしたいとき

小さな typo 修正、コメント修正、単純なドキュメント修正など、計画を残す必要がない作業では ExecPlan を作成しなくてもよい。

## ファイル配置

ExecPlan は以下に配置する。

```text
docs/plans/
```

ファイル名は issue 番号と内容が分かる名前にする。

```text
docs/plans/issue-123-docking-ui.md
docs/plans/issue-124-tab-drag-preview.md
docs/plans/issue-125-docking-area.md
```

一時的な下書きや作業メモは Git 管理しない場所に置く。

```text
.agent/tmp-plans/
```

## parent issue 用 ExecPlan

child issue が存在する parent issue では、parent issue 用 ExecPlan を作成する。

ただし、parent issue では原則として実装作業を行わない。  
parent issue 用 ExecPlan は、child issue の統合と最終確認のために使用する。

parent issue 用 ExecPlan には、主に以下を記載する。

- parent issue の目的
- child issue 一覧
- child issue 間の依存関係
- 並列実行できる child issue
- 直列で対応すべき child issue
- ブランチ戦略
- PR の向き
- 統合後の最終確認項目
- parent issue の完了条件

### parent issue 用 ExecPlan テンプレート

```markdown
# ExecPlan: issue-xxx <parent issue title>

## 目的

この parent issue 全体で達成する目的を記載する。

## child issue 一覧

- issue-xxx: <child issue title>
- issue-xxx: <child issue title>
- issue-xxx: <child issue title>

## child issue 間の依存関係

- issue-xxx と issue-xxx は並列対応可能
- issue-xxx は issue-xxx の後に対応する

## ブランチ戦略

- parent issue ブランチ: `issue/xxx-<name>`
- child issue ブランチは parent issue ブランチから作成する
- child issue の PR は parent issue ブランチに向ける
- parent issue ブランチの最終 PR は `main` に向ける

## parent issue での作業範囲

parent issue では原則として実装作業を行わない。

すべての child issue が parent issue ブランチにマージされた後、統合確認・動作確認・最終的な差分確認のみ行う。

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
```

## child issue 用 ExecPlan

child issue は実際の作業単位である。  
child issue を実装する場合は、その child issue 用の ExecPlan を作成する。

child issue 用 ExecPlan には、主に以下を記載する。

- child issue の目的
- 作業対象
- 影響範囲
- 実装方針
- 作業手順
- 検証方法
- 完了条件
- PR の向き

### child issue 用 ExecPlan テンプレート

```markdown
# ExecPlan: issue-xxx <child issue title>

## 目的

この child issue で達成する目的を記載する。

## 対象範囲

- 変更対象プロジェクト:
- 変更対象ファイル:
- 変更対象クラス:
- 影響する機能:

## 前提

- parent issue:
- parent issue ブランチ:
- この child issue のブランチ:
- PR の向き:

## 実装方針

この child issue 内で行う実装方針を記載する。

Xelqoria のレイヤー責務に関わる場合は、どのレイヤーに責務を置くかを明確にする。

## 作業手順

1. 関連コードを確認する
2. 変更対象を特定する
3. 実装する
4. 必要なテストを追加・更新する
5. ビルドまたはテストを実行する
6. 必要に応じてドキュメントを更新する
7. parent issue ブランチに向けた PR を作成する

## 検証方法

- 実行するビルド:
- 実行するテスト:
- 手動確認項目:

## 完了条件

- この child issue の目的を満たしている
- ビルドが通る
- 関連テストが通る
- レイヤー責務に違反していない
- 不要な変更が含まれていない
- parent issue ブランチに向けた PR が作成されている
```

## ブランチ戦略

child issue が存在しない場合は、作業ブランチを `main` から作成し、PR は `main` に向ける。

```text
main
  └─ issue/xxx-feature
```

child issue が存在する場合は、parent issue ブランチを作成し、child issue ブランチは parent issue ブランチから作成する。

```text
main
  └─ issue/123-parent-feature
       ├─ issue/124-child-a
       ├─ issue/125-child-b
       └─ issue/126-child-c
```

child issue の PR は parent issue ブランチに向ける。

```text
issue/124-child-a → issue/123-parent-feature
issue/125-child-b → issue/123-parent-feature
issue/126-child-c → issue/123-parent-feature
```

すべての child issue が parent issue ブランチにマージされた後、parent issue ブランチから `main` に PR を作成する。

```text
issue/123-parent-feature → main
```

## 並列作業時のルール

child issue を並列で対応する場合、各 child issue ブランチでは対応する child issue 用 ExecPlan のみ編集する。

原則として、child issue ブランチでは以下を編集しない。

- parent issue 用 ExecPlan
- 他の child issue 用 ExecPlan
- 関係のない計画ファイル

parent issue 用 ExecPlan の更新が必要な場合は、統合作業または parent issue 側の専用 PR でまとめて行う。

## 更新ルール

ExecPlan と実装内容がずれた場合は、実装内容に合わせて ExecPlan も更新する。

ただし、並列作業中の child issue ブランチでは、parent issue 用 ExecPlan を更新しない。

作業中に新しい課題が見つかった場合は、ExecPlan に勝手に大きなスコープ追加をしない。  
必要に応じて、新しい issue として分離する。

## Xelqoria のレイヤー確認

実装前後に、以下のレイヤー責務を確認する。

```text
Game → Graphics → RHI → Backends
Core は共通基盤として利用する
```

- Game は Graphics を利用できる
- Graphics は RHI を利用できる
- RHI は Backends に依存しない
- Backends は RHI の実装を提供する
- Core は共通基盤として利用するが、描画ロジックを持たない
- Graphics は Direct3D 型を直接扱わない
- RHI は Sprite、Camera、Material、Renderer などの描画概念を持たない
- Direct3D などのプラットフォーム固有 API は Backends に閉じ込める
- データオブジェクト自身に描画処理を持たせない

判断に迷う場合は、architecture.md、coding-rules.md、workflows.md、project-map.md を確認する。

## PR 作成時の確認

PR を作成する前に、以下を確認する。

- 対応 issue の目的を満たしている
- ExecPlan の完了条件を満たしている
- 余計な変更が含まれていない
- ビルドが通る
- 必要なテストが通る
- 必要なドキュメントが更新されている
- child issue の PR は parent issue ブランチに向いている
- parent issue の最終 PR は `main` に向いている

## Git 管理するもの

以下は Git 管理する。

- `.agent/PLANS.md`
- `docs/plans/` 配下の正式な ExecPlan

以下は Git 管理しない。

- `.agent/tmp-plans/`
- 一時的な調査メモ
- Codex の下書き
- 途中で破棄する作業メモ
