# Manual do MySimpleSIP

O MySimpleSIP é um softphone SIP para Linux: faz e recebe chamadas de voz e vídeo, troca
mensagens, mostra a presença dos contatos e funciona com PBX (Asterisk, FreePBX, Issabel,
3CX...) e provedores de telefonia IP. É o port para Linux do [MicroSIP](https://www.microsip.org)
3.22.16 e tem as mesmas telas, opções e comportamento.

Este manual descreve o uso do aplicativo. Para instalar, veja o
[guia de instalação](INSTALACAO.md). Os botões **?** das janelas de configuração abrem este
manual direto na explicação do campo.

## Sumário

1. [Primeiros passos](#primeiros-passos)
2. [A janela principal](#a-janela-principal)
3. [Fazer e receber chamadas](#fazer-e-receber-chamadas)
4. [Durante a chamada](#durante-a-chamada)
5. [Mensagens](#mensagens)
6. [Contatos e presença](#contatos-e-presença)
7. [Registros de chamadas](#registros-de-chamadas)
8. [Botões de atalho](#botões-de-atalho)
9. [Conta SIP](#conta-sip)
10. [Configurações](#configurações)
11. [Fone de ouvido, teclas de mídia e bandeja](#fone-de-ouvido-teclas-de-mídia-e-bandeja)
12. [Teclado e linha de comando](#teclado-e-linha-de-comando)
13. [Arquivos, cópia de segurança e atualizações](#arquivos-cópia-de-segurança-e-atualizações)
14. [Solução de problemas](#solução-de-problemas)
15. [Sobre e licença](#sobre-e-licença)

---

## Primeiros passos

1. Abra o MySimpleSIP pelo menu de aplicativos (ou `mysimplesip` no terminal).
2. Na primeira execução abre a janela **Conta**. Preencha pelo menos **Servidor SIP**,
   **Usuário**, **Domínio** e **Senha**, com os dados do seu PBX ou provedor (veja
   [Conta SIP](#conta-sip)), e clique em **Salvar**.
3. A barra de status, embaixo da janela, mostra **Online** quando a conta registra. Se
   mostrar um erro, confira os dados (veja [Solução de problemas](#solução-de-problemas)).
4. Digite um número na aba **Telefone** e clique em **Chamar**.

Sem conta, o MySimpleSIP ainda pode chamar direto por endereço IP (`192.168.0.10` ou
`sip:1001@192.168.0.10`) se a [conta local](#enablelocal) estiver habilitada.

## A janela principal

A janela tem três abas e um botão de menu (▾) no canto superior direito.

- **Telefone** — o discador: campo do número (com os últimos números discados), teclado,
  botões **Chamar**, **Chamada de vídeo** e **Mensagem**, controles de volume do alto-falante
  e do microfone (com medidores de nível durante a chamada) e, embaixo, os botões de estado.
- **Registros** — o histórico de chamadas (veja [Registros de chamadas](#registros-de-chamadas)).
- **Contatos** — a agenda (veja [Contatos e presença](#contatos-e-presença)).

**Barra de status:** mostra o estado da conta (Online, Ausente, Não perturbe, erro de
registro...) e, à direita, informações da chamada atual.

**Botões de estado do discador** (aparecem conforme as [Configurações](#configurações)).
Quando um deles está ligado, fica colorido:

| Botão | Cor quando ligado | O que faz |
|---|---|---|
| **DND** | laranja | *Não perturbe*: recusa todas as chamadas recebidas. Veja [Bloquear chamada entrante](#denyincoming). |
| **FWD** | azul | Encaminha as chamadas recebidas para o número definido em [Encaminhamento de chamadas](#forwarding). |
| **AC** | verde | *Conferência automática*: novas chamadas atendidas entram na conferência. |
| **AA** | verde | *Atendimento automático*: atende sozinho as chamadas recebidas. Veja [Atendimento automático](#autoanswer). |
| **CONF** | azul | Abre o menu de conferência; fica azul enquanto houver uma conferência em andamento. |
| **REC** | vermelho | Inicia/para a gravação da chamada atual; fica vermelho enquanto grava. |
| **Correio de voz** | — | Liga para o [número do correio de voz](#voicemail); acende quando há mensagens novas. |

**Menu (▾):** contas (editar, adicionar, trocar), Configurações, Atalhos, Sempre no topo,
Exportar/Importar, Ver arquivo de log (se o log estiver habilitado), Visitar site (a página do
projeto no GitHub), Ajuda (este manual), Sobre e Sair.

Fechar a janela (X) não encerra o programa: ele continua na [bandeja](#bandeja). Para sair de
verdade, use **menu → Sair** (Ctrl+Q).

## Fazer e receber chamadas

### Fazer uma chamada

Digite o número (ou nome de usuário SIP) e pressione **Enter** ou clique em **Chamar**. Também
dá para chamar:

- com duplo clique num contato ou num registro (a ação do duplo clique é definida em
  [Lista de ação padrão](#defaultaction));
- pelo menu de contexto (botão direito) de contatos e registros: **Chamar**,
  **Chamada de vídeo**, **Mensagem**;
- por um link `sip:`, `tel:` ou `callto:` num site ou e-mail (se o MySimpleSIP for o
  aplicativo padrão para esses links);
- pela linha de comando (`mysimplesip 1001`, veja [Teclado e linha de comando](#teclado-e-linha-de-comando)).

O número passa pelo [prefixo de discagem](#dialingprefix) e pelo [plano de discagem](#dialplan)
da conta antes de ser enviado. Para chamar sem nenhuma transformação, escreva o endereço entre
`<` e `>`: `<sip:1001@pbx.exemplo.com>`.

### Receber uma chamada

Uma chamada recebida abre uma pequena janela com o nome e o número de quem liga e os botões
**Atender**, **Atender com Vídeo** (em chamadas de vídeo) e **Recusar**. O toque sai no
dispositivo escolhido em [Ouvir toque em](#ringtone). Com várias chamadas ao mesmo tempo, cada
uma tem a sua janela.

- **F2** atende e **F4** desliga, com a janela do MySimpleSIP em foco.
- Com [teclas de mídia](#handlemediabuttons) ligadas, *tocar/pausar* atende.
- Com [fone HID](#headsetsupport), o botão de atender do fone atende.
- Com [Aparecer no topo ao receber chamada](#bringtofrontonincoming), a janela principal vem
  para frente.

Se uma chamada chega quando você já está em outra, ouve-se o aviso de
[chamada em espera](#callwaiting).

## Durante a chamada

Cada chamada abre uma aba na janela de chamadas, com o nome/número, a duração e os botões:

- **Espera** — coloca a chamada em espera (e retira dela). Ao atender uma segunda chamada, a
  primeira vai para a espera automaticamente.
- **Mudo** — o botão do microfone no discador silencia o microfone; o do alto-falante silencia
  o que você ouve (Ctrl+I e Ctrl+O).
- **Teclado (DTMF)** — os dígitos digitados no discador durante a chamada são enviados como
  tons DTMF, para menus de atendimento ("digite 1 para..."). O método de envio é definido em
  [Método DTMF](#dtmfmethod).
- **Transferir** — abre o menu de transferência:
  - **Transferência cega:** digite o número de destino; a chamada é passada direto, sem
    falar antes com o destino.
  - **Transferência assistida:** ligue primeiro para o destino (a chamada original vai para
    a espera), converse e depois escolha **Transferir** para juntar as duas pontas.
  - Em [Modo chamada única](#singlemode), a transferência usa os
    [códigos de recurso](#featurecodes) do PBX.
- **Conferência** (botão **CONF** ou o menu da chamada) — **Adicionar** liga para mais uma
  pessoa e a junta à conversa; **Juntar** une chamadas que já estão em andamento. Na
  conferência, cada participante ouve todos os outros; **Separar** tira uma chamada da
  conferência. O mixer de áudio é o do próprio MySimpleSIP, então não precisa de recurso de
  conferência no PBX.
- **REC** — grava a chamada (veja [Gravação de chamada](#recording)).
- **Vídeo** — numa chamada de voz, o botão de vídeo pede ao outro lado para ativar o vídeo.
- **Desligar** — encerra a chamada (F4 desliga todas).

### Videochamada

Use **Chamada de vídeo** no discador (ou no menu de contatos e registros), ou **Atender com
Vídeo** numa chamada recebida. O vídeo de quem está do outro lado abre numa janela própria, que
pode ser redimensionada; fechá-la só a esconde, sem desligar. A sua câmera aparece na janela
**Vídeo local**, que fecha sozinha ao fim da última chamada. Câmera, codec e qualidade ficam em
[Vídeo](#video).

### Gravação

<a name="recording"></a>As gravações vão para a pasta definida em **Gravação de chamada** nas
Configurações (padrão: a pasta `Recordings` na sua Área de Trabalho, como no MicroSIP), em MP3 ou WAV, com a data, a
hora e o número no nome do arquivo. Com a caixa **Gravação de chamada** marcada, todas as
chamadas são gravadas automaticamente; com a caixa **REC** marcada, o botão **REC** aparece no
discador para gravar quando você quiser. Avise o outro lado quando gravar: em muitos lugares
isso é exigido por lei.

## Mensagens

Clique em **Mensagem** no discador (ou no menu de um contato) para abrir uma conversa por
mensagens SIP (SIP MESSAGE). As mensagens recebidas abrem a conversa automaticamente, piscam
o ícone da bandeja e ficam na aba do contato. Durante uma chamada, a mesma aba mostra a
chamada e a conversa.

O PBX precisa aceitar mensagens SIP entre ramais (no Asterisk/FreePBX, por exemplo, isso é
uma opção do ramal). Para não receber mensagens, marque [Desativar mensagens](#dismess).

## Contatos e presença

A aba **Contatos** guarda nome, número e dados adicionais (telefones, e-mail, endereço,
observação) de cada contato.

- **Adicionar:** botão direito → **Adicionar**, ou pelo menu de um registro de chamada.
  **Favorito** coloca o contato no topo da lista.
- **Editar / Excluir:** botão direito no contato. **Delete** exclui os selecionados.
- **Pesquisar:** digite no campo de busca acima da lista.
- **Presença:** com **Inscrição de presença** marcada no contato, o ícone mostra se ele está
  online, ocupado (em chamada), ausente ou offline — se o PBX tiver presença (BLF) habilitada.
- **Exportar / Importar:** botão direito → **Exportar** (CSV ou XML) ou **Importar** (CSV),
  nos mesmos formatos do MicroSIP.

Se o seu PBX publica uma lista de ramais, informe o endereço em
[Diretório de usuários](#directory) para os ramais aparecerem nos contatos sozinhos.

## Registros de chamadas

A aba **Registros** lista as chamadas feitas, recebidas e perdidas, com data, duração e
informações de encaminhamento. Duplo clique repete a chamada; o botão direito permite chamar,
mandar mensagem, adicionar aos contatos, copiar o número ou excluir. As chamadas perdidas
ficam destacadas e o ícone da bandeja muda até você abrir a aba.

## Botões de atalho

**Menu → Atalhos** (Ctrl+S) cria até 8 botões (×2 colunas) no discador para ações de um
clique: discar um ramal, transferir para ele, ver a presença dele.

| Campo | Para que serve |
|---|---|
| **Habilitar** | Mostra os atalhos no discador. |
| **Posição inferior** | Mostra os atalhos embaixo do discador, em vez de ao lado. |
| **Etiqueta** | O texto do botão. |
| **Número** | O número ou endereço usado pela ação. |
| **Tipo** | A ação do botão (veja abaixo). |
| **Alterar** | Botão de duas posições (veja abaixo). |
| **BLF** | Mostra no botão a presença do número (veja abaixo). |

<a name="shortcuttype"></a>**Tipo:** **Chamar** ou **Chamada de vídeo** discam o número;
**Mensagem** abre a conversa; **DTMF** envia o número como tons durante a chamada (ex.: um
ramal de menu ou uma senha); **Transferência cega**, **Transferência assistida** e
**Conferência** agem sobre a chamada atual; os tipos **Chamar/...** discam quando não há
chamada e transferem ou fazem conferência quando há.

<a name="toggle"></a>**Alterar:** transforma o atalho num botão de liga/desliga com dois
números: **Número** é discado ao ligar e **Número 2** ao desligar. Serve para códigos de
recurso que ativam e desativam algo no PBX (ex.: `*76` e `*77` para não perturbe, ou
login/logout de fila).

<a name="blf"></a>**BLF** (*Busy Lamp Field*): o botão mostra o estado do ramal (livre,
tocando, em chamada), como as teclas de um telefone de mesa. O PBX precisa permitir a
assinatura de presença do ramal (no Asterisk, *hint* configurado). Um clique num ramal que
está tocando pode capturar a chamada, se o [código de captura](#featurecodes) estiver
configurado.

## Conta SIP

**Menu → Editar Conta** (Ctrl+M) ou **Adicionar Conta**. Cada conta é guardada separadamente;
a ativa é a que aparece marcada no menu. A senha fica no chaveiro do GNOME, não no arquivo de
configuração.

### Nome da Conta
Um nome para identificar a conta no menu (ex.: "Escritório"). Opcional.

<a name="sipserver"></a>
### Servidor SIP
O endereço do PBX ou do provedor, onde a conta se registra: nome ou IP, com porta opcional
(`pbx.exemplo.com`, `192.168.0.10:5080`). Se ficar vazio, o MySimpleSIP não registra e só faz
chamadas diretas pelo domínio.

<a name="sipproxy"></a>
### Proxy SIP
Um proxy de saída por onde passam todas as mensagens SIP (*outbound proxy*). Deixe vazio, a
não ser que o provedor peça.

<a name="username"></a>
### Usuário
O ramal ou nome de usuário SIP (a parte antes do `@` no endereço `1001@pbx.exemplo.com`).
Obrigatório.

<a name="domain"></a>
### Domínio
O domínio SIP da conta (a parte depois do `@`). Quase sempre é o mesmo do Servidor SIP.
Obrigatório.

<a name="login"></a>
### Login
O usuário de autenticação, quando é diferente do Usuário (alguns provedores usam um código
de autenticação separado). Deixe vazio para usar o próprio Usuário.

<a name="password"></a>
### Senha
A senha da conta SIP. **exibir senha** mostra o que foi digitado. Fica guardada no chaveiro do
GNOME (aplicativo *Senhas e chaves*, item "MySimpleSIP").

<a name="name"></a>
### Nome de Exibição
O nome que aparece para quem recebe as suas chamadas (se o PBX não o substituir).

<a name="voicemail"></a>
### Nº do Correio de Voz
O número do correio de voz (ex.: `*97`). Com ele preenchido, aparece o botão de correio de voz
no discador, que acende quando o PBX avisa que há mensagens novas.

<a name="dialingprefix"></a>
### Prefixo de Discagem
Um prefixo acrescentado aos números de telefone comuns (com mais de 3 dígitos) antes de
discar, como o `0` para linha externa. Um `+` no início do número é trocado pelo prefixo.
Ramais curtos e endereços SIP não recebem o prefixo.

<a name="dialplan"></a>
### Plano de discagem
Regras que transformam o número antes de discar, separadas por `|`. Em cada regra:

- `x` vale qualquer dígito e `.` repete o que vem antes (`x.` = um ou mais dígitos);
- `<a:b>` troca `a` por `b` (com `b` vazio, `a` é removido);
- vale a primeira regra que combinar com o número inteiro.

Exemplos:

| Plano | Efeito |
|---|---|
| `<0:+5511>xxxxxxxx` | 0 seguido de 8 dígitos → +5511 e os 8 dígitos |
| `<00:+>x.` | 00 no início vira + |
| `<9:>x.` | remove o 9 do início |
| `<0:+5511>xxxxxxxx\|<00:+>x.` | as duas primeiras regras juntas |

<a name="hidecid"></a>
### Ocultar identificador de chamadas
Pede para não mostrar o seu número a quem você chama. **Suave** envia o cabeçalho
`Privacy: id` (o PBX ou provedor decide se oculta); **Rígido** também troca o remetente por
"Anonymous". Depende de suporte do PBX ou provedor.

<a name="encryption"></a>
### Criptografia
Criptografia do áudio e do vídeo (SRTP):

- **Desabilitada** — mídia sem criptografia (o mais compatível);
- **Opcional SRTP (RTP/AVP)** — criptografa se o outro lado aceitar;
- **Obrigatório SRTP (RTP/SAVP)** — só aceita chamadas criptografadas;
- **DTLS-SRTP/SRTP** e **DTLS-SRTP** — troca de chaves por DTLS (usado por WebRTC e alguns PBX).

Para proteger também a sinalização (números, senhas), use o transporte **TLS**.

<a name="transport"></a>
### Transporte
O protocolo da sinalização SIP: **UDP** (o padrão e o mais comum), **TCP**, **UDP+TCP** ou
**TLS** (criptografado; geralmente na porta 5061). Use o que o PBX ou provedor indicar.

<a name="publicaddress"></a>
### Endereço Público
O endereço IP que vai nas mensagens SIP. **Auto** deixa o MySimpleSIP descobrir (o
recomendado). Escolha um IP da lista para forçar uma interface de rede específica, ou digite o
IP público quando o roteador não ajudar com o NAT.

**Atualizar Registro** é o intervalo, em segundos, para renovar o registro (padrão 300).
**Manter ativo** é o intervalo dos pacotes que mantêm a conexão aberta no NAT (padrão 15).

<a name="publishpresence"></a>
### Publicar presença
Publica o seu estado (online, ocupado, ausente) para quem assina a sua presença. Desmarque se
o PBX não aceitar publicação (PUBLISH) e mostrar erros.

<a name="allowrewrite"></a>
### Permitir reescrever IP
Atualiza o endereço de contato quando o servidor informa um IP público diferente (útil atrás
de NAT). Desmarque se a conta registrar e cair repetidamente.

<a name="ice"></a>
### ICE
Usa ICE para descobrir o melhor caminho para o áudio e o vídeo atravessarem NAT e firewalls.
Ajuda em conexões diretas sem PBX ou com STUN; com a maioria dos PBX não é necessário.

<a name="sessiontimers"></a>
### Desabilitar timers de sessão
Desliga os *session timers* (RFC 4028), renovações periódicas da chamada. Marque se as
chamadas caírem sempre após o mesmo tempo (ex.: 15 ou 30 minutos).

## Configurações

**Menu → Configurações** (Ctrl+P). As mudanças valem ao clicar em **Salvar**.

<a name="singlemode"></a>
### Modo Chamada Única
Permite só uma chamada por vez, como num telefone simples: uma segunda chamada recebe
"ocupado", e a transferência e a conferência usam os [códigos de recurso](#featurecodes) do
PBX em vez dos recursos do próprio MySimpleSIP.

<a name="ringtone"></a>
### Toque de chamada e dispositivos de áudio
- **Toque de Chamada:** o arquivo WAV tocado nas chamadas recebidas (**...** escolhe outro,
  **X** volta ao padrão). **Volume do toque** ajusta o volume dele.
- **Ouvir toque em:** onde o toque sai (ex.: alto-falantes, enquanto a conversa vai no fone).
- **Ouvir chamada em** e **Microfone:** os dispositivos da conversa.

**Padrão** segue o dispositivo escolhido em *Configurações do Ubuntu → Som*, inclusive quando
um fone Bluetooth conecta — é o recomendado.

<a name="microphoneamplification"></a>
### Amplificação do microfone
Para microfones fracos: a metade de cima do controle de volume do microfone passa a
amplificar o sinal além de 100%. Volumes altos podem distorcer.

<a name="softwareleveladjustment"></a>
### Ajuste de volume por software
Os controles de volume do discador mudam o volume dentro do MySimpleSIP, em vez de mudar o
volume do dispositivo no sistema.

<a name="audiocodecs"></a>
### CODECs de áudio
À esquerda, os codecs disponíveis; à direita, os habilitados, em ordem de preferência
(na ordem em que foram habilitados). **Barra de espaço** habilita o selecionado e **Delete**
desabilita; para mudar a ordem, desabilite e habilite de novo.
O padrão (G.711 A-law e µ-law, PCMA/PCMU) funciona com praticamente qualquer PBX. Para
economizar banda, use G.729 ou Opus (se o PBX aceitar); para mais qualidade, G.722 ou Opus.
**Opus 2ch** usa Opus estéreo.

<a name="vad"></a>
### VAD
*Detecção de atividade de voz*: não envia áudio nos silêncios, economizando banda. Pode
cortar o início das falas; deixe desmarcado se ouvirem a sua voz picotada.

<a name="ec"></a>
### EC
*Cancelamento de eco*: evita que quem está do outro lado ouça a própria voz voltando pelo seu
alto-falante. Deixe marcado, a não ser que use um fone com cancelamento próprio.

<a name="forcecodec"></a>
### Forçar CODEC de Entrada
Nas chamadas recebidas, usa a sua ordem de codecs em vez da ordem de quem liga.

<a name="video"></a>
### Vídeo
- **Desativar vídeo:** esconde os botões de vídeo e recusa vídeo nas chamadas.
- **Câmera:** a câmera usada. **P** abre a prévia para testar.
- **CODEC de vídeo:** o codec preferido (H.264, H.263+, VP8 ou VP9). H.264 é o mais compatível.
- **Amostragem do vídeo:** a taxa de bits em kbit/s (mais alta = melhor imagem e mais banda).

<a name="ports"></a>
### Portas
- **Porta de Origem:** a porta local da sinalização SIP (padrão 5060; 0 = automática). Mude se
  outro programa SIP já usar a 5060.
- **rport:** pede ao servidor para responder na porta de onde a mensagem saiu (ajuda com NAT).
  Deixe marcado.
- **Portas RTP:** a faixa de portas do áudio e do vídeo (padrão automática). Defina uma faixa
  se o firewall precisar de portas fixas.

<a name="dnssrv"></a>
### Servidor DNS e DNS SRV
**Servidor DNS** usa servidores de nomes específicos em vez dos do sistema. **DNS SRV**
procura o servidor SIP pelos registros SRV do domínio (quando o provedor publica vários
servidores ou portas).

<a name="stunserver"></a>
### Servidor STUN
Um servidor STUN (ex.: `stun.l.google.com:19302`) descobre o seu IP público, para o áudio
atravessar o NAT do roteador. Use quando a chamada completa mas o áudio só vai num sentido ou
não vai.

<a name="dtmfmethod"></a>
### Método DTMF
Como os dígitos digitados durante a chamada são enviados:

- **Auto** — RFC 2833 se o outro lado aceitar, senão no áudio (recomendado);
- **No áudio (in-band)** — como tons no próprio áudio (pode falhar com codecs comprimidos);
- **RFC2833** — como eventos no fluxo de áudio (o mais comum em PBX);
- **SIP-INFO** — como mensagens SIP INFO.

Se os menus de atendimento não reconhecem os dígitos, tente outro método.

<a name="autoanswer"></a>
### Atendimento automático
- **Não** — nunca atende sozinho;
- **Botão de controle** — mostra o botão **AA** no discador; com ele ligado, atende tudo;
- **Cabeçalho SIP** — atende quando o PBX pede, pelos cabeçalhos `X-AUTOANSWER: true` ou
  `Call-Info` com *auto answer* ou *answer-after=N* (usados em interfone e *paging*);
- **Todas as chamadas** — atende sempre.

O botão **...** define o **Atraso** (segundos antes de atender), quais **Números de origem**
são atendidos e se atende também com **Chamadas ativas**.

<a name="forwarding"></a>
### Encaminhamento de Chamadas
Encaminha as chamadas recebidas para outro número: **Não**, **Botão de controle** (o botão
**FWD**, quando há número de encaminhamento) ou **Todas as chamadas**. Defina o número e,
opcionalmente, um tempo em segundos: com tempo, o MySimpleSIP toca e só encaminha se você não
atender nesse prazo.

<a name="featurecodes"></a>
### Códigos de recurso
O link **Códigos de recurso** define os códigos do PBX para **Captura de chamada** (ex.:
`**` ou `*8`, usado pelos atalhos BLF), **Transferência cega** e **Transferência assistida**
(usados no [Modo Chamada Única](#singlemode)). Os valores dependem do PBX.

<a name="denyincoming"></a>
### Bloquear chamada entrante
- **Não** — aceita todas;
- **Botão de controle** — mostra o botão **DND** (não perturbe);
- **Usuário diferente / Domínio diferente / Usuário ou domínio diferente** — recusa chamadas
  dirigidas a outro usuário ou domínio que não o da conta (evita chamadas indevidas de
  varredores SIP);
- **Domínio remoto diferente** — recusa chamadas de domínios diferentes do da conta;
- **Todas as chamadas** — recusa todas.

<a name="directory"></a>
### Diretório de usuários
Endereço (URL) de uma lista de usuários em XML ou JSON publicada pelo PBX ou por um servidor
da empresa. Os usuários aparecem na aba Contatos, com presença, e são atualizados
periodicamente. **Desativar pesquisa de nomes no diretório** impede usar essa lista para
mostrar o nome de quem liga.

<a name="defaultaction"></a>
### Lista de ação padrão
O que acontece no duplo clique em contatos e registros: **Padrão** (chamar), **Chamar**,
**Chamada de vídeo** ou **Mensagem**.

<a name="handlemediabuttons"></a>
### Usar teclas multimídia
Durante uma chamada (ou com uma chamada tocando), as teclas de mídia do teclado e os botões de
fones Bluetooth vão para o MySimpleSIP: *tocar/pausar* atende ou põe em espera, *parar* recusa
ou desliga. Fora das chamadas, as teclas continuam com o seu player de música.

<a name="headsetsupport"></a>
### Suporte para Fone de Ouvido
Usa os botões e as luzes de fones USB de escritório (Jabra, Poly, EPOS, Yealink...): atender e
desligar pelo gancho, mudo e as luzes de toque, chamada e mudo. Precisa liberar o acesso ao
fone uma vez (veja [Fone de ouvido](#fone-de-ouvido-usb-hid)).

<a name="soundevents"></a>
### Tons do teclado
Toca localmente os tons das teclas digitadas no discador (também fora das chamadas) e o som
de fim de chamada.

<a name="log"></a>
### Habilitar arquivo de log
Grava um registro detalhado (inclusive as mensagens SIP) em
`~/.local/state/MySimpleSIP/MySimpleSIP_log.txt`, para diagnosticar problemas. **Menu → Ver
arquivo de log** o abre. Desligue depois: o arquivo cresce e contém números discados.

<a name="bringtofrontonincoming"></a>
### Aparecer no topo ao receber chamada
Traz a janela principal para frente quando chega uma chamada. (No Wayland, o GNOME pode só
avisar que o aplicativo pede atenção.)

<a name="enablelocal"></a>
### Habilitar conta local
Cria a conta local (**menu → Editar Conta Local**, Ctrl+L), para chamadas diretas por IP
(`sip:1001@192.168.0.10`) sem servidor, ao lado da conta registrada.

<a name="randomanswerbox"></a>
### Popup - Posição Aleatória
Abre a janela de chamada recebida numa posição aleatória da tela, em vez de sempre no mesmo
lugar (útil com várias chamadas simultâneas).

<a name="callwaiting"></a>
### Outras opções
- **Chamada em Espera:** toca um aviso quando chega uma chamada durante outra.
- <a name="dismess"></a>**Desativar mensagens:** recusa mensagens SIP e esconde os botões de
  mensagem.
- **Suporte para vários monitores:** permite que as janelas fiquem em qualquer monitor (senão
  voltam para o principal).
- **Lidar com alterações de IP:** refaz o registro quando a rede muda (troca de Wi-Fi,
  cabo, VPN, volta da suspensão).
- **Verificar por atualizações:** com que frequência procurar versão nova (veja
  [atualizações](#atualizações)).
- **Executar na inicialização do sistema:** abre o MySimpleSIP ao entrar na sessão
  (cria `~/.config/autostart/mysimplesip.desktop`).

## Fone de ouvido, teclas de mídia e bandeja

### Fone de ouvido USB (HID)

Para usar os botões e luzes de um fone USB de escritório:

1. Libere o acesso ao fone, uma vez (o pacote `.deb` já faz isso):
   `sudo ./scripts/install-hid-rules.sh` na pasta do código-fonte. A regra só libera
   dispositivos com controles de telefonia, não teclados ou mouses.
2. Desconecte e conecte o fone.
3. Marque [Suporte para Fone de Ouvido](#headsetsupport) nas Configurações.

Tirar do gancho atende a chamada que toca (ou disca o número digitado); pôr no gancho
desliga; o mudo e as luzes acompanham o aplicativo.

### Teclas de mídia

Veja [Usar teclas multimídia](#handlemediabuttons).

<a name="bandeja"></a>
### Ícone na bandeja

O ícone ao lado do relógio mostra o estado: colorido (online), cinza (sem conta ou
desconectado) e com aviso (chamada perdida ou mensagem nova). Um clique mostra ou esconde a
janela; o botão direito abre o menu. No Ubuntu, a bandeja depende da extensão *Ubuntu
AppIndicators* (já ativa por padrão).

## Teclado e linha de comando

| Tecla | Ação |
|---|---|
| **Enter** | Chamar o número digitado |
| **F2** | Atender |
| **F4** | Desligar todas as chamadas |
| **Ctrl+Tab** / **Ctrl+Shift+Tab** | Próxima / anterior aba |
| **Ctrl+M** | Editar conta |
| **Ctrl+L** | Editar conta local |
| **Ctrl+P** | Configurações |
| **Ctrl+S** | Atalhos |
| **Ctrl+I** (ou Ctrl+\`) | Microfone mudo |
| **Ctrl+O** | Alto-falante mudo |
| **Ctrl+W** | Visitar site |
| **Ctrl+Q** | Sair |
| **F10** | Ir para o botão de menu |
| **0–9, \*, #** | Digitar no discador / enviar DTMF na chamada |

Com o MySimpleSIP aberto, comandos no terminal são repassados a ele — úteis em atalhos de
teclado do GNOME e em scripts:

| Comando | O que faz |
|---|---|
| `mysimplesip 1001` / `sip:1001@pbx` / `tel:+5511...` | disca |
| `mysimplesip "1001 Olá"` | envia a mensagem "Olá" para 1001 |
| `mysimplesip /answer` | atende |
| `mysimplesip /hangupall` | desliga todas |
| `mysimplesip /hangupincoming` / `/hangupcalling` | recusa as recebidas / cancela as que estão chamando |
| `mysimplesip /dtmf:123` | envia DTMF |
| `mysimplesip /transfer:1002` | transfere a chamada atual |
| `mysimplesip 1001 /account:2` | troca para a conta 2 e disca |
| `mysimplesip msip:hold` | espera (alterna) |
| `mysimplesip msip:micmute` / `msip:micunmute` / `msip:micmuteclick` | microfone mudo / com som / alterna |
| `mysimplesip msip:speakmute` / `msip:speakunmute` / `msip:speakmuteclick` | saída muda / com som / alterna |
| `mysimplesip msip:micup` / `micdown` / `speakup` / `speakdown` | volumes |
| `mysimplesip msip:minimize` | esconde na bandeja |
| `mysimplesip --minimized` | inicia escondido na bandeja |
| `mysimplesip /exit` | fecha o MySimpleSIP |
| `mysimplesip /reset` | apaga configurações, contas, histórico e contatos (pede confirmação) |
| `mysimplesip --version` | mostra a versão |

## Arquivos, cópia de segurança e atualizações

| Pasta | Conteúdo |
|---|---|
| `~/.config/MySimpleSIP/` | `MySimpleSIP.ini` — configurações e contas (sem as senhas) |
| `~/.local/share/MySimpleSIP/` | contatos e histórico |
| `~/Área de Trabalho/Recordings` | gravações (ou a pasta escolhida nas Configurações) |
| `~/.local/state/MySimpleSIP/` | arquivo de log |
| chaveiro do GNOME | as senhas das contas (item "MySimpleSIP") |

**Exportar/Importar** (no menu) salva e restaura a configuração, o histórico e os contatos —
use para passar para outro computador. O arquivo de configuração é o mesmo formato do
`microsip.ini`, então dá para importar a configuração de um MicroSIP do Windows.

<a name="atualizações"></a>
### Atualizações

Conforme **Verificar por atualizações**, o MySimpleSIP consulta a última versão publicada na
página de [Releases](https://github.com/GGTY81/MySimpleSIP/releases) e, se houver uma mais
nova, oferece abrir a página. Nenhum dado seu é enviado. Para atualizar, baixe o `.deb` novo e
instale com `sudo apt install ./mysimplesip_<versão>_amd64.deb`, ou, se compilou do código,
`git pull` e `./build.sh` (veja o [guia de instalação](INSTALACAO.md#atualizar)).

O MySimpleSIP não envia telemetria nem relatórios de falha.

## Solução de problemas

**Erro de registro na barra de status.** Confira servidor, usuário, domínio e senha. Se o
PBX usar outra porta ou transporte, ajuste na [Conta](#transport). Habilite o
[log](#log) e procure as respostas do servidor (`401`, `403`, `404`, `408`).

**A chamada completa, mas não há áudio ou ele vai num só sentido.** Problema de NAT ou
firewall: tente um [servidor STUN](#stunserver), [Permitir reescrever IP](#allowrewrite) ou o
[ICE](#ice), e confira se o firewall libera as [portas RTP](#ports).

**Não ouço nada ou o som sai no lugar errado.** Deixe os dispositivos em **Padrão** e escolha
o dispositivo em *Configurações do Ubuntu → Som*.

**Os menus de atendimento não reconhecem os dígitos.** Mude o [Método DTMF](#dtmfmethod).

**As chamadas caem sempre após o mesmo tempo.** Marque
[Desabilitar timers de sessão](#sessiontimers) ou reduza **Manter ativo**.

**O fone USB não atende nem acende as luzes.** Veja [Fone de ouvido USB](#fone-de-ouvido-usb-hid).

**A câmera não aparece.** Confira se ela funciona em outro programa (ex.: *Câmera* do Ubuntu)
e se nenhum outro programa a está usando.

Mais problemas de instalação e compilação: [guia de instalação](INSTALACAO.md#problemas-comuns).
Se nada resolver, abra uma [issue](https://github.com/GGTY81/MySimpleSIP/issues) com o
trecho do log (apague números e senhas antes).

## Sobre e licença

**Menu → Sobre** mostra a versão e os créditos.

- **MySimpleSIP** — Copyright © 2026 Giovanni Grimaldi Torelly.
- Baseado no **MicroSIP** — Copyright © 2011-2026 MicroSIP ([www.microsip.org](https://www.microsip.org)).
- Usa **PJSIP** (pjproject, Teluu), **Qt** (The Qt Company) e as bibliotecas de código aberto
  listadas no [README](../README.md).

Software livre, distribuído sob a [GNU General Public License, versão 2 ou
posterior](../COPYING), sem nenhuma garantia.
