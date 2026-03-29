---
name: xelqoria-doc-sync
description: Use when implementation changes in the Xelqoria repository may require documentation updates. Check AGENTS guidance, inspect changed files, update the smallest relevant docs under docs/agents, docs/quality, or docs/class_diagram, and avoid speculative rewrites.
---

# Xelqoria Doc Sync

Xelqoria で、実装変更に合わせてドキュメント更新が必要かを判断し、必要最小限の文書だけを更新する時に使う。

## 使いどころ

- ユーザーが「関連ドキュメントも更新して」と依頼した時
- `.h` や workflow、成果物境界、アセット解決経路を変えた時
- 変更に対して `docs/agents`、`docs/quality`、`docs/class_diagram` が古くなりそうな時

## 最初にやること

1. [AGENTS.md]($XELQORIA_ROOT/AGENTS.md) を確認する
2. 変更ファイルを確認する
3. どの文書が責務上もっとも近いかを 1 つずつ判定する

## 標準ワークフロー

1. `git diff --name-only` などで変更範囲を確認する
2. 変更を次のどれに当てはめるか判断する
- レイヤー責務: `docs/agents/architecture.md`
- 実装の流れ: `docs/agents/workflows.md`
- 判断補助: `docs/agents/project-map.md`
- Runtime / Editor 境界: `docs/agents/runtime-vs-editor-boundary.md`
- アセット解決: `docs/agents/asset-flow.md`
- クラス関係: `docs/class_diagram/*.md`
- CI / 品質: `docs/quality/*.md`
3. 関係する文書だけを更新する
4. 実装内容から説明できない推測は書かない
5. 最後に「どの変更に合わせてどの文書を直したか」を整理する

## 判断基準

- まず既存文書を直し、同じ内容の新規文書を増やしすぎない
- 仕様未確定の内容は断定しない
- 実装が 1 つしかない時は、その実装ベースで書く
- 「将来こうしたい」は、現状説明と混ぜない
