#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

CONFIGURATION="Debug"
PLATFORM="x64"
NO_BUILD=0

usage() {
    cat <<'USAGE'
Usage: tools/wsl/test.sh [options]

Options:
  -c, --configuration <Debug|Release>  Build configuration. Default: Debug
  -p, --platform <x64|Win32>           Build platform. Default: x64
  --no-build                           Run existing test binaries without building first.
  -h, --help                           Show this help.
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
        --no-build)
            NO_BUILD=1
            shift
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
            echo "Unexpected argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

cd "$REPO_ROOT"

if [[ "$NO_BUILD" -eq 0 ]]; then
    "${SCRIPT_DIR}/build.sh" -c "$CONFIGURATION" -p "$PLATFORM"
fi

TEST_DIR="${REPO_ROOT}/artifacts/${PLATFORM}/${CONFIGURATION}"
tests=(
    "Xelqoria.Tests.Core.exe"
    "Xelqoria.Tests.Game.exe"
    "Xelqoria.Tests.Graphics.exe"
    "Xelqoria.Tests.Editor.exe"
)

for test_binary in "${tests[@]}"; do
    test_path="${TEST_DIR}/${test_binary}"
    if [[ ! -x "$test_path" ]]; then
        echo "Test binary was not found: $test_path" >&2
        exit 1
    fi

    echo "[wsl-test] $test_binary"
    "$test_path"
done
