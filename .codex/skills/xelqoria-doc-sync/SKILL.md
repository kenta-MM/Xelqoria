---
name: xelqoria-doc-sync
description: PR作成後、変更に基づき必要最小限のドキュメントのみ更新する
---

# ドキュメント同期（PRベース）

## 使用タイミング

- PR作成後

## 手順

1. PRを確認（タイトル・説明・変更ファイル）
2. 主対象ドキュメントを特定
3. そのドキュメントを更新
4. 明確に必要な場合のみ他ドキュメントも更新
5. 変更は最小限にする
6. 自明でない変更のみ、理由を1〜2行で説明（誤字修正等は不要）


## 対応表

主対象となるドキュメントを選択する。


- 依存関係 / アーキテクチャ → architecture.md（docs/agents 配下）
- レイヤー責務 / 配置 → project-map.md（docs/agents 配下）
- 実装フロー → workflows.md（docs/agents 配下）
- コーディングルール → coding-rules.md（docs/agents 配下）
- ランタイム / エディタ境界 → runtime-vs-editor-boundary.md（docs/agents 配下）
- アセットフロー → asset-flow.md（docs/agents 配下）
- CI / バリデーション → docs/quality 配下


## 更新対象外

- docs/class_diagram/**


## ルール

- PRの変更内容のみを根拠とする
- リポジトリ全体を探索しない
- 必要がない限り複数ドキュメントを更新しない
- 新規ドキュメントを作成しない
- 大規模な書き換えをしない
- 推測で書かない


## フェイルセーフ

- 対象が不明な場合は更新しない