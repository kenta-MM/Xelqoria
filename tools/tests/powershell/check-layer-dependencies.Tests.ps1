Set-StrictMode -Version Latest

$script:RepoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$script:TargetScript = Join-Path $script:RepoRoot "tools/check-layer-dependencies.ps1"
$script:PowerShellExe = Join-Path $env:SystemRoot "System32\WindowsPowerShell\v1.0\powershell.exe"
$script:LayerProjectDirs = @{
    "App" = "App"
    "Backends" = "Backends.D3D11"
    "Core" = "Core"
    "Game" = "Game"
    "Graphics" = "Graphics"
    "RHI" = "RHI"
}
$script:AllowedDependencies = @{
    "App" = @("App", "Backends", "Core", "Graphics", "RHI")
    "Backends" = @("Backends", "RHI")
    "Core" = @("Core")
    "Game" = @("Core", "Game", "Graphics")
    "Graphics" = @("Core", "Graphics", "RHI")
    "RHI" = @("RHI")
}

function New-TempDirectory {
    $directoryName = "xelqoria-layer-test-{0}" -f ([System.Guid]::NewGuid().ToString("N"))
    $directoryPath = Join-Path ([System.IO.Path]::GetTempPath()) $directoryName
    [System.IO.Directory]::CreateDirectory($directoryPath) | Out-Null
    return $directoryPath
}

function New-TestRepoFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,
        [Parameter(Mandatory = $true)]
        [string]$RelativePath,
        [Parameter(Mandatory = $true)]
        [string]$Content
    )

    $fullPath = Join-Path $Root $RelativePath
    $parentDirectory = Split-Path -Parent $fullPath
    if (-not (Test-Path $parentDirectory)) {
        [System.IO.Directory]::CreateDirectory($parentDirectory) | Out-Null
    }

    [System.IO.File]::WriteAllText($fullPath, $Content)
}

function Invoke-LayerCheck {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RepoDir
    )

    $stdoutPath = Join-Path ([System.IO.Path]::GetTempPath()) ("xelqoria-layer-stdout-{0}.txt" -f ([System.Guid]::NewGuid().ToString("N")))
    $stderrPath = Join-Path ([System.IO.Path]::GetTempPath()) ("xelqoria-layer-stderr-{0}.txt" -f ([System.Guid]::NewGuid().ToString("N")))

    try {
        $process = Start-Process `
            -FilePath $script:PowerShellExe `
            -ArgumentList @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $script:TargetScript, "-RepoDir", $RepoDir) `
            -NoNewWindow `
            -Wait `
            -PassThru `
            -RedirectStandardOutput $stdoutPath `
            -RedirectStandardError $stderrPath

        $output = @()
        if (Test-Path $stdoutPath) {
            $output += Get-Content -Path $stdoutPath
        }
        if (Test-Path $stderrPath) {
            $output += Get-Content -Path $stderrPath
        }

        return @{
            ExitCode = $process.ExitCode
            Output = $output
        }
    }
    finally {
        Remove-Item -Path $stdoutPath -Force -ErrorAction SilentlyContinue
        Remove-Item -Path $stderrPath -Force -ErrorAction SilentlyContinue
    }
}

function New-LayerDependencyFixture {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,
        [Parameter(Mandatory = $true)]
        [string]$SourceLayer,
        [Parameter(Mandatory = $true)]
        [string]$TargetLayer,
        [string]$IncludeDirective
    )

    $sourceProjectDir = $script:LayerProjectDirs[$SourceLayer]
    $targetProjectDir = $script:LayerProjectDirs[$TargetLayer]
    $headerName = "{0}Dependency.h" -f $TargetLayer

    if (-not $IncludeDirective) {
        $IncludeDirective = '#include "{0}"' -f $headerName
    }

    New-TestRepoFile -Root $Root -RelativePath ("{0}/Source/{1}Consumer.cpp" -f $sourceProjectDir, $SourceLayer) -Content $IncludeDirective
    New-TestRepoFile -Root $Root -RelativePath ("{0}/Source/{1}" -f $targetProjectDir, $headerName) -Content "#pragma once"
}

function Get-DisallowedDependencies {
    $allLayers = @($script:AllowedDependencies.Keys)
    $disallowed = @()

    foreach ($sourceLayer in $allLayers) {
        foreach ($targetLayer in $allLayers) {
            if ($script:AllowedDependencies[$sourceLayer] -contains $targetLayer) {
                continue
            }

            $disallowed += @{
                Source = $sourceLayer
                Target = $targetLayer
            }
        }
    }

    return $disallowed
}

Describe "check-layer-dependencies.ps1" {
    foreach ($sourceLayer in $script:AllowedDependencies.Keys) {
        foreach ($targetLayer in $script:AllowedDependencies[$sourceLayer]) {
            It ("passes when {0} depends on allowed layer {1}" -f $sourceLayer, $targetLayer) {
                $testRepo = New-TempDirectory
                try {
                    New-LayerDependencyFixture -Root $testRepo -SourceLayer $sourceLayer -TargetLayer $targetLayer

                    $result = Invoke-LayerCheck -RepoDir $testRepo

                    $result.ExitCode | Should Be 0
                    ($result.Output -join [Environment]::NewLine) | Should Match "Layer dependency check passed\."
                }
                finally {
                    Remove-Item -Path $testRepo -Recurse -Force -ErrorAction SilentlyContinue
                }
            }
        }
    }

    foreach ($case in Get-DisallowedDependencies) {
        It ("fails when {0} depends on disallowed layer {1}" -f $case.Source, $case.Target) {
            $testRepo = New-TempDirectory
            try {
                New-LayerDependencyFixture -Root $testRepo -SourceLayer $case.Source -TargetLayer $case.Target

                $result = Invoke-LayerCheck -RepoDir $testRepo
                $outputText = $result.Output -join [Environment]::NewLine

                $sourceProjectDir = $script:LayerProjectDirs[$case.Source]
                $targetProjectDir = $script:LayerProjectDirs[$case.Target]
                $expectedMessage = "{0}[\\/]Source[\\/]{1}Consumer\.cpp:1: {1} cannot depend on {2} \({3}[\\/]Source[\\/]{2}Dependency\.h\)" -f $sourceProjectDir, $case.Source, $case.Target, $targetProjectDir

                $result.ExitCode | Should Be 1
                $outputText | Should Match "Layer dependency violations detected:"
                $outputText | Should Match $expectedMessage
            }
            finally {
                Remove-Item -Path $testRepo -Recurse -Force -ErrorAction SilentlyContinue
            }
        }
    }

    It "ignores angle-bracket includes" {
        $testRepo = New-TempDirectory
        try {
            New-LayerDependencyFixture -Root $testRepo -SourceLayer "Game" -TargetLayer "RHI" -IncludeDirective '#include <RHIDependency.h>'

            $result = Invoke-LayerCheck -RepoDir $testRepo

            $result.ExitCode | Should Be 0
            ($result.Output -join [Environment]::NewLine) | Should Match "Layer dependency check passed\."
        }
        finally {
            Remove-Item -Path $testRepo -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}
