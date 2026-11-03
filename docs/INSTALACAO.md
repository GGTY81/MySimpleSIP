# Instalação no Ubuntu 26.04 LTS

Este guia instala o MySimpleSIP no Ubuntu 26.04 LTS (64 bits, desktop GNOME). O uso do
aplicativo está no [manual](MANUAL.md).

## Jeito mais fácil: o pacote `.deb`

1. Baixe o arquivo `mysimplesip_<versão>_amd64.deb` da última versão na página de
   [Releases](https://github.com/GGTY81/MySimpleSIP/releases).
2. No Terminal (Ctrl+Alt+T), na pasta onde o arquivo foi salvo (normalmente `Downloads`):

   ```bash
   cd ~/Downloads
   sudo apt install ./mysimplesip_*_amd64.deb
   ```

   O `apt` instala junto as bibliotecas necessárias. Use `./` antes do nome do arquivo: sem
   isso, o `apt` procura o pacote nos repositórios do Ubuntu e não o encontra.
3. Abra o **MySimpleSIP** pelo menu de aplicativos e siga para
   [Configurar a conta SIP](#5-configurar-a-conta-sip).

O pacote já registra os links `sip:`/`tel:`/`callto:` e a regra do fone USB HID. Para remover:
`sudo apt remove mysimplesip`.

## Pelo código-fonte

Os passos 1 a 4 compilam o MySimpleSIP na sua máquina. Leva uns 10 minutos, quase todos de
compilação automática.

## O que você precisa

- Ubuntu 26.04 LTS, 64 bits (x86_64), com acesso à internet.
- Uns 2 GB livres em disco (código, dependências e compilação).
- Uma conta SIP (do seu PBX ou provedor de telefonia IP) para fazer chamadas.
- Microfone e alto-falantes ou um fone (com fio, USB ou Bluetooth).

## 1. Baixar o código

Abra o **Terminal** (Ctrl+Alt+T) e execute:

```bash
sudo apt install git
git clone https://github.com/GGTY81/MySimpleSIP.git
cd MySimpleSIP
```

Isso cria a pasta `MySimpleSIP` na sua pasta pessoal (ou onde o Terminal estiver). Todos os
comandos seguintes são executados dentro dela.

## 2. Instalar as dependências

```bash
sudo ./scripts/install-deps.sh
```

O script instala, pelo `apt`, o compilador, o CMake, o Qt 6 e as bibliotecas de áudio,
vídeo, criptografia e banco de dados usadas pelo MySimpleSIP (a lista completa está no
próprio script). Só é preciso fazer isso uma vez.

## 3. Compilar

```bash
./build.sh
```

Na primeira vez, o script baixa e compila o PJSIP (pjproject 2.15.1, com verificação
SHA-256 do arquivo baixado) e depois compila o MySimpleSIP. Ao final aparece:

```
Executável: /.../MySimpleSIP/build/src/mysimplesip
```

Para conferir:

```bash
./build/src/mysimplesip --version
```

## 4. Abrir o aplicativo

```bash
./build/src/mysimplesip
```

Para que o MySimpleSIP apareça no menu de aplicativos do GNOME (e possa ser fixado no
dock), crie o atalho — não precisa de `sudo`, nada é copiado para o sistema:

```bash
./scripts/install-desktop-entry.sh
```

Para que os links de telefone (`sip:`, `tel:` e `callto:`, por exemplo num site ou num
e-mail) abram direto no MySimpleSIP, como faz o instalador do MicroSIP no Windows, use:

```bash
./scripts/install-desktop-entry.sh --default-handler
```

Para iniciar junto com o sistema, marque **Executar na inicialização do sistema** em
menu → **Configurações** (cria `~/.config/autostart/mysimplesip.desktop`).

### Ou: instalar como pacote `.deb`

Em vez de rodar de dentro da pasta, você pode gerar o pacote e instalá-lo no sistema. Ele
coloca o MySimpleSIP no menu de aplicativos, registra os links `sip:`/`tel:`/`callto:`, já
traz a regra do udev do fone USB HID e pode ser removido pelo `apt`:

```bash
./scripts/build-deb.sh                              # gera dist/mysimplesip_<versão>_amd64.deb
sudo apt install ./dist/mysimplesip_*_amd64.deb     # instala (o apt resolve as dependências)
```

Depois disso, abra pelo menu de aplicativos ou com `mysimplesip` no terminal. Para atualizar,
repita os dois comandos depois de `git pull`. Para remover: `sudo apt remove mysimplesip`.
Configurações, contatos e histórico ficam na sua pasta pessoal nos dois modos, então dá para
passar de um para o outro sem perder nada. Se usar o pacote, não precisa do
`install-desktop-entry.sh` nem do `install-hid-rules.sh` (se já tiver usado, remova com
`--remove` para não ficar com dois atalhos).

### Ícone na bandeja

O MySimpleSIP fica na bandeja do sistema (ao lado do relógio). No Ubuntu isso funciona com
a extensão **AppIndicator**, que já vem ativa. Fechar a janela só a esconde; para reabrir,
clique no ícone da bandeja e escolha **MySimpleSIP**. Para sair de verdade: menu → **Sair**.

## 5. Configurar a conta SIP

Na primeira execução o diálogo **Conta** abre automaticamente (depois: menu → **Editar
Conta**). Preencha os dados fornecidos pelo seu PBX ou provedor:

| Campo | Exemplo |
|---|---|
| Servidor SIP | `pbx.suaempresa.com.br` |
| Usuário | `1001` |
| Domínio | `pbx.suaempresa.com.br` |
| Login (opcional) | o mesmo do usuário, se o provedor não pedir outro |
| Senha | a senha do ramal |

A senha é guardada no **chaveiro do GNOME** (Senhas e chaves), e não no arquivo de
configuração. Quando a conta registra, a barra de status mostra **Online**.

## 6. Áudio

Em menu → **Configurações**, os campos **Ouvir chamada em**, **Ouvir toque em** e
**Microfone** listam os dispositivos com os nomes do sistema. **Padrão** segue o
dispositivo padrão escolhido em *Configurações do Ubuntu → Som* — inclusive quando um fone
Bluetooth conecta. Recomenda-se deixar **Padrão**.

### Teclas de mídia

Marque **Usar teclas multimídia** nas Configurações. Durante uma chamada (ou com uma
chamada tocando), as teclas de tocar/pausar e parar do teclado — e os botões de fones
Bluetooth — passam a ir para o MySimpleSIP: tocar/pausar atende ou põe em espera, parar
recusa ou desliga. Fora das chamadas, as teclas continuam com o seu player de música.

### Fone USB com botões (HID)

Fones USB de escritório (Jabra, Poly, EPOS, Yealink...) têm botão de atender/desligar,
mudo e luzes de chamada. Para o MySimpleSIP usá-los:

1. Libere o acesso ao fone (uma vez; pede a senha de administrador):

   ```bash
   sudo ./scripts/install-hid-rules.sh
   ```

   A regra do udev só libera dispositivos com controles de telefonia: teclados e mouses
   continuam protegidos. Para desfazer: `sudo ./scripts/install-hid-rules.sh --remove`.
2. Desconecte e conecte o fone de novo.
3. Marque **Suporte para Fone de Ouvido** nas Configurações.

Com o fone, tirar do gancho (botão de atender) atende a chamada que toca ou disca o número
digitado, pôr no gancho desliga, e o botão de mudo e as luzes de toque, chamada e mudo
seguem o aplicativo.

## 7. Vídeo

Em menu → **Configurações**, escolha a câmera em **Câmera** e use **P** (Visualizar) para
abrir a prévia ("Vídeo local"). Para uma videochamada, use o botão de vídeo do discador
(ou **Chamada de vídeo** no menu dos contatos e do histórico), ou **Atender com Vídeo** na janela de
chamada recebida. O vídeo de quem está do outro lado abre numa janela própria, que pode ser
redimensionada; fechá-la só a esconde, sem desligar a chamada. A prévia da câmera fecha
sozinha ao fim da última chamada.

Se a câmera não aparecer, confira se ela funciona em outro programa (ex.: o aplicativo
*Câmera* do Ubuntu) e se nenhum outro programa a está usando.

## 8. Linha de comando

Os mesmos comandos do MicroSIP funcionam no terminal (ou em atalhos de teclado do GNOME).
Com o MySimpleSIP já aberto, o comando é repassado para ele:

| Comando | O que faz |
|---|---|
| `mysimplesip 1001` / `mysimplesip sip:1001@pbx` / `tel:+5511...` | disca o número |
| `mysimplesip "1001 Olá"` | envia a mensagem SIP "Olá" para 1001 |
| `mysimplesip /answer` | atende a chamada que está tocando |
| `mysimplesip /hangupall` | desliga todas as chamadas |
| `mysimplesip /hangupincoming` / `/hangupcalling` | recusa as recebidas / cancela as que estão chamando |
| `mysimplesip /dtmf:123` | envia os dígitos DTMF |
| `mysimplesip /transfer:1002` | transfere a chamada atual para 1002 |
| `mysimplesip 1001 /account:2` | troca para a conta 2 e disca |
| `mysimplesip msip:hold` | coloca/retira a chamada da espera |
| `mysimplesip msip:micmute` / `msip:micunmute` / `msip:micmuteclick` | microfone mudo / com som / alterna |
| `mysimplesip msip:speakmute` / `msip:speakunmute` / `msip:speakmuteclick` | saída de áudio muda / com som / alterna |
| `mysimplesip msip:micup` / `msip:micdown` / `msip:speakup` / `msip:speakdown` | volume do microfone / da saída |
| `mysimplesip msip:minimize` | esconde a janela na bandeja |
| `mysimplesip --minimized` (ou `/minimized`) | inicia escondido na bandeja |
| `mysimplesip /exit` | fecha o MySimpleSIP aberto |
| `mysimplesip /reset` | apaga configurações, contas, histórico e contatos (pede confirmação duas vezes; `/resetnoask` não pede) |

Para usar `mysimplesip` sem o caminho completo, chame `./build/src/mysimplesip` ou crie um
link em `~/.local/bin`.

## Atualizar

Com o pacote: baixe o `.deb` da versão nova e instale do mesmo jeito
(`sudo apt install ./mysimplesip_*_amd64.deb`). Pelo código-fonte:

```bash
cd MySimpleSIP
git pull
./build.sh
```

Se uma atualização trouxer dependências novas, rode também `sudo ./scripts/install-deps.sh`.
Configurações, contatos e histórico ficam na sua pasta pessoal e são mantidos.

O aviso de versão nova vem do próprio aplicativo: conforme **Verificar por atualizações**
nas Configurações (semanalmente, por padrão; "Nunca" desliga), ele consulta a última versão
publicada na página de [Releases](https://github.com/GGTY81/MySimpleSIP/releases) e, se houver
uma mais nova, oferece abrir a página. Nenhum dado seu é enviado nessa consulta.

## Remover

```bash
cd MySimpleSIP
./scripts/install-desktop-entry.sh --remove   # remove o atalho do menu
sudo ./scripts/install-hid-rules.sh --remove  # remove a regra do fone HID (se instalou)
cd .. && rm -rf MySimpleSIP                   # remove o código e a compilação
```

Seus dados continuam em `~/.config/MySimpleSIP`, `~/.local/share/MySimpleSIP` e
`~/.local/state/MySimpleSIP`; apague essas pastas se quiser removê-los também. A senha fica
no chaveiro do GNOME (aplicativo *Senhas e chaves*, item "MySimpleSIP").

## Problemas comuns

**A barra de status mostra erro de registro.**
Confira servidor, usuário, domínio e senha. Se o PBX usar outra porta ou transporte (TCP,
TLS), ajuste no diálogo Conta. Para diagnóstico, habilite **Habilitar arquivo de log** nas
Configurações e veja `~/.local/state/MySimpleSIP/MySimpleSIP_log.txt`.

**Não há áudio ou ele sai no dispositivo errado.**
Deixe os três dispositivos em **Padrão** nas Configurações e escolha o dispositivo em
*Configurações do Ubuntu → Som*. O MySimpleSIP usa o PipeWire (servidor de som padrão do
Ubuntu).

**O fone USB não atende nem acende as luzes.**
Confira se a regra do udev foi instalada (`sudo ./scripts/install-hid-rules.sh`), se o fone
foi reconectado depois disso e se **Suporte para Fone de Ouvido** está marcado. Com
**Habilitar arquivo de log**, o log mostra os dispositivos encontrados (`Hid.cpp`); fones
sem controles de telefonia (só áudio) não aparecem como "Telephony Device".

**Mensagens no terminal como `Failed to register with host portal`.**
São avisos de bibliotecas do sistema e podem ser ignorados.

**A janela não aparece no lugar onde foi deixada.**
No Wayland (padrão do Ubuntu) quem decide a posição das janelas é o GNOME. Para usar a
posição salva, rode pelo XWayland: `QT_QPA_PLATFORM=xcb ./build/src/mysimplesip`.

**O ícone da bandeja não aparece.**
Verifique se a extensão *Ubuntu AppIndicators* está ativa (aplicativo *Extensões*).

**O `./build.sh` para no download do pjproject.**
O script baixa o pjproject do GitHub na primeira compilação. Confira a conexão e rode de novo:
um download incompleto é apagado e refeito sozinho. (Até a versão 0.9.1, o `./build.sh`
podia terminar sem mensagem nenhuma numa pasta recém-clonada; isso foi corrigido na 1.0.0.)

**Erro ao compilar.**
Rode de novo `sudo ./scripts/install-deps.sh` (pode faltar alguma dependência nova) e
depois `rm -rf build && ./build.sh`. Se persistir, abra uma *issue* no GitHub com a saída
do erro.
