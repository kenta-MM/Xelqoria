# Runtime Artifact Validation

Runtime 成果物の最終リンク検証では、`App` 実行ファイルに Editor 依存が混入していないことをリリース判定条件として扱う。

## 判定基準

- `App/Xelqoria.App.vcxproj` が `Editor` プロジェクトを参照していない
- `Xelqoria.App.exe` の依存 DLL 一覧に `Xelqoria.Editor` や `Editor` 由来の依存が含まれない
- `Xelqoria.App.exe` のシンボル情報に `Xelqoria::Editor`、`Editor::`、`Editor` 配下パスなどの Editor 参照が含まれない
- GitHub Actions の `Runtime Artifact Validation` が `Debug` と `Release` の両構成で成功している

## ローカル確認手順

PowerShell から次を実行する。

```powershell
./tools/RuntimeArtifactValidation/Check-RuntimeArtifactIsolation.ps1 `
  -Configuration Release `
  -Platform x64 `
  -PlatformToolset v143
```

必要に応じて `-Configuration Debug` でも同じ確認を実行する。

## CI 運用

- `.github/workflows/runtime-artifact-validation.yml` が Pull Request と `develop` / `issue-94` への push で実行される
- CI が失敗した場合は Runtime への Editor 混入の可能性があるため、リリース判定を保留する
