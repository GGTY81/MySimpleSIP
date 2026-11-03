#!/usr/bin/env bash
# Compila o pjproject 2.15.1 (bibliotecas estáticas) e instala em third_party/pjproject/install.
# Uso: ./scripts/build-pjproject.sh [--clean]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PJ_VERSION="2.15.1"
PJ_SHA256="8f3bd99caf003f96ed8038b8a36031eb9d8cd9eaea1eaff7e01c2eef6bd55706"
TARBALL="$ROOT/third_party/downloads/pjproject-$PJ_VERSION.tar.gz"
BASE="$ROOT/third_party/pjproject"
SRC="$BASE/pjproject-$PJ_VERSION"
PREFIX="$BASE/install"
JOBS="$(nproc)"

if [[ "${1:-}" == "--clean" ]]; then
    rm -rf "$SRC" "$PREFIX"
fi

URL="https://github.com/pjsip/pjproject/archive/refs/tags/$PJ_VERSION.tar.gz"
if [[ ! -f "$TARBALL" ]]; then
    mkdir -p "$(dirname "$TARBALL")"
    echo "Baixando pjproject $PJ_VERSION..."
    # baixa num .part: um download interrompido não fica no lugar do arquivo completo
    if command -v curl >/dev/null; then
        curl -fSL --retry 3 -o "$TARBALL.part" "$URL"
    elif command -v wget >/dev/null; then
        wget -O "$TARBALL.part" "$URL"
    else
        echo "Falta o curl ou o wget: rode antes sudo ./scripts/install-deps.sh" >&2
        exit 1
    fi
    mv "$TARBALL.part" "$TARBALL"
fi
if ! echo "$PJ_SHA256  $TARBALL" | sha256sum -c --quiet -; then
    rm -f "$TARBALL"
    echo "O arquivo baixado do pjproject está corrompido e foi apagado; rode o script de novo." >&2
    exit 1
fi

if [[ ! -f "$SRC/aconfigure" ]]; then
    rm -rf "$SRC"   # extração anterior incompleta
    tar xzf "$TARBALL" -C "$BASE"
fi

# Patches próprios (aplicados uma única vez)
for p in "$BASE"/patches/*.patch; do
    [[ -e "$p" ]] || continue
    stamp="$SRC/.applied-$(basename "$p")"
    if [[ ! -f "$stamp" ]]; then
        patch -d "$SRC" -p1 < "$p"
        touch "$stamp"
    fi
done

cp "$BASE/config_site.h" "$SRC/pjlib/include/pj/config_site.h"

cd "$SRC"
CFLAGS="-O2 -fPIC" CXXFLAGS="-O2 -fPIC" ./aconfigure \
    --prefix="$PREFIX" \
    --disable-shared \
    --with-external-srtp \
    --with-external-gsm \
    --with-external-speex \
    --with-bcg729 \
    --with-opus \
    --with-vpx \
    --with-openh264 \
    --with-ffmpeg \
    --with-sdl \
    --disable-libwebrtc-aec3 \
    --disable-silk \
    --disable-lyra \
    --disable-upnp

make dep -j"$JOBS"
make -j"$JOBS"
make install
echo "pjproject $PJ_VERSION instalado em $PREFIX"
