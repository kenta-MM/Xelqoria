# AGENTS.md

このドキュメントは **Xelqoria エンジン開発に参加する AI エージェント（Codex 等）向けの入口ガイド**です。  
詳細は `docs/agents/` 以下を参照してください。

## 最重要ルール

1. 依存方向を崩さない: `Game -> Graphics -> RHI -> Backends`
2. `Graphics` に Direct3D 型を入れない
3. `RHI` に `Sprite` や `Camera` などの描画概念を入れない
4. `Game` から `Backends` を参照しない
5. `Sprite` は描画しない。描画は `SpriteRenderer` など Renderer が担当する

不明な場合は **Graphics レイヤー優先で責務を再確認**すること。

## ドキュメントマップ

- アーキテクチャ原則: `docs/agents/architecture.md`
- 実装フローと追加手順: `docs/agents/workflows.md`
- コーディングルール: `docs/agents/coding-rules.md`
- Git 管理する Skill: `docs/skills/`

## 目的

- エンジン設計の崩壊を防ぐ
- レイヤー責務を維持する
- Graphics API 抽象を守る
- 将来の 2D / 3D 拡張を可能にする
- AI が誤ったレイヤーにコードを書かないようにする

## 推奨運用

- GitHub Issue の実装依頼では `xelqoria-issue-implementation` Skill を優先して使う
- Issue 番号または URL が渡されたら、内容確認、責務判定、最小変更での実装、検証、報告までを標準フローとして扱う
