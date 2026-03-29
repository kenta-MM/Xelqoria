# CI Matrix

この文書は、Xelqoria の GitHub Actions が何を保証するかを簡潔に整理する。

## Runtime Artifact Validation

- Workflow: `.github/workflows/runtime-artifact-validation.yml`
- 主目的:
  - Runtime 成果物へ Editor 依存が混入していないことを検証する
- トリガー:
  - `develop` 向け Pull Request
  - `main` 向け Pull Request
  - 手動実行 `workflow_dispatch`
- 実行環境:
  - `windows-2025`
  - `.NET 8`
  - `MSBuild 17.x`
- 構成:
  - `Debug`
  - `Release`
- 実行スクリプト:
  - `tools/RuntimeArtifactValidation/Check-RuntimeArtifactIsolation.ps1`
- 関連文書:
  - `docs/quality/runtime-artifact-validation.md`

## 運用メモ

- 新しい workflow を追加したら、この文書へも目的と保証内容を追記する
- 可能なら「何を実行するか」より「何を保証するか」を先に書く
- 同じ workflow でも、matrix ごとに保証対象が異なるなら明記する
