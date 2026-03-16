param(
    [Parameter(Mandatory = $true)]
    [string]$RepoDir
)

$RepoDir = $RepoDir.Trim('"')
$RepoDir = [System.IO.Path]::GetFullPath($RepoDir)

# Allowed layer dependencies keyed by source layer name.
$allowedDependencies = @{
    "App" = @("App", "Backends", "Core", "Game", "Graphics", "RHI")
    "Backends" = @("Backends", "RHI")
    "Core" = @("Core")
    "Game" = @("Core", "Game", "Graphics")
    "Graphics" = @("Core", "Graphics", "RHI")
    "RHI" = @("RHI")
}

function Get-LayerName {
    param(
        [string]$FilePath,
        [System.Collections.Generic.List[string]]$SourceRoots,
        [hashtable]$SourceRootLayers
    )
    $normalizedPath = [System.IO.Path]::GetFullPath($FilePath)

    foreach ($sourceRoot in $SourceRoots) {
        $normalizedRoot = [System.IO.Path]::GetFullPath($sourceRoot)
        if (-not $normalizedPath.StartsWith($normalizedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            continue
        }

        return $SourceRootLayers[$normalizedRoot]
    }

    return $null
}

function Resolve-IncludePath {
    param(
        [string]$SourceFile,
        [string]$IncludePath,
        [System.Collections.Generic.List[string]]$SourceRoots
    )
    $sourceDirectory = Split-Path $SourceFile -Parent
    $localCandidate = [System.IO.Path]::GetFullPath((Join-Path $sourceDirectory $IncludePath))
    if (Test-Path $localCandidate) {
        return $localCandidate
    }

    foreach ($root in $SourceRoots) {
        $candidate = Join-Path $root $IncludePath
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $localCandidate
}

function Get-RelativePath {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    $normalizedBasePath = [System.IO.Path]::GetFullPath($BasePath)
    $normalizedTargetPath = [System.IO.Path]::GetFullPath($TargetPath)

    if (-not $normalizedBasePath.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $normalizedBasePath += [System.IO.Path]::DirectorySeparatorChar
    }

    $baseUri = New-Object System.Uri($normalizedBasePath)
    $targetUri = New-Object System.Uri($normalizedTargetPath)
    $relativeUri = $baseUri.MakeRelativeUri($targetUri)
    return [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace('/', [System.IO.Path]::DirectorySeparatorChar)
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
$sourceRootLayers = @{}

# Map each existing Source directory to its logical layer name.
foreach ($projectName in $projectDirs) {
    $sourceRoot = Join-Path (Join-Path $RepoDir $projectName) "Source"
    if (-not (Test-Path $sourceRoot)) {
        continue
    }

    $sourceRoots.Add($sourceRoot)
    $normalizedSourceRoot = [System.IO.Path]::GetFullPath($sourceRoot)
    $layerName = $projectName
    if ($projectName.StartsWith("Backends")) {
        $layerName = "Backends"
    }

    $sourceRootLayers[$normalizedSourceRoot] = $layerName
}

# Collect all .cpp and .h files under the discovered Source directories.
$sourceFiles = foreach ($root in $sourceRoots) {
    Get-ChildItem -Path $root -Recurse -File -Include *.h,*.cpp
}

# 
$violations = New-Object System.Collections.Generic.List[string]
foreach ($file in $sourceFiles) {
    $sourceLayer = Get-LayerName -FilePath $file.FullName -SourceRoots $sourceRoots -SourceRootLayers $sourceRootLayers
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
        $resolvedInclude = Resolve-IncludePath -SourceFile $file.FullName -IncludePath $includePath -SourceRoots $sourceRoots
        $targetLayer = Get-LayerName -FilePath $resolvedInclude -SourceRoots $sourceRoots -SourceRootLayers $sourceRootLayers

        if (-not $targetLayer) {
            continue
        }

        if ($allowedTargets -notcontains $targetLayer) {
            $relativeSource = Get-RelativePath -BasePath $RepoDir -TargetPath $file.FullName
            $relativeTarget = Get-RelativePath -BasePath $RepoDir -TargetPath $resolvedInclude
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
