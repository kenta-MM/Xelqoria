param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [string]$PlatformToolset = "v143"
)

$ErrorActionPreference = "Stop"

function Find-FirstExistingPath {
    param(
        [string[]]$Candidates
    )

    foreach ($candidate in $Candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }

        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Find-VisualStudioInstallationPath {
    $vswherePath = Find-FirstExistingPath @(
        (Get-Command vswhere -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue)
    )

    if (-not $vswherePath) {
        throw "vswhere.exe was not found."
    }

    $installationPath = & $vswherePath -latest -products * -requires Microsoft.Component.MSBuild  -property installationPath
    if ($LASTEXITCODE -ne 0) {
        throw "vswhere.exe failed to locate a Visual Studio installation."
    }

    return ($installationPath | Select-Object -First 1).Trim()
}

function Find-LatestMsvcToolPath {
    param(
        [string]$VisualStudioInstallationPath,
        [string]$RelativeToolPath
    )

    if ([string]::IsNullOrWhiteSpace($VisualStudioInstallationPath)) {
        return $null
    }

    $msvcRoot = Join-Path $VisualStudioInstallationPath "VC\Tools\MSVC"
    if (-not (Test-Path $msvcRoot)) {
        return $null
    }

    $toolPath = Get-ChildItem -Path $msvcRoot -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending |
        ForEach-Object { Join-Path $_.FullName $RelativeToolPath } |
        Where-Object { Test-Path $_ } |
        Select-Object -First 1

    return $toolPath
}

function Convert-ToNativePath {
    param(
        [string]$Path
    )

    if ($Path -like "/mnt/*") {
        return (& wslpath -w $Path).Trim()
    }

    return $Path
}

function Invoke-CheckedCommand {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed: $FilePath $($Arguments -join ' ')"
    }
}

function Assert-NoEditorProjectReference {
    param(
        [string]$ProjectPath
    )

    [xml]$projectXml = Get-Content -Path $ProjectPath
    $projectReferences = @($projectXml.Project.ItemGroup.ProjectReference | ForEach-Object { $_.Include })

    if ($projectReferences | Where-Object { $_ -match "(^|\\)Editor(\\|/)" }) {
        throw "Runtime project '$ProjectPath' must not reference Editor projects."
    }
}

function Assert-NoEditorDependents {
    param(
        [string]$DumpbinPath,
        [string]$BinaryPath
    )

    $dumpbinOutput = & $DumpbinPath /dependents $BinaryPath
    $dumpbinText = $dumpbinOutput -join [Environment]::NewLine

    if ($dumpbinText -match "(?i)xelqoria\.editor|editor") {
        throw "Runtime artifact '$BinaryPath' has an unexpected Editor dependent.`n$dumpbinText"
    }
}

function Assert-NoEditorSymbols {
    param(
        [string]$DumpbinPath,
        [string]$BinaryPath
    )

    $dumpbinOutput = & $DumpbinPath /symbols $BinaryPath
    $dumpbinText = $dumpbinOutput -join [Environment]::NewLine
    $editorSymbolPattern = "(?i)(xelqoria(\.|\:\:)editor|editor::|\\editor\\|/editor/|xelqoria\.editor\.pdb)"

    if ($dumpbinText -match $editorSymbolPattern) {
        throw "Runtime artifact '$BinaryPath' contains unexpected Editor symbol references.`n$dumpbinText"
    }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$layerCheckerProject = Convert-ToNativePath (Join-Path $repoRoot "tools\LayerDependencyChecker\LayerDependencyChecker.csproj")
$appProjectPath = Join-Path $repoRoot "App\Xelqoria.App.vcxproj"
$appProject = Convert-ToNativePath $appProjectPath
$appArtifactPathCandidates = @(
    (Join-Path $repoRoot "artifacts\$Platform\$Configuration\Xelqoria.App.exe"),
    (Join-Path $repoRoot "App\artifacts\$Platform\$Configuration\Xelqoria.App.exe")
)

$dotnetPath = Find-FirstExistingPath @(
    (Get-Command dotnet -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
    "${env:ProgramFiles}\dotnet\dotnet.exe",
    "${env:ProgramFiles(x86)}\dotnet\dotnet.exe",
    "/mnt/c/Program Files/dotnet/dotnet.exe"
)
if (-not $dotnetPath) {
    throw "dotnet.exe was not found."
}

$visualStudioInstallationPath = Find-VisualStudioInstallationPath
$latestDumpbinPath = Find-LatestMsvcToolPath -VisualStudioInstallationPath $visualStudioInstallationPath -RelativeToolPath "bin\Hostx64\x64\dumpbin.exe"

$msbuildPath = Find-FirstExistingPath @(
    (Get-Command msbuild -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue)
)
if (-not $msbuildPath) {
    throw "MSBuild.exe was not found."
}

$dumpbinPath = Find-FirstExistingPath @(
    $latestDumpbinPath,
    "${env:ProgramFiles}\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\dumpbin.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\17\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\dumpbin.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\17\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe",
    "/mnt/c/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.50.35717/bin/Hostx64/x64/dumpbin.exe",
    "/mnt/c/Program Files/Microsoft Visual Studio/17/Community/VC/Tools/MSVC/14.50.35717/bin/Hostx64/x64/dumpbin.exe",
    "/mnt/c/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/dumpbin.exe",
    "/mnt/c/Program Files/Microsoft Visual Studio/17/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/dumpbin.exe"
)
if (-not $dumpbinPath) {
    throw "dumpbin.exe was not found."
}

Write-Host "Building LayerDependencyChecker..."
Invoke-CheckedCommand -FilePath $dotnetPath -Arguments @(
    "msbuild",
    $layerCheckerProject,
    "/p:Configuration=$Configuration"
)

Write-Host "Checking runtime project references..."
Assert-NoEditorProjectReference -ProjectPath $appProjectPath

Write-Host "Building runtime artifact..."
Invoke-CheckedCommand -FilePath $msbuildPath -Arguments @(
    $appProject,
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform",
    "/p:PlatformToolset=$PlatformToolset"
)

$appArtifactPath = Find-FirstExistingPath -Candidates $appArtifactPathCandidates
if (-not $appArtifactPath) {
    throw "Runtime artifact was not produced. Tried:`n$($appArtifactPathCandidates -join [Environment]::NewLine)"
}

Write-Host "Inspecting runtime dependents..."
Assert-NoEditorDependents -DumpbinPath $dumpbinPath -BinaryPath (Convert-ToNativePath $appArtifactPath)

Write-Host "Inspecting runtime symbols..."
Assert-NoEditorSymbols -DumpbinPath $dumpbinPath -BinaryPath (Convert-ToNativePath $appArtifactPath)

Write-Host "Runtime artifact validation passed."
