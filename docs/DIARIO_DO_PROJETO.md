# MySimpleSIP — Diário do Projeto (etapas feitas e a fazer)

> Este é o documento vivo do projeto. **Atualize-o ao final de cada sessão de trabalho.**
> A análise técnica completa e as justificativas estão em [ANALISE_E_PLANO.md](ANALISE_E_PLANO.md).

## Estado atual

| Campo | Valor |
|---|---|
| Versão vigente | **0.5.2** (ver arquivo `VERSION`) |
| Fase | 6 — Configurações e mensagens (→ 0.6.0), em andamento. Fases 0 a 5 concluídas |
| Base de referência | MicroSIP 3.22.16 (fonte em `reference/MicroSIP-3.22.16/`) + sons/langpacks do binário 3.22.12 (`reference/MicroSIP-3.22.12-assets/`), ambos somente leitura. MicroSIP 3.22.12 instalado no Bottles serve de comparação visual/funcional |
| Pilha decidida | C++20 · CMake/Ninja · Qt 6 Widgets · pjproject 2.15.1 (mesma versão do binário do MicroSIP; tarball em `third_party/`) · SQLite/SQLiteCpp · QtKeychain · hidapi |
| Alvo | Ubuntu 26.04 LTS, x86_64, GNOME (Wayland, rodando via XWayland por padrão) |
| Próxima ação | Fase 6, parte 2: `ShortcutsDlg` + atalhos/speed-dial no Discador (com presença dos atalhos), teclas de atalho do menu (aceleradores, ex. Ctrl+P), exportar/importar configuração (`ExportDlg`/`ImportDlg`) e chat SIP (`onPager`/`MessagesIncoming`). Pendente: testes reais do responsável (Configurações, agenda, presença, diretório, transferência, gravação) |

## Convenções

- **Versionamento X.Y.Z** (estilo kernel Linux): `X` série (0 = pré-paridade, 1 = paridade com MicroSIP 3.22), `Y` marco/fase concluída, `Z` incrementos e correções dentro do marco. Cada versão concluída recebe entrada no histórico abaixo (sem git até a 1.0.0; depois, tag `vX.Y.Z`).
- Status das etapas: `[x]` feito · `[~]` em andamento · `[ ]` a fazer · `[-]` cancelado (com motivo).
- Ao concluir uma etapa: marcar aqui, anotar data e observações relevantes (decisões, problemas, desvios do plano).
- O código original em `reference/` **nunca** é editado; ele é a fonte da verdade para "como o MicroSIP faz".

---

## Histórico de versões

| Versão | Data | Resumo |
|---|---|---|
| 0.0.1 | 2026-10-30 | Análise completa do código-fonte do MicroSIP 3.22.16; escolha da arquitetura (Qt 6 + pjproject); criação de `docs/ANALISE_E_PLANO.md`, deste diário, `CLAUDE.md` e `VERSION`. Nenhuma linha de código do aplicativo escrita ainda. |
| 0.0.2 | 2026-10-30 | Repositório git (`main`), `.gitignore`, `.gitattributes` (`reference/` sem normalização de CRLF), `COPYING` (GPL-2.0), `README.md`. `reference/` versionado intacto (231 arquivos); `.7z` ignorado. |
| 0.0.3 | 2026-10-30 | Levantamento da instalação de referência (MicroSIP 3.22.12 no Bottles): versões de pjproject/OpenSSL/FFmpeg/SDL/LAME, módulos e codecs compilados, recursos do .exe (REA), formato de langpack, INI, Contacts.xml e banco. Sons e langpacks copiados para `reference/MicroSIP-3.22.12-assets/`. Git removido (será criado na 1.0.0). |
| 0.0.4 | 2026-10-30 | Dependências de compilação instaladas via `scripts/install-deps.sh`; versões reais registradas na seção "Ambiente de desenvolvimento verificado". |
| 0.0.5 | 2026-10-30 | pjproject 2.15.1 compilado (estático) em `third_party/pjproject/install` via `scripts/build-pjproject.sh`, com `config_site.h` próprio e patch de compatibilidade com FFmpeg 8; teste de fumaça `tests/pj-smoke` aprovado. |
| 0.0.6 | 2026-10-30 | Esqueleto CMake/Ninja + Qt 6: `CMakeLists.txt`, `cmake/FindPJPROJECT.cmake`, `src/main.cpp` (janela com versões, `--version`, `--quit-after`), `build.sh`. Primeiro executável `build/src/mysimplesip`. |
| 0.0.7 | 2026-10-30 | Recursos: 57 ícones (217 PNGs em vários tamanhos) e 27 bitmaps convertidos para `resources/` com `resources.qrc` embutido no executável; sons e langpack pt-BR em `resources/`; layout dos 16 diálogos extraído para `docs/layout/` (DLU e px). Fim da Fase 0. |
| 0.0.8 | 2026-10-30 | Fase 1, parte 1: camada `src/compat/` (`CString`/`CStringA`, `CTime`/`CTimeSpan`, `CArray`, `CList` com `POSITION`, `CStringArray`, `CStringList`, `CByteArray`, tipos Win32) como biblioteca `msip_compat`; 21 testes GoogleTest (também limpos sob ASan/UBSan). |
| 0.0.9 | 2026-10-30 | Fase 1, parte 2: `IniFile` + `Get/WritePrivateProfileString`, `GetPrivateProfileInt`, `WritePrivateProfileSection` com semântica do Win32 (em `msip_compat`); `ResolveAppPaths` (XDG/portátil) em `src/settings/` (`msip_settings`); importação de `MicroSIP.ini` UTF-16 testada com cópia anonimizada; 32 testes. |
| 0.0.10 | 2026-10-30 | Fase 1, parte 3: `settings.cpp`/`settings.h` portados quase linha a linha (`AccountSettings`, contas, atalhos); `define.h`/`const.h` em `src/common/`; `Account.h` (de `global.h`), `Shortcut.h`; senhas via `IPasswordStore` (chaveiro) com fallback ofuscado; `XdgUserDir`; 41 testes. |
| **0.1.0** | 2026-10-30 | **Fim da Fase 1 (Fundação).** Port de `lib/langpack` (`Translate`, `LoadLangPack`, `LoadLangPackModule` com escolha por idioma do sistema) em `src/i18n/` (`msip_i18n`); decisão sobre logging (usa o log do pjsip, como o original); 47 testes. |
| 0.1.1 | 2026-10-30 | Fase 2, parte 1: `global.h/.cpp` e `lib/MSIP.h/.cpp` portados para `src/core/` (`msip_core`); emulação de `PostMessage/SetTimer/CWnd/AfxGetApp` e `CCriticalSection/CSingleLock` em `compat/`; plano de discagem com tradutor ATL→`std::wregex`; HTTP com Qt Network; ganchos `CoreHost`; 60 testes (inclusive build ASan/UBSan via CMake em `build-asan/`). |
| 0.1.2 | 2026-10-30 | Fase 2, parte 2: parte pjsua do `CmainDlg` portada para `CmainDlgCore` (`PJCreate/PJCreateRaw/PJDestroy`, codecs, dispositivos, contas, `PublishStatus`, player, `onCallAnswer/onCallHangup`) e callbacks `on_*` em `callbacks.cpp`; `QtMessageBackend` (mensagens/timers na thread do Qt); CLI `msip-cli`; teste de chamada ponta a ponta `tests/sip-lab/loopback.sh` aprovado. |
| **0.2.0** | 2026-10-30 | **Fim da Fase 2 (Núcleo SIP headless).** Registro `200 OK` no PBX real do responsável e chamada de saída com áudio nos dois sentidos (PipeWire, dispositivo padrão) pela `msip-cli`. |
| 0.2.1 | 2026-10-30 | Fase 3, parte 1: senhas no chaveiro do sistema (`KeychainPasswordStore`, QtKeychain/Secret Service); janela principal Qt `CmainDlg` (deriva de `CmainDlgCore`): abas, botão de menu, barra de status com ícone, `UpdateWindowText`, bandeja, menu principal, posição/tamanho salvos; `main.cpp` passa a abrir a janela real. |
| 0.2.2 | 2026-10-30 | Fase 3, parte 2: `CDialogLayout` (monta qualquer `IDD_*` a partir de `dialogs.json`, controles acessados pelo ID do `.rc`); `AccountDlg` portado (nova/editar/excluir conta, conta local, senha no chaveiro); teste de interface offscreen `test_ui`. |
| 0.2.3 | 2026-10-30 | Fase 3, parte 3: `Dialer` (aba Telefone) portado — teclado, número com histórico, Chamar/Encerrar, volumes/mudo, VU-meter, DTMF, DND/AA/FWD/AC/CONF/REC; controles `CButtonDialer/CButtonEx/CLevelsSliderCtrl`; `CDialogLayout` com controles próprios e `AutoMove/AutoUnmove`; chamadas pela janela com provisórios (`MakeCall/PerformCall`, toque e caixa Atender/Rejeitar). |
| 0.5.2 | 2026-10-31 | Áudio pelo **PulseAudio/PipeWire**: backend próprio do pjmedia (`PulseAudioDev`) com os nomes dos dispositivos do sistema e o "Padrão" seguindo o dispositivo padrão do PipeWire (ex.: fone Bluetooth); ALSA como reserva sem servidor de som; comando `devices` na CLI de teste; 89 testes. |
| 0.5.1 | 2026-10-31 | Fase 6, parte 1: janela de **Configurações** (`SettingsDlg`) portada com `AAOptionsDlg` (opções do atendimento automático) e `FeatureCodesDlg` (códigos de função) — toque e volume, dispositivos, codecs (habilitar/ordenar), vídeo, portas/rport, DNS SRV, STUN, gravação (pasta, MP3/WAV, automática, botão REC), DTMF, atendimento automático, desvio, recusa, diretório de usuários, ação padrão das listas, opções gerais e inicialização automática (XDG Autostart); salvar recria a sessão do pjsua (`OnRecreate`); 88 testes. |
| **0.5.0** | 2026-10-31 | **Fim da Fase 5 (Chamadas avançadas).** Gravação em MP3 (WAV do pjsua convertido com a libmp3lame ao fim da gravação; padrão do MicroSIP); testes reais de conferência com três pontas (mesclar/separar no bridge), gravação automática e comandos externos por evento com desligamento automático; 87 testes. |
| 0.4.1 | 2026-10-31 | Fase 5, parte 1: diálogo **Transfer** (transferência cega/assistida, convite à conferência, desvio de chamada recebida; lista de contatos/favoritos e teclado), menus de transferência e conferência do `MessagesDlg` (transferir para outra chamada, mesclar, separar, desconectar), `onCallTransferStatus`, captura de chamada (`CommandCallPickup`), aviso de correio de voz (MWI); 83 testes, com transferência real entre três pontas. |
| **0.4.0** | 2026-10-31 | **Fim da Fase 4 (Contatos e histórico).** Presença/BLF (`Subscribe/Unsubscribe/SubsribeNumber/UnsubscribeNumber/onBuddyState` → ícones de status e "tocando" piscando nos contatos) e diretório de usuários remoto (`UsersDirectoryLoad/onUsersDirectoryLoaded`: XML do MicroSIP, Yealink e genérico, JSON com presença, CSV; recarga por `refresh`/`Cache-Control` e reconexão progressiva); cabeçalhos HTTP no formato do WinINet; 79 testes. |
| 0.3.2 | 2026-10-30 | Fase 4, parte 2: aba **Contatos** e diálogo de contato (`AddDlg`) portados — `Contacts.xml` (leitura/gravação, migração do `[Contacts]` do INI), favoritos no topo, busca, menu de contexto (chamar número/telefone/celular, vídeo, mensagem, adicionar, editar, copiar, apagar, exportar CSV/XML, importar CSV), nome de quem liga pela agenda (`GetNameForCall`), "Adicionar contato" do Histórico; `CMarkup` próprio (o original não pode ser redistribuído); correção de leitura fora do texto no `CCSVFile` original; 77 testes. |
| 0.3.1 | 2026-10-30 | Fase 4, parte 1: aba **Histórico** (`Calls`) portada — lista do `call_log.db` (SQLite, limite de 1.000), ícones por tipo, ordenação por hora/duração, busca, menu de contexto (chamar, vídeo, mensagem, copiar, apagar, exportar/importar CSV), larguras salvas, migração da seção `[Calls]` do INI; camada de dados `msip_data` (`Domain/`, `Data/`, `Service/CallsService`, `lib/CSVFile`); `CStdioFileEx` na camada compat; 73 testes. |
| **0.3.0** | 2026-10-30 | **Fim da Fase 3 (MVP gráfico).** `RinginDlg` (janela de chamada recebida: nome, endereço/User-Agent/Para/Desvio, Atender, Atender com vídeo, Recusar, Desviar, fechar = ignorar; posição aleatória/em cascata/salva/centralizada) no lugar da caixa provisória; `CmainDlg::GetNameForCall` portado; testes de chamada recebida (atender e recusar) pela janela; 68 testes. |
| 0.2.5 | 2026-10-30 | Fase 3, parte 4: `MessagesDlg` (janela de chamadas e mensagens) portado — abas por contato com ícones de estado, Chamar/Atender, Vídeo, Espera, Encerrar, histórico (HTML, links), envio de mensagem SIP, mesclar/separar conferência; `CmainDlg::onCallState/onCallMediaState/onIncomingCall/onCreateRingingDlg/onPlayerPlay/AutoAnswer/OnTimer(Call/Tone/AutoAnswer/Forwarding)` e `MakeCall/MessagesOpen/CurrentCallId` como no original; provisórios do `CmainDlg`/`Dialer` removidos; teste de chamada real pela janela (`MSIP_TEST_SIP=1`); 66 testes. |
| 0.2.4 | 2026-10-30 | Chamadas reais pela janela validadas pelo responsável (saída e entrada, 2 contas, áudio perfeito nos dois sentidos, espera e mudo). Correções: cancelador de eco igual ao MicroSIP (WebRTC AEC, Speex AEC desligado — fim dos avisos `xruns`); barra de status presa após a chamada; item "MySimpleSIP" (mostrar janela) no menu da bandeja; instância única (2ª execução traz a 1ª para a frente). |

---

## Registro de sessões

### 2026-10-30 — Sessão 1: análise
- Extraído `MicroSIP-3.22.16-src.7z` para `reference/MicroSIP-3.22.16/` (231 arquivos, ≈ 46.500 linhas C/C++).
- Constatado: MFC estático + Unicode + pjsua-lib; lógica de telefonia concentrada em `mainDlg.cpp` (6.100 linhas) e `global.cpp` (1.900); camadas `Data/Domain/Service` já limpas.
- Faltam no pacote: árvore do pjproject usada pelo MicroSIP, binários `third_party`, sons WAV, langpacks e a chave `_GLOBAL_KEY` (mascarada) → senhas do `microsip.ini` do Windows não são importáveis.
- Avaliadas 7 alternativas (Wine, Winelib+MFC, wxWidgets, Qt 6, GTK4, Electron, outra pilha SIP). **Escolhido Qt 6 Widgets + pjproject upstream**, portando a lógica quase linha a linha sobre uma camada `compat/` mínima.
- Riscos registrados: Wayland (posicionamento/sempre-no-topo), áudio PipeWire (backend pjmedia próprio na fase 7), vídeo (renderer próprio), bandeja no GNOME, HID (udev), MP3 (libmp3lame), ausência de patches do pjproject do MicroSIP.
- Ambiente verificado: Ubuntu 26.04.1 LTS, GCC 15.2, CMake 4.2.3, GNOME Wayland. `apt-cache policy` não retornou dados na sessão → confirmar nomes/versões de pacotes na etapa 0.0.2.

### 2026-10-30 — Sessão 2: etapa 0.0.2
- `git init -b main`; criados `.gitignore`, `.gitattributes`, `COPYING` (cópia de `/usr/share/common-licenses/GPL-2`), `README.md` com créditos ao MicroSIP.
- Decidido versionar `reference/` sem alterações (byte a byte, CRLF) para permitir diffs com futuras versões do MicroSIP; o arquivo `.7z` fica fora do git.
- Arquivos adicionados ao stage; commit ainda não feito (git sem identidade configurada).

### 2026-10-30 — Sessão 3: instalação de referência (Bottles)
- Responsável pediu para **seguir sem git até a 1.0.0**: o `.git` (vazio, sem commits) foi removido; `.gitignore`/`.gitattributes` mantidos para uso futuro.
- Instalação em `~/.var/app/com.usebottles.bottles/data/bottles/bottles/MicroSIP/drive_c/users/<usuário>/AppData/Local/MicroSIP/`: `MicroSIP.exe` **3.22.12** (o fonte é 3.22.16), 32 bits; DLLs FFmpeg 6.1.4, SDL2 2.32.10, `lame_enc.dll` 3.100; sons WAV; `langpack_portuguesebr.txt`; `License.txt` (GPL v2).
- Strings do .exe: **pjproject 2.15.1**, OpenSSL 1.1.1o, AEC WebRTC (`echo_webrtc.c`), áudio **WMME** (não WASAPI), vídeo DirectShow + render SDL, H.264/H.263 via FFmpeg, VP8/VP9 via libvpx, codecs opus, G.722, G.722.1, G.729, GSM, iLBC, Speex, SILK, AMR-WB (IPP), L16; `mp3_writer.c` (usa `lame_enc.dll`).
- REA (`inspect_pe_resources`): o .exe tem 18 diálogos, 5 menus, 56 grupos de ícones, 29 bitmaps, 13 string tables — coerente com os `.rc2` do fonte; nenhum som embutido. Como o fonte está disponível, descompilação não foi necessária.
- Langpack pt-BR cobre as 308 frases do modelo; só 14 rótulos curtos do fonte (AA, AC, DND, FWD, REC, RFC2833…) não constam — ficam em inglês também no original.
- Configuração real do responsável (sem copiar dados pessoais): 2 contas UDP com registro 300 s, keep-alive 15 s, publish=1, allowRewrite=1; headset USB "PH503"; gravação em MP3; **diretório de usuários remoto (HTTP) em uso**; `Contacts.xml` com 10 contatos; `contacts.db` vazio; `call_log.db` com o mesmo esquema do fonte. Há dois `MicroSIP.ini` (Local, ao lado do .exe, e Roaming).
- Copiados para `reference/MicroSIP-3.22.12-assets/`: 6 WAV, 2 langpacks, `License.txt`, `SHA256SUMS`, `README.md`.

### 2026-10-30 — Sessão 4: etapa 0.0.4
- Criado `scripts/install-deps.sh` (precisa de `sudo`; executado pelo responsável no terminal).
- Todos os pacotes instalados; versões verificadas com `dpkg-query`/`pkg-config` (tabela abaixo).
- Detectado: FFmpeg do Ubuntu é 8.0 → possível incompatibilidade com o pjproject 2.15.1 (tratar na 0.0.5).

### 2026-10-30 — Sessão 5: etapa 0.0.5 (pjproject)
- Tarball oficial `pjproject-2.15.1.tar.gz` (GitHub, SHA-256 `8f3bd99c…5706`, conferido pelo script) em `third_party/downloads/`; fonte extraído em `third_party/pjproject/pjproject-2.15.1/`.
- `third_party/pjproject/config_site.h`: vídeo, SRTP+DTLS, G.722.1, L16, VP9, `PJSIP_MAX_PKT_LEN 8000`, `PJSUA_MAX_BUDDIES 1000`.
- `scripts/build-pjproject.sh` (idempotente; `--clean` refaz do zero): confere hash, extrai, aplica `third_party/pjproject/patches/*.patch`, copia o `config_site.h`, `aconfigure` com SRTP/GSM/Speex externos, bcg729, Opus, VPX, OpenH264, FFmpeg, SDL; sem SILK/Lyra/UPnP/AEC3; instala em `third_party/pjproject/install` (12 MB, `libpjproject.pc`).
- **FFmpeg 8 quebrou o build** como previsto (`avcodec_close` e `AVFrame.key_frame` removidos). Criado `patches/0001-ffmpeg-8-compat.patch` (helper `ff_close_free_ctx` + macro `FF_FRAME_IS_KEY`, condicionados a libavcodec ≥ 61); conferido que aplica no fonte original.
- Teste `tests/pj-smoke/pj_smoke.c` (compilar: `gcc tests/pj-smoke/pj_smoke.c $(PKG_CONFIG_PATH=third_party/pjproject/install/lib/pkgconfig pkg-config --cflags --libs --static libpjproject)`): pjsua inicia; transportes UDP/TCP/TLS ok; **19 codecs de áudio** (speex 8/16/32k, iLBC, GSM, PCMU, PCMA, G722, G7221 16/32k, AMR, AMR-WB, opus/48000/2, G729, L16); **4 de vídeo** (H264/97 via OpenH264, H263-1998/96 via FFmpeg, VP8/102, VP9/105); vídeo: câmera integrada (V4L2), SDL renderer, colorbar.
- Achados:
  - Payload types de vídeo do upstream diferem dos do MicroSIP (que usa `H264/99`, `H263-1998/98`, `VP8/100`, `VP9/101` fixos em `PJVideoCodecs()`). Decisão: no port, localizar o codec pelo nome (prefixo `H264/` etc.) em vez de patchear o enum.
  - O ALSA lista **32 dispositivos** (plugins `lavrate`, `jack`, `oss`…, placas `hw:`/`hdmi:`/`dmix:`), com mensagens de erro do plugin JACK no stderr; só `pipewire`, `pulse` e `default` são úteis. Confirma a necessidade do backend PulseAudio/PipeWire (fase 7); até lá, filtrar a lista e usar `pipewire`/`default`.
- `.gitignore` atualizado para não versionar downloads, fonte e instalação do pjproject.

### 2026-10-30 — Sessão 6: etapa 0.0.6 (esqueleto CMake + Qt)
- `CMakeLists.txt` raiz: versão lida do arquivo `VERSION`, `MSIP_BASE_VERSION=3.22.16`, C++20, Qt 6 (Core, Gui, Widgets, Network, DBus), `qt_standard_project_setup()`.
- `cmake/FindPJPROJECT.cmake`: usa o `libpjproject.pc` de `third_party/pjproject/install` (variável `PJPROJECT_PREFIX`) e cria `PJPROJECT::pjproject`; flags estáticas em `INTERFACE_LINK_LIBRARIES` para ficarem depois dos objetos na ligação.
- `src/CMakeLists.txt` + `src/version.h.in` (gerado em `build/src/version.h`); `src/main.cpp` com `QCommandLineParser` (`--version`, `--quit-after <ms>`), janela com as versões e verificação `pjsua_create/init/destroy`.
- `build.sh` (compila o pjproject se faltar, configura com Ninja em `build/`, compila). Build sem avisos; executável de 3,6 MB (pjproject estático).
- Validação: `./build/src/mysimplesip --version` → `MySimpleSIP 0.0.6 (base MicroSIP 3.22.16) / pjproject 2.15.1 / Qt 6.10.2 / pjsua: ok`; janela abre e fecha com `QT_QPA_PLATFORM=offscreen --quit-after 1500`.
- Observado: o `pjsua_init` gera no stderr mensagens do plugin ALSA do JACK (enumeração de dispositivos) — mesmo problema da sessão 5, a tratar com o filtro/backend de áudio.

### 2026-10-30 — Sessão 7: etapa 0.0.7 (recursos e layout)
- `scripts/convert-resources.py` (Pillow): cada `.ico` vira `resources/icons/<nome>/<tamanho>.png` (16/20/24/28; app 16–256), cada `.bmp` vira `resources/bitmaps/<nome>.png`; gera `resources/resources.qrc` e `resources/RESOURCES.md` (mapa `IDI_*`/`IDB_*` → arquivo, inclusive os definidos por macro como `IDI_MAINFRAME`). Único ícone sem uso no `.rc`: `button_speaker`.
- Prancha visual em `docs/layout/icones.png` (transparência preservada).
- Sons (6 WAV) e `langpack_portuguesebr.txt` copiados para `resources/sounds` e `resources/langpacks` (instalados em `share/mysimplesip`; ficam fora do `.qrc` porque o pjsua lê WAV do disco).
- `scripts/extract-dialogs.py`: expande `res/dialog.rc2` com o `cpp` (só `define.h`, para os IDs manterem os nomes) e gera `docs/layout/dialogs.json` + `docs/layout/DIALOGOS.md`: **16 diálogos, 412 controles**, todos com geometria resolvida, em DLU e px (base 6×13 → ×1,5 / ×1,625), estilo, visibilidade e widget Qt sugerido. Conferência: `IDD_MAIN` 162 DLU = 243 px + bordas = 251 px, igual ao `mainW` do INI real.
- Build: `resources.qrc` embutido (`CMAKE_AUTORCC ON` — o `qt_standard_project_setup` não liga o RCC); janela usa o ícone do app; `--version` mostra "ícone: 8 tamanhos". Executável: 4,1 MB.
- Observação para a Fase 3: vários diálogos são reposicionados/redimensionados em código (`MoveWindow`, DPI) — o layout do `.rc` é o ponto de partida, não o resultado final.

### 2026-10-30 — Sessão 8: Fase 1, camada de compatibilidade (0.0.8)
- Levantamento no fonte do MicroSIP dos métodos de `CString` realmente usados (contagem): `IsEmpty` 279, `GetBuffer` 162, `ReleaseBuffer` 160, `Format` 128, `GetAt` 102, `Find` 95, `Mid` 77, `Append` 69, `GetLength` 63, `AppendFormat` 60, `Trim` 57, `CompareNoCase` 46, `Replace` 44, `Tokenize` 19, além de `Left/Right/MakeLower/SetAt/Compare/AppendChar/Remove/ReverseFind/Delete/Insert/FindOneOf`. Coleções: `CList` (com `POSITION`), `CArray`, `CStringArray`, `CStringList`, `CByteArray`; `CMapStringToString` só na UI (`ButtonDialer`) — não implementado.
- `src/compat/` (biblioteca `msip_compat`, sem Qt): `WinTypes.h` (`TCHAR`, `LPCTSTR`, `_T`, `BOOL`, `DWORD`, `_wtoi`, `_wcsicmp`, `lstrcmp`…), `CString.h/.cpp` (template `CStringT<Ch>`), `CTime.h/.cpp`, `Collections.h`, `afx.h` (cabeçalho único, substitui o `stdafx.h`).
- Decisões de implementação:
  - `CString` = `CStringT<wchar_t>` sobre `std::wstring` (wchar_t de 32 bits no Linux; índices iguais aos do Windows para todo texto fora de pares substitutos — irrelevante na prática). `_T("x")` continua sendo `L"x"`.
  - `CStringA` ↔ `CString` convertem em **UTF-8** (o MFC usa a code page ANSI); é o que o pjsip espera.
  - `Format/AppendFormat` são templates variádicos: aceitam `CString` direto nos argumentos (como o código do MFC faz) e traduzem o formato MFC→glibc (`%s` largo / `%S` estreito no `CString`, `%I64d` → `%lld`).
  - Semântica do MFC reproduzida e testada: `Tokenize` (delimitadores iniciais, `-1` ao fim), limites de `Mid/Left/Right`, `Find` com início fora do texto, contagens de `Replace/Remove`, `Insert/Delete` com índices ajustados, `GetBuffer/ReleaseBuffer(-1)` (corta no primeiro NUL), `CompareNoCase` com acentos (LC_CTYPE UTF-8).
  - `CList` é lista duplamente ligada própria (`POSITION` = nó), permitindo `RemoveAt` durante a iteração como o MicroSIP faz.
- Testes: `tests/` com GoogleTest (`include(CTest)`, `gtest_discover_tests`); `tests/test_main.cpp` fixa `LC_ALL=C.UTF-8` e `TZ=America/Sao_Paulo`. 21 testes (CString, CStringA, CTime, CArray, CList), casos tirados do uso real (lista de codecs, STUN, `GetPrivateProfileString`, `FormatGmt`). Passam no build normal, via `ctest` e com `-fsanitize=address,undefined`.

### 2026-10-30 — Sessão 9: Fase 1, INI e caminhos (0.0.9)
- Uso no MicroSIP: `GetPrivateProfileString` 143×, `WritePrivateProfileString` 164×, `WritePrivateProfileSection` 3× (sempre com `NULL` = apagar seção); `GetPrivateProfileString(section, NULL, …, 3, …)` usado para saber se uma seção tem chaves. O MicroSIP grava o INI em UTF-16 LE com BOM e converte arquivos ANSI para esse formato.
- `src/compat/IniFile.*`: modelo de linhas que preserva comentários, linhas em branco e ordem; seções/chaves sem diferenciar caixa, primeira ocorrência vence; leitura apara chave/valor e remove aspas externas pareadas; chave nova no fim da seção (antes das linhas em branco), seção nova no fim do arquivo; lê UTF-8 (com/sem BOM) e UTF-16 LE/BE; grava **UTF-8 sem BOM, `\n`**, de forma atômica (temporário + `rename`).
- `src/compat/PrivateProfile.*`: funções com assinatura e retornos do Win32 (truncamento `size-1`, listas `a\0b\0\0` com `size-2`, default sem espaços finais, `value NULL` apaga chave, `key NULL` apaga seção), cache por arquivo com recarga se `mtime`/tamanho mudarem, gravação imediata, `mutex`.
- `src/settings/Paths.*` (`ResolveAppPaths`): instalado → INI em `$XDG_CONFIG_HOME/MySimpleSIP/MySimpleSIP.ini`, `pathRoaming` (bancos, contatos, gravações) em `$XDG_DATA_HOME/MySimpleSIP/`, `pathLocal` (log `MySimpleSIP_log.txt`) em `$XDG_STATE_HOME/MySimpleSIP/`; **portátil** se houver `MySimpleSIP.ini` ao lado do executável. INI ausente/vazio → `firstRun` e criação com `[Global]` (como o original).
- `tests/data/microsip_win.ini`: cópia do `MicroSIP.ini` (Roaming) do Bottles com **todos os dados pessoais trocados por fictícios** (contas, senha, servidor, token do diretório, números discados, instância, caminho); conferido que nenhum valor original restou. Mantém UTF-16 LE + BOM + CRLF.
- Testes novos (11): regras de parse, gravação preservando ordem, decodificação UTF-16/UTF-8/BOM, truncamentos e listas do Win32, apagar chave/seção, recarga externa, importação do INI do Windows (todas as chaves preservadas após gravar em UTF-8) e caminhos XDG/portátil/primeira execução. Total: 32, também sob ASan/UBSan.

### 2026-10-30 — Sessão 10: Fase 1, port do settings.cpp (0.0.10)
- **Método:** o `settings.cpp` original (1.421 linhas) foi copiado (LF) para `src/settings/settings.cpp` e só os trechos dependentes do Windows foram trocados por script; cada troca está marcada com `// MySimpleSIP:`. `SettingsLoad/SettingsSave/AccountLoad/AccountSave/AccountDelete/GlobalSave/ShortcutEncode/ShortcutDecode/ShortcutsLoad/ShortcutsSave` permanecem iguais ao original, com as mesmas chaves, padrões e limites.
- Trocas: (1) includes/globais (`Statistics` de telemetria não portado; `msip_command_line` substitui `AfxGetApp()->m_lpCmdLine`); (2) caminhos de `Init()` (registro do instalador, `SHGetSpecialFolderPath`, criação do INI UTF-16) → `ResolveAppPaths`; leitura de `RunAtSystemStartup` do registro removida (autostart na Fase 7); (3) `IniDecrypt/IniEncrypt` → `PasswordLoad/PasswordSave`; (4) pasta padrão de gravações: `CSIDL_DESKTOPDIRECTORY` → `XdgUserDir("DESKTOP")` + `/Recordings`; (5) `MSIP::IniSectionExists` trazido como função local; (6) `AccountDelete` também remove a senha do chaveiro.
- `src/common/define.h` (copiado do original; ícones apontam para o `.qrc`; URLs do microsip.org **desativadas** — vazias — até a decisão sobre site/ajuda/atualização) e `src/common/const.h` (identidade MySimpleSIP, versão do `VERSION`). Biblioteca de interface `msip_common`.
- `src/settings/Account.h`: `struct Account` e `MSIP_SHORTCUT_*` extraídos do `global.h` (sem pjsua); `src/domain/Shortcut.h` copiado do original.
- `src/settings/PasswordStore.*`: interface `IPasswordStore` (o chaveiro via QtKeychain será implementado quando a UI precisar de login — fim da Fase 2/Fase 3); sem chaveiro, o INI guarda `msip1:` + Base64 (só ofuscação, como a cifra de chave fixa do original); valor em outro formato é tratado como texto puro e migrado (comportamento do MicroSIP quando `IniDecrypt` falha). Senhas cifradas vindas do Windows não são decifráveis → redigitar.
- `WinTypes.h`: `_T` agora em dois níveis (como `tchar.h`) para expandir `_T(_GLOBAL_CODECS_ENABLED)`; `RGB`/`COLORREF` adicionados.
- Testes novos (9): padrões da primeira execução, salvar/recarregar, importação do `MicroSIP.ini` do Windows (conta, transporte, registro, gravação, códigos de função, diretório), limites `registerRefresh/keepAlive/hideCID`, senha no chaveiro, senha ofuscada sem chaveiro, migração de texto puro, Base64, atalhos (formatos novo e antigo) e `ShortcutsLoad/Save`. Total 41, limpos sob ASan/UBSan.

### 2026-10-30 — Sessão 11: Fase 1, idiomas e logging (0.1.0)
- Uso no MicroSIP: `Translate()` 213×, `TranslateDialog()` 16×, `TranslateMenu()` 6×, `langPack.rtl` em 9 diálogos.
- `src/i18n/langpack.*` (`msip_i18n`), port de `lib/langpack.*`: mesmo formato e mesmas regras (1ª linha "Language Pack", cabeçalhos `Nome: valor`, `;` comentário também no corpo, escapes `\n \t \r \x`, traduções em várias linhas unidas por `\n`, entrada sem tradução ignorada, primeira ocorrência vence, códigos de erro 1/2/3). `Translate()` devolve o próprio ponteiro quando não há tradução.
  - Diferenças intencionais: busca pelo texto exato (o original compara só o hash MurmurHash2, sujeito a colisão); arquivo sem BOM e não-UTF-8 lido como Windows-1252 (o original usava a code page do campo `Locale`); recarregar substitui o langpack anterior.
  - `LoadLangPackModule`: pasta do executável → `$XDG_DATA_HOME/MySimpleSIP/langpacks` → `<prefixo>/share/mysimplesip/langpacks` → `$XDG_DATA_DIRS/mysimplesip/langpacks`; um arquivo na pasta = usa; vários = escolhe pelo idioma do sistema (`LC_ALL/LC_MESSAGES/LANG`, tabela `pt_BR → portuguesebr`, `pt → portuguese`, …; lista parcial a confirmar com os nomes reais dos langpacks); nenhum compatível = inglês.
  - `TranslateDialog/TranslateMenu` serão reescritos para widgets Qt na Fase 3 (percorrer `QLabel`, botões, `QGroupBox`, título da janela e `QMenu`, preservando o texto após `\t`).
- **Logging:** o MicroSIP não tem um logger próprio — tudo passa por `PJ_LOG` e pelo log de arquivo do pjsua (`enableLog`, `logFile`, rotação para `_1` e `PJ_LOG_HAS_CR`), configurado em `PJCreateRaw`. Decisão: manter assim; o port acontece na Fase 2 junto com `PJCreateRaw` (sem `PJ_LOG_HAS_CR` no Linux). Antes do `pjsua_create`, mensagens vão para o stderr.
- Testes novos (6): langpack pt-BR real (`Call`→`Chamar` etc.), sem langpack, regras do formato (BOM, CRLF, repetidas, escapes, multilinha, comentários), códigos de erro e Windows-1252, mapeamento de locale e escolha de arquivo por pasta/idioma. Total 47, limpos sob ASan/UBSan.
- Pendência para a Fase 3: em execução a partir de `build/`, nenhum langpack é encontrado (só existem em `resources/langpacks`); resolver copiando sons/langpacks para uma pasta de dados no build ou adicionando um caminho de desenvolvimento.

### 2026-10-30 — Sessão 12: Fase 2, global.cpp e MSIP.cpp (0.1.1)
- **Compatibilidade nova:** `compat/Win32Window.*` — `HWND/WPARAM/LPARAM/TIMERPROC`, `PostMessage/SendMessage/SetTimer/KillTimer` encaminhados a um `IMessageBackend` instalado pela UI (sem backend: não fazem nada), `CWnd` mínimo (`m_hWnd`, `PostMessage`, `SetTimer`), `AfxGetApp()->m_pMainWnd`, `Sleep`, `GetTickCount`, `CRect/CPoint/CSize`; `compat/Sync.h` — `CCriticalSection` (reentrante, `recursive_mutex`) e `CSingleLock`. Assim os trechos `AfxGetApp()->m_pMainWnd->PostMessage(UM_…)` e os timers de DTMF compilam sem alteração.
- **`src/core/MSIP.*`** (de `lib/MSIP.*`): mantidas as funções de URI SIP, strings, volumes, durações, números, MD5, port knock (Winsock → POSIX). Reescritas para Linux (marcadas): `GetScreenRect`/`ShowErrorMessage` → ganchos da UI; `IsIP` → `inet_pton`; conversões UTF-8 (sem `MultiByteToWideChar`/`pj_unicode_to_ansi`); `OpenURL/OpenFile` → `xdg-open`; `CommandLineToShell`/`RunCmd` → `wordexp` + `posix_spawnp` (espera até 10 s como o `ShellExecuteEx`); `IsConnectedToNetworkCOM` → `getifaddrs` + rota padrão; `IsNetworkChanged` → endereço de origem de um socket UDP conectado; `ExpandEnvironmentStrings` → `$VAR` via `wordexp`; `GetSID` → UID; `IsCameraAccessAllowed` → sempre 1. Corrigido `%.*s` com `slen` (long no Linux).
- **`src/core/global.*`**: `struct Account`/`MSIP_SHORTCUT_*` vêm de `settings/Account.h`. Trocas: `CAtlRegExp` → `DialPlanRegex` (o `atlrx.h` é código da Microsoft "All rights reserved" e não pode ser usado); `URLGetAsync/URLGetSync` (WinINet) → `http_qt.cpp` (Qt Network, `QThread`, Basic/Digest, ignora erros de certificado como o original, 10 s); `mainDlg->messagesDlg->OnEndCall` → `msip_host.onEndCall`; `LoadImageIcon` → fica na UI; gravação: separador `/`, `mkdir`, **sempre WAV por enquanto** (MP3 via libmp3lame na Fase 5); `msip_startup_set` → `~/.config/autostart/mysimplesip.desktop`; `time_t` com `%lld`; construções que só o MSVC aceita (`&pj_str(...)`, temporário em `CString&`, `S_un.S_un_b`, `CA2T`) ajustadas.
- **`src/core/DialPlanRegex.*`**: tradutor do dialeto ATL (`{}` captura, `()` sem captura, `\d \a \b \c \h \w \z \n \q`, escapes) para ECMAScript; `!` (negação ATL) não suportado → item do plano ignorado, como erro de parse no original. Algoritmo de substituição `<busca:troca>` de `FormatNumber` reproduzido com as posições dos grupos.
- **`src/core/CoreHost.h`**: ganchos `showWarning`, `getScreenRect`, `onEndCall` (preenchidos pela UI).
- Avisos: o código original gera avisos de estilo do MSVC (`NULL` como inteiro, parâmetros sem uso, `-Wreorder` no `global.h`); silenciados só para `global.cpp`/`MSIP.cpp`/`msip_core`, para não alterar o original. Build sem avisos.
- Testes novos (13): `test_core` (tradução ATL, grupos, `SanitizeNumber`, `IsPSTNNnmber`, `GetSIPURI` com/sem transporte e domínio, `AddTransportSuffix`, `FormatNumber` com comandos DTMF, prefixo e planos `(<0:>xxxxxxxxxx|xxxx)` e `<00:+>x.|9x.`, `ParseSIPURI/BuildSIPURI`, conversões, `urlencode/urldecode`, MD5, `CommandLineToShell`, `ExpandEnvironmentStrings`, `msip_url_mask`, `GetAccountLabel`) e `test_win32window` (backend de mensagens/timers, `CCriticalSection` entre threads). `tests/TestPrint.h` imprime `CString` legível nas falhas. Total 60 testes; todos limpos na build `build-asan/` (`-fsanitize=address,undefined`).
- Não testado ainda: funções que exigem pjsua em execução (tonegen, DTMF, hold, conferência, gravação) e o HTTP real — ficam para a CLI de teste com Asterisk.

### 2026-10-30 — Sessão 13: Fase 2, pjsua do mainDlg e CLI (0.1.2)
- **Divisão do `CmainDlg`:** a parte pjsua virou `src/core/mainDlgCore.{h,cpp}` — classe `CmainDlgCore : public CWnd`, com os mesmos nomes de membros (`autoAnswerTimerCallId`, `forwardingTimerCallId`, `callIdIncomingIgnore`, `player_eof_data`, `toneCalls`, `audioCodecList`, `m_PresenceStatus`…) e métodos copiados do original: `PJCreate`, `PJCreateRaw`, `PJAudioCodecs`, `PJVideoCodecs`, `UpdateSoundDevicesIds`, `PJDestroy`, `PJAccountConfig`, `PJAccountAdd/AddRaw/AddLocal/Delete/DeleteLocal`, `PublishStatus`, `PlayerPlay/PlayerStop`, `VideoCaptureDeviceId`, `onCallAnswer`, `onCallHangup`. As chamadas à interface viraram **métodos virtuais vazios** (`UpdateWindowText`, `SetPaneText2`, `SetWindowText`, `DialerSetName`, `DialerHwnd`, `BaloonPopup`, `OnAccount`, `InitUI`, `CheckUpdates`, `UsersDirectoryLoad`, `Subscribe/Unsubscribe`, `onPlayerStop`, `AskRetryNetwork`, `createPreviewWin`, `IsForegroundApp`, `RestoreWindow`, `GetNameForCall`, `PlatformMonitorsStart/Stop`), que a janela Qt sobrescreverá. A janela da Fase 3 derivará de `CmainDlgCore` e portará os handlers `UM_ON_*` (`onCallState`, `onIncomingCall`…), que ficaram fora por serem quase só interface.
- **`src/core/callbacks.cpp`:** todos os callbacks `on_*` do pjsua copiados na ordem original (`on_reg_started2`, `on_reg_state2`, `call_timeout_callback`, `on_call_state`, `on_call_media_state`, `on_call_media_event`, `on_incoming_call`, `on_nat_detect`, `on_buddy_state`, `on_pager2`, `on_pager_status2`, `on_call_transfer_status`, `on_call_transfer_request2`, `on_call_replace_request2`, `on_call_replaced`, `on_mwi_info`, `on_dtmf_digit`, `on_call_tsx_state`, `on_call_redirected`) + `msip_set_pjsua_callbacks`. Trocas: `answer-after={[0-9]+}` com `DialPlanRegexMatch`; `pageDialer->m_hWnd` → `DialerHwnd()`; callbacks de rede do Windows removidos (Fase 7 enviará `UM_NETWORK_CHANGE`).
- Outras trocas em `mainDlgCore.cpp`: TaskDialog de rede → `AskRetryNetwork`; checagem de privacidade da câmera removida; rotação do log com `std::filesystem` (sem `PJ_LOG_HAS_CR`); WTS/MMNotification/NotifyRoute/Hid → `PlatformMonitorsStart/Stop`; codecs de vídeo por nome (`msip_vid_codec_id`); opção `nullSoundDevice` (`pjsua_set_null_snd_dev`) para testes.
- Portados também: `lib/CMask.*` (curingas do número de auto-atendimento), `resource.h` (IDs), `Domain/Statistics.h` (telemetria usada pelos callbacks; jsoncpp do sistema). Compat: `IsWindow`, `CString::SetString`, `NIIF_*`.
- **`src/app/QtMessageBackend.*`** (`msip_appqt`, Qt Core): implementa `IMessageBackend` — `PostMessage` de qualquer thread entregue na thread principal do Qt, `SendMessage` bloqueante, `SetTimer/KillTimer` com `QTimer` (criados na thread principal mesmo se pedidos pelas threads do pjsua); destinos registram-se como `IMessageTarget` e recebem um `HWND`. Será reutilizado pela janela.
- **`src/cli/msip_cli.cpp`** (`build/src/cli/msip-cli`): usa o INI e o núcleo reais; opções `--profile DIR` (perfil XDG isolado), `--null-audio`, `--auto-answer`, `--script ARQ`, `--log`; comandos `call`, `answer`, `hangup`, `hold`, `unhold`, `dtmf`, `status`, `wait`, `quit`; handlers simplificados das mensagens `UM_ON_*`.
- **Achado:** `msip_call_answer` apenas posta `UM_CALL_ANSWER`; quem atende é `CmainDlg::onCallAnswer` — por isso ele foi trazido para o núcleo (a primeira tentativa ficava em CALLING até 487).
- **Teste de ponta a ponta `tests/sip-lab/loopback.sh`:** duas instâncias (perfis temporários, sem áudio); B liga para `127.0.0.1` pela conta local; resultado: CALLING → CONNECTING → CONFIRMED, mídia ativa, **DTMF RFC 2833** 1-2-3 recebido, **espera** local/remota e retomada, desligamento `200 Normal call clearing`, encerramento limpo. 11/11 verificações; também limpo com as duas instâncias compiladas com ASan/UBSan. 60 testes unitários passando.
- Ainda não testado: **registro** em servidor (sem servidor disponível nesta sessão), TCP/TLS/SRTP, áudio real (PipeWire), transferência, conferência, gravação, MESSAGE.
- **Registro em servidor real (mesma sessão):** perfil de teste `~/msip-perfil-teste` criado com cópia do `MicroSIP.ini` ativo do responsável (só `Account1`, UDP, senha em branco — a cifrada do Windows não é decifrável; o responsável digitou a senha). `msip-cli --profile ~/msip-perfil-teste` → `[registro] código 200` e `status` → `registro: 200 OK` no PBX de produção do responsável. Confirma `PJAccountConfig/PJAccountAddRaw`, credenciais Digest, keep-alive e a importação do INI do Windows. Em seguida, chamada de saída pelo PBX com **áudio nos dois sentidos** confirmada pelo responsável (dispositivo padrão do ALSA → PipeWire; os nomes de dispositivo do Windows no INI não existem e caem no padrão). **Critério da Fase 2 cumprido → 0.2.0.**

### 2026-10-30 — Sessão 14: Fase 3, chaveiro e janela principal (0.2.1)
- **`src/app/KeychainPasswordStore.*`**: `IPasswordStore` sobre QtKeychain (Secret Service/GNOME Keyring, serviço "MySimpleSIP", chave `<seção>@<INI>`), operações síncronas; sem chaveiro → fallback ofuscado no INI. Teste `tests/app/test_keychain.cpp` (só com `MSIP_TEST_KEYRING=1`) aprovado contra o chaveiro real (gravar/ler/apagar com acentos e emoji). Instalado no `main.cpp` antes de `accountSettings.Init()`.
- **`scripts/convert-resources.py`** gera também `resources/icon_table.inc` (`{ IDI_..., "pasta" }`), usado por `LoadImageIcon(id)` (`src/ui/UiCommon.*`, com `QS/CS`, `TR` e `DluX/DluY`).
- **`src/ui/mainDlg.*`** (`msip_ui`, Qt Widgets) — `class CmainDlg : public QWidget, public CmainDlgCore, public IMessageTarget`. Portados: construtor (lista de nomes de codecs), `OnInitDialog` (abas de largura fixa `_GLOBAL_TAB_WIDTH`, botão de menu `IDI_DROPDOWN`, barra de status com ícone/textos, regra de posição/tamanho com `mainX/mainY/mainW/mainH` — 1ª execução no canto inferior direito da área de trabalho —, `alwaysOnTop`, `noResize`, aba ativa), `InitUI`, `ShowTrayIcon` (QSystemTrayIcon: ícones ativo/inativo/perdida, clique alterna a janela, menu da bandeja = menu principal), `OnCreated` (`PJCreate` + `PJAccountAdd`), `TrayIconUpdateTip`, `BaloonPopup` (notificação), `UpdateWindowText` (integral: Online/Idle/Connecting/Incorrect Password/DND/Away/Forwarding/Auto Answer, ícone da bandeja), `onRegState2`, `OnUpdateWindowText`, `MainPopupMenu` (contas com troca de conta ativa, Always on Top, Export/Import, log, Exit), `OnMenuAccountChange`, `OnMenuAlwaysOnTop`, `onTrayNotify`, `OnClose`, fechar = esconder na bandeja (`SC_CLOSE`), `OnMove/OnSize` + `AccountSettingsPendingSave` (`IDT_TIMER_SAVE`), `AskRetryNetwork` (QMessageBox).
- Ainda **marcadores** (próximas etapas): páginas Telefone/Registros/Contatos, `AccountDlg`, `SettingsDlg`, `ShortcutsDlg`, import/export, e os handlers de chamada `onIncomingCall/onCallState/...` (por ora só liberam a memória). `messagesDlg->GetCallsCount()` substituído temporariamente por `pjsua_call_get_count()`.
- Log do pjsip: sem `enableLog`, nenhum log (no Windows ia para um console inexistente); com `enableLog`, só no arquivo. `pj_log_set_level(0)` antes de `pjsua_create`.
- Execução a partir de `build/`: o CMake copia `resources/langpacks` e `resources/sounds` para `build/src/` (resolve a pendência da sessão 11); o langpack pt-BR é carregado (abas "Telefone/Registros/Contatos").
- `main.cpp`: `QApplication` sem encerrar ao fechar a última janela; opções `--minimized` (`/minimized`), `--quit-after`, `--screenshot ARQ` (captura da janela para testes/documentação). Captura em `docs/layout/janela-principal-0.2.1.png`.
- Verificado: build sem avisos; 61 testes (1 opcional ignorado); loopback SIP 11/11; janela abre/fecha limpa em modo offscreen com perfil novo.

### 2026-10-30 — Sessão 15: Fase 3, construtor de diálogos e AccountDlg (0.2.2)
- **`src/ui/DialogLayout.*` (`CDialogLayout`)**: lê `:/layout/dialogs.json` (o `docs/layout/dialogs.json`, embutido por `resources/layout.qrc`) e cria os controles do `IDD_*` pedido nas posições originais (DLU→px), com textos, `WS_DISABLED`, ocultos (`NOT WS_VISIBLE`/`~WS_VISIBLE`), `ES_PASSWORD/READONLY/NUMBER/MULTILINE`, `CBS_DROPDOWN` (editável) × `CBS_DROPDOWNLIST`, `BS_PUSHLIKE`, `DEFPUSHBUTTON`, SysLink (QLabel com link), trackbar (QSlider), ListView (QTreeWidget), RichEdit (QTextBrowser). Cada controle recebe `objectName` = ID do `.rc`; acesso por `m_layout.Edit("IDC_...")`/`DLG_ITEM(m_layout, IDC_...)` — o código portado fica quase linha a linha com o `GetDlgItem` original. `TranslateDialog` equivalente (título, rótulos, botões, grupos). `SS_WORDELLIPSIS` → texto cortado com "…" e tooltip com o texto completo (rótulos em português são mais longos).
- Fonte dos diálogos: família do sistema em **11 px** (equivale à MS Sans Serif 8 pt a 96 DPI) — `DialogFont()` em `UiCommon`.
- **`src/ui/AccountDlg.*`**: port de `AccountDlg.cpp` — `OnInitDialog` (listas Ocultar ID/Criptografia/Transporte/Endereço público com IPs locais, desativado com STUN), `Load` (nova/editar/conta local, regras de visibilidade e habilitação), `OnBnClickedOk` (validação de campos obrigatórios, novo ID, `AccountSave`, troca da conta ativa com `PJAccountDelete/PJAccountAdd`, conta local), excluir conta (renumera as seguintes), "exibir senha", `OnChangePassword`. Links "?" de ajuda ocultos enquanto `_GLOBAL_HELP_WEBSITE` estiver vazio. Arrastar o diálogo por qualquer ponto (`OnNcHitTest`→`HTCAPTION`) não foi reproduzido.
- `CmainDlg`: `accountDlg`, `OnAccount` (com foco na senha após 401/403), `OnMenuAccountAdd/Edit/LocalEdit` portados e ligados ao menu. Diálogos são janelas independentes (sem pai) para não sumirem com a principal escondida.
- Captura do diálogo: `MSIP_SCREENSHOT_DIALOG=account ... --screenshot ARQ` → `docs/layout/dialogo-conta-0.2.2.png`.
- **Teste `tests/ui/test_accountdlg.cpp` (`test_ui`, offscreen)**: abre "Adicionar conta", preenche pelos IDs, salva e confere o INI (rótulo, usuário, nome com acento, TLS, SRTP obrigatório, ocultar ID "hard", publish, `registerRefresh` 0→300, `accountId=1`, senha aparada e ofuscada sem chaveiro) e reabre para edição. 62 testes passando.
- **ASan/LeakSanitizer**: vazamentos pequenos e fixos na inicialização herdados do original (`MSIP::StrToPjStr` sem `free` em `PJAudioCodecs`/`PJAccountAddRaw`) e internos do OpenSSL/SDL. Mantidos por fidelidade (correção opcional futura); rodar os testes ASan com `ASAN_OPTIONS=detect_leaks=0` — assim, 62/62 e nenhum erro de memória.

### 2026-10-30 — Sessão 16: Fase 3, análise do Dialer (interrompida, sem código)
- Sessão interrompida por limite de uso **antes de escrever código**; nada foi alterado no código (versão continua 0.2.2).
- Lidos por completo: `Dialer.h/.cpp` (1.734 linhas), `CmainDlg::DialNumber/MakeCall/Redial/MessagesOpen/AutoAnswer/CurrentCallId`, `MessagesDlg::PerformCall/Call/GetCallsCount` e a lista de funções do `MessagesDlg.cpp` (2.253 linhas).
- **Achado de dependência:** o `Dialer` depende do `MessagesDlg` para quase tudo que envolve chamada (`GetCallsCount`, `GetMessageContact`, `Call`/`StartCall`/`PerformCall`, hold, conferência, gravação); `CmainDlg::MakeCall` = `MessagesOpen` (`AddTab`) + `messagesDlg->Call`. Portanto a ordem recomendada para continuar é:
  1. Portar o **Dialer** com `CDialogLayout("IDD_DIALER")`: teclado (fonte 19 px), `IDC_NUMBER` com histórico `[Dialed]` (`DialedLoad/Save/Add`, máx. 10), `UpdateCallButton` (troca Chamar/Vídeo/Mensagem ↔ Espera/Encerrar/Transferir no modo único), `Input/DTMF/SetDTMF` (botão `IDC_DIALER_DTMF`), atalhos de teclado do `PreTranslateMessage` (dígitos, `*`, `#`, `+`, Backspace, Esc), sliders de volume com +/- e mudo (`OnHScroll` → `msip_audio_conf_set_volume`/`msip_audio_input_set_volume`), VU-meter (`IDT_TIMER_VU_METER`, `TimerVuMeter`), botão de correio de voz (bitmaps `voicemail*`), botões inferiores `RebuildButtons` (DND/FWD/AA/AC/CONF/REC como `BS_PUSHLIKE`). Atalhos (`RebuildShortcuts`) podem ficar para a fase 6.
  2. Portar o **MessagesDlg** (pelo menos `AddTab`, `GetMessageContact`, `GetCallsCount`, `PerformCall`, `StartCall`, `Call`, `CallCheck`, `OnEndCall`, `UpdateHoldButton`, `UpdateCallButton`, `OnBnClickedHold/End`) e então `CmainDlg::MakeCall/MessagesOpen/Redial/DialNumber/AutoAnswer/CurrentCallId` exatamente como no original.
  3. Portar os handlers `CmainDlg::onIncomingCall`, `onCallState`, `onCallMediaState` (removendo `FreeUnhandledMessage`) e o **RinginDlg** → fecha o MVP (0.3.0).
- Enquanto o `MessagesDlg` não existir, os pontos do `Dialer` que o usam podem chamar funções provisórias no `CmainDlg` (marcadas `// MySimpleSIP: provisório até o port do MessagesDlg`), a remover no passo 2.

### 2026-10-30 — Sessão 17: Fase 3, Discador (0.2.3)
- **`src/ui/DialerControls.*`**: `CButtonDialer` (tecla owner-draw: dígito + letras em cinza, legendas R/C/</+ centralizadas), `CButtonEx` (botão colorido: Chamar `RGB(76,217,100)`, Encerrar `RGB(255,59,48)`), `CLevelsSliderCtrl` (slider de volume com canal mostrando o nível — vermelho no máximo — e cursor cinza).
- **`CDialogLayout`**: `customFactory` (controles próprios por ID), `Build(..., fixedSize=false)` para páginas, `AutoMove`/`ApplyAutoMove`/`AutoUnmove` (reprodução do `CBaseDialog::AutoMove`: mover/crescer frações do redimensionamento).
- **`src/ui/Dialer.*`**: port de `Dialer.cpp` — `OnInitDialog` (AutoMove de todas as teclas/botões, fontes 19/13 px, cores, ícones, sliders), `RebuildButtons` (DND/FWD/AA/AC/CONF/REC à esquerda do botão de correio de voz), `UpdateVoicemailButton` (bitmaps por DPI), `PreTranslateMessage` → `keyPressEvent`/`eventFilter` (dígitos/`*`/`#`/`/`/`+`/Backspace/`-`/`.`/Esc/Enter; no campo, digita e envia DTMF), `DTMF/DTMFDelayed/SetDTMF` (botão Enviar), `Input`, `DialedLoad/Save/Add` (`[Dialed]`, máx. 10), `SetNumber`, `UpdateCallButton` (modo único: Chamar/Vídeo/Mensagem ↔ Espera/Encerrar/Transferir; Rediscar ↔ Apagar), `Action`, `Clear`, volume (`OnHScroll`, +/−, mudo), `TimerVuMeter` (`IDT_TIMER_VU_METER` enviado pelo núcleo ao `DialerHwnd()`), REC/DND/FWD/AA/AC. O Dialer é `IMessageTarget` com HWND próprio (timers).
- **`CmainDlg`**: `pageDialer` na aba Telefone; `DialerHwnd/DialerSetName`, `SwitchDND`, `GotoTab`, `Redial`, `onRefreshLevels` (`UM_REFRESH_LEVELS`) portados. **Provisórios até o MessagesDlg** (marcados): `MakeCall` (mesmas verificações do original + `PerformCall` copiado do `MessagesDlg`), `CallsCount` (≈`GetCallsCount`), `CurrentCallId`, `HoldCurrentCall`, `MessagesOpen`/`OpenTransferDlg` (aviso "ainda não portado"), `ProvisionalIncomingCall` (toque `ringtone.wav` + caixa Atender/Rejeitar; auto-atendimento respeitado), `ProvisionalCallState` (ringback `ringing.wav` no 180, status "Chamando/Conectado <número>", libera `user_data`). `SoundPath()` procura os sons ao lado do executável e depois em `/usr/share/mysimplesip/sounds`.
- **Bug encontrado pelo teste e corrigido:** `RebuildButtons` destruía os botões inferiores, que continuavam na lista do `AutoMove` (segfault ao salvar conta) → `AutoUnmove` antes de destruir, como no original.
- Teste novo `tests/ui/test_dialer.cpp` (teclas, teclado físico, Chamar/Rediscar/Apagar, limpar, `[Dialed]` com reordenação sem duplicar, mudo). 63 testes; ASan (sem detecção de vazamentos) limpo; loopback SIP 11/11. Captura: `docs/layout/discador-0.2.3.png`.
- Ainda não validado com chamada real pela janela (o fluxo usa o mesmo núcleo testado na CLI).
- Primeira execução pelo responsável (`./build/src/mysimplesip`): o plugin ALSA do PipeWire imprimiu `discarded first playback frame` e `Auto-filling the buffer (your application is buggy and/or got xruns)` ao tocar áudio — sinais de *underrun* entre o backend ALSA do pjmedia e o PipeWire (risco previsto na ANALISE §7.2). Qualidade do áudio a confirmar com o responsável. Correções possíveis: ajustar latência/período do ALSA (`PJMEDIA_SND_DEFAULT_PLAY_LATENCY`/`REC_LATENCY` no `config_site.h`, ou `snd_pcm` com períodos maiores) a curto prazo; backend pjmedia PulseAudio/PipeWire nativo (Fase 7) como solução definitiva.

### 2026-10-30 — Sessão 18: teste real pela janela e correções (0.2.4)
- **Teste do responsável pela interface:** chamada para o celular completou com áudio perfeito; com uma segunda conta SIP configurada pelo `AccountDlg`, fez e **recebeu** chamadas (caixa provisória Atender), áudio perfeito nos dois sentidos; espera, mudo do microfone e do alto-falante funcionando; botões **AA** (auto-atendimento) e **DND** (não perturbe) também validados.
- **Avisos `discarded first playback frame` / `Auto-filling the buffer (your application is buggy and/or got xruns)`:** vinham do **Speex AEC** (`libspeexdsp`, `mdf.c`), não do PipeWire. O `MicroSIP.exe` não contém o Speex AEC (usa `echo_webrtc.c`); no pjproject o Speex AEC é o padrão quando habilitado. `config_site.h`: `PJMEDIA_HAS_SPEEX_AEC 0` → o padrão passa a ser o **WebRTC AEC** (já compilado), como no original. pjproject recompilado; avisos não aparecem mais nos testes.
- **Barra de status presa após a chamada:** `UpdateWindowText` (e `OnUpdateWindowText`) usavam `pjsua_call_get_count()`, que ainda conta a chamada sendo encerrada no momento do evento de desconexão → trocado pelo provisório `CallsCount()` (ignora chamadas DISCONNECTED) e acrescentada uma atualização 500 ms após a desconexão.
- **Janela não voltava da bandeja:** no GNOME/AppIndicator o clique esquerdo abre o menu (não envia "ativar"), e o menu do MicroSIP não tem item de mostrar. Acrescentado no topo do menu da bandeja o item **"MySimpleSIP"** (negrito, ação padrão) que mostra/traz a janela (`onTrayNotify`).
- **Instância única** (no original: `FindWindow` + `WM_COPYDATA`): `QLocalServer` por usuário e por arquivo INI (`mysimplesip-<uid>-<md5 do INI>`); a segunda execução pede à primeira para mostrar a janela e sai (testado: 70 ms). Repasse da linha de comando (`/answer`, `sip:`…) fica para a Fase 7. Perfis temporários dos testes usam INI diferente, então não colidem com o app aberto.
- 63 testes; loopback SIP 11/11.
- **Responsável confirmou que as correções funcionaram** (bandeja, status após a chamada, instância única, AEC).

### 2026-10-30 — Sessão 19: janela de chamadas `MessagesDlg` (0.2.5)
- **`src/ui/MessagesDlg.{h,cpp}`**: port de `MessagesDlg.cpp` sobre o `IDD_MESSAGES` (`CDialogLayout`), mesmos nomes: `AddTab`, `OnChangeTab`, `CloseTab`, `PerformCall` (agora com o cabeçalho `X-Conf-Call-ID`), `StartCall`, `Call/CallCheck`, `OnEndCall`, `UpdateCallButton/UpdateHoldButton/UpdateRecButton`, `UpdateTabIcon/TabIconUpdate`, `AddMessage`, `SendInstantMessage`, `GetMessageContact(InCall)`, `GetCallDuration`, `GetCallsCount`, `CallAction` (desvio 302, transferência cega/assistida por código, convite à conferência), `Merge/Separate/OnMergeAll/OnSeparateAll`, `OnGoToLastTab/OnCloseAllTabs`, `GotoTab`. `CClosableTabCtrl` = `QTabBar` com botão de fechar, clique do meio e duplo clique (`UM_CLOSETAB`); `SetCurSel` sem notificação, como no Win32. Menus `IDR_MENU_TABS` e `IDR_MENU_MESSAGES`. Janela criada no `OnInitDialog` do `CmainDlg` com a mesma regra de posição/tamanho (`messagesX/Y/W/H`).
- **Diferenças marcadas:** histórico de cada contato em **HTML** (no original, RTF do RichEdit), com detecção de links (`SetAutoURLDetect`) abertos por `MSIP::OpenURL`; mensagens de erro do microfone específicas do Windows removidas; erros de socket do Linux reconhecidos em `OnEndCall` (`ECONNREFUSED`/`ETIMEDOUT`); `PerformCall` libera `user_data` quando `pjsua_call_make_call` falha (o original vaza); a mensagem "Call failed: …" passa a aparecer (no original uma variável sombreada a deixava vazia); `lastCall` zerado ao fechar a aba; `closeEvent` aceita o fechamento quando o app está encerrando (no Qt 6 `QApplication::quit()` é cancelado se uma janela recusar — causava travamento intermitente ao sair com a janela de chamadas aberta).
- **`CmainDlg`**: `onCallState` (texto de estado com codec/SRTP/ICE, nome reverso do IP do contato com espera de 500 ms em thread, comandos externos `cmd*`, timer de duração, ringback por `IDT_TIMER_TONE`, `AddTab`/`OnEndCall`/`AddMessage`, `m_ButtonRec`), `onCallMediaState`, `onIncomingCall` (desvio, auto-atendimento, 180 Ringing, toque ou bipe de 2ª chamada), `onPlayerPlay`, `onPlayerStop` (com `AutoAnswer`), `AutoAnswer`, `OnTimerCall` (duração, espera, ícone de qualidade MOS) e `OnTimer` portados; `MakeCall`/`MessagesOpen`/`CurrentCallId` iguais ao original; `onTrayNotify` reabre caixas de toque e a janela de chamadas; gancho `msip_host.onEndCall` ligado ao `MessagesDlg`.
- **Ainda provisório:** `onCreateRingingDlg` mostra uma `QMessageBox` (texto igual ao do `RinginDlg`: nome, endereço, User-Agent, Para, Desvio; Atender/Recusar) guardada em `ringinDlgs`; `pageCalls` é um objeto `Calls` vazio (`src/ui/Calls.h`) até a Fase 4; Transferir/Conferência (menus e diálogo `Transfer`) → Fase 5; `Hid` e impedir suspensão → Fase 7.
- **`Dialer`**: chamadas ao `messagesDlg` restauradas como no original (`GetMessageContact`, `GetCallsCount`, `OnBnClickedHold`, `UpdateRecButton`); removida a habilitação provisória de Espera/Transferir/REC no `UpdateCallButton`.
- `compat`: `GetRValue/GetGValue/GetBValue`, `WM_LBUTTONUP/WM_RBUTTONUP` (+ teste).
- **Testes:** `MessagesDlg.AbasMensagensEBotoes` (offscreen); `MessagesDlg.ChamadaPorIpComCli` (opcional, `MSIP_TEST_SIP=1`): o `msip-cli` atende em 127.0.0.1:5060 e o `CmainDlg` (porta 5070, sem placa de som) liga pela janela — conecta, espera/retoma pelo botão do `MessagesDlg`, encerra e a aba fecha (modo de chamada única). 66 testes no build normal e no ASan/UBSan (chamada real também limpa no ASan); loopback 11/11. Captura: `docs/layout/janela-chamadas-0.2.5.png` (`MSIP_SCREENSHOT_DIALOG=messages`).

### 2026-10-30 — Sessão 20: janela de chamada recebida `RinginDlg` (0.3.0)
- O responsável não conseguiu testar a janela de chamadas (o modo de chamada única só é desligado pelo `SettingsDlg`, ainda não portado) e pediu para seguir para o `RinginDlg`.
- **`src/ui/RinginDlg.{h,cpp}`**: port de `RinginDlg.cpp` sobre o `IDD_RINGIN`: `CallAccept/CallAcceptDefault`, `OnAnswer`, `Close`, `OnClose` (ignorar, ou recusar com `noIgnoreCall`), `OnBnClickedDecline` (486, `hangup`, `callIdIncomingIgnore`), `OnBnClickedAudio/Video`, `OnMove` (`ringinX/Y`), regras de posição (aleatória com `randomAnswerBox`/`multiMonitor`, em cascata +22 px sobre o último, posição salva limitada à tela, centralizada) e de primeiro plano (`bringToFrontOnIncoming`/`alwaysOnTop`). `CmainDlg::ringinDlgs` volta a ser `CArray<RinginDlg*>`; `onCreateRingingDlg`, `onCallState` e `onTrayNotify` iguais ao original.
- **Diferenças marcadas:** nome e endereço em `QLabel` centralizado e selecionável (no original, EDIT somente leitura sem borda); `IDC_RINGIN_NAME_BLIND` (0×0, leitor de tela) não é criado → nome acessível da janela; `DestroyWindow` = esconder + `deleteLater`; `closeEvent` aceita no encerramento do app; timer `IDT_TIMER_INIT_RINGIN` (vazio no original) omitido; Desviar → aviso "não portado" (Fase 5).
- **`CmainDlg::GetNameForCall`** portado (grava `user_data->name`; a busca nos contatos fica para a Fase 4) — sem ele o `RinginDlg` mostrava nome vazio.
- Corrigido texto com UTF-8 duplamente codificado em `mainDlgCore.cpp` (travessão do balão "Auto Answer").
- **Testes:** `RinginDlg.TextosEIgnorar` (offscreen: textos, botões, cascata, fechar = ignorar); `RinginDlg.ChamadaRecebidaAtenderERecusar` (opcional, `MSIP_TEST_SIP=1`: o `msip-cli` liga duas vezes para a janela; a 1ª é atendida pelo botão Atender e encerrada, a 2ª recusada pelo botão Recusar → 486). 68 testes no build normal e no ASan/UBSan (chamadas reais também limpas); loopback 11/11. Captura: `docs/layout/chamada-recebida-0.3.0.png` (`MSIP_SCREENSHOT_DIALOG=ringin`).
- **Responsável testou com o PBX real (ligação do celular) e o `RinginDlg` funcionou perfeitamente**; também rodou os testes de ponta a ponta na máquina dele (3/3). Decidiu seguir o plano: Fase 4.

### 2026-10-30 — Sessão 21: Fase 4, parte 1 — aba Histórico (0.3.1)
- **Levantamento da Fase 4:** histórico em SQLite (`Data/Database`, `CallsRepository`, `Service/CallsService`; tabela `call_log` com `callId` único e limite de 1.000 registros). No MicroSIP 3.22.16 o `ContactsRepository` é um esboço (campos comentados): **os contatos continuam em `Contacts.xml`** (CMarkup) — a corrigir no plano (que previa `contacts.db`). Divisão: 0.3.1 histórico; 0.3.2 contatos + `AddDlg`; depois presença/BLF, importação/exportação e diretório remoto.
- **`msip_data`** (`src/data/`, `src/service/`, `src/domain/`, `src/utils/`): `Call.h`, `Contact.h`, `Database`, `ICallsRepository/CallsRepository`, `IContactsRepository/ContactsRepository`, `CallsService` (validação, filtro, exportação/importação CSV, migração do `[Calls]` do INI), `StringConverter.h` e `CSVFile` copiados do original; trocas marcadas (`stdafx.h`, `#pragma comment(lib)`, `CW2A/CA2W` → UTF-8 da camada compat, `%d` com `int64_t`, `std::sort`). Liga com `SQLiteCpp` do sistema.
- **compat:** `CFile`/`CStdioFileEx` (Open/Close, SetCodePage, ReadString/WriteString, GetPosition; UTF-8 com BOM, `\r\n` como no modo texto do Windows) e `ULONGLONG` (+ teste).
- **`src/ui/Calls.{h,cpp}`** (substitui o objeto vazio provisório): `IDD_CALLS` via `CDialogLayout`; `CListCtrl_SortItemsEx` → `QTreeWidget` com item que ordena hora/duração pelo valor e demais colunas pelo texto (empate pelo nome), padrão hora decrescente; `Add/SetName/SetDuration/SetInfo/DeleteAll/Export`, busca (`Esc` limpa), `Delete` apaga com confirmação, `Enter`/duplo clique = ação padrão (`defaultAction`), menu de contexto com as mesmas regras de habilitação, larguras `callsWidth0..4` salvas, timer de 5 min que refaz as horas na virada do dia. Menu principal: Exportar/Importar registro de chamadas ligados. `CmainDlg::CopyStringToClipboard` (QClipboard); aba Histórico limpa o aviso de chamada perdida.
- **Diferenças marcadas:** banco/serviço pertencem à aba (no original, `static`); itens novos são posicionados pela ordenação atual; "Adicionar contato" mostra "ainda não portado" até a 0.3.2. O ícone `SS_ICON` dos diálogos (`IDI_SEARCH`, `IDI_CONTACT`) passou a ser carregado pelo `CDialogLayout`. `NotYetPorted` movido para `UiCommon`.
- **Testes:** `StdioFileEx.GravaELeUtf8ComBom`; `CallsServiceTest` (adicionar/validar/atualizar/filtrar/apagar, CSV com aspas e vírgulas e importação sem duplicar, migração do INI); `Calls.CargaOrdenacaoBuscaEAtualizacao` (offscreen); os de ponta a ponta agora conferem o histórico (saída "Chamada encerrada" com duração; recebidas atendida e "Recusada"). 73 testes no build normal e no ASan/UBSan; ponta a ponta 3/3 também sob ASan. Captura: `docs/layout/historico-0.3.1.png`.

- O responsável abriu o app com a 0.3.1 (só os avisos conhecidos do portal/JACK) e pediu para seguir para Contatos.

### 2026-10-30 — Sessão 22: Fase 4, parte 2 — aba Contatos e `AddDlg` (0.3.2)
- **Licença do CMarkup:** `lib/Markup.*` (CMarkup Lite 6.5, First Objective Software) diz "Redistributing this source code requires written permission" → **não copiado**. Escrita implementação própria (`src/utils/Markup.{h,cpp}`, sobre o `QXmlStreamReader`) só da interface usada pelo código portado (`SetDoc/GetDoc`, `FindElem/FindChildElem/IntoElem/OutOfElem/ResetPos/ResetChildPos`, `GetAttrib/FindAttrib/GetData/GetChildData`, `AddElem/AddChildElem/AddAttrib/AddChildAttrib`), com a mesma semântica de posições e o mesmo formato de saída (`<contacts>\r\n<contact .../>\r\n</contacts>\r\n`).
- **`ContactsService`** copiado (preparo do contato, filtro, exportação CSV/XML, importação CSV com os cabeçalhos alternativos do Outlook/Google); troca marcada: `CListCtrl*` → `std::vector<Contact*>` na ordem da lista.
- **Defeito do original corrigido** (achado pelo ASan): `CCSVFile::ReadData` com o último campo da linha entre aspas avança além do terminador e lê fora do texto.
- **`src/ui/Contacts.{h,cpp}`** (substitui o marcador da aba): `IDD_CONTACTS` via `CDialogLayout`; `QTreeWidget` ordenado à mão como o `CListCtrl_Sortable` (o cabeçalho inverte a ordem; favoritos sempre no topo, depois texto da coluna, empate pelo nome); ícones da lista de status (`imageListStatus`, +7 para favoritos); `ContactsLoad/ContactsSave` (`Contacts.xml` em `pathRoaming`, UTF-8 com `<?xml version="1.0"?>`), migração do `[Contacts]` do INI; `ContactAdd/ContactUpdate/ContactsAdd/ContactDelete`, `FindContact`, `GetNameByNumber`/`GetNameAltByNumber`, `PresenceReset/PresenceReceived/OnTimerContactsBlink`, `SetCanditates/DeleteCanditates` (diretório); menu de contexto com submenu de números (telefone/celular) e "Captura de chamada" (código de função → Fase 5); `Delete`/`Enter`/`Esc`/duplo clique como no original. Como no 3.22.16, o `contacts.db` é criado vazio (contatos ficam no XML).
- **`src/ui/AddDlg.{h,cpp}`**: `IDD_ADD` não modal; `Load`, `OnBnClickedOk` (confirmação ao substituir número existente), composição do nome a partir de nome/sobrenome e cópia do telefone para o número como no original.
- **`CmainDlg`**: `pageContacts`; `GetNameForCall` consulta a agenda (`disableNameLookup`); Exportar/Importar contatos no menu principal; `SubsribeNumber/UnsubscribeNumber` provisórios vazios (no original só agem com `isSubscribed`, que depende do port da presença). **Histórico:** "Adicionar contato" abre o `AddDlg` com nome e número e fica desabilitado se o número já está na agenda. Removido o marcador `Placeholder` (todas as abas portadas).
- `CDialogLayout`: ícones `SS_ICON` com `SS_REALSIZEIMAGE` em 48 px e `SS_CENTERIMAGE` centralizado (ícone do `IDD_ADD`).
- **Mantido por fidelidade:** em `GetNameByNumber` o nome aproximado por sufixo (`nameAlt`) é sobrescrito a cada contato do laço (só vale se o contato for o último).
- **Testes:** `Markup.NavegacaoEGeracao`; `ContactsServiceTest` (preparo/filtro; importação CSV do Google com campo entre aspas no fim da linha; exportação CSV/XML); `Contacts.CargaFavoritosBuscaEAddDlg` (offscreen: carga do XML, favoritos, clique no cabeçalho, busca, nome por número, novo contato e edição pelo `AddDlg` com gravação no XML, "Adicionar contato" do Histórico, migração do INI). 77 testes no build normal e no ASan/UBSan; ponta a ponta 3/3 sob ASan. Capturas: `docs/layout/contatos-0.3.2.png`, `docs/layout/dialogo-contato-0.3.2.png` (`MSIP_SCREENSHOT_DIALOG=contact`).

### 2026-10-31 — Sessão 23: Fase 4, parte 3 — presença/BLF e diretório de usuários (0.4.0)
- **Presença (`CmainDlg`)**: `SubsribeNumber` (`pjsua_buddy_add` com o número formatado como dado do buddy; buddy repetido só reenvia o estado), `UnsubscribeNumber` (só apaga se nenhum contato ou atalho com presença usar o número), `Subscribe/Unsubscribe` (sobrescrevem os ganchos do `CmainDlgCore`; chamados após o registro 200 em `onRegState2` e no encerramento) e `onBuddyState` (`UM_ON_BUDDY_STATE`: offline/online/ausente/ocupado/ao telefone e "Ring…" = tocando) → `Contacts::PresenceReceived` (ícone, informação, piscar). A parte dos atalhos do discador (`pageDialer->Presence*`) fica para a Fase 6 (atalhos).
- **Diretório de usuários**: `UsersDirectoryLoad` (URL com `%s`/máscaras, `sequence=N`, `URLGetAsync`) e `onUsersDirectoryLoaded` copiados do original: CSV (`Content-Type: text/csv`, arquivo temporário → `QTemporaryFile`), JSON (`items`, `presence`, `refresh`, `silent`), XML `<contacts>`, `YealinkIPPhoneBook` e genérico (`entry`/`DirectoryEntry`); `ContactsAdd(..., directory=true)` remove do diretório quem saiu; recarga pelo `refresh` ou `Cache-Control: max-age` (60 s a 1 dia) e reconexão 10·n³ s; `IDT_TIMER_DIRECTORY` no `OnTimer`. Troca marcada: a notificação só aparece com mensagem (no Windows o balão vazio não aparece; no Qt apareceria). Mantido do original: contatos de diretório em CSV entram sem a marca "diretório".
- **HTTP (`http_qt.cpp`)**: os cabeçalhos da resposta passam a vir como no WinINet (`HTTP_QUERY_RAW_HEADERS_CRLF`): linha de status e nomes canônicos — o Qt 6 entrega os nomes em minúsculas e o código do MicroSIP procura `Content-Type: text/csv` e `\r\ncache-control:`.
- **CLI de teste**: comando `online` (`PublishStatus(true)`), para o teste de presença.
- **Testes:** `UsersDirectory.XmlJsonCsvEPresenca` (servidor HTTP local no teste: carga automática na inicialização, XML com favorito/presença, JSON com `phone` e presença "ocupado" com informação, CSV; contato manual preservado); `Presence.AssinaturaRealComCli` (opcional, `MSIP_TEST_SIP=1`: a janela assina a presença do `msip-cli`, recebe o NOTIFY "online" e volta ao ícone padrão no `Unsubscribe`). 79 testes no build normal e no ASan/UBSan; ponta a ponta 4/4 (também sob ASan); loopback 11/11.
- **Fim da Fase 4.** `ImportDlg`/`ExportDlg` do original são de configuração (seções do INI) e foram movidos para a Fase 6.

### 2026-10-31 — Sessão 24: Fase 5, parte 1 — transferência e conferência (0.4.1)
- **`src/ui/Transfer.{h,cpp}`**: port do `Transfer.cpp` sobre o `IDD_TRANSFER`: `SetAction` (títulos e botões — OK, ou Assistida/Cega quando há código de função de transferência assistida), `LoadFromContacts` (só favoritos se houver; telefone/celular como itens extras; número no dado do item), `Action` (texto digitado ou item escolhido → `MessagesDlg::CallAction`), teclado `Input`, `OnClose` = destruir e zerar `mainDlg->transferDlg`. `msip_action` passa a morar em `Transfer.h`, como no original. Troca marcada: `GetCurSel` = -1 quando o texto digitado difere do item.
- **`MessagesDlg`**: `OnBnClickedTransfer` (diálogo, se houver código de transferência assistida; senão menu), `OnBnClickedConference`, `OnBnClickedActions` com as mesmas regras do original (submenu "Transferir para" com as outras chamadas, mesclar com uma conferência só, Adicionar/Mesclar/Mesclar todas/Separar/Separar todas/Desconectar habilitados conforme o estado), `OnTransfer/OnAttendedTransfer/OnAttendedTransferRange` (`pjsua_call_xfer_replaces`), `OnConference`, `OnMerge`; `UpdateCallButton` fecha o `Transfer` quando a chamada acaba. Troca marcada: menus montados com `QMenu` (IDs de menu → `call_id` direto).
- **`CmainDlg`**: `OpenTransferDlg(pai, ação, call_id, contato)` real, `onCallTransferStatus` (mensagens na aba; 2xx encerra a chamada transferida), `CommandCallPickup` (código de captura), `onMWIInfo` (botão de correio de voz). Chamadores atualizados: Discador (Transferir, Conferência), `RinginDlg` (Desviar), Contatos (duplo clique durante chamada = transferir para o contato; Captura de chamada).
- **Núcleo**: `msip_null_sound_device` — com `nullSoundDevice` (testes/CLI) `msip_set_sound_device` mantém o dispositivo nulo; antes, cada chamada reabria o ALSA e a chamada gerada por uma transferência falhava quando o dispositivo não abria.
- **Achados nos testes (não eram defeitos do port):** (1) com `sourcePort` ≠ 5060 o MicroSIP também abre a 5060 para a conta local (`PJCreateRaw`), então a CLI que subir primeiro fica com ela — os testes agora sobem a CLI da 5060 antes das outras; (2) um `ASSERT` falho destruía a janela sem `OnClose` e o pjsua seguia chamando a janela destruída (queda vista uma vez) — os testes de ponta a ponta agora sempre chamam `OnClose` ao sair; (3) `accountSettings = AccountSettings()` nos testes copiava `bool` sem inicialização (o construtor é vazio como no original; no app o global começa zerado) — novo auxiliar `tests/TestSettings.h` (`ResetAccountSettings`).
- **Testes:** `Transfer.TitulosBotoesListaETeclado` (offscreen); `Transfer.TransferenciaCegaRealComDuasClis` (janela → CLI A, transfere para a CLI B; A recebe o REFER e liga para B; o NOTIFY 200 encerra a chamada da janela; repetido 6×), `Transfer.TransferenciaCegaRecusadaPeloDestino` (B com `denyIncoming=all` → "Busy Here [Final]", a chamada continua) e `Transfer.TransferenciaParaAPropriaPonta` (opcionais, `MSIP_TEST_SIP=1`; variáveis de diagnóstico `MSIP_TEST_CLI_LOG`, `MSIP_TEST_KEEP`). 83 testes no build normal e no ASan/UBSan; os 14 de interface (8 com SIP real) num só processo sob ASan/UBSan sem avisos; loopback 11/11. Captura: `docs/layout/transferencia-0.4.1.png` (`MSIP_SCREENSHOT_DIALOG=transfer`).

### 2026-10-31 — Sessão 25: Fase 5, parte 2 — gravação MP3, conferência e comandos (0.5.0)
- **Gravação em MP3** (`src/core/Mp3Encode.{h,cpp}`): no Windows o MicroSIP grava direto em MP3 (gravador do pjmedia + `lame_enc.dll`, inexistente no Linux) e só usa WAV com `recordingFormat=wav`. Aqui `msip_call_recording_start` grava em `.wav` e guarda o destino `.mp3`; `msip_call_recording_stop`, ao destruir o gravador, converte em segundo plano com a libmp3lame (mono/estéreo, VBR qualidade 5, cabeçalho Xing/LAME com a duração) e apaga o WAV se der certo; leitor de WAV tolera gravação interrompida. `CmainDlg::OnClose` espera as conversões (`msip_wav_to_mp3_wait`). `msip_core` liga com `mp3lame`.
- **Teste de conferência real** (`Conference.MesclarESepararComDuasClis`): modo de várias chamadas, janela liga para A e B (A vai para espera), "Mesclar todas" → as duas chamadas no bridge, cada uma ouvindo a outra (`pjsua_conf_get_port_info`), "Separar todas" desfaz.
- **Gravação real** (`Recording.ChamadaGravadaEmMp3`): `autoRecording=1` com formato padrão — WAV durante a chamada, só o `.mp3` no fim (nome `data-hora-usuário-direção-conta`, como no original).
- **Comandos e desligamento automático** (`Commands.ComandosPorEventoEAutoDesligar`): `cmdOutgoingCall/cmdCallStart/cmdCallEnd` executados (script de teste registra os eventos) e `autoHangUpTime=2` encerra a chamada sozinha. O limite de chamadas (`maxConcurrentCalls`/`callWaiting`) está no `on_incoming_call` portado com o núcleo.
- **Unidade:** `Mp3Encode.ConverteWavMonoEEstereo` (16 kHz mono, 44,1 kHz estéreo, WAV com tamanho de dados 0, entradas inválidas, conversão assíncrona que apaga o WAV).
- 87 testes no build normal e no ASan/UBSan; os 17 de interface (11 com SIP real: chamadas, presença, transferência, conferência, gravação, comandos) num só processo sob ASan/UBSan sem avisos; loopback 11/11. **Fim da Fase 5.**

### 2026-10-31 — Sessão 26: Fase 6, parte 1 — Configurações (0.5.1)
- **`src/ui/SettingsDlg.{h,cpp}`**: port do `SettingsDlg.cpp` sobre o `IDD_SETTINGS` (900×562 px): `OnInitDialog` (mesmos valores e listas — dispositivos de áudio/vídeo enumerados pelo pjsua, codecs habilitados/disponíveis a partir do `audioCodecList` com o padrão `_GLOBAL_CODECS_ENABLED`, DTMF, atendimento automático, desvio, recusa, ação padrão, atualizações), `OnBnClickedOk` (esconde, `PJDestroy`, e no próximo ciclo `OnUpdateSettings`: lê tudo, `SettingsSave`, fecha a janela de chamadas no modo de chamada única, `RebuildButtons/UpdateCallButton` do Discador e `CmainDlg::OnRecreate` = `PJCreate` + `OnAccountChanged` + `PJAccountAdd`), Cancelar (para o toque), teste do volume do toque ao soltar o controle, procurar toque (WAV) e pasta de gravação, setas e teclas (Espaço/Delete) para habilitar/desabilitar e ordenar codecs.
- **Diferenças marcadas:** LISTBOX → `QListWidget`; `msctls_updown32` → dois botões de seta (horizontal: → habilita/← desabilita; vertical: ↑/↓ — mesma convenção do `iDelta` invertido do Windows); links "?" da ajuda ocultos (`_GLOBAL_HELP_WEBSITE` vazio, decisão pendente); inicialização automática por XDG Autostart (`msip_startup_set`, nova `msip_startup_get`); taxa de vídeo padrão buscada pelo ID real do H.264 (`msip_vid_codec_id`); sem pjsua rodando a lista de codecs fica vazia e o valor atual é mantido; prévia da câmera → Fase 8. Pendentes no app: teclas de mídia, fone HID e mudanças de rede (Fase 7), verificação de atualizações (servidor pendente).
- **`AAOptionsDlg`** (atraso, números, chamadas ativas) e **`FeatureCodesDlg`** (captura, transferência cega/assistida) portados; abrem perto do cursor como no original e gravam ao confirmar.
- **`CmainDlg`**: `settingsDlg`, `OnMenuSettings` (menu principal → Configurações), `OnRecreate`.
- **Teste:** `SettingsDlg.CarregaEditaSalvaERecria` (offscreen, com pjsua no dispositivo nulo): valores carregados, codecs (habilitar pela seta, ordenar, Delete), opções, `AAOptionsDlg` e `FeatureCodesDlg`, Salvar (INI gravado, pjsua recriado, ordem dos codecs) e Cancelar (não grava). 88 testes no build normal e no ASan/UBSan; os 18 de interface (11 com SIP real) sob ASan/UBSan sem avisos; loopback 11/11. Captura: `docs/layout/configuracoes-0.5.1.png` (`MSIP_SCREENSHOT_DIALOG=settings`, com `HOME` temporário).

### 2026-10-31 — Sessão 27: áudio pelo PulseAudio/PipeWire (0.5.2)
- **Relato do responsável:** Configurações funcionaram bem, mas o áudio não saía pelo dispositivo padrão (fone Bluetooth PH503) e a lista de dispositivos nas Configurações era incompreensível.
- **Causa:** o backend ALSA do pjproject listava 32 PCMs (`lavrate`, `samplerate`, `hw:CARD=PCH,DEV=0`, `surround51:...`) e o "padrão" do pjmedia é o primeiro dispositivo da lista (`lavrate`), não o padrão do sistema — o PipeWire tinha o PH503 como saída/entrada padrão.
- **Correção (antecipa o item "PipeWire" da Fase 7):** `src/core/PulseAudioDev.{h,cpp}` — fábrica de dispositivos do pjmedia sobre a libpulse (PipeWire via `pipewire-pulse`): enumeração síncrona de sinks e sources (sem os "monitor"), com as descrições do sistema; índice 0 = pseudo-dispositivo `MSIP_PULSE_DEFAULT_DEVICE` que segue o padrão do servidor (é o "Padrão" do MicroSIP, oculto nas Configurações); fluxos com `pa_simple` (thread de captura e de reprodução registradas no pjlib, quadros de `samples_per_frame`, latência pelos parâmetros do pjmedia). `msip_pulse_audio_install()` no `PJCreateRaw` (exceto com `nullSoundDevice`): registra o PulseAudio, retira o ALSA e refaz a lista (`pjmedia_aud_dev_refresh`, porque o `unregister` só invalida as entradas); sem servidor de som o ALSA continua. Nomes repetidos só ganham sufixo dentro da mesma direção (entrada e saída do fone têm o mesmo nome, como no Windows).
- **CLI de teste:** comando `devices` (lista os dispositivos e os padrões).
- **Teste:** `PulseAudioDev.ListaEFluxoReal` (opcional, `MSIP_TEST_AUDIO=1`: só dispositivos do PulseAudio, padrões = pseudo-dispositivo, 1 s de captura + reprodução de silêncio com ~50 quadros/s em cada sentido; também limpo sob ASan). 89 testes no build normal e no ASan/UBSan; interface/SIP 18/18 sob ASan; loopback 11/11.
- **Responsável testou o diretório de usuários pelas Configurações: usuários carregados e presença publicada corretamente.** Criou o repositório público no GitHub e pediu o envio do código, documentação de instalação e, na versão estável, instalador e release.
- **Responsável confirmou que a lista de dispositivos e o áudio pelo dispositivo padrão (fone Bluetooth) funcionam perfeitamente.**

### 2026-10-31 — Sessão 28: repositório público no GitHub
- **Decisão do responsável:** antecipar o repositório (antes previsto para a 1.0.0) — https://github.com/GGTY81/MySimpleSIP (público, código aberto). Token de envio em `.credentials` (ignorado; usado só via `GIT_ASKPASS` temporário; o responsável revoga ao fim do projeto).
- **Preparação para o público:** `.gitignore` passa a ignorar `.credentials`, `*.token` e `reference/` — o código original do MicroSIP inclui `lib/Markup.*` (CMarkup: "redistributing this source code requires written permission") e `atlrx.h` (Microsoft), que não podem ser redistribuídos; nenhum dos dois foi copiado para `src/` (CMarkup próprio; regex ATL traduzido). Os sons/langpacks necessários já estão em `resources/`. Varredura dos arquivos versionados: sem tokens, senhas, contas ou IPs reais; o nome de usuário em caminhos do Bottles (3 linhas da documentação) virou `<usuário>`; `tests/data/microsip_win.ini` conferido (anonimizado).
- **Documentação pública:** `README.md` reescrito (recursos, situação, instalação resumida, uso, arquivos, licença/créditos), `docs/INSTALACAO.md` (passo a passo no Ubuntu 26.04: clonar, dependências, compilar, atalho no menu, conta, áudio, atualizar, remover, problemas comuns), `docs/DESENVOLVIMENTO.md` (compilação, sanitizadores, testes opcionais, estrutura, método de port, como recriar `reference/`, como contribuir); `scripts/install-desktop-entry.sh` (atalho no menu do GNOME sem sudo); `install-deps.sh` instala também `git` e `ca-certificates`.
- **Instalador (decisão de arquitetura, para a 1.0.0):** pacote **`.deb`** gerado pelo CPack (gerador DEB, dependências calculadas com `dpkg-shlibdeps`; pjproject já estático), instalando em `/usr` (`bin/mysimplesip`, `share/mysimplesip/{sounds,langpacks}`, `.desktop`, ícones hicolor, metainfo AppStream, manipulador `sip:`/`tel:`), publicado como asset da Release do GitHub (`sudo apt install ./mysimplesip_X.Y.Z_amd64.deb`); depois, opcionalmente, PPA no Launchpad para atualizações pelo `apt`. Motivo: alvo é o Ubuntu 26.04, o `.deb` integra com o `apt` e as bibliotecas do sistema (Qt 6, PipeWire, chaveiro) e é o formato que o usuário instala com dois cliques; Flatpak/AppImage ficam como opção futura para outras distribuições.

---

## Ambiente de desenvolvimento verificado (2026-10-30)

Instalação: `sudo ./scripts/install-deps.sh`.

| Componente | Versão | Como é encontrado |
|---|---|---|
| Ubuntu | 26.04.1 LTS (GNOME, Wayland) | — |
| GCC / CMake / Ninja | 15.2 / 4.2.3 / 1.13.2 | — |
| Qt 6 (Widgets, DBus, Network, Tools) | 6.10.2 | CMake `Qt6` |
| QtKeychain | 0.15.0 | CMake `Qt6Keychain` |
| OpenSSL | 3.5.5 | pkg-config `openssl` |
| libsrtp2 | 2.7.0 | pkg-config `libsrtp2` |
| Opus / Speex DSP / GSM / bcg729 / LAME | 1.6.1 / 1.2.1 / 1.0.23 / 1.1.1 / 3.101 | pkg-config (`opus`, `speexdsp`); headers `gsm/gsm.h`, `bcg729/*.h`, `lame/lame.h` |
| AMR (opencore-amrnb/wb, vo-amrwbenc) | 0.1.6 / 0.1.3 | pacotes `-dev` |
| ALSA / PulseAudio (libpulse) | 1.2.15.3 / 17.0 | pkg-config `alsa`, `libpulse` |
| libvpx / OpenH264 | 1.16.0 / 2.6.0 | pkg-config `vpx`, `openh264` |
| FFmpeg | **8.0.1** (libavcodec 62.11) | pkg-config `libavcodec` etc. — ver risco abaixo |
| V4L2 / SDL2 | 1.32.0 / 2.32.10 | pkg-config `libv4l2`, `sdl2` |
| SQLite / SQLiteCpp / jsoncpp | 3.46.1 / 3.3.3 / 1.9.6 | pkg-config `sqlite3`; CMake `SQLiteCpp` (sem `.pc`); pkg-config `jsoncpp` |
| hidapi / libudev | 0.15.0 / 259 | pkg-config `hidapi-hidraw` |
| GoogleTest | 1.17.0 | CMake `GTest` |
| sngrep / Podman | 1.8.3 / 5.7.0 | — |

Indisponíveis nos repositórios: `sipp` (usar contêiner com Podman) e `libilbc` (o pjproject traz o iLBC embutido). Não instalado: `libwebrtc-audio-processing-dev` (o pjproject 2.15.1 traz seu próprio AEC WebRTC).

**Risco para 0.0.5:** o pjproject 2.15.1 é anterior ao FFmpeg 8.0; o codec de vídeo FFmpeg (`ffmpeg_vid_codecs.c`) pode não compilar. Alternativas: H.264 via OpenH264 (suportado pelo pjproject) e H.263 via FFmpeg com pequeno patch, ou desativar o FFmpeg até a fase 8.

---

## Decisões pendentes (perguntar ao responsável)

- [ ] Nome final do produto e ícone ("MySimpleSIP"?).
- [ ] Vídeo desde o início ou versão só-áudio primeiro (vídeo na 0.8.0)?
- [ ] Formatos de distribuição: `.deb` / Flatpak / AppImage / Snap.
- [ ] Atualização automática e crash report: desativar ou servidor próprio?
- [ ] Padrão XWayland (recomendado) ou Wayland nativo.
- [ ] Repositório remoto (GitHub/GitLab) e licença (GPL-2.0-or-later recomendada) — adiado para a 1.0.0.

## Decisões tomadas

| Data | Decisão | Motivo |
|---|---|---|
| 2026-10-30 | UI em Qt 6 Widgets | Bandeja (SNI), D-Bus, rede, HTTP, DPI, Wayland/X11 prontos; ver ANALISE §5 |
| 2026-10-30 | Manter PJSIP (upstream, tag fixa, compilado estático) | Mesmo motor do original = mesmo comportamento SIP |
| 2026-10-30 | Mesmo formato INI e mesmo esquema SQLite do MicroSIP | Fidelidade e importação de configurações/histórico |
| 2026-10-30 | Senhas no chaveiro do sistema (QtKeychain) | Sem CryptoAPI no Linux; chave original indisponível |
| 2026-10-30 | Versionamento X.Y.Z, começando em 0.0.1 | Pedido do responsável |
| 2026-10-30 | `reference/` mantido intacto (será versionado quando o git for criado) | Base para diffs com novas versões do MicroSIP |
| 2026-10-30 | Sem git até a 1.0.0 | Pedido do responsável: foco em fazer funcionar primeiro |
| 2026-10-30 | Usar pjproject **2.15.1** como base | Mesma versão do binário do MicroSIP 3.22.12 → menor diferença de comportamento; atualizar depois |
| 2026-10-30 | Reutilizar sons WAV e langpack pt-BR do MicroSIP | Distribuídos sob GPL v2 com o instalador; manter atribuição |
| 2026-10-30 | H.264 via OpenH264; H.263 via FFmpeg 8 com patch próprio | OpenH264 é suportado nativamente; patch pequeno e isolado em `third_party/pjproject/patches/` |
| 2026-10-30 | Codecs de vídeo localizados pelo nome, não por payload type fixo | Upstream usa PTs diferentes do pjproject do MicroSIP; PT dinâmico não afeta interoperabilidade |
| 2026-10-30 | `CString` sobre `std::wstring`; `CStringA` em UTF-8; `Format` variádico com tradução MFC→glibc | Portar o código do MicroSIP com o mínimo de alterações e sem depender do Qt no núcleo |
| 2026-10-30 | INI gravado em UTF-8 sem BOM (lê também UTF-16 do Windows); API `Get/WritePrivateProfileString` reimplementada | Padrão Linux; `settings.cpp` portável quase sem mudanças; importa `MicroSIP.ini` |
| 2026-10-30 | Portar arquivos do MicroSIP copiando o original e trocando só o necessário, com marcas `// MySimpleSIP:` | Fidelidade e facilidade de comparar com futuras versões do MicroSIP |
| 2026-10-30 | Links do microsip.org desativados no port (macros vazias) | Produto distinto; decisão pendente sobre site/ajuda/atualização |
| 2026-10-30 | Langpack escolhido pelo idioma do sistema quando há vários; busca por texto exato | Linux distribui vários idiomas juntos; evita colisões de hash do original |
| 2026-10-30 | Sem logger próprio: `PJ_LOG` + log de arquivo do pjsua | É o que o MicroSIP faz |
| 2026-10-30 | Núcleo conversa com a UI por `PostMessage`/`SetTimer` emulados (`IMessageBackend`) e ganchos `CoreHost` | Mantém o código do MicroSIP quase intacto; a UI Qt implementa o backend |
| 2026-10-30 | Plano de discagem: tradutor ATL→`std::wregex` próprio, sem `atlrx.h` | `atlrx.h` é proprietário da Microsoft |
| 2026-10-30 | HTTP do núcleo com Qt Network (núcleo pode usar Qt Core/Network, nunca Widgets) | Sem libcurl instalada; Qt já é dependência |
| 2026-10-30 | Diálogos montados em tempo de execução a partir do layout do `.rc` (`CDialogLayout`), controles acessados pelo ID original | Fidelidade visual e código portado quase linha a linha |
| 2026-10-30 | Vazamentos herdados do original (`StrToPjStr` sem `free`) mantidos por enquanto | Fidelidade; são pequenos e só na inicialização |
| 2026-10-30 | `CmainDlg` dividido: `CmainDlgCore` (pjsua, no núcleo, com ganchos virtuais) + janela Qt derivada (Fase 3) | Testar o núcleo sem interface mantendo o código original |
| 2026-10-30 | Locais: INI em `~/.config/MySimpleSIP`, dados em `~/.local/share/MySimpleSIP`, log em `~/.local/state/MySimpleSIP`; portátil com INI ao lado do executável | Especificação XDG; mesma lógica instalado/portátil do MicroSIP |

---

## Plano de etapas

### Fase 0 — Análise e preparação → 0.0.x
- [x] 0.0.1 Extrair e analisar o código-fonte; documento de análise; diário; `CLAUDE.md`; `VERSION`.
- [x] 0.0.2 `git init`, `.gitignore`, `.gitattributes`, `COPYING` (GPL-2.0), `README.md`; `reference/` entra no git intacto (2026-10-30). Depois, por decisão do responsável, o `.git` foi removido (sem git até a 1.0.0).
- [x] 0.0.3 Levantar a instalação de referência no Bottles (versões, codecs, sons, langpacks, formatos de dados) e copiar os recursos reutilizáveis (2026-10-30).
- [x] 0.0.4 Instalar dependências (`scripts/install-deps.sh`) e registrar as versões reais (2026-10-30).
- [x] 0.0.5 (2026-10-30, ver Sessão 5) Baixar o tarball do pjproject 2.15.1 para `third_party/pjproject/`, criar `third_party/pjproject/config_site.h` (vídeo, SRTP, Opus, G.722, bcg729, V4L2, `PJMEDIA_HAS_*`) e script/ExternalProject no CMake para compilá-lo estático.
- [x] 0.0.6 (2026-10-30, ver Sessão 6) Esqueleto CMake (`src/main.cpp` com Qt, link com pjsua) que abre janela vazia e mostra versão do app e do pjsip. Script `build.sh`.
- [x] 0.0.7 (2026-10-30, ver Sessão 7) Converter recursos: `.ico`/`.bmp` → PNG/SVG em `resources/icons/` com `resources.qrc`; extrair layout dos 15 diálogos (`res/dialog.rc2`) para uma planilha de coordenadas (DLU → px) que servirá de base aos `.ui`.

### Fase 1 — Fundação → 0.1.0
- [x] `src/compat/CString` (subconjunto usado: Format/AppendFormat, Tokenize, Find/ReverseFind, Mid/Left/Right, Trim*, Make*, Replace, Compare/CompareNoCase, IsEmpty, GetLength, conversões `CStringA` UTF-8) + testes (0.0.8).
- [x] `compat/CTime`, `CArray`, `CList`/`CStringList`, `CByteArray` mínimos (0.0.8). `CMap` não é usado fora da UI.
- [x] `IniFile` + `PrivateProfile` (mesma semântica de `Get/WritePrivateProfileString`, UTF-16/UTF-8 na leitura) + testes de roundtrip (0.0.9; ficaram em `src/compat/`).
- [x] `settings/Paths` (XDG: `~/.config`, `~/.local/share`, `~/.local/state`; modo portátil) (0.0.9).
- [x] Portar `settings.cpp` → `AccountSettings` (todas as chaves, mesmos padrões) e `ShortcutsLoad/Save` (0.0.10).
- [x] `IPasswordStore` real com QtKeychain (Secret Service) (0.2.1).
- [x] `i18n/LangPack` + `Translate()` com mesmo formato do `langpack_*.txt` (0.1.0).
- [x] Logging: decidido usar o log do pjsip como o original; port em `PJCreateRaw` (Fase 2) (0.1.0).

### Fase 2 — Núcleo SIP headless → 0.2.0
- [x] `core/mainDlgCore` (`CmainDlgCore`): `PJCreateRaw`, `PJDestroy`, transportes, `PJAccountConfig`, `PJAccountAdd(Local)`, codecs, `UpdateSoundDevicesIds`, `PublishStatus`, player, `onCallAnswer/onCallHangup` (0.1.2).
- [x] `core/callbacks.cpp`: todos os `on_*` → `PostMessage(UM_ON_*)` para a thread principal (0.1.2).
- [x] `app/QtMessageBackend`: mensagens e timers na thread do Qt (0.1.2).
- [x] `global.cpp` portado para `src/core/global.cpp` (mantido inteiro em vez de dividir em CallControl/NumberFormat, para fidelidade): hold/unhold, conferência, DTMF (3 métodos), tonegen, gravação, `FormatNumber`, `GetSIPURI`, `URLMask`, `SelectSIPAccount`, HTTP (0.1.1).
- [x] `lib/MSIP.cpp` portado para `src/core/MSIP.cpp` (0.1.1).
- [x] Emulação de `PostMessage/SetTimer/CWnd` e `CCriticalSection` em `compat/` (0.1.1).
- [ ] `core/pj_internal`: isolar uso de `pjsua_internal.h`.
- [x] CLI de teste `msip-cli` + `tests/sip-lab/loopback.sh` (chamada por IP entre duas instâncias) (0.1.2).
- [x] Teste de registro em servidor real: `200 OK` no PBX do responsável (UDP) (0.1.2).
- [x] Chamada via servidor com áudio real (PipeWire) (0.2.0).

### Fase 3 — MVP gráfico → 0.3.0
- [x] Janela principal `CmainDlg` (abas + botão menu + menu principal + barra de status + `UpdateWindowText`) (0.2.1).
- [x] `Dialer` (aba Telefone): teclado, número com histórico, chamar/vídeo/mensagem, volumes, VU-meter, botões inferiores (0.2.3; atalhos/speed-dial na Fase 6).
- [x] `AccountDlg` (0.2.2), com `CDialogLayout` para montar os diálogos a partir do `.rc`.
- [x] `MessagesDlg` (janela de chamadas: abas, chamar/atender, espera, encerrar, mensagens) (0.2.5; transferência/conferência na Fase 5).
- [x] `RinginDlg` (janela de chamada recebida) (0.3.0; Desviar → Fase 5).
- [x] Bandeja básica (ícones de estado, menu, mostrar/ocultar) (0.2.1).

### Fase 4 — Contatos e histórico → 0.4.0
- [x] Histórico em SQLite (`call_log.db`) portado de `Data/`, `Service/` (0.3.1). Contatos: no 3.22.16 continuam em `Contacts.xml` (o `ContactsRepository` do original é um esboço).
- [x] `CallsPage` (aba Histórico) com menu de contexto, ordenação, busca e larguras salvas (0.3.1).
- [x] `ContactsPage`, `ContactDialog` (AddDlg), menus de contexto, ordenação, larguras de colunas salvas (0.3.2).
- [x] Presença/BLF (subscribe/unsubscribe), import/export CSV/XML de contatos e histórico, `Contacts.xml` (0.3.2–0.4.0). `ImportDlg`/`ExportDlg` são de **configuração** (seções do INI) → movidos para a Fase 6.
- [x] Diretório de usuários remoto (HTTP XML/JSON/CSV, recarga periódica) (0.4.0).

### Fase 5 — Chamadas avançadas → 0.5.0
- [x] Transferência cega/assistida (`Transfer`), conferência (menus mesclar/separar/desconectar), múltiplas chamadas, chamada em espera (0.4.1).
- [x] Gravação WAV e MP3 (libmp3lame) (0.5.0), auto-atendimento, encaminhamento, DND, rejeição, códigos de função (transferência e captura), pickup e MWI (0.3.0–0.4.1).
- [x] Comandos externos por evento (`cmd*`), auto-hangup, limite de chamadas (portados com o núcleo; `cmd*` e auto-hangup testados com chamada real em 0.5.0).

### Fase 6 — Configurações e mensagens → 0.6.0
- [x] `SettingsDialog` completo, `AAOptionsDialog`, `FeatureCodesDialog` (0.5.1).
- [ ] `ShortcutsDialog` + speed-dial (com presença dos atalhos: `Dialer::PresenceSubscribe/PresenceReset/PresenceReceived`), `AAOptionsDialog`, `FeatureCodesDialog`, exportar/importar configuração (`ExportDlg`/`ImportDlg`).
- [ ] Chat (SIP MESSAGE) na `MessagesWindow`, port knock.

### Fase 7 — Integração Linux → 0.7.0
- [~] Instância única (feita na 0.2.4: 2ª execução mostra a 1ª) + comandos de linha (`/minimized` feito; `/exit`, `/reset`, `/answer`, `/hangupall`, `/hold`, `/mic*`, `/speak*`, URIs a fazer).
- [ ] Handler `sip:`/`tel:`/`callto:` (.desktop), autostart, chaveiro, notificações.
- [ ] Monitores: rede (netlink/QNetworkInformation), suspensão/sessão (logind), dispositivos de áudio.
- [ ] Backend de áudio pjmedia PulseAudio/PipeWire.
- [ ] Teclas multimídia (MPRIS/D-Bus), headset HID (hidapi + udev).
- [ ] Tema claro/escuro seguindo o sistema.

### Fase 8 — Vídeo → 0.8.0
- [ ] V4L2 + renderer Qt próprio, preview, janela de vídeo, codecs VP8/VP9/H.264/H.263.

### Fase 9 — Polimento e empacotamento → 0.9.0
- [ ] Langpack pt-BR, sons e ícone próprios, `.deb` (CPack, com udev/.desktop/metainfo), Flatpak/AppImage.

### Fase 10 — Paridade → 1.0.0
- [ ] Testes de interoperabilidade e comparação lado a lado com o MicroSIP (3.22.12 no Bottles); fechar o checklist de paridade (ANALISE §3.4).
- [ ] Criar o repositório git (`git init`, primeiro commit, tag `v1.0.0`, remoto).

---

## Como retomar o projeto

1. Ler a seção **Estado atual** e a última entrada do **Registro de sessões**.
2. Pegar a primeira etapa `[ ]` (ou `[~]`) do **Plano de etapas**.
3. Consultar a parte correspondente em `ANALISE_E_PLANO.md` e o código original em `reference/MicroSIP-3.22.16/`.
4. Ao terminar: atualizar este diário (etapa, sessão, histórico de versões se mudou a versão) e o arquivo `VERSION`.
