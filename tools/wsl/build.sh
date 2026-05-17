#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

CONFIGURATION="Debug"
PLATFORM="x64"
TARGET="Xelqoria.slnx"
MSBUILD_VERBOSITY="minimal"

usage() {
    cat <<'USAGE'
Usage: tools/wsl/build.sh [options] [msbuild-target]

Options:
  -c, --configuration <Debug|Release>  Build configuration. Default: Debug
  -p, --platform <x64|Win32>           Build platform. Default: x64
  -v, --verbosity <level>              MSBuild verbosity. Default: minimal
  -h, --help                           Show this help.

Examples:
  tools/wsl/build.sh
  tools/wsl/build.sh -c Release
  tools/wsl/build.sh tests/Editor/Xelqoria.Tests.Editor.vcxproj
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--configuration)
            CONFIGURATION="${2:?missing configuration}"
            shift 2
            ;;
        -p|--platform)
            PLATFORM="${2:?missing platform}"
            shift 2
            ;;
        -v|--verbosity)
            MSBUILD_VERBOSITY="${2:?missing verbosity}"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
        *)
            TARGET="$1"
            shift
            ;;
    esac
done

to_wsl_path() {
    local path="$1"
    if [[ "$path" =~ ^[A-Za-z]:\\ ]]; then
        wslpath -u "$path"
    else
        printf '%s\n' "$path"
    fi
}

find_msbuild() {
    local vswhere="/mnt/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
    local found=""
    if [[ -x "$vswhere" ]]; then
        found="$("$vswhere" -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\amd64\MSBuild.exe' 2>/dev/null | tr -d '\r' | head -n 1 || true)"
        if [[ -n "$found" ]]; then
            to_wsl_path "$found"
            return 0
        fi
    fi

    local candidates=(
        "/mnt/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/amd64/MSBuild.exe"
        "/mnt/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe"
        "/mnt/c/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/MSBuild/Current/Bin/amd64/MSBuild.exe"
        "/mnt/c/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/MSBuild/Current/Bin/MSBuild.exe"
        "/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/BuildTools/MSBuild/Current/Bin/amd64/MSBuild.exe"
        "/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/BuildTools/MSBuild/Current/Bin/MSBuild.exe"
    )

    for candidate in "${candidates[@]}"; do
        if [[ -x "$candidate" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    return 1
}

find_dotnet() {
    local candidates=(
        "/mnt/c/Program Files/dotnet/dotnet.exe"
        "/mnt/c/Program Files (x86)/dotnet/dotnet.exe"
    )

    for candidate in "${candidates[@]}"; do
        if [[ -x "$candidate" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    if command -v dotnet.exe >/dev/null 2>&1; then
        command -v dotnet.exe
        return 0
    fi

    if command -v dotnet >/dev/null 2>&1; then
        command -v dotnet
        return 0
    fi

    return 1
}

MSBUILD="$(find_msbuild)" || {
    echo "MSBuild.exe was not found. Install Visual Studio/Build Tools with MSBuild and C++ workload on Windows." >&2
    exit 1
}

DOTNET="$(find_dotnet)" || {
    echo "dotnet was not found. Install .NET SDK 8+ on Windows or WSL." >&2
    exit 1
}

cd "$REPO_ROOT"

echo "[wsl-build] dotnet: $DOTNET"
"$DOTNET" build tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c "$CONFIGURATION" /nologo /v:minimal
"$DOTNET" run --project tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c "$CONFIGURATION" -- "$REPO_ROOT"

echo "[wsl-build] msbuild: $MSBUILD"
"$MSBUILD" "$TARGET" /m "/p:Configuration=${CONFIGURATION}" "/p:Platform=${PLATFORM}" "/v:${MSBUILD_VERBOSITY}"
