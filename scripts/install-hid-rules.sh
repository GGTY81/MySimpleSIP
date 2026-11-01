#!/usr/bin/env bash
# Instala (ou remove) a regra do udev que dá ao usuário da sessão acesso aos fones USB com
# controles de telefonia (gancho, mudo, LEDs), usada pela opção "Suporte a fone USB HID".
# Só os dispositivos com coleção HID de telefonia são liberados (o verificador
# mysimplesip-hid-check lê o descritor); teclados e mouses continuam só do root.
# Uso: sudo ./scripts/install-hid-rules.sh            (instala/atualiza)
#      sudo ./scripts/install-hid-rules.sh --remove   (remove)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HELPER_SRC="$ROOT/build/src/tools/mysimplesip-hid-check"
HELPER="/usr/local/libexec/mysimplesip/mysimplesip-hid-check"
RULE="/etc/udev/rules.d/70-mysimplesip-headset.rules"

if [[ $EUID -ne 0 ]]; then
    echo "Rode com sudo: sudo $0 ${1:-}" >&2
    exit 1
fi

reload() {
    udevadm control --reload
    udevadm trigger --subsystem-match=hidraw --action=change
}

if [[ "${1:-}" == "--remove" ]]; then
    rm -f "$RULE" "$HELPER"
    rmdir --ignore-fail-on-non-empty "$(dirname "$HELPER")" 2>/dev/null || true
    reload
    echo "Regra do fone HID removida."
    exit 0
fi

if [[ ! -x "$HELPER_SRC" ]]; then
    echo "Compile primeiro: ./build.sh" >&2
    exit 1
fi

install -D -m 0755 "$HELPER_SRC" "$HELPER"
sed "s|@HID_CHECK@|$HELPER|" "$ROOT/resources/udev/70-mysimplesip-headset.rules.in" > "$RULE"
chmod 0644 "$RULE"
reload
echo "Regra instalada em $RULE."
echo "Fones já conectados passam a valer agora; se o fone não responder, desconecte e conecte de novo."
