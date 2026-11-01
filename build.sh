#!/usr/bin/env bash
# Compila o MySimpleSIP.
# Uso: ./build.sh [Debug|Release|RelWithDebInfo]   (padrão: RelWithDebInfo)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_TYPE="${1:-RelWithDebInfo}"
BUILD_DIR="$ROOT/build"

# pjproject: compila na primeira vez e de novo quando chega um patch ainda não aplicado
PJ_BASE="$ROOT/third_party/pjproject"
PJ_SRC=$(ls -d "$PJ_BASE"/pjproject-*/ 2>/dev/null | head -1)
PJ_BUILD=0
[[ -f "$PJ_BASE/install/lib/pkgconfig/libpjproject.pc" ]] || PJ_BUILD=1
for p in "$PJ_BASE"/patches/*.patch; do
    [[ -e "$p" ]] || continue
    [[ -n "$PJ_SRC" && -f "$PJ_SRC/.applied-$(basename "$p")" ]] || PJ_BUILD=1
done
if [[ $PJ_BUILD == 1 ]]; then
    "$ROOT/scripts/build-pjproject.sh"
fi

cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR"
echo "Executável: $BUILD_DIR/src/mysimplesip"
