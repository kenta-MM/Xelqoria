---
name: xelqoria-architecture-review
description: Use when reviewing Xelqoria changes for architecture safety. Focus on layer responsibility, forbidden dependency direction, Graphics and RHI abstraction boundaries, and whether Runtime or Editor responsibilities leaked into the wrong project.
---

# Xelqoria Architecture Review

Xelqoria の変更をレビューする時に、設計崩れを先に見つけるための skill。

## 使いどころ

- ユーザーが「レビューして」と依頼した時
- PR や未コミット差分の設計妥当性を見たい時
- 依存方向や責務分離が崩れていないか確認したい時

## 最初にやること

1. [AGENTS.md]($XELQORIA_ROOT/AGENTS.md) を確認する
2. 次を必要に応じて読む
- [docs/agents/architecture.md]($XELQORIA_ROOT/docs/agents/architecture.md)
- [docs/agents/project-map.md]($XELQORIA_ROOT/docs/agents/project-map.md)
- [docs/agents/runtime-vs-editor-boundary.md]($XELQORIA_ROOT/docs/agents/runtime-vs-editor-boundary.md)

## レビュー観点

1. 依存方向
- `Game -> Graphics -> RHI -> Backends` を崩していないか
- `Game` から `Backends` を参照していないか

2. Graphics / RHI 境界
- `Graphics` に Direct3D 型が漏れていないか
- `RHI` に `Sprite`、`Camera`、`Scene` などが入っていないか

3. Runtime / Editor 境界
- Runtime へ Editor 専用概念が混入していないか
- Editor の都合で共通層が汚れていないか

4. 保存データと実行時オブジェクトの分離
- `AssetId` ベースの保存経路が保たれているか
- GPU リソースや API 実体が保存データへ混ざっていないか

## 出力スタイル

- 重大な設計問題を先に並べる
- どのファイルがどの原則に反しているかを短く示す
- 問題がなければ、その旨と残る軽微なリスクだけを述べる
