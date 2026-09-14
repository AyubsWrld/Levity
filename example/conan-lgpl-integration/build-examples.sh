#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

echo "=== Installing dependencies ==="

conan install . \
    -s build_type=Release \
    --build=missing

TOOLCHAIN="${SCRIPT_DIR}/build/Release/generators/conan_toolchain.cmake"

if [ ! -f "${TOOLCHAIN}" ]; then
    echo "error: toolchain not found at:"
    echo "  ${TOOLCHAIN}"
    exit 1
fi

echo
echo "=== Building linked example ==="

cmake -S . -B build/linked \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_LGPL_LINK=ON

cmake --build build/linked

echo
echo "=== Building unlinked example ==="

cmake -S . -B build/unlinked \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_LGPL_LINK=OFF

cmake --build build/unlinked

echo
echo "Build complete:"
echo "  linked:   ${SCRIPT_DIR}/build/linked/myapp"
echo "  unlinked: ${SCRIPT_DIR}/build/unlinked/myapp"
