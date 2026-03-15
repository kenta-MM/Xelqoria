---
name: xelqoria-issue-implementation
description: Use when implementing a GitHub issue in the Xelqoria repository. Follow the repo architecture rules, inspect the affected layer before editing, implement the issue with minimal changes, add Japanese XML doc comments where needed, and verify with appropriate build or tests before reporting.
---

# Xelqoria Issue Implementation

Xelqoria で GitHub Issue を実装する時に使う。

`XELQORIA_ROOT` には Xelqoria リポジトリのルートパスが入っている前提で扱う。

## 使いどころ

- ユーザーが「この issue を実装して」など Xelqoria の Issue 実装を依頼した時

## 最初にやること

1. まず [AGENTS.md]($XELQORIA_ROOT/AGENTS.md) を確認する
2. 必要に応じて次を読む
- アーキテクチャ: [docs/agents/architecture.md]($XELQORIA_ROOT/docs/agents/architecture.md)
- 実装フロー: [docs/agents/workflows.md]($XELQORIA_ROOT/docs/agents/workflows.md)
- コーディング規約: [docs/agents/coding-rules.md]($XELQORIA_ROOT/docs/agents/coding-rules.md)

## 標準ワークフロー

1. Issue の内容を取得する
- 現在の作業ブランチ名から Issue 番号を取得する
- ブランチ名は `issue-<番号>` を前提とし、`issue-26` なら Issue `26` を参照する
- ブランチ名から番号を取得できない場合のみ、ユーザーが貼った本文や URL を使う
- 番号が取得できたら `gh issue view` などで内容取得を試みる
- 取得できない場合は止まらず、ローカル文脈で補えるか確認し、それでも無理なら短く不足情報を聞く

2. 変更対象の責務を判定する
- 判断基準は [AGENTS.md]($XELQORIA_ROOT/AGENTS.md) と `docs/agents` に従う

3. 影響範囲を調べる
- まず関連ファイル、既存実装、テスト、ビルド対象を確認する
- 既存の設計パターンに寄せる
- 不要な横展開や大規模リファクタは避ける

4. 実装する
- 変更は最小限に留める
- 重要な宣言には日本語の XML ドキュメントコメントを付ける

5. 検証する
- 可能なら関連テスト、ビルド、静的チェックを実行する
- 失敗時は原因を切り分け、修正可能なら修正する
- 環境制約で実行できない場合は明示する

6. 報告する
- 何を変えたか
- どの Issue 要件に対応したか
- 実行した検証
- 残っているリスクや未確認点

## 出力スタイル

- まず実装と検証を進め、可能な限り最後までやる
- 回答は簡潔にする
- レビュー依頼なら、所見を重大度順に並べる
