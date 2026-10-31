#!/usr/bin/env bash
# Compila o MySimpleSIP.
# Uso: ./build.sh [Debug|Release|RelWithDebInfo]   (padrão: RelWithDebInfo)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_TYPE="${1:-RelWithDebInfo}"
BUILD_DIR="$ROOT/build"

if [[ ! -f "$ROOT/third_party/pjproject/install/lib/pkgconfig/libpjproject.pc" ]]; then
    "$ROOT/scripts/build-pjproject.sh"
fi

cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR"
echo "Executável: $BUILD_DIR/src/mysimplesip"
