#!/usr/bin/env bash
# Gera o pacote .deb do MySimpleSIP (CPack) em dist/: compilação Release separada em
# build-deb/, prefixo /usr, sem os testes. O pjproject é compilado antes, se preciso.
# Uso: ./scripts/build-deb.sh
# Instalar: sudo apt install ./dist/mysimplesip_<versão>_amd64.deb
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-deb"
cd "$ROOT"

# pjproject (mesma regra do build.sh: primeira vez ou patch novo)
PJ_BASE="$ROOT/third_party/pjproject"
# (sem "ls | head": com set -e/pipefail, o ls sem fontes ainda baixadas encerrava o script calado)
PJ_SRC=""
for d in "$PJ_BASE"/pjproject-*/; do
    if [[ -d "$d" ]]; then PJ_SRC="$d"; break; fi
done
PJ_BUILD=0
[[ -f "$PJ_BASE/install/lib/pkgconfig/libpjproject.pc" ]] || PJ_BUILD=1
for p in "$PJ_BASE"/patches/*.patch; do
    [[ -e "$p" ]] || continue
    [[ -n "$PJ_SRC" && -f "$PJ_SRC/.applied-$(basename "$p")" ]] || PJ_BUILD=1
done
if [[ $PJ_BUILD -eq 1 ]]; then
    "$ROOT/scripts/build-pjproject.sh"
fi

cmake -S "$ROOT" -B "$BUILD" -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DBUILD_TESTING=OFF
cmake --build "$BUILD"
(cd "$BUILD" && cpack)
ls -l "$ROOT"/dist/mysimplesip_*.deb
