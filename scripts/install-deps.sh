#!/usr/bin/env bash
# Instala as dependências de compilação do MySimpleSIP no Ubuntu 26.04.
# Uso: sudo ./scripts/install-deps.sh
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    echo "Execute com sudo: sudo $0" >&2
    exit 1
fi

PACKAGES=(
    # ferramentas
    build-essential cmake ninja-build pkg-config git curl wget patch xz-utils bzip2 ca-certificates
    # Qt 6
    qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools qtkeychain-qt6-dev
    # SIP / segurança
    libssl-dev libsrtp2-dev uuid-dev
    # codecs de áudio
    libopus-dev libbcg729-dev libspeex-dev libspeexdsp-dev libgsm1-dev libmp3lame-dev
    libopencore-amrnb-dev libopencore-amrwb-dev libvo-amrwbenc-dev
    # dispositivos de áudio
    libasound2-dev libpulse-dev
    # vídeo
    libvpx-dev libopenh264-dev libavcodec-dev libavformat-dev libswscale-dev libavutil-dev libv4l-dev libsdl2-dev
    # dados
    libsqlite3-dev libsqlitecpp-dev libjsoncpp-dev
    # headset HID
    libhidapi-dev libudev-dev
    # testes / diagnóstico
    libgtest-dev sngrep
)

apt-get update
apt-get install -y --no-install-recommends "${PACKAGES[@]}"
echo "Dependências instaladas."
