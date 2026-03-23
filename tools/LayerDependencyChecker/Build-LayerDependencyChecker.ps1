param(
    [ValidateSet("Debug", "Release", "All")]
    [string]$Configuration = "All"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectPath = Join-Path $scriptDir "LayerDependencyChecker.csproj"

if (-not (Test-Path $projectPath)) {
    throw "LayerDependencyChecker.csproj was not found: $projectPath"
}

$configurations = if ($Configuration -eq "All") {
    @("Debug", "Release")
}
else {
    @($Configuration)
}

foreach ($currentConfiguration in $configurations) {
    Write-Host "[Build] LayerDependencyChecker ($currentConfiguration)"
    & dotnet build $projectPath -c $currentConfiguration

    if ($LASTEXITCODE -ne 0) {
        throw "LayerDependencyChecker build failed for configuration: $currentConfiguration"
    }
}

Write-Host "[Done] LayerDependencyChecker build completed."
