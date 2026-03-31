---
name: xelqoria-runtime-editor-boundary-check
description: Use when checking Xelqoria changes for Runtime and Editor artifact separation. Verify that App stays independent from Editor, review project references and binary-boundary assumptions, and align findings with runtime artifact validation documentation and workflow behavior.
---

# Xelqoria Runtime Editor Boundary Check

Xelqoria の Runtime と Editor の成果物境界を確認する時に使う。

## 使いどころ

- Runtime 側へ Editor 依存が混入していないか確認したい時
- `App`、`Editor`、workflow、成果物境界文書を変更した時
- Runtime Artifact Validation の失敗原因を切り分けたい時

## 最初にやること

1. [AGENTS.md]($XELQORIA_ROOT/AGENTS.md) を確認する
2. 次を必要に応じて読む
- [docs/agents/runtime-vs-editor-boundary.md]($XELQORIA_ROOT/docs/agents/runtime-vs-editor-boundary.md)
- [docs/quality/runtime-artifact-validation.md]($XELQORIA_ROOT/docs/quality/runtime-artifact-validation.md)
- `.github/workflows/runtime-artifact-validation.yml`

## チェック項目

1. プロジェクト参照
- `App/Xelqoria.App.vcxproj` が `Editor` を参照していないか
- Runtime 側へ Editor 専用ライブラリがぶら下がっていないか

2. コード参照
- Runtime 側から `Xelqoria::Editor` 名前空間や Editor ヘッダを参照していないか
- Editor 専用状態が共通層へ入っていないか

3. 成果物想定
- Runtime 単体で起動・描画が完結するか
- Editor が Runtime 共有層の上に載る構造を保っているか

4. CI 整合
- 変更内容が `runtime-artifact-validation.yml` と `docs/quality/runtime-artifact-validation.md` の説明と一致しているか

## 出力スタイル

- 問題があれば Runtime 影響の大きい順に示す
- 問題がなければ、確認した境界を短くまとめる
