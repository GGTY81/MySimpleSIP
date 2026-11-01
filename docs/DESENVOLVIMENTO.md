# Desenvolvimento

Guia para quem quer compilar, testar ou contribuir com o MySimpleSIP. O estado do projeto e a
próxima etapa ficam sempre em [DIARIO_DO_PROJETO.md](DIARIO_DO_PROJETO.md); a arquitetura e o
plano de fases em [ANALISE_E_PLANO.md](ANALISE_E_PLANO.md).

## Pilha

C++20 · CMake/Ninja · Qt 6 Widgets · pjproject 2.15.1 (compilado estaticamente por
`scripts/build-pjproject.sh`, com `third_party/pjproject/config_site.h` e patches próprios) ·
SQLiteCpp · jsoncpp · QtKeychain · libpulse · libmp3lame · GoogleTest.

## Compilar

```bash
sudo ./scripts/install-deps.sh     # dependências do Ubuntu 26.04
./build.sh                         # pjproject (1ª vez) + aplicativo, em build/ (RelWithDebInfo)
./build.sh Debug                   # outra configuração
```

Build com sanitizadores (AddressSanitizer + UBSan):

```bash
cmake -S . -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build build-asan
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan   # vazamentos herdados do original
```

## Testes

```bash
ctest --test-dir build                       # todos (unidade, núcleo, dados, interface offscreen)
tests/sip-lab/loopback.sh                    # chamada SIP real entre duas instâncias da CLI
```

Testes opcionais (usam recursos reais da máquina):

| Variável | Testes | Requisito |
|---|---|---|
| `MSIP_TEST_SIP=1` | chamadas, presença, transferência, conferência, gravação, comandos e mensagens SIP reais pela janela, contra o `msip-cli` | portas UDP 5060, 5070 e 5080 livres |
| `MSIP_TEST_AUDIO=1` | backend PulseAudio/PipeWire (1 s tocando silêncio e lendo o microfone) | servidor de som |
| `MSIP_TEST_KEYRING=1` | senha no chaveiro do GNOME | sessão gráfica com chaveiro |

```bash
MSIP_TEST_SIP=1 QT_QPA_PLATFORM=offscreen ./build/tests/test_ui
MSIP_TEST_AUDIO=1 ./build/tests/test_core --gtest_filter='PulseAudio*'
```

Para ver uma tela sem abri-la (usa um perfil temporário para não tocar no seu):

```bash
T=$(mktemp -d); XDG_CONFIG_HOME=$T/c XDG_DATA_HOME=$T/d XDG_STATE_HOME=$T/s \
  MSIP_SCREENSHOT_DIALOG=settings QT_QPA_PLATFORM=offscreen \
  ./build/src/mysimplesip --screenshot /tmp/tela.png
```

(`MSIP_SCREENSHOT_DIALOG`: vazio = janela principal, `account`, `messages`, `ringin`,
`contact`, `transfer`, `settings`, `shortcuts`, `export`.)

A CLI de teste do núcleo (`build/src/cli/msip-cli`) faz chamadas sem interface — veja o
comentário no topo de `src/cli/msip_cli.cpp` (comandos `call`, `answer`, `hangup`, `hold`,
`dtmf`, `message`, `online`, `devices`, `status`, `wait`, `quit`).

## Estrutura

| Caminho | Conteúdo |
|---|---|
| `src/compat/` | Camada que imita o MFC/Win32 usado pelo MicroSIP (`CString`, `CTime`, coleções, INI, `PostMessage/SetTimer`, `CStdioFileEx`...). |
| `src/common/` | `define.h`, `const.h`, `resource.h` do original. |
| `src/settings/` | Port de `settings.cpp` (INI), caminhos XDG, senhas. |
| `src/i18n/` | Langpacks do MicroSIP. |
| `src/core/` | Núcleo de telefonia: `global.cpp`, `MSIP.cpp`, parte pjsua do `CmainDlg`, callbacks, HTTP, MP3, backend PulseAudio. |
| `src/data/`, `src/service/`, `src/domain/`, `src/utils/` | Histórico (SQLite), contatos, CSV, XML (`CMarkup` próprio). |
| `src/app/` | Integração Qt: fila de mensagens, chaveiro. |
| `src/ui/` | Janelas Qt: principal, discador, chamadas, chamada recebida, histórico, contatos, configurações e diálogos. |
| `src/cli/` | `msip-cli`, CLI de teste do núcleo. |
| `resources/` | Ícones, bitmaps, sons e langpacks (gerados por `scripts/convert-resources.py`). |
| `docs/layout/` | Layout dos diálogos do `.rc` original (`dialogs.json`, `DIALOGOS.md`) e capturas. |
| `tests/` | GoogleTest (`compat`, `core`, `storage`, `ui`, `app`), `sip-lab/loopback.sh`, `pj-smoke`. |

## Método de port

O objetivo é fidelidade ao MicroSIP 3.22.16: mesmos nomes de funções, chaves do INI e regras
de decisão. Para portar um arquivo:

1. Copiar o original (sem CR) para `src/...`.
2. Trocar só o que depende do Windows/MFC, marcando cada troca com `// MySimpleSIP:`.
3. Se faltar algo da API do MFC, implementar em `src/compat/` com a mesma semântica e um teste.
4. Diálogos: montar com `CDialogLayout::Build(this, "IDD_...")` a partir do layout extraído do
   `.rc` e acessar os controles pelo ID original.

### Código original de referência (`reference/`)

O código-fonte do MicroSIP 3.22.16 **não é versionado** neste repositório: ele inclui arquivos
que não podem ser redistribuídos (`lib/Markup.*` — CMarkup, "redistributing this source code
requires written permission" — e `atlrx.h`, da Microsoft). Para consultá-lo, baixe
`MicroSIP-3.22.16-src.7z` na área de código-fonte do site oficial (<https://www.microsip.org/>)
e extraia em
`reference/MicroSIP-3.22.16/` (a pasta é ignorada pelo git). Os arquivos têm CRLF: use
`tr -d '\r'` ao inspecionar. Nada desses dois arquivos foi copiado para `src/` — o `CMarkup`
do projeto é uma implementação própria (`src/utils/Markup.*`) e o regex do ATL foi traduzido
para `std::wregex` (`src/core/DialPlanRegex.*`).

## Contribuindo

- Abra uma *issue* descrevendo o problema ou a proposta.
- Siga o método de port acima e o estilo do código ao redor; documentação e comentários em
  português do Brasil.
- Rode `ctest --test-dir build` (e, se mexer em SIP, os testes com `MSIP_TEST_SIP=1`) antes de
  enviar o *pull request*.
- Ao concluir uma etapa, registre no `docs/DIARIO_DO_PROJETO.md`.
