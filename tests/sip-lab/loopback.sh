#!/usr/bin/env bash
# Teste de ponta a ponta do núcleo SIP sem servidor: duas instâncias do msip-cli com
# perfis isolados e sem placa de som; A atende automaticamente, B liga para 127.0.0.1
# pela conta local (chamada por IP), envia DTMF, coloca em espera, retoma e desliga.
# Requer a porta UDP 5060 livre.
# Uso: tests/sip-lab/loopback.sh [caminho/do/msip-cli]   (padrão: build/src/cli/msip-cli)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CLI="${1:-$ROOT/build/src/cli/msip-cli}"
WORK="$(mktemp -d /tmp/msip-loopback-XXXXXX)"
trap 'rm -rf "$WORK"' EXIT

printf 'wait 16\nstatus\nquit\n' > "$WORK/a.txt"
printf 'wait 2\ncall 127.0.0.1\nwait 3\nstatus\ndtmf 123\nwait 2\nhold\nwait 2\nunhold\nwait 2\nhangup\nwait 2\nquit\n' > "$WORK/b.txt"

timeout 40 "$CLI" --profile "$WORK/A" --null-audio --auto-answer --script "$WORK/a.txt" > "$WORK/a.log" 2>&1 &
PID_A=$!
sleep 1
timeout 40 "$CLI" --profile "$WORK/B" --null-audio --script "$WORK/b.txt" > "$WORK/b.log" 2>&1
wait "$PID_A"

fail=0
check() { # arquivo padrão descrição
    if grep -qE "$2" "$WORK/$1"; then
        echo "ok   - $3"
    else
        echo "FALHOU - $3"
        fail=1
    fi
}
check b.log 'estado CONFIRMED' 'B: chamada confirmada'
check a.log 'atendendo automaticamente' 'A: atendimento automático'
check a.log 'estado CONFIRMED' 'A: chamada confirmada'
check a.log 'DTMF recebido: 1' 'A: DTMF 1 recebido'
check a.log 'DTMF recebido: 3' 'A: DTMF 3 recebido'
check b.log 'mídia em espera \(local\)' 'B: espera local'
check a.log 'mídia em espera \(remota\)' 'A: espera remota'
check b.log 'estado DISCONNECTED \(200' 'B: desligamento normal'
check a.log 'DISCONNECTED \(200 Normal call clearing\)' 'A: desligamento recebido'
check a.log '\[fim\] encerrando' 'A: encerrou'
check b.log '\[fim\] encerrando' 'B: encerrou'

if [[ $fail -ne 0 ]]; then
    echo "--- log A"; grep -E '^\[|^>' "$WORK/a.log" || true
    echo "--- log B"; grep -E '^\[|^>' "$WORK/b.log" || true
    exit 1
fi
echo "Loopback SIP: todos os testes passaram."
