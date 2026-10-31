/*
 * config_site.h do MySimpleSIP para o pjproject 2.15.1.
 * Copiado para pjlib/include/pj/config_site.h por scripts/build-pjproject.sh.
 * Objetivo: aproximar a configuração do binário do MicroSIP 3.22.12
 * (ver docs/ANALISE_E_PLANO.md §2.4).
 */

/* Vídeo (o MicroSIP é compilado com _GLOBAL_VIDEO) */
#define PJMEDIA_HAS_VIDEO                   1

/* SRTP com SDES e DTLS (transport_srtp_sdes.c / transport_srtp_dtls.c no binário) */
#define PJMEDIA_HAS_SRTP                    1
#define PJMEDIA_SRTP_HAS_DTLS               1

/* Codecs presentes no binário além dos padrões */
#define PJMEDIA_HAS_G7221_CODEC             1
#define PJMEDIA_HAS_L16_CODEC               1

/* VP9 (desligado por padrão no pjproject; o MicroSIP oferece VP8 e VP9).
 * Obs.: os payload types de vídeo do upstream (H263-1998/96, H264/97, VP8/102,
 * VP9/105) diferem dos do MicroSIP (98/99/100/101); o port localiza os codecs
 * pelo nome em vez de usar IDs fixos. */
#define PJMEDIA_HAS_VPX_CODEC_VP9           1

/* Cancelamento de eco: o MicroSIP usa o WebRTC AEC (echo_webrtc.c no binário) e não tem
 * o Speex AEC. Com o Speex AEC ligado (padrão do pjproject), ele é escolhido primeiro e,
 * com o PipeWire, imprime "Auto-filling the buffer (your application is buggy and/or got
 * xruns)". Desligado, o padrão (PJMEDIA_ECHO_DEFAULT) passa a ser o WebRTC AEC. */
#define PJMEDIA_HAS_SPEEX_AEC               0

/* Mensagens SIP grandes (TCP/TLS, SDP com vídeo e muitos codecs) */
#define PJSIP_MAX_PKT_LEN                   8000

/* Presença/BLF: lista de contatos pode ser grande (diretório de usuários) */
#define PJSUA_MAX_BUDDIES                   1000

/* Logs do pjsip disponíveis até o nível 5 (o MicroSIP grava log opcional) */
#define PJ_LOG_MAX_LEVEL                    5
