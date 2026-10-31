/*
 * Teste de fumaça do pjproject compilado para o MySimpleSIP.
 * Inicializa o pjsua como o MicroSIP (PJCreateRaw) e lista versão,
 * dispositivos de áudio/vídeo, codecs e transportes. Não registra conta.
 */
#include <pjsua-lib/pjsua.h>
#include <stdio.h>

#define THIS_FILE "pj_smoke"

int main(void)
{
    pjsua_config ua_cfg;
    pjsua_logging_config log_cfg;
    pjsua_media_config media_cfg;
    pjsua_transport_config tcfg;
    pjsua_transport_id tid;
    pj_status_t status;
    unsigned i, count;

    printf("pjproject %s\n", pj_get_version());

    if (pjsua_create() != PJ_SUCCESS)
        return 1;

    pjsua_config_default(&ua_cfg);
    pjsua_logging_config_default(&log_cfg);
    log_cfg.console_level = 1;
    pjsua_media_config_default(&media_cfg);
    media_cfg.ec_tail_len = 20;

    status = pjsua_init(&ua_cfg, &log_cfg, &media_cfg);
    if (status != PJ_SUCCESS) {
        pjsua_perror(THIS_FILE, "pjsua_init", status);
        pjsua_destroy();
        return 1;
    }

    pjsua_transport_config_default(&tcfg);
    printf("\nTransportes:\n");
    printf("  UDP: %s\n", pjsua_transport_create(PJSIP_TRANSPORT_UDP, &tcfg, &tid) == PJ_SUCCESS ? "ok" : "falhou");
    printf("  TCP: %s\n", pjsua_transport_create(PJSIP_TRANSPORT_TCP, &tcfg, &tid) == PJ_SUCCESS ? "ok" : "falhou");
    printf("  TLS: %s\n", pjsua_transport_create(PJSIP_TRANSPORT_TLS, &tcfg, &tid) == PJ_SUCCESS ? "ok" : "falhou");

    status = pjsua_start();
    if (status != PJ_SUCCESS) {
        pjsua_perror(THIS_FILE, "pjsua_start", status);
        pjsua_destroy();
        return 1;
    }

    {
        pjmedia_aud_dev_info info[PJMEDIA_AUD_MAX_DEVS];
        count = PJ_ARRAY_SIZE(info);
        pjsua_enum_aud_devs(info, &count);
        printf("\nDispositivos de audio (%u):\n", count);
        for (i = 0; i < count; ++i)
            printf("  [%u] %-40s in=%u out=%u (%s)\n", i, info[i].name,
                   info[i].input_count, info[i].output_count, info[i].driver);
    }

    {
        pjsua_codec_info ci[64];
        count = PJ_ARRAY_SIZE(ci);
        pjsua_enum_codecs(ci, &count);
        printf("\nCodecs de audio (%u):\n", count);
        for (i = 0; i < count; ++i)
            printf("  %.*s (prio %u)\n", (int)ci[i].codec_id.slen,
                   ci[i].codec_id.ptr, ci[i].priority);
    }

#if PJMEDIA_HAS_VIDEO
    {
        pjmedia_vid_dev_info vi[16];
        pjsua_codec_info ci[32];
        count = PJ_ARRAY_SIZE(vi);
        pjsua_vid_enum_devs(vi, &count);
        printf("\nDispositivos de video (%u):\n", count);
        for (i = 0; i < count; ++i)
            printf("  [%u] %-40s (%s) dir=%d\n", i, vi[i].name, vi[i].driver, vi[i].dir);

        count = PJ_ARRAY_SIZE(ci);
        pjsua_vid_enum_codecs(ci, &count);
        printf("\nCodecs de video (%u):\n", count);
        for (i = 0; i < count; ++i)
            printf("  %.*s (prio %u)\n", (int)ci[i].codec_id.slen,
                   ci[i].codec_id.ptr, ci[i].priority);
    }
#endif

    pjsua_destroy();
    printf("\nOK\n");
    return 0;
}
