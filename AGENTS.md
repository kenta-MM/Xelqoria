# AGENTS.md

## ルール

- 依存方向・レイヤー責務は architecture.md に従う
- 詳細ルールは各ドキュメントに従う


## 禁止事項

- Graphics で Direct3D 型を使用しない
- RHI に描画概念を持ち込まない
- Game から Backends を参照しない
- 便宜的なレイヤー違反をしない

## 判断基準

- 配置に迷った場合 → project-map.md を参照する
- 実装フローに迷った場合 → workflows.md を参照する
- アーキテクチャに迷った場合 → architecture.md を参照する
- コーディングルールに迷った場合 → coding-rules.md を参照する
