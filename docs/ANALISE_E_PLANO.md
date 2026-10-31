# MySimpleSIP — Análise do MicroSIP 3.22.16 e Plano de Port para Linux

> Documento de referência técnica. Para o andamento do trabalho (o que já foi feito e o que falta), veja [DIARIO_DO_PROJETO.md](DIARIO_DO_PROJETO.md).
>
> Data da análise: 2026-10-30 · Versão do documento: 0.0.1 · Alvo: Ubuntu 26.04 LTS (GNOME/Wayland), x86_64

---

## 1. Resumo executivo

O MicroSIP é um softphone SIP para Windows escrito em **C++ com MFC (Microsoft Foundation Classes)**, usando a biblioteca **PJSIP (pjsua-lib)** como motor SIP/RTP/mídia. O código analisado tem **≈ 46.500 linhas** (incluindo bibliotecas embutidas) em 128 arquivos C/C++, sob licença **GPL v2 ou posterior**.

**Conclusão principal:** um port para Linux é **viável**, mas **não é uma recompilação**. O motor (PJSIP) já é multiplataforma e roda nativamente no Linux; o que não existe no Linux é tudo o que está acima dele — a interface MFC/Win32, os recursos de diálogo (`.rc`), o registro do Windows, a bandeja do sistema Win32, WASAPI/COM, CryptoAPI, WinINet, HID do Windows, Jump Lists etc.

**Recomendação:** reescrever a camada de interface em **Qt 6 (Qt Widgets)**, portar a lógica de telefonia (callbacks pjsua, regras de discagem, conferência, transferência, gravação, auto-atendimento, encaminhamento etc.) **quase linha a linha** sobre uma pequena camada de compatibilidade de tipos (`CString`, `CTime`, `CArray`…), compilar com **CMake**, e embutir o **pjproject** compilado a partir do código-fonte com configuração controlada. O resultado é um aplicativo nativo, com o mesmo layout, mesmos menus, mesmo comportamento e mesmo formato de configuração do MicroSIP, distribuível como `.deb` (e opcionalmente Flatpak/AppImage).

Estimativa grosseira de esforço (1 desenvolvedor experiente em C++/Qt/PJSIP): **4 a 6 meses** até paridade funcional (1.0.0), com um MVP que registra, liga e atende em **4 a 6 semanas** (0.3.0).

---

## 2. Inventário do código-fonte

### 2.1 Conteúdo do pacote `MicroSIP-3.22.16-src.7z`

Extraído (somente leitura, como referência) em `reference/MicroSIP-3.22.16/`.

| Área | Arquivos principais | Linhas aprox. | Função |
|---|---|---:|---|
| Janela principal / núcleo SIP | `mainDlg.cpp/.h` | 6.400 | Todos os callbacks do pjsua, criação/destruição do pjsua, contas, transportes, bandeja, timers, linha de comando, diretório de usuários, atualização, assinatura de presença |
| Funções globais de telefonia | `global.cpp/.h` | 2.300 | Formatação de números, URIs SIP, DTMF, hold/unhold, conferência, gravação, HTTP assíncrono, máscaras de URL, ajuste de dispositivo de som |
| Configurações | `settings.cpp/.h` | 1.600 | Leitura/gravação do `microsip.ini` (≈ 300 chamadas `Get/WritePrivateProfileString`), caminhos, senha cifrada, atalhos |
| Janela de chamadas/mensagens | `MessagesDlg.cpp/.h` | 2.400 | Abas por chamada/contato, chat (RichEdit), botões hold/transfer/conferência/gravação, estatísticas |
| Discador | `Dialer.cpp/.h`, `ButtonDialer.*` | 2.150 | Teclado, campo de número, volumes, VU-meter, botões de função (DND, AA, encaminhamento, conferência), atalhos (speed dial) |
| Contatos | `Contacts.cpp/.h`, `AddDlg.*`, `Service/ContactsService.*`, `Data/ContactsRepository.*` | 2.100 | Lista de contatos, presença (BLF), importação de XML legado, SQLite (`contacts.db`) |
| Histórico | `Calls.cpp/.h`, `Service/CallsService.*`, `Data/CallsRepository.*` | 1.450 | Histórico de chamadas em SQLite (`call_log.db`, máx. 1000 registros) |
| Diálogos de configuração | `SettingsDlg.*`, `AccountDlg.*`, `ShortcutsDlg.*`, `AAOptionsDlg.*`, `FeatureCodesDlg.*`, `ImportDlg.*`, `ExportDlg.*`, `Transfer.*`, `RinginDlg.*`, `Preview.*` | 3.900 | Preferências, conta SIP, atalhos, auto-atendimento, códigos de função, import/export, transferência, janela de chamada recebida, preview de vídeo |
| Controles customizados | `Button*.cpp`, `IconButton`, `ClosableTabCtrl`, `LevelsSliderCtrl`, `StatusBar`, `CListCtrl_*`, `ThemeDialog`, `BaseDialog` | 1.900 | Botões owner-draw, abas com botão fechar, slider com nível, ordenação de listas, tema claro/escuro |
| Aplicação | `microsip.cpp/.h` | 900 | `CWinApp`, instância única, `/minimized`, `/exit`, `/reset`, crash dump/report, idioma |
| Integrações Windows | `jumplist.*`, `MMNotificationClient.h`, `lib/Hid.*`, `lib/hid.c`, `lib/VisualStylesXP.*` | 2.200 | Jump List da barra de tarefas, notificação de dispositivos de áudio (WASAPI), headsets USB HID, temas visuais |
| Bibliotecas utilitárias embutidas | `lib/jsoncpp`, `lib/Markup` (CMarkup XML), `lib/CSVFile`, `lib/StdioFileEx`, `lib/MessageBoxX`, `lib/langpack`, `lib/Crypto`, `lib/CMask`, `lib/ggets`, `atlrx.h` | 10.500 | JSON, XML, CSV, arquivos texto Unicode, caixas de mensagem, tradução, criptografia, regex ATL |
| Recursos | `main.rc`, `res/*.rc2`, `resource.h`, `res/*.ico`, `res/*.bmp` | 1.500 + 107 imagens | 16 diálogos (412 controles, ver `docs/layout/DIALOGOS.md`), 5 menus, 57 ícones, 27 bitmaps (voicemail em 4 escalas de DPI), versão |

### 2.2 O que **não** está no pacote (precisa ser obtido/recriado)

| Item | Situação | Ação no port |
|---|---|---|
| **pjproject** (`..\pjlib`, `..\pjsip`, `..\pjmedia`…) | O projeto referencia o pjproject num diretório-irmão. O MicroSIP usa uma árvore própria do pjproject (possivelmente com patches) que **não veio** no pacote. O binário instalado revela **pjproject 2.15.1** (§2.4). | Usar pjproject **2.15.1 upstream** compilado do fonte (mesma versão do binário). Todas as APIs usadas (§3.3) existem no upstream. Verificar diferenças de comportamento em testes. |
| `third_party` (opus, bcg729, silk, ffmpeg, vpx, SDL, OpenSSL, AMR, SQLiteCpp, IPP) | Binários Windows pré-compilados, ausentes. | Usar pacotes do Ubuntu (`-dev`) ou submódulos. |
| Arquivos de som (`ringtone.wav`, `ringing.wav`, `ringing2.wav`, `hangup.wav`, `msgin.wav`, `msgout.wav`) | Vêm no instalador binário, não no fonte. **Obtidos** da instalação no Bottles → `reference/MicroSIP-3.22.12-assets/sounds/` (PCM 16 bit mono 8 kHz). O instalador os distribui sob a `License.txt` (GPL v2). | Reutilizar com atribuição; sons próprios ficam como opção. |
| Pacotes de idioma (`langpack_*.txt`) | Distribuídos à parte. **Obtidos** `langpack_portuguesebr.txt` (308 frases, cobertura completa) e o modelo `langpack_english.txt.sample` → `reference/MicroSIP-3.22.12-assets/langpacks/`. | Manter **o mesmo formato** e reutilizar o pt-BR; acrescentar as frases novas do MySimpleSIP. |
| `_GLOBAL_KEY` (chave de cifra de senha) | Mascarada no fonte (`"*********"`). | Senhas cifradas de um `microsip.ini` do Windows **não poderão** ser decifradas. No Linux a senha vai para o chaveiro do sistema (Secret Service / GNOME Keyring). |
| Alguns ícones referenciados no `.vcxproj` (`res\active.ico`, `res\online.ico`…) | Referências antigas não usadas pelos `.rc2`; irrelevantes. | Ignorar. |
| Arquivo de licença (COPYING) | Ausente; licença declarada nos cabeçalhos (GPL v2+). | Incluir `COPYING` (GPL-2.0) no nosso repositório. |

### 2.3 Configuração de compilação original

- Visual Studio, `UseOfMfc=Static`, `CharacterSet=Unicode` (strings `wchar_t`, `_T()`), subsistema Windows, alvo Windows 7+ (`_WIN32_WINNT 0x0601`).
- Bibliotecas: `bcg729.lib`, `opus.lib`, `Wtsapi32.lib`, SQLiteCpp/sqlite3, OpenSSL, libs do pjproject; inclui headers WinRT (para checagem de permissão de câmera).
- Macro `_GLOBAL_VIDEO` ativa: versão com vídeo (há também a variante "Lite" sem vídeo).

### 2.4 Instalação de referência (MicroSIP 3.22.12 no Bottles)

Levantamento feito em 2026-10-30 em `~/.var/app/com.usebottles.bottles/data/bottles/bottles/MicroSIP/drive_c/users/<usuário>/AppData/Local/MicroSIP/`, por inspeção de arquivos, strings do executável e REA (`inspect_pe_resources`). Por ser 3.22.12, há pequenas diferenças em relação ao fonte 3.22.16 (ex.: o binário ainda grava contatos em `Contacts.xml`; o fonte usa `contacts.db`).

| Componente | Encontrado | Equivalente no Linux |
|---|---|---|
| `MicroSIP.exe` | 3.22.12, PE32 (32 bits), 13 MB, MFC estático | — |
| pjproject | **2.15.1** (compilado em `C:\work\pjsip`) | pjproject 2.15.1 do fonte |
| TLS | OpenSSL 1.1.1o (estático) | OpenSSL 3.x do sistema (`libssl-dev`) |
| Áudio | backend **WMME** (`wmme_dev.c`), AEC **WebRTC** (`echo_webrtc.c`) + `echo_suppress` | ALSA/PulseAudio; AEC WebRTC do pjproject (`third_party/webrtc`) |
| Vídeo | captura DirectShow (`dshow_dev.c`), render SDL 2.32.10 (`SDL2.dll`), barras de cor | V4L2 + renderer Qt próprio (SDL opcional) |
| Codecs de vídeo | H.264 e H.263 via FFmpeg 6.1.4 (`avcodec-60`, `avformat-60`, `avutil-58`, `swscale-7`), VP8/VP9 via libvpx | FFmpeg do sistema / OpenH264, libvpx |
| Codecs de áudio | PCMA, PCMU, G.722, G.722.1 (16/32), G.729, GSM, iLBC, Speex (8/16 kHz), SILK, Opus (48000/2), AMR-WB (via `ipp_codecs.c`), L16, telephone-event | pjmedia + libopus, bcg729 (G.729), libgsm, speex; AMR via opencore (opcional); SILK sem pacote no Ubuntu (opcional) |
| Gravação MP3 | `mp3_writer.c` + `lame_enc.dll` 3.100 (API BladeEnc) | libmp3lame (`libmp3lame-dev`) |
| Banco | SQLite + SQLiteCpp (estáticos) | idem |
| Recursos do .exe (REA) | 18 diálogos, 5 menus, 56 grupos de ícones (213 imagens), 29 bitmaps, 13 string tables, manifesto; **nenhum som embutido** | coerente com `res/*.rc2` do fonte |
| Sons | 6 WAV (ver §2.2) | reutilizados |
| Langpacks | pt-BR + modelo inglês | reutilizados |
| Dados do usuário | `MicroSIP.ini` (UTF-16 LE com BOM, CRLF) ao lado do .exe e em `Roaming`; `Contacts.xml` (atributos `name, number, firstname, lastname, phone, mobile, email, address, city, state, zip, comment, id, info, presence, starred, directory`); `call_log.db` (mesmo esquema do fonte) | O importador deve ler INI em UTF-16 LE e `Contacts.xml` |

Uso real observado (relevante para priorizar): contas UDP com registro de 300 s e keep-alive de 15 s, `publish=1`, `allowRewrite=1`, gravação em MP3, headset USB, e **diretório de usuários remoto via HTTP** (agenda do PBX) — este último deve entrar cedo no MVP (fase 4 em vez da 6).

---

## 3. Arquitetura do MicroSIP

### 3.1 Visão em camadas

```
┌──────────────────────────────────────────────────────────────────────┐
│ UI (MFC/Win32)                                                        │
│  CmainDlg (janela principal + abas) ─┬─ CDialer   (aba Discador)      │
│                                      ├─ CCalls    (aba Histórico)     │
│                                      ├─ CContacts (aba Contatos)      │
│  MessagesDlg (janela de chamadas/chat, abas por chamada)              │
│  RinginDlg (popup de chamada recebida), Transfer, Settings, Account…  │
│  Bandeja (Shell_NotifyIcon), Jump List, temas, DPI                    │
├──────────────────────────────────────────────────────────────────────┤
│ Lógica de telefonia (misturada com a UI)                              │
│  global.cpp: msip_call_*, conferência, DTMF, gravação, FormatNumber,  │
│  GetSIPURI, URLMask, HTTP; mainDlg.cpp: on_* callbacks, PJCreate,     │
│  PJAccountConfig, AutoAnswer, encaminhamento, presença, diretório     │
├──────────────────────────────────────────────────────────────────────┤
│ Dados                                                                 │
│  settings.cpp (microsip.ini) · SQLite (call_log.db, contacts.db) ·    │
│  Service/Repository/Domain (camada nova, já bem separada)             │
├──────────────────────────────────────────────────────────────────────┤
│ PJSIP / pjsua-lib  (SIP, SDP, RTP/SRTP, ICE/STUN, codecs, conf bridge,│
│  dispositivos de áudio/vídeo)                                         │
└──────────────────────────────────────────────────────────────────────┘
```

Observação importante: **a lógica de negócio está acoplada à UI** (ex.: `CmainDlg::onCallState` decide regras de chamada e mexe em controles na mesma função). Apenas `Domain/`, `Data/` e `Service/` são camadas limpas (refatoração recente do autor). O port deve **separar** isso.

### 3.2 Modelo de threads e eventos

1. O pjsua roda suas próprias threads (worker + áudio). Os callbacks `on_call_state`, `on_incoming_call`, `on_reg_state2`, `on_pager2`, `on_buddy_state`, `on_mwi_info`, `on_dtmf_digit`, `on_call_transfer_status` etc. são chamados **nessas threads**.
2. Cada callback copia os dados mínimos e faz `PostMessage(mainDlg->m_hWnd, UM_ON_CALL_STATE, …)` para a thread da UI (≈ 47 `PostMessage`).
3. Na thread da UI, `CmainDlg::onCallState`, `onIncomingCall`… executam a lógica e atualizam a interface.
4. ~20 timers Win32 (`SetTimer`: `IDT_TIMER_CALL`, `IDT_TIMER_AUTOANSWER`, `IDT_TIMER_FORWARDING`, `IDT_TIMER_VU_METER`, `IDT_TIMER_NETWORK_CHANGED`, `IDT_TIMER_HEADSET`…) controlam duração, piscar, auto-atendimento, encaminhamento, VU-meter, reconexão.
5. Duas `CreateThread` avulsas: HTTP assíncrono (`URLGetAsync`) e DNS reverso (`gethostbyaddr`).

**No Qt:** `PostMessage` → `QMetaObject::invokeMethod(obj, …, Qt::QueuedConnection)` (ou sinais com conexão enfileirada); `SetTimer` → `QTimer`; threads avulsas → `QNetworkAccessManager` (assíncrono nativo) e `QtConcurrent`/`QHostInfo`. Toda thread não criada pelo pjlib que chame pjsua deve ser registrada com `pj_thread_register()`.

### 3.3 Uso do PJSIP

Todas as funções abaixo existem no **pjproject upstream** e funcionam no Linux:

- Núcleo: `pjsua_create/init/start/destroy`, `pjsua_transport_create` (UDP/TCP/TLS, porta 5060/5061 para "conta local" P2P), `pjsua_acc_add/modify/del`, `pjsua_acc_set_online_status2`, `pjsua_handle_ip_change`.
- Chamadas: `pjsua_call_make_call`, `answer/answer2`, `hangup`, `set_hold`, `reinvite`, `xfer`, `xfer_replaces`, `dial_dtmf`, `send_request` (DTMF via SIP INFO), estatísticas de stream (MOS).
- Conferência: `pjsua_conf_connect/disconnect/adjust_rx_level/adjust_tx_level/get_signal_level`.
- Mídia: `pjsua_player_*` (WAV), `pjsua_recorder_*` (gravação), `pjmedia_tonegen_*` (DTMF local), `pjsua_set_snd_dev2`, `pjsua_enum_aud_devs`, `pjsua_codec_set_priority/param`.
- Vídeo: `pjsua_vid_preview_*`, `pjsua_vid_win_*`, `pjsua_vid_codec_*`, `pjsua_vid_enum_devs`.
- Presença/IM: `pjsua_buddy_*`, `pjsua_im_send`, `on_pager2`, MWI.
- Uso de internos: `pjsua_internal.h` (`pjsua_var.acc[]`, `pjsua_var.tpdata[]`, `pjsua_acc_update_contact_on_ip_change`). Funciona, mas é frágil entre versões → isolar num único arquivo.
- `pj_unicode_to_ansi` (só existe com `PJ_NATIVE_STRING_IS_UNICODE`, ou seja, Windows) → substituir por conversão UTF-8 própria.

Callbacks registrados: `on_reg_started2`, `on_reg_state2`, `on_call_state`, `on_dtmf_digit`, `on_call_tsx_state`, `on_call_redirected`, `on_call_media_state`, `on_call_media_event`, `on_incoming_call`, `on_nat_detect`, `on_buddy_state`, `on_pager2`, `on_pager_status2`, `on_call_transfer_request2`, `on_call_transfer_status`, `on_call_replace_request2`, `on_call_replaced`, `on_mwi_info`.

### 3.4 Funcionalidades a reproduzir (checklist de paridade)

**Telefonia:** múltiplas contas SIP (uma ativa por vez) + conta local P2P; UDP/TCP/TLS; SRTP (SDES/DTLS); STUN, ICE, rport, keep-alive, reescrita de Contact/Via; proxy(s), DNS SRV com nameservers próprios; registro com refresh; publicação de presença; chamadas múltiplas simultâneas, hold/unhold, chamada em espera; transferência cega e assistida (com Replaces); conferência local (merge/separate/disconnect); DTMF RFC2833/SIP INFO/in-band; auto-atendimento (com atraso, por número, por header); encaminhamento (incondicional/ocupado/sem resposta com atraso); DND; rejeição automática; call pickup e códigos de função (`**`, BT, AT); prefixo de discagem, plano de discagem, ocultar CallerID; voicemail/MWI; gravação de chamadas em WAV ou MP3 (`recordingFormat`; o gravador MP3 do pjmedia só existe no Windows → no Linux gravar WAV e converter com **libmp3lame** ao fim da chamada, ou escrever um `pjmedia_port` gravador MP3 próprio), gravação automática; auto-desligamento por tempo; limite de chamadas concorrentes; codecs de áudio (PCMA, PCMU, G.722, G.729/bcg729, Opus mono/estéreo, GSM, iLBC, Speex, AMR…); VAD, cancelamento de eco, amplificação de microfone; vídeo (H.264, H.263, VP8, VP9, bitrate, câmera); mensagens instantâneas SIP MESSAGE; presença/BLF dos contatos; diretório de usuários remoto (XML/JSON via HTTP com autenticação, recarga periódica); "port knocking"; User-Agent customizável.

**Interface:** janela compacta com abas (Discador, Contatos, Histórico) e botão de menu; janela de chamadas/chat com abas fecháveis; popup de chamada recebida (posição salva, opção de posição aleatória, trazer à frente); atalhos/speed-dial (8 botões, embaixo ou ao lado); VU-meter e sliders de volume; barra de status; ícone de bandeja com estados (ativo, inativo, perdida) e balões; sempre-no-topo; múltiplos monitores; tema claro/escuro; DPI alto; ordenação/colunas redimensionáveis com largura salva; tradução por langpack.

**Integração com o sistema:** instância única com repasse de linha de comando (`/minimized`, `/exit`, `/reset`, `/answer`, `/hangupall`, `/hold`, `/micmute`, `/speakup`…, `sip:`/`tel:`/`callto:`/`msip:`); handler de URLs; iniciar com o sistema; executar comandos externos em eventos (`cmdIncomingCall`, `cmdCallAnswer`, `cmdCallEnd`…); teclas multimídia; headsets USB HID (gancho, mute, redial, flash, LEDs); troca automática de dispositivo ao conectar/desconectar áudio; reação a mudança de rede, suspensão/retomada e bloqueio de sessão; verificação de atualização; relatório de crash; importação/exportação (INI, CSV, XML).

---

## 4. Mapa de dependências Windows → Linux

| Recurso Windows (onde) | Ocorrências | Substituto no Linux (proposto) | Dificuldade |
|---|---:|---|---|
| MFC `CDialog`/`CWnd`/controles comuns (todos os diálogos) | centenas | Qt Widgets (`QDialog`, `QTabWidget`, `QTreeView`/`QTableView`, `QComboBox`, `QPushButton` customizado) | **Alta** (é a maior parte do trabalho) |
| Recursos `.rc` (15 diálogos, 5 menus) | — | Arquivos `.ui` do Qt Designer reproduzindo as mesmas posições (conversão de *dialog units* → pixels) ou layout em código | Média |
| `CString` / `CStringA` | 1.186 | Classe de compatibilidade `msip::CString` sobre `QString` (subconjunto da API: `Format`, `Tokenize`, `Find`, `Mid`, `Left`, `Trim`, `MakeLower`…) | Média (mecânica) |
| `CTime`, `CArray`, `CStringList`, `CMap`, `CByteArray` | ~150 | Shims sobre `QDateTime`/`std::vector`/`QStringList`/`std::map` | Baixa |
| `Get/WritePrivateProfileString` (INI) | 307 | Implementação própria de INI **com o mesmo formato** (permite importar `microsip.ini`). Evitar `QSettings` (escapa chaves/valores de forma incompatível) | Baixa |
| Registro (`HKCU\...\Run`, protocolos, tema) | ~8 | `~/.config/autostart/*.desktop`; `x-scheme-handler/sip` em `.desktop` + `xdg-mime`; tema via `QStyleHints::colorScheme()` / portal `org.freedesktop.appearance` | Baixa |
| Bandeja `Shell_NotifyIcon` + balões | 7 | `QSystemTrayIcon` (StatusNotifierItem via D-Bus; no Ubuntu funciona com a extensão AppIndicator, instalada por padrão) + notificações `org.freedesktop.Notifications` | Média (ver §7) |
| `TaskDialog`, `MessageBoxX` | 14+ | `QMessageBox` customizado | Baixa |
| WinINet `CInternetSession` (HTTP/HTTPS) | 3 + async | `QNetworkAccessManager` | Baixa |
| CryptoAPI MD5→RC2 (senha no INI) | 1 | Chaveiro do sistema via **QtKeychain** (libsecret); opção "salvar em arquivo" com cifra própria (libsodium/OpenSSL) só para modo portátil | Baixa |
| `IMMNotificationClient` (WASAPI) | 1 | Assinatura de eventos do PipeWire/PulseAudio (`libpulse` subscribe, compatível com `pipewire-pulse`) ou udev | Média |
| `NotifyUnicastIpAddressChange`, `NotifyRouteChange2` | 2 | `QNetworkInformation` + netlink (`RTMGRP_IPV4_IFADDR/ROUTE`) ou D-Bus NetworkManager | Média |
| `WM_POWERBROADCAST`, `WTSRegisterSessionNotification` | 2 | D-Bus `org.freedesktop.login1` (`PrepareForSleep`, `Lock`/`Unlock`) | Baixa |
| `WM_COPYDATA` + `FindWindow` (instância única) | 3 | `QLocalServer`/`QLocalSocket` (ou D-Bus `org.mysimplesip.MySimpleSIP`) | Baixa |
| `RegisterShellHookWindow`, `WM_APPCOMMAND` (teclas multimídia) | 1 | MPRIS (`org.mpris.MediaPlayer2`) ou `org.gnome.SettingsDaemon.MediaKeys` via D-Bus | Média |
| HID do Windows (`HidP_GetUsages`, `lib/hid.c`) | 1 módulo | **hidapi** backend `hidraw` + parser de *report descriptor* para a Usage Page Telephony (0x0B) e LED (0x08); regra **udev** para acesso sem root | Média/Alta |
| Jump List (`ICustomDestinationList`) | 1 | Ações no `.desktop` (`[Desktop Action ...]`) | Baixa |
| `ShellExecute` (abrir URL/arquivo, executar comandos) | 7 | `QDesktopServices::openUrl`, `QProcess::startDetached` | Baixa |
| `CFileDialog` | 8 | `QFileDialog` (usa o portal do GNOME) | Baixa |
| `CRichEditCtrl` (chat) | 23 | `QTextBrowser` (HTML) | Baixa |
| Owner-draw (`DrawItem`, `CDC`, GDI) | ~25 | `paintEvent` com `QPainter` | Média |
| DPI (`dpiY`, bitmaps 100/125/150/175%) | — | DPI automático do Qt + SVG/PNG @2x | Baixa |
| Tema escuro (`DwmSetWindowAttribute`, registro) | 1 | Estilo Fusion + paleta clara/escura seguindo o sistema | Baixa |
| Crash dump (`SetUnhandledExceptionFilter`, minidump, envio HTTP) | 1 | Desativar no início; opcional: `systemd-coredump`/Breakpad | Baixa |
| Permissão de câmera (WinRT) | 1 | Remover (no Linux é acesso a `/dev/video*`; em Flatpak via portal Camera) | Baixa |
| `GetLocaleInfoA`/code pages no langpack | 1 | Langpack lido como UTF-8 (aceitar UTF-16/ANSI na importação) | Baixa |
| `atlrx.h` (regex ATL) | 1 | `std::regex`/`QRegularExpression` | Baixa |
| `CMarkup`, jsoncpp, CSVFile, SQLiteCpp | — | Mantidos (portáveis) ou trocados por versões do sistema (`libjsoncpp`, `libsqlitecpp`) | Baixa |

---

## 5. Alternativas avaliadas

| # | Abordagem | Prós | Contras | Veredito |
|---|---|---|---|---|
| A | **Rodar o .exe com Wine** | Zero código; idêntico. | Não é nativo; áudio via Wine (latência, troca de dispositivos), bandeja e HID instáveis; não atende "versão para Linux". | Útil só como **referência visual** durante o port. |
| B | **Winelib + MFC** (recompilar com headers do Wine) | Mantém o código quase intacto. | MFC é proprietária (licença do Visual Studio não permite redistribuir/usar fora do Windows) e incompatível com a GPL; Winelib ainda depende do Wine em runtime. | **Descartado** (jurídico e técnico). |
| C | **wxWidgets (GTK3)** | API inspirada no MFC (tabelas de eventos, `wxString` ≈ `CString`, diálogos parecidos); visual nativo GTK. | Bandeja no GNOME/Wayland problemática (usa `GtkStatusIcon` obsoleto); ecossistema menor; integração D-Bus/rede/HID tem que ser feita à mão; owner-draw menos flexível. | Segunda opção. |
| D | **Qt 6 Widgets** | Maduro, LGPL; `QSystemTrayIcon` com StatusNotifierItem; D-Bus, rede, `QLocalServer`, HTTP, teclado, DPI, tema claro/escuro, Wayland/X11 prontos; Qt Designer para reproduzir os diálogos; mesma base serviria para Windows/macOS no futuro. | Reescrita da UI inteira (inevitável em qualquer opção nativa); dependência grande. | **Recomendado.** |
| E | GTK4 (gtkmm) | Visual GNOME nativo. | GTK4 removeu ícone de bandeja; mais distante do modelo MFC. | Descartado. |
| F | Electron/Tauri + PJSIP por binding | UI web fácil. | Pesado, foge do "MicroSIP" leve; bindings e áudio complexos. | Descartado. |
| G | Trocar PJSIP por outra pilha (Sofia-SIP, baresip, linphone) | — | Mudaria todo o comportamento; perde a fidelidade. | Descartado: **manter PJSIP**. |

---

## 6. Arquitetura proposta (MySimpleSIP)

### 6.1 Pilha tecnológica

| Item | Escolha | Observação |
|---|---|---|
| Linguagem | C++20 | GCC 15 (disponível no Ubuntu 26.04) |
| Build | CMake ≥ 3.25 + Ninja | CMake 4.2 instalado |
| UI | Qt 6 Widgets (+ Qt Network, Qt DBus) | Versão do Qt 6 do repositório Ubuntu |
| SIP/mídia | pjproject **2.15.1** (tarball em `third_party/`, mesma versão do binário do MicroSIP), compilado estático com `config_site.h` próprio | Vídeo, SRTP, Opus, G.722, G.722.1, G.729 (bcg729), GSM, iLBC, Speex, AEC WebRTC; AMR opcional |
| Áudio | ALSA do pjmedia apontando para o dispositivo `pipewire`/`pulse`/`default` (via plugin ALSA do PipeWire); **fase posterior:** backend pjmedia nativo PipeWire ou PulseAudio para enumerar dispositivos com nomes amigáveis | Ver §7.2 |
| Vídeo | V4L2 (captura) + renderização própria em widget Qt via callback de frames | Ver §7.3 |
| Banco | SQLite3 + SQLiteCpp (mesmo esquema: `call_log`, `contacts`) | Compatível com bancos do MicroSIP |
| Config | INI próprio, mesmo formato/chaves do `microsip.ini`, API `Get/WritePrivateProfileString` reimplementada (`src/compat/`) | `~/.config/MySimpleSIP/MySimpleSIP.ini` (UTF-8); dados em `~/.local/share/MySimpleSIP/`, log em `~/.local/state/MySimpleSIP/`; portátil se o INI estiver ao lado do executável |
| Senhas | QtKeychain (Secret Service) | fallback cifrado em modo portátil |
| HID | hidapi (hidraw) | regra udev `70-mysimplesip-headset.rules` |
| JSON/XML | jsoncpp do sistema / CMarkup (modo STL) ou QtXml | diretório de usuários |
| Testes | GoogleTest ou Qt Test; PJSIP contra Asterisk/FreeSWITCH em contêiner | |
| Empacotamento | `.deb` (CPack), depois Flatpak e AppImage | |

### 6.2 Estrutura de diretórios do repositório

```
MySimpleSIP/
├── CMakeLists.txt
├── VERSION                       # X.Y.Z
├── COPYING                       # GPL-2.0
├── README.md
├── CLAUDE.md                     # instruções para continuar o projeto com o Claude
├── docs/
│   ├── ANALISE_E_PLANO.md        # este documento
│   └── DIARIO_DO_PROJETO.md      # etapas feitas / a fazer
├── reference/MicroSIP-3.22.16/   # fonte original (somente leitura; não compilado)
├── third_party/
│   └── pjproject/                # pjproject 2.15.1 (tarball) + config_site.h
├── cmake/                        # FindPJPROJECT.cmake, versão, CPack
├── src/
│   ├── compat/                   # CString, CTime, CArray… (shims sobre Qt/STL)
│   ├── core/                     # SEM Qt Widgets: lógica de telefonia portada
│   │   ├── SipEngine.*           # PJCreate/PJDestroy/transportes/contas (ex-mainDlg)
│   │   ├── Callbacks.*           # on_* do pjsua → eventos enfileirados
│   │   ├── CallControl.*         # msip_call_* (ex-global.cpp)
│   │   ├── Conference.*, Dtmf.*, Recorder.*, Player.*
│   │   ├── NumberFormat.*        # FormatNumber, GetSIPURI, URLMask…
│   │   ├── Presence.*, Messaging.*, UsersDirectory.*
│   │   └── pj_internal.*         # único ponto que usa pjsua_internal.h
│   ├── settings/                 # IniFile, AccountSettings, Shortcuts, Paths (XDG)
│   ├── data/  domain/  service/  # portados quase sem mudanças
│   ├── platform/linux/           # SingleInstance, Autostart, UrlHandler, Keyring,
│   │                             # NetworkMonitor, PowerMonitor, AudioDeviceMonitor,
│   │                             # MediaKeys, HidHeadset, Notifications
│   ├── ui/                       # Qt Widgets
│   │   ├── MainWindow.*  DialerPage.*  ContactsPage.*  CallsPage.*
│   │   ├── MessagesWindow.*  RingingDialog.*  TransferDialog.*
│   │   ├── SettingsDialog.*  AccountDialog.*  ShortcutsDialog.*  …
│   │   ├── widgets/              # DialerButton, IconButton, ClosableTabBar, LevelsSlider…
│   │   └── *.ui
│   ├── i18n/                     # leitor de langpack (mesmo formato) + Translate()
│   └── main.cpp
├── resources/
│   ├── icons/  (PNG/SVG convertidos dos .ico/.bmp)  sounds/  (próprios, livres)
│   ├── langpacks/
│   ├── mysimplesip.desktop  mysimplesip.metainfo.xml  70-mysimplesip-headset.rules
│   └── resources.qrc
├── tests/
└── packaging/  (debian/, flatpak/, appimage/)
```

### 6.3 Princípios do port

1. **Fidelidade primeiro, refatoração depois.** A lógica do `core/` é portada mantendo nomes de funções, ordem de decisões e chaves de configuração do original, para facilitar comparação e aplicar futuras versões do MicroSIP (diff upstream 3.22.16 → 3.22.x → nosso código).
2. **Separar núcleo e UI.** O `core/` não inclui Qt Widgets; comunica-se com a UI por uma interface de eventos (`ICoreEvents`/sinais Qt) — substitui os `PostMessage(UM_*)`. Isso permite testar o núcleo sem interface.
3. **Camada de compatibilidade mínima e transitória.** `src/compat/` implementa só o subconjunto de `CString`/`CTime`/etc. realmente usado, sem depender do Qt. *Implementado (0.0.8):* `CString` sobre `std::wstring` (wchar_t), `CStringA` em UTF-8, `Format` variádico que aceita `CString` nos argumentos; conversão para `pj_str_t` sempre em UTF-8; conversão para a UI via `QString::fromStdWString`.
4. **Mesma configuração.** Mesmo nome de seções/chaves INI, mesmos valores padrão (`_GLOBAL_CODECS_ENABLED="PCMA/8000/1 PCMU/8000/1"`, etc.). Importador de `microsip.ini` do Windows (exceto senha).
5. **Mesmo visual.** Dimensões base do original (`_GLOBAL_WIDTH 162` DLUs etc.) convertidas para pixels; mesmos ícones; mesmas posições de botões; textos via langpack.
6. **Identidade própria.** Nome **MySimpleSIP**, ícone próprio na versão final; "baseado no MicroSIP" nos créditos (exigência da GPL: manter avisos de copyright). Não usar a marca "MicroSIP" como nome do produto nem os endpoints `microsip.org` (atualização/crash report) — apontar para o nosso servidor ou desativar.

### 6.4 Correspondência de módulos (original → port)

| Original | Destino | Notas |
|---|---|---|
| `microsip.cpp` (`CmicrosipApp`) | `main.cpp`, `platform/linux/SingleInstance` | `/minimized`, `/exit`, `/reset`, repasse de comandos |
| `mainDlg.cpp` callbacks + PJ* | `core/mainDlgCore` (`CmainDlgCore`), `core/callbacks.cpp` (portados, 0.1.2) | Ganchos virtuais no lugar das chamadas de UI |
| `mainDlg.cpp` UI/bandeja/menu | `ui/mainDlg` (`CmainDlg`, 0.2.1) | janela, bandeja e menu |
| `global.cpp` | `core/global.cpp` (portado inteiro, 0.1.1), `core/DialPlanRegex`, `core/http_qt.cpp` | |
| `settings.cpp` | `settings/settings.cpp` (portado, 0.0.10), `compat/IniFile`+`PrivateProfile`, `settings/Paths`, `settings/PasswordStore` | Caminhos XDG; modo portátil se existir `MySimpleSIP.ini` ao lado do executável |
| `Dialer.*`, `ButtonDialer.*`, `LevelsSliderCtrl.*`, `ButtonEx.*` | `ui/Dialer`, `ui/DialerControls` (portados, 0.2.3) | |
| `Contacts.*`, `AddDlg.*` | `ui/ContactsPage`, `ui/ContactDialog` | modelo `QAbstractTableModel` |
| `Calls.*` | `ui/CallsPage` | |
| `MessagesDlg.*`, `ClosableTabCtrl.*` | `ui/MessagesWindow`, `ui/widgets/ClosableTabBar` | `QTextBrowser` no chat |
| `RinginDlg.*` | `ui/RingingDialog` | |
| `AccountDlg.*` | `ui/AccountDlg` (portado, 0.2.2) | via `CDialogLayout` |
| `Transfer.*`, `SettingsDlg.*`, `ShortcutsDlg.*`, `AAOptionsDlg.*`, `FeatureCodesDlg.*`, `ImportDlg.*`, `ExportDlg.*` | `ui/*Dialog` | |
| `Preview.*` | `ui/VideoWindow` | renderização própria |
| `Data/`, `Domain/`, `Service/` | `data/`, `domain/`, `service/` | quase sem alterações |
| `lib/langpack.*` | `i18n/langpack` (portado, 0.1.0) | mesmo formato de arquivo; escolha por idioma do sistema |
| `lib/Hid.*`, `lib/hid.c` | `platform/linux/HidHeadset` | hidapi |
| `MMNotificationClient.h` | `platform/linux/AudioDeviceMonitor` | |
| `jumplist.*` | `.desktop` actions | |
| `ThemeDialog.*`, `VisualStylesXP.*` | removidos (tema do Qt) | |
| `lib/Crypto.*` | `platform/linux/Keyring` | |
| `lib/MessageBoxX.*`, `ModelessMessageBox.*` | `ui/MessageBox` | |
| `lib/jsoncpp`, `lib/Markup`, `lib/CSVFile`, `lib/StdioFileEx` | sistema/STL ou mantidos | |

---

## 7. Pontos críticos e riscos

### 7.1 Wayland (padrão no Ubuntu 26.04)
O Wayland **proíbe** que um aplicativo posicione suas janelas em coordenadas absolutas, se mantenha "sempre no topo" ou roube o foco. O MicroSIP depende disso: posição salva da janela (`mainX/mainY`), posição do popup de chamada (`ringinX/ringinY`, `randomAnswerBox`), `alwaysOnTop`, `bringToFrontOnIncoming`.
- **Mitigação:** por padrão iniciar com `QT_QPA_PLATFORM=xcb` (XWayland), onde tudo isso funciona como no Windows; oferecer modo Wayland nativo como opção, aceitando as limitações (usar notificação + `xdg-activation` para pedir atenção).

### 7.2 Áudio (PipeWire)
O pjmedia no Linux usa ALSA. Com PipeWire, o dispositivo ALSA `default`/`pipewire` funciona, mas a lista de dispositivos do ALSA mostra placas "hw" cruas (nomes pouco amigáveis, podem estar ocupadas) e **não** os dispositivos/Bluetooth do PipeWire.
- **Constatado na 0.0.5:** com o pjproject compilado, o ALSA enumera 32 dispositivos (plugins e placas cruas, inclusive `jack` que gera erros no stderr); só `pipewire`, `pulse` e `default` são utilizáveis.
- **Fase 1:** usar ALSA + dispositivo `pipewire`/`pulse`, filtrando a lista; seleção de dispositivo via nome do sink/source do PipeWire passado ao plugin (`PULSE_SINK`/`PIPEWIRE_NODE`), ou via PortAudio.
- **Fase 2 (recomendada para paridade):** escrever um **backend de áudio pjmedia para PulseAudio API** (`libpulse`, atendida pelo `pipewire-pulse`) — enumeração com nomes amigáveis, dispositivo de toque separado (`audioRingDevice`), troca a quente, eventos de conexão/desconexão (substitui `IMMNotificationClient`). Isso é um módulo de ~800–1500 linhas.
- Cancelamento de eco: o MicroSIP usa o **WebRTC AEC** do pjmedia. *Constatado na 0.2.4:* com o Speex AEC habilitado (padrão do pjproject) ele é escolhido primeiro e gera avisos de xrun; desligado no `config_site.h`, o WebRTC AEC passa a ser o padrão. Testado em chamadas reais: áudio perfeito via ALSA→PipeWire (dispositivo padrão).

### 7.3 Vídeo
No Windows o pjsua cria janelas nativas (SDL/DirectShow) e o MicroSIP as reposiciona por `HWND`. No Linux/Wayland não dá para embutir uma janela SDL em um widget Qt de forma confiável.
- **Constatado na 0.0.5:** captura V4L2 (câmera integrada), SDL renderer e codecs H264 (OpenH264), H263-1998 (FFmpeg 8 + patch), VP8 e VP9 funcionando no pjproject 2.15.1. Payload types do upstream diferem dos do MicroSIP → localizar codecs pelo nome.
- **Solução:** compilar pjmedia com captura **V4L2** e, para renderização, registrar um dispositivo de vídeo "renderer" próprio (pjmedia_vid_dev factory) que entrega frames (I420/RGB) a um `QWidget`/`QOpenGLWidget`. Codecs: VP8/VP9 (libvpx), H.264 (OpenH264 ou FFmpeg do sistema), H.263 (FFmpeg).
- Vídeo pode ficar para a fase 0.8.x sem bloquear o restante.

### 7.4 Bandeja do sistema no GNOME
O GNOME puro não tem bandeja; o Ubuntu inclui a extensão "Ubuntu AppIndicators" que exibe StatusNotifierItem (usado pelo `QSystemTrayIcon`). Limitações: clique simples/duplo e tooltip podem se comportar diferente; balões viram notificações do sistema. Prever fallback: se não houver bandeja, fechar a janela minimiza em vez de esconder.

### 7.5 Headsets HID
Sem permissões, `/dev/hidraw*` é só do root → instalar regra udev (pacote .deb faz isso). Cada fabricante (Jabra, Poly, EPOS, Logitech) pode exigir sequências específicas para LEDs/ring; o código original é genérico (Telephony page) — portar o mesmo comportamento genérico.

### 7.6 pjproject sem os patches do MicroSIP
O MicroSIP compila contra sua própria árvore do pjproject, que pode ter ajustes não publicados neste pacote. Risco: pequenas diferenças de comportamento (ex.: SRTP, reescrita de Contact, áudio). Mitigação: testes de interoperabilidade (Asterisk, FreeSWITCH, provedores reais) e comparação de logs SIP lado a lado com o MicroSIP Windows (pode rodar no Wine/VM).

### 7.7 Licenças
- MicroSIP: GPL-2.0-or-later → **MySimpleSIP deve ser GPL-2.0-or-later** (ou GPL-3.0), com código-fonte público e avisos de copyright preservados.
- pjproject: GPL-2.0 (ou comercial) — compatível.
- Qt 6: LGPL-3.0 — compatível com GPL-2.0-or-later (a obra combinada é distribuída sob GPLv3).
- Codecs: G.729 sem patentes ativas (bcg729, GPL); H.264 — usar OpenH264 da Cisco (binário com licença de patente) ou FFmpeg do sistema; AMR com cuidado (patentes em algumas jurisdições) → opcional/desligado por padrão.
- Sons e langpacks do MicroSIP: não incluídos no fonte, licença indefinida → criar próprios.

### 7.8 Outros
- Diferenças de comportamento de `CString::Format` (`%s` com wide) — padronizar no shim.
- Caminhos com `\\` espalhados pelo código (`res\\microsip.ico`, `pathRoaming + "call_log.db"`) → centralizar em `Paths`.
- O original grava log do pjsip com `PJ_LOG_HAS_CR` (CRLF) → remover no Linux.

---

## 8. Plano de execução por fases e versões

Versionamento **X.Y.Z** (padrão do kernel Linux): `X` = série principal (0 = pré-paridade, 1 = paridade com MicroSIP 3.22), `Y` = marco funcional (incrementa a cada fase concluída), `Z` = correções/incrementos dentro do marco. A versão vigente fica no arquivo `VERSION` e na tag git `vX.Y.Z`. A versão do MicroSIP de referência é registrada separadamente (`MSIP_BASE_VERSION=3.22.16`).

| Versão | Fase | Entregáveis | Critério de pronto |
|---|---|---|---|
| **0.0.x** | 0 — Análise e preparação | Este documento, diário, `CLAUDE.md`, `VERSION`; levantamento da instalação de referência; dependências; pjproject 2.15.1 compilando; esqueleto CMake; script `build.sh` (git só na 1.0.0) | `cmake --build` gera um executável "hello" ligado ao pjsua e imprime a versão do pjsip |
| **0.1.0** | 1 — Fundação | `compat/` (CString, CTime…), `IniFile`, `Paths` XDG, `AccountSettings` load/save com as mesmas chaves, leitor de langpack, logs; testes unitários dos shims e do INI (roundtrip com um `microsip.ini` real) | Lê e regrava um `microsip.ini` sem perder chaves |
| **0.2.0** | 2 — Núcleo SIP headless | `core/SipEngine` (PJCreate/Destroy, transportes, contas, `PJAccountConfig`, codecs), callbacks → fila de eventos, `CallControl` (make/answer/hangup/hold/DTMF), CLI de teste | Registra num Asterisk de teste e faz/recebe chamada de áudio pela linha de comando |
| **0.3.0** | 3 — MVP gráfico | `MainWindow` com abas, `DialerPage` completo, `RingingDialog`, `MessagesWindow` (só aba de chamada), bandeja básica, `AccountDialog` | Usuário configura conta, liga, atende, desliga, hold, mute, volume pela UI |
| **0.4.0** | 4 — Contatos e histórico | SQLite (`contacts.db`, `call_log.db`), `ContactsPage`, `CallsPage`, `ContactDialog`, menus de contexto, ordenação/colunas, presença/BLF, import/export CSV/XML, **diretório de usuários remoto (HTTP XML/JSON)**, importação de `Contacts.xml` | Paridade das abas Contatos/Histórico |
| **0.5.0** | 5 — Chamadas avançadas | Transferência cega/assistida, conferência (merge/separate), múltiplas chamadas, chamada em espera, gravação, auto-atendimento, encaminhamento, DND, códigos de função, call pickup, MWI/voicemail, comandos externos em eventos, auto-hangup | Cenários de teste de §9 passam |
| **0.6.0** | 6 — Configurações e mensagens | `SettingsDialog` completo, `ShortcutsDialog` + speed-dial, `AAOptionsDialog`, `FeatureCodesDialog`, chat SIP MESSAGE, User-Agent, port knock | Todas as opções do INI editáveis pela UI |
| **0.7.0** | 7 — Integração Linux | Instância única + comandos (`/answer`, `sip:` …), handler de URL, autostart, chaveiro, monitor de rede/suspensão/sessão, monitor de dispositivos de áudio, backend PulseAudio/PipeWire, teclas multimídia, headset HID + udev, notificações, tema claro/escuro | Integração de §3.4 completa |
| **0.8.0** | 8 — Vídeo | V4L2, renderer Qt, preview, codecs VP8/VP9/H.264/H.263, janela de vídeo | Videochamada bidirecional com outro softphone |
| **0.9.0** | 9 — Polimento e empacotamento | Langpack pt-BR, ícones/sons próprios, `.deb` (CPack) com regra udev e `.desktop`, Flatpak/AppImage, verificação de atualização própria (opcional), documentação de usuário | Instala limpo num Ubuntu 26.04 novo |
| **1.0.0** | 10 — Paridade | Testes de interoperabilidade, correções, comparação lado a lado com MicroSIP 3.22.16; criação do repositório git | Checklist de paridade (§3.4) 100% ou com exceções documentadas |
| 1.1.0+ | Evolução | Acompanhar novas versões do MicroSIP (diff de fonte) e melhorias próprias | — |

---

## 9. Estratégia de testes

1. **Unitários** (`tests/`): shims `CString` (Format, Tokenize, Find, Mid…), `IniFile` (roundtrip, Unicode, seções), `FormatNumber`/`GetSIPURI`/`URLMask` com casos extraídos do comportamento original, repositórios SQLite.
2. **Integração SIP headless**: Asterisk ou FreeSWITCH em contêiner Docker/Podman com ramais de teste; segundo agente pjsua (ou `sipp`) para cenários automatizados: registro (UDP/TCP/TLS), chamada de saída/entrada, hold/unhold, DTMF (3 métodos), transferência cega/assistida, conferência a 3, SRTP, MWI, MESSAGE, presença.
3. **Comparação com o original**: MicroSIP 3.22.16 rodando em VM Windows (ou Wine) com a mesma conta; comparar traces SIP (`sngrep`) e comportamento da UI.
4. **Manual de áudio**: PipeWire com fone USB, Bluetooth e placa onboard; troca a quente; eco.
5. **Ambiente**: Ubuntu 26.04 GNOME Wayland e Xorg/XWayland.

---

## 10. Dependências de sistema previstas (Ubuntu 26.04)

> Lista definitiva e instalável em `scripts/install-deps.sh`; versões reais na seção "Ambiente de desenvolvimento verificado" do diário. `sipp` e `libilbc` não existem nos repositórios do Ubuntu 26.04; o FFmpeg é o 8.0 (risco de incompatibilidade com o pjproject 2.15.1).

```
build-essential cmake ninja-build pkg-config git
qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools libqt6dbus6 qtkeychain-qt6-dev
libssl-dev libsrtp2-dev libopus-dev libbcg729-dev libspeex-dev libspeexdsp-dev libgsm1-dev libmp3lame-dev
libasound2-dev libpulse-dev
libvpx-dev libopenh264-dev libavcodec-dev libavformat-dev libswscale-dev libavutil-dev libv4l-dev libsdl2-dev
libsqlite3-dev libsqlitecpp-dev libjsoncpp-dev
libhidapi-dev libudev-dev
libgtest-dev
# ferramentas de teste
sngrep sipp podman (ou docker)
```

Ambiente verificado na análise: Ubuntu 26.04.1 LTS, GCC 15.2, CMake 4.2.3, sessão GNOME sobre Wayland.

---

## 11. Decisões em aberto (a confirmar com o responsável pelo projeto)

1. **Nome e identidade**: "MySimpleSIP" como nome do produto? Ícone próprio?
2. **Vídeo**: manter a variante com vídeo (como `_GLOBAL_VIDEO`) ou começar só com áudio ("Lite") e adicionar vídeo na 0.8?
3. **Distribuição**: `.deb` apenas, ou também Flatpak/AppImage/Snap?
4. **Servidor de atualização/crash report**: desativar ou criar endpoint próprio?
5. **Wayland nativo vs XWayland** como padrão (recomendação: XWayland por fidelidade).
6. **Repositório remoto** (GitHub/GitLab) e licença final (GPL-2.0-or-later recomendada, igual ao original).
