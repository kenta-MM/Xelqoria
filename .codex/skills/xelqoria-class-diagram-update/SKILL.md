---
name: xelqoria-class-diagram-update
description: Use when public headers or project dependencies change in the Xelqoria repository and the class diagrams under docs/class_diagram must be updated. Prefer project-scoped diagrams, include directly referenced lower layers, and keep Mermaid syntax simple and preview-friendly.
---

# Xelqoria Class Diagram Update

Xelqoria の公開ヘッダやプロジェクト参照が変わった時に、`docs/class_diagram` の mermaid 図を更新するための skill。

## 使いどころ

- `.h` の型定義や継承、保持関係を変えた時
- `.vcxproj` の `ProjectReference` を変えた時
- クラス図を新規作成または更新したい時

## 最初にやること

1. [AGENTS.md]($XELQORIA_ROOT/AGENTS.md) を確認する
2. 対象プロジェクトの `.vcxproj` と `.h` を読む
3. 既存の `docs/class_diagram/*.md` を確認する

## 標準ワークフロー

1. どのプロジェクト図を更新するか決める
2. 対象プロジェクト自身の公開型を拾う
3. そのプロジェクトが直接参照している下位レイヤーだけを図へ含める
4. mermaid は小さく保つ
- 全体図を 1 枚へ戻さない
- ネストしすぎたジェネリクス表記を避ける
- quoted namespace など、プレビュー互換性が低い構文を避ける
5. 図と実装のズレがないかを見直す

## 判断基準

- `RHI` は単体で完結させる
- `Backends` の図には、実装対象である `RHI` を含める
- `App` と `Editor` は組み立て対象なので、直接参照している共有層を含める
- 実装から読み取れない関係は図へ足さない
