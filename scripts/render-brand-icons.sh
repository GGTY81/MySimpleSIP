#!/usr/bin/env bash
# Gera os PNGs do ícone próprio do MySimpleSIP a partir dos SVGs de resources/brand/
# (aplicativo, bandeja inativa e chamada perdida), nos tamanhos do hicolor.
# Usa o plugin de SVG do Qt (pacote qt6-svg-plugins); compila um programa Qt de uma linha.
# Uso: ./scripts/render-brand-icons.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BRAND="$ROOT/resources/brand"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/render.cpp" <<'CPP'
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    QImageReader reader(argv[1]);
    const int size = atoi(argv[2]);
    reader.setScaledSize(QSize(size, size));
    QImage img = reader.read();
    return img.isNull() || !img.save(argv[3]) ? 1 : 0;
}
CPP
g++ -std=c++17 -fPIC "$TMP/render.cpp" -o "$TMP/render" $(pkg-config --cflags --libs Qt6Gui)

for name in mysimplesip tray_inactive tray_missed; do
    for size in 16 24 32 48 64 128 256; do
        mkdir -p "$BRAND/png/$size"
        QT_QPA_PLATFORM=offscreen "$TMP/render" "$BRAND/$name.svg" "$size" "$BRAND/png/$size/$name.png"
    done
done
echo "PNGs gerados em $BRAND/png/"
