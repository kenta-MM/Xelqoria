# CI Matrix

Xelqoria の GitHub Actions が保証する内容を定義する。

## Runtime Artifact Validation

- Workflow: .github/workflows/runtime-artifact-validation.yml
- 保証:
  - Runtime 成果物に Editor 依存が混入していない
- トリガー:
  - develop 向け Pull Request
  - main 向け Pull Request
  - workflow_dispatch
- 実行環境:
  - windows-2025
  - .NET 8
  - MSBuild 17.x
- 構成:
  - Debug
  - Release
- 実行スクリプト:
  - tools/RuntimeArtifactValidation/Check-RuntimeArtifactIsolation.ps1
- 関連文書:
  - docs/quality/runtime-artifact-validation.md

## 運用

- workflow を追加したら目的と保証内容を追記する
- 可能なら「何を実行するか」より「何を保証するか」を先に書く
- matrix ごとに保証対象が異なる場合は明記する
