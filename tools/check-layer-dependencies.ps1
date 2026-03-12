param(
    [Parameter(Mandatory = $true)]
    [string]$RepoDir
)

$allowedDependencies = @{
    "App" = @("App", "Backends", "Core", "Graphics", "RHI")
    "Backends" = @("Backends", "RHI")
    "Core" = @("Core")
    "Game" = @("Core", "Game", "Graphics")
    "Graphics" = @("Core", "Graphics", "RHI")
    "RHI" = @("RHI")
}

function Get-LayerName {
    param(
        [string]$FullPath
    )

    $normalizedPath = [System.IO.Path]::GetFullPath($FullPath)

    foreach ($root in $engineRoots) {
        $normalizedRoot = [System.IO.Path]::GetFullPath($root)
        if (-not $normalizedPath.StartsWith($normalizedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            continue
        }

        $relative = $normalizedPath.Substring($normalizedRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return $null
        }

        $segments = $relative -split '[\\/]'
        if ($segments.Length -eq 0) {
            return $null
        }

        return $segments[0]
    }

    return $null
}

function Resolve-IncludePath {
    param(
        [string]$SourceFile,
        [string]$IncludePath
    )

    if ($IncludePath.StartsWith("Engine/") -or $IncludePath.StartsWith("Engine\")) {
        foreach ($root in $sourceRoots) {
            $candidate = Join-Path $root $IncludePath
            if (Test-Path $candidate) {
                return $candidate
            }
        }

        return Join-Path $sourceRoots[0] $IncludePath
    }

    $sourceDirectory = Split-Path $SourceFile -Parent
    return [System.IO.Path]::GetFullPath((Join-Path $sourceDirectory $IncludePath))
}

$projectDirs = @(
    "RHI",
    "Graphics",
    "Backends.D3D11",
    "Backends.D3D12",
    "Core",
    "App",
    "Game"
)

$sourceRoots = New-Object System.Collections.Generic.List[string]
$engineRoots = New-Object System.Collections.Generic.List[string]

foreach ($projectName in $projectDirs) {
    $sourceRoot = Join-Path (Join-Path $RepoDir $projectName) "Source"
    $engineRoot = Join-Path $sourceRoot "Engine"

    if (Test-Path $engineRoot) {
        $sourceRoots.Add($sourceRoot)
        $engineRoots.Add($engineRoot)
    }
}

$violations = New-Object System.Collections.Generic.List[string]
$sourceFiles = foreach ($root in $engineRoots) {
    Get-ChildItem -Path $root -Recurse -File -Include *.h,*.cpp
}

foreach ($file in $sourceFiles) {
    $sourceLayer = Get-LayerName -FullPath $file.FullName
    if (-not $sourceLayer) {
        continue
    }

    $allowedTargets = $allowedDependencies[$sourceLayer]
    if (-not $allowedTargets) {
        continue
    }

    $lineNumber = 0
    foreach ($line in Get-Content -Path $file.FullName) {
        $lineNumber += 1

        if ($line -notmatch '^\s*#include\s+"([^"]+)"') {
            continue
        }

        $includePath = $matches[1]
        $resolvedInclude = Resolve-IncludePath -SourceFile $file.FullName -IncludePath $includePath
        $targetLayer = Get-LayerName -FullPath $resolvedInclude

        if (-not $targetLayer) {
            continue
        }

        if ($allowedTargets -notcontains $targetLayer) {
            $relativeSource = [System.IO.Path]::GetRelativePath($RepoDir, $file.FullName)
            $relativeTarget = [System.IO.Path]::GetRelativePath($RepoDir, $resolvedInclude)
            $violations.Add(("{0}:{1}: {2} cannot depend on {3} ({4})" -f $relativeSource, $lineNumber, $sourceLayer, $targetLayer, $relativeTarget))
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Host "Layer dependency violations detected:" -ForegroundColor Red
    foreach ($violation in $violations) {
        Write-Host "  $violation" -ForegroundColor Red
    }

    exit 1
}

Write-Host "Layer dependency check passed."
