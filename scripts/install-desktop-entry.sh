#!/usr/bin/env bash
# Cria (ou remove) o atalho do MySimpleSIP no menu de aplicativos do usuário, apontando
# para o executável compilado em build/ — sem sudo, sem copiar nada para o sistema.
# Uso: ./scripts/install-desktop-entry.sh                    (instala/atualiza)
#      ./scripts/install-desktop-entry.sh --default-handler  (instala e passa a abrir os
#                                                             links sip:, tel: e callto:)
#      ./scripts/install-desktop-entry.sh --remove           (remove)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EXE="$ROOT/build/src/mysimplesip"
APPS="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
ICONS="${XDG_DATA_HOME:-$HOME/.local/share}/icons/hicolor"
DESKTOP="$APPS/mysimplesip.desktop"
SCHEMES="sip tel callto"

if [[ "${1:-}" == "--remove" ]]; then
    rm -f "$DESKTOP"
    for size in 16 24 32 48 64 128 256; do
        rm -f "$ICONS/${size}x${size}/apps/mysimplesip.png"
    done
    rm -f "$ICONS/scalable/apps/mysimplesip.svg"
    update-desktop-database "$APPS" 2>/dev/null || true
    echo "Atalho removido."
    exit 0
fi

if [[ ! -x "$EXE" ]]; then
    echo "Compile primeiro: ./build.sh" >&2
    exit 1
fi

mkdir -p "$APPS"
for size in 16 24 32 48 64 128 256; do
    mkdir -p "$ICONS/${size}x${size}/apps"
    cp "$ROOT/resources/brand/png/${size}/mysimplesip.png" "$ICONS/${size}x${size}/apps/mysimplesip.png"
done
mkdir -p "$ICONS/scalable/apps"
cp "$ROOT/resources/brand/mysimplesip.svg" "$ICONS/scalable/apps/mysimplesip.svg"
cat > "$DESKTOP" <<DESK
[Desktop Entry]
Type=Application
Name=MySimpleSIP
GenericName=Softphone SIP
Comment=Softphone SIP (port do MicroSIP para Linux)
Exec="$EXE" %u
Icon=mysimplesip
Terminal=false
Categories=Network;Telephony;
MimeType=x-scheme-handler/sip;x-scheme-handler/tel;x-scheme-handler/callto;
Keywords=SIP;VoIP;telefone;softphone;
StartupWMClass=mysimplesip
DESK
update-desktop-database "$APPS" 2>/dev/null || true
gtk-update-icon-cache -q "$ICONS" 2>/dev/null || true
echo "Atalho criado: $DESKTOP"

if [[ "${1:-}" == "--default-handler" ]]; then
    # como o instalador do MicroSIP no Windows: o softphone abre os links de telefone
    for scheme in $SCHEMES; do
        xdg-mime default mysimplesip.desktop "x-scheme-handler/$scheme"
    done
    echo "Links sip:, tel: e callto: passam a abrir no MySimpleSIP."
fi
