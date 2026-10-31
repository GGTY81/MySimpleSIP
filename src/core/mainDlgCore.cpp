/*
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// MySimpleSIP: funções do CmainDlg (mainDlg.cpp do MicroSIP 3.22.16) que controlam o
// pjsua, copiadas na ordem original. Alterações marcadas com "MySimpleSIP:".

#define THIS_FILENAME "mainDlg.cpp"

#include "mainDlgCore.h"
#include "PulseAudioDev.h"
#include "langpack.h"

#include <filesystem>

CmainDlgCore* mainDlg = nullptr;
Statistics statistics = {};

CmainDlgCore::CmainDlgCore()
{
}

CmainDlgCore::~CmainDlgCore()
{
}

CString msip_vid_codec_id(const CString& name)
{
    pjsua_codec_info info[32];
    unsigned count = PJ_ARRAY_SIZE(info);
    if (pjsua_vid_enum_codecs(info, &count) == PJ_SUCCESS) {
        for (unsigned i = 0; i < count; i++) {
            CString id = MSIP::PjToStr(&info[i].codec_id);
            if (id.Left(name.GetLength() + 1).CompareNoCase(name + _T("/")) == 0) {
                return id;
            }
        }
    }
    return name + _T("/0");
}

void CmainDlgCore::PJCreate()
{
    while (!is_pjsua_running()) {
        statistics.pjCreateTime = CTime::GetCurrentTime().GetTime();
        PJCreateRaw();
        if (is_pjsua_running()) {
            break;
        }
        UpdateWindowText();
        // MySimpleSIP: TaskDialog (Repetir/Cancelar) → gancho AskRetryNetwork
        if (!AskRetryNetwork()) {
            exit(0);
        }
    }
}

void CmainDlgCore::PJCreateRaw()
{
    player_eof_data = nullptr;
    autoAnswerTimerCallId = PJSUA_INVALID_ID;
    autoAnswerPlayCallId = PJSUA_INVALID_ID;
    forwardingTimerCallId = PJSUA_INVALID_ID;

    isSubscribed = false;
    if (accountSettings.audioCodecs.IsEmpty())
    {
        accountSettings.audioCodecs = _T(_GLOBAL_CODECS_ENABLED);
    }

    // check updates
    if (accountSettings.updatesInterval != _T("never"))
    {
        CTime t = CTime::GetCurrentTime();
        time_t time = t.GetTime();
        int days;
        if (accountSettings.updatesInterval == _T("daily"))
        {
            days = 1;
        }
        else if (accountSettings.updatesInterval == _T("monthly"))
        {
            days = 30;
        }
        else if (accountSettings.updatesInterval == _T("quarterly"))
        {
            days = 90;
        }
        else
        {
            days = 7;
        }
        if (accountSettings.updatesInterval == _T("always") || accountSettings.checkUpdatesTime + days * 86400 < time) {
            CheckUpdates();
            accountSettings.checkUpdatesTime = time;
            accountSettings.SettingsSave();
        }
    }

// MySimpleSIP: verificação de privacidade da câmera do Windows (ms-settings:privacy-webcam)
    // removida; MSIP::IsCameraAccessAllowed() é sempre verdadeiro no Linux.

    // pj create
    pj_status_t status;
    pjsua_config         ua_cfg;
    pjsua_media_config   media_cfg;
    pjsua_transport_config cfg;

    // MySimpleSIP: as mensagens de pjsua_create (antes de pjsua_init configurar o log)
    // iriam para o terminal; no Windows não aparecem
    pj_log_set_level(0);
    // Must create pjsua before anything else!
    status = pjsua_create();
    if (status != PJ_SUCCESS) {
        return;
    }

    pjsip_cfg()->endpt.disable_rport = accountSettings.rport ? PJ_FALSE : PJ_TRUE;

    // Initialize configs with default settings.
    pjsua_config_default(&ua_cfg);
    pjsua_media_config_default(&media_cfg);

    char* ua_cfg_user_agent;
    if (accountSettings.userAgent.IsEmpty()) {
        CString userAgent;
        userAgent.Format(_T("%s/%s"), _T(_GLOBAL_NAME_VISIBLE), _T(_GLOBAL_VERSION));
        ua_cfg_user_agent = MSIP::WideCharToPjStr(userAgent);
        pj_strset2(&ua_cfg.user_agent, ua_cfg_user_agent);
    }
    else {
        ua_cfg_user_agent = MSIP::WideCharToPjStr(accountSettings.userAgent);
        pj_strset2(&ua_cfg.user_agent, ua_cfg_user_agent);
    }

    // MySimpleSIP: os callbacks on_* ficam em callbacks.cpp (mesma lista do original)
    msip_set_pjsua_callbacks(&ua_cfg);

    ua_cfg.srtp_secure_signaling = 0;

    /*
    TODO: accountSettings.account: public_addr
    */

    if (accountSettings.enableSTUN && !accountSettings.stun.IsEmpty()) {
        int pos = 0;
        int i = 0;
        while (i < 8) {
            CString resToken = accountSettings.stun.Tokenize(_T(";,"), pos);
            if (pos == -1) {
                break;
            }
            resToken.Trim();
            if (!resToken.IsEmpty()) {
                ua_cfg.stun_srv[i] = MSIP::StrToPjStr(resToken);
                i++;
            }
        }
        ua_cfg.stun_srv_cnt = i;
    }

    media_cfg.enable_ice = PJ_FALSE;

    media_cfg.no_vad = accountSettings.vad ? PJ_FALSE : PJ_TRUE;
    media_cfg.ec_tail_len = accountSettings.ec && !accountSettings.opusStereo ? 20 : 0;

    int maxClockRate = 8000;
    int maxChannelCount = 1;
    int curPos = 0;
    CString resToken = accountSettings.audioCodecs.Tokenize(_T(" "), curPos);
    while (!resToken.IsEmpty()) {
        int pos = 0;
        bool isOpus = resToken.Tokenize(_T("/"), pos) == _T("opus");
        int clockRate = 0;
        if (isOpus) {
            clockRate = 24000;
        }
        else {
            if (pos != -1) {
                clockRate = _wtoi(resToken.Tokenize(_T("/"), pos));
            }
        }
        if (clockRate > maxClockRate) {
            maxClockRate = clockRate;
        }
        if (!accountSettings.ec && !isOpus) {
            if (pos != -1) {
                BYTE channelCount = resToken.Tokenize(_T("/"), pos) == _T("2") ? 2 : 1;
                if (channelCount > maxChannelCount) {
                    maxChannelCount = channelCount;
                }
            }
        }
        resToken = accountSettings.audioCodecs.Tokenize(_T(" "), curPos);
    }
    media_cfg.clock_rate = maxClockRate;
    if (accountSettings.opusStereo) {
        media_cfg.channel_count = 2;
    }
    else {
        media_cfg.channel_count = maxChannelCount;
    }

    if (accountSettings.dnsSrv && !accountSettings.dnsSrvNs.IsEmpty()) {
        int pos = 0;
        int i = 0;
        while (i < 4) {
            CString resToken = accountSettings.dnsSrvNs.Tokenize(_T(";,"), pos);
            if (pos == -1) {
                break;
            }
            resToken.Trim();
            if (!resToken.IsEmpty()) {
                ua_cfg.nameserver[i] = MSIP::StrToPjStr(resToken);
                i++;
            }
        }
        ua_cfg.nameserver_count = i;
    }

    // Initialize pjsua
    if (accountSettings.enableLog) {
        // MySimpleSIP: PathFileExists/MoveFile → std::filesystem; o "." procurado é o
        // da extensão (no Windows o caminho raramente tem outro ponto antes)
        std::error_code ec;
        if (std::filesystem::exists(CStringA(accountSettings.logFile).GetString(), ec)) {
            CString filename = accountSettings.logFile;
            filename.Insert(filename.ReverseFind('.'), _T("_1"));
            std::filesystem::rename(CStringA(accountSettings.logFile).GetString(), CStringA(filename).GetString(), ec);
        }
        pjsua_logging_config log_cfg;
        pjsua_logging_config_default(&log_cfg);
        // MySimpleSIP: sem PJ_LOG_HAS_CR (quebra de linha CRLF) no Linux
        char* buf = MSIP::WideCharToPjStr(accountSettings.logFile);
        log_cfg.log_filename = pj_str(buf);
        log_cfg.console_level = 0; // MySimpleSIP: só no arquivo, como no Windows

        status = pjsua_init(&ua_cfg, &log_cfg, &media_cfg);
        free(buf);
    }
    else {
        // MySimpleSIP: no Windows o log padrão do pjsua vai para um console inexistente; no
        // Linux iria para o terminal. Sem enableLog, nenhum log.
        pjsua_logging_config log_cfg;
        pjsua_logging_config_default(&log_cfg);
        log_cfg.console_level = 0;
        log_cfg.level = 0;
        status = pjsua_init(&ua_cfg, &log_cfg, &media_cfg);
    }

    free(ua_cfg_user_agent);

    if (status != PJ_SUCCESS) {
        pjsua_destroy();
        return;
    }

    // Start pjsua
    status = pjsua_start();
    if (status != PJ_SUCCESS) {
        pjsua_destroy();
        return;
    }

    g_bRunning = true;

    // MySimpleSIP: dispositivos do PulseAudio/PipeWire (nomes do sistema e o padrão do
    // servidor) no lugar dos PCMs do ALSA, se houver servidor de som
    if (!nullSoundDevice) {
        msip_pulse_audio_install();
    }

    // Set snd devices
    UpdateSoundDevicesIds();
    // MySimpleSIP: opção para rodar sem placa de som (testes/CLI)
    msip_null_sound_device = nullSoundDevice;
    if (nullSoundDevice) {
        pjsua_set_null_snd_dev();
    }

    PJAudioCodecs();
#ifdef _GLOBAL_VIDEO
    PJVideoCodecs();
#endif

    // Create transport
    PJ_LOG(3, (THIS_FILENAME, "Create transport"));
    transport_udp_local = -1;
    transport_udp = -1;
    transport_tcp = -1;
    transport_tls = -1;

    pjsua_transport_config_default(&cfg);
    if (accountSettings.sourcePort) {
        cfg.port = accountSettings.sourcePort;
        status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, &transport_udp);
        if (status != PJ_SUCCESS) {
            cfg.port = 0;
            pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, &transport_udp);
        }
        if (MACRO_ENABLE_LOCAL_ACCOUNT) {
            if (accountSettings.sourcePort == 5060) {
                transport_udp_local = transport_udp;
            }
            else {
                cfg.port = 5060;
                status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, &transport_udp_local);
                if (status != PJ_SUCCESS) {
                    transport_udp_local = transport_udp;
                }
            }
        }
    }
    else {
        if (MACRO_ENABLE_LOCAL_ACCOUNT) {
            cfg.port = 5060;
            status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, &transport_udp_local);
            if (status != PJ_SUCCESS) {
                transport_udp_local = -1;
            }
        }
        cfg.port = 0;
        pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, &transport_udp);
        if (transport_udp_local == -1) {
            transport_udp_local = transport_udp;
        }
    }

        cfg.port = MACRO_ENABLE_LOCAL_ACCOUNT ? 5060 : 0;
        status = pjsua_transport_create(PJSIP_TRANSPORT_TCP, &cfg, &transport_tcp);
        if (status != PJ_SUCCESS && cfg.port) {
            cfg.port = 0;
            pjsua_transport_create(PJSIP_TRANSPORT_TCP, &cfg, &transport_tcp);
        }
        cfg.port = MACRO_ENABLE_LOCAL_ACCOUNT ? 5061 : 0;
        status = pjsua_transport_create(PJSIP_TRANSPORT_TLS, &cfg, &transport_tls);
        if (status != PJ_SUCCESS && cfg.port) {
            cfg.port = 0;
            pjsua_transport_create(PJSIP_TRANSPORT_TLS, &cfg, &transport_tls);
        }

    if (accountSettings.usersDirectory.Find(_T("%s")) == -1 && accountSettings.usersDirectory.Find(_T("{")) == -1) {
        UsersDirectoryLoad();
    }

    SetTimer(IDT_TIMER_IDLE, 5000, NULL);

    account = PJSUA_INVALID_ID;
    account_local = PJSUA_INVALID_ID;

    PJAccountAddLocal();

    // MySimpleSIP: WTSRegisterSessionNotification, CMMNotificationClient,
    // NotifyUnicastIpAddressChange/NotifyRouteChange2 e Hid::OpenDevice → monitores de
    // plataforma (sessão, dispositivos de áudio, rede, headset — Fase 7)
    MSIP::IsNetworkChanged();
    PlatformMonitorsStart();
}

void CmainDlgCore::PJAudioCodecs()
{
    if (!is_pjsua_running()) {
        return;
    }
    //Set aud codecs prio
    PJ_LOG(3, (THIS_FILENAME, "Set audio codecs"));
    if (accountSettings.audioCodecs.GetLength())
    {
        // add unknown new codecs to the list
        unsigned count = PJMEDIA_CODEC_MGR_MAX_CODECS;
        pjsua_codec_info codec_info[PJMEDIA_CODEC_MGR_MAX_CODECS];
        if (pjsua_enum_codecs(codec_info, &count) == PJ_SUCCESS) {
            for (unsigned i = 0; i < count; i++) {
                pjsua_codec_set_priority(&codec_info[i].codec_id, PJMEDIA_CODEC_PRIO_DISABLED);
                CString rab = MSIP::PjToStr(&codec_info[i].codec_id);
                if (!audioCodecList.Find(rab)) {
                    audioCodecList.AddTail(rab);
                    rab.Append(_T("~"));
                    audioCodecList.AddTail(rab);
                }
            }
        }
        // remove unsupported codecs from list
        POSITION pos = audioCodecList.GetHeadPosition();
        while (pos) {
            POSITION posKey = pos;
            CString key = audioCodecList.GetNext(pos);
            POSITION posValue = pos;
            CString value = audioCodecList.GetNext(pos);
            pj_str_t codec_id = MSIP::StrToPjStr(key);
            pjmedia_codec_param param;
            if (pjsua_codec_get_param(&codec_id, &param) != PJ_SUCCESS) {
                audioCodecList.RemoveAt(posKey);
                audioCodecList.RemoveAt(posValue);
            }
        };

        int curPos = 0;
        int i = PJMEDIA_CODEC_PRIO_NORMAL;
        CString resToken = accountSettings.audioCodecs.Tokenize(_T(" "), curPos);
        while (!resToken.IsEmpty()) {
            int pos = resToken.Find('/', 0);
            if (pos > 0 && resToken.Find('/', pos + 1) > 0) {
                pj_str_t codec_id = MSIP::StrToPjStr(resToken);
                pjmedia_codec_param param;
                if (pjsua_codec_get_param(&codec_id, &param) == PJ_SUCCESS) {
                    if (accountSettings.opusStereo) {
                        if (pj_strcmp2(&codec_id, "opus/48000/2") == 0) {
                            for (int j = 0; j < param.setting.dec_fmtp.cnt; j++) {
                                if (pj_strcmp2(&param.setting.dec_fmtp.param[j].name, "maxaveragebitrate") == 0) {
                                    param.setting.dec_fmtp.param[j].val = pj_str("96000");
                                }
                            }
                            param.info.avg_bps = 96000;
                            param.info.max_bps = 96000;
                            param.setting.dec_fmtp.param[param.setting.dec_fmtp.cnt].name = pj_str("stereo");
                            param.setting.dec_fmtp.param[param.setting.dec_fmtp.cnt].val = pj_str("1");
                            param.setting.dec_fmtp.cnt++;
                            pjsua_codec_set_param(&codec_id, &param);
                        }
                    }
                    pjsua_codec_set_priority(&codec_id, i);
                }
            }
            resToken = accountSettings.audioCodecs.Tokenize(_T(" "), curPos);
            i--;
        }
    }
}

#ifdef _GLOBAL_VIDEO
// MySimpleSIP: os IDs fixos "H264/99", "H263-1998/98", "VP8/100" e "VP9/101" do MicroSIP
// foram trocados por msip_vid_codec_id (payload types diferentes no pjproject upstream).
void CmainDlgCore::PJVideoCodecs()
{
    if (!is_pjsua_running()) {
        return;
    }
    //Set vid codecs prio
    PJ_LOG(3, (THIS_FILENAME, "Set video codecs"));
    if (accountSettings.videoCodec.GetLength())
    {
        pj_str_t codec_id = MSIP::StrToPjStr(accountSettings.videoCodec);
        pjsua_vid_codec_set_priority(&codec_id, 255);
    }
    int bitrate;
    if (!accountSettings.videoH264) {
        pj_str_t vid_codec_id = MSIP::StrToPjStr(msip_vid_codec_id(_T("H264")));
        pjsua_vid_codec_set_priority(&vid_codec_id, 0);
        free(vid_codec_id.ptr);
    }
    else
    {
        CStringA codec_id_str(msip_vid_codec_id(_T("H264"))); // MySimpleSIP: id pelo nome
            const pj_str_t codec_id = pj_str(const_cast<char*>(codec_id_str.GetString()));
        pjmedia_vid_codec_param param;
        pjsua_vid_codec_get_param(&codec_id, &param);
        if (accountSettings.videoBitrate) {
            bitrate = 1000 * accountSettings.videoBitrate;
            param.enc_fmt.det.vid.avg_bps = bitrate;
            param.enc_fmt.det.vid.max_bps = bitrate;
        }
        pjsua_vid_codec_set_param(&codec_id, &param);
    }
    if (!accountSettings.videoH263) {
        pj_str_t vid_codec_id = MSIP::StrToPjStr(msip_vid_codec_id(_T("H263-1998")));
        pjsua_vid_codec_set_priority(&vid_codec_id, 0);
        free(vid_codec_id.ptr);
    }
    else {
        if (accountSettings.videoBitrate) {
            bitrate = 1000 * accountSettings.videoBitrate;
            CStringA codec_id_str(msip_vid_codec_id(_T("H263-1998"))); // MySimpleSIP: id pelo nome
            const pj_str_t codec_id = pj_str(const_cast<char*>(codec_id_str.GetString()));
            pjmedia_vid_codec_param param;
            pjsua_vid_codec_get_param(&codec_id, &param);
            param.enc_fmt.det.vid.avg_bps = bitrate;
            param.enc_fmt.det.vid.max_bps = bitrate;
            pjsua_vid_codec_set_param(&codec_id, &param);
        }
    }
    if (!accountSettings.videoVP8) {
        pj_str_t vid_codec_id = MSIP::StrToPjStr(msip_vid_codec_id(_T("VP8")));
        pjsua_vid_codec_set_priority(&vid_codec_id, 0);
        free(vid_codec_id.ptr);
    }
    else {
        if (accountSettings.videoBitrate) {
            bitrate = 1000 * accountSettings.videoBitrate;
            CStringA codec_id_str(msip_vid_codec_id(_T("VP8"))); // MySimpleSIP: id pelo nome
            const pj_str_t codec_id = pj_str(const_cast<char*>(codec_id_str.GetString()));
            pjmedia_vid_codec_param param;
            pjsua_vid_codec_get_param(&codec_id, &param);
            param.enc_fmt.det.vid.avg_bps = bitrate;
            param.enc_fmt.det.vid.max_bps = bitrate;
            pjsua_vid_codec_set_param(&codec_id, &param);
        }
    }
    if (!accountSettings.videoVP9) {
        pj_str_t vid_codec_id = MSIP::StrToPjStr(msip_vid_codec_id(_T("VP9")));
        pjsua_vid_codec_set_priority(&vid_codec_id, 0);
        free(vid_codec_id.ptr);
    }
    else {
        if (accountSettings.videoBitrate) {
            bitrate = 1000 * accountSettings.videoBitrate;
            CStringA codec_id_str(msip_vid_codec_id(_T("VP9"))); // MySimpleSIP: id pelo nome
            const pj_str_t codec_id = pj_str(const_cast<char*>(codec_id_str.GetString()));
            pjmedia_vid_codec_param param;
            pjsua_vid_codec_get_param(&codec_id, &param);
            param.enc_fmt.det.vid.avg_bps = bitrate;
            param.enc_fmt.det.vid.max_bps = bitrate;
            pjsua_vid_codec_set_param(&codec_id, &param);
        }
    }
}
#endif

void CmainDlgCore::UpdateSoundDevicesIds()
{
    msip_audio_input = -1;
    msip_audio_output = -2;
    msip_audio_ring = -2;
    CString audioOutputDevice = accountSettings.audioOutputDevice;
    CString audioInputDevice = accountSettings.audioInputDevice;
    unsigned count = PJMEDIA_AUD_MAX_DEVS;
    pjmedia_aud_dev_info aud_dev_info[PJMEDIA_AUD_MAX_DEVS];
    pjsua_enum_aud_devs(aud_dev_info, &count);
    for (unsigned i = 0; i < count; i++)
    {
        CString audDevName = MSIP::Utf8DecodeUni(aud_dev_info[i].name);
        if (aud_dev_info[i].input_count && !audioInputDevice.Compare(audDevName)) {
            msip_audio_input = i;
        }
        if (aud_dev_info[i].output_count) {
            if (!audioOutputDevice.Compare(audDevName)) {
                msip_audio_output = i;
            }
            if (!accountSettings.audioRingDevice.Compare(audDevName)) {
                msip_audio_ring = i;
            }
        }
    }
}

void CmainDlgCore::PJDestroy(bool exit)
{
    if (!is_pjsua_running()) {
        return;
    }
    statistics.pjDestroyTime = CTime::GetCurrentTime().GetTime();
    statistics.pjDestroyThreadId = (int)pj_getpid(); // MySimpleSIP: GetCurrentThreadId

    // MySimpleSIP: CancelMibChangeNotify2, CMMNotificationClient e
    // WTSUnRegisterSessionNotification → monitores de plataforma
    PlatformMonitorsStop();

    KillTimer(IDT_TIMER_IDLE);
    KillTimer(IDT_TIMER_TONE);
    KillTimer(IDT_TIMER_CALL);
    KillTimer(IDT_TIMER_DIRECTORY);
    KillTimer(IDT_TIMER_NETWORK_CHANGED);
    KillTimer(IDT_TIMER_SWITCH_DEVICES);

    usersDirectoryLoaded = false;
    shortcutsURLLoaded = false;

    // MySimpleSIP: Hid::CloseDevice faz parte de PlatformMonitorsStop
    Unsubscribe();
    msip_call_deinit_tonegen(-1, NULL);

    toneCalls.RemoveAll();

    pjsua_call_id call_ids[PJSUA_MAX_CALLS];
    unsigned calls_count = PJSUA_MAX_CALLS;
    if (pjsua_enum_calls(call_ids, &calls_count) == PJ_SUCCESS) {
        for (unsigned i = 0; i < calls_count; ++i) {
            call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(call_ids[i]);
            msip_call_deinit_tonegen(call_ids[i], user_data);
        }
    }

    msip_call_hangup_all();

    PlayerStop();

    if (player_eof_data) {
        pj_pool_release(player_eof_data->pool);
        player_eof_data = nullptr;
    }

    if (accountSettings.accountId) {
        PJAccountDelete(false, exit);
    }

    g_bRunning = false;
    statistics.pjDestroyTime2 = CTime::GetCurrentTime().GetTime();
    pjsua_destroy();
    statistics.pjDestroyTime3 = CTime::GetCurrentTime().GetTime();

    if (!exit) {
        InitUI();
    }

    transport_udp_local = -1;
    transport_udp = -1;
    transport_tcp = -1;
    transport_tls = -1;
}

void CmainDlgCore::PJAccountConfig(pjsua_acc_config * acc_cfg, Account * account)
{
    bool isLocal = (account == &accountSettings.accountLocal);
    pjsua_acc_config_default(acc_cfg);
    // global
    acc_cfg->ka_interval = account->keepAlive;
#ifdef _GLOBAL_VIDEO
    acc_cfg->vid_in_auto_show = PJ_TRUE;
    acc_cfg->vid_out_auto_transmit = PJ_TRUE;
    acc_cfg->vid_cap_dev = VideoCaptureDeviceId();
    acc_cfg->vid_wnd_flags = PJMEDIA_VID_DEV_WND_BORDER | PJMEDIA_VID_DEV_WND_RESIZABLE;
#endif

    if (accountSettings.rtpPortMin > 0) {
        acc_cfg->rtp_cfg.port = accountSettings.rtpPortMin;
        if (accountSettings.rtpPortMax > accountSettings.rtpPortMin) {
            acc_cfg->rtp_cfg.port_range = accountSettings.rtpPortMax - accountSettings.rtpPortMin;
        }
    }
    // account
    if (account->disableSessionTimer) {
        acc_cfg->use_timer = PJSUA_SIP_TIMER_INACTIVE;
    }

    acc_cfg->reg_timeout = account->registerRefresh;

    if (account->srtp == _T("optional")) {
        acc_cfg->use_srtp = PJMEDIA_SRTP_OPTIONAL;
    }
    else if (account->srtp == _T("mandatory")) {
        acc_cfg->use_srtp = PJMEDIA_SRTP_MANDATORY;
    }
    else if (account->srtp == _T("dtls-sdes")) {
        acc_cfg->use_srtp = PJMEDIA_SRTP_MANDATORY;
        acc_cfg->enable_rtcp_mux = PJ_TRUE;
        acc_cfg->srtp_opt.keying_count = 2;
        acc_cfg->srtp_opt.keying[0] = PJMEDIA_SRTP_KEYING_DTLS_SRTP;
        acc_cfg->srtp_opt.keying[1] = PJMEDIA_SRTP_KEYING_SDES;
    }
    else if (account->srtp == _T("dtls")) {
        acc_cfg->use_srtp = PJMEDIA_SRTP_MANDATORY;
        acc_cfg->enable_rtcp_mux = PJ_TRUE;
        acc_cfg->srtp_opt.keying_count = 1;
        acc_cfg->srtp_opt.keying[0] = PJMEDIA_SRTP_KEYING_DTLS_SRTP;
    }
    else {
        acc_cfg->use_srtp = PJMEDIA_SRTP_DISABLED;
    }
    if (!accountSettings.enableSTUN || accountSettings.stun.IsEmpty()) {
        acc_cfg->rtp_cfg.public_addr = MSIP::StrToPjStr(get_public_addr(account));
    }
    acc_cfg->ice_cfg_use = PJSUA_ICE_CONFIG_USE_CUSTOM;
    acc_cfg->ice_cfg.enable_ice = account->ice ? PJ_TRUE : PJ_FALSE;
    acc_cfg->allow_via_rewrite = account->allowRewrite ? PJ_TRUE : PJ_FALSE;
    acc_cfg->allow_sdp_nat_rewrite = acc_cfg->allow_via_rewrite;
    acc_cfg->allow_contact_rewrite = acc_cfg->allow_via_rewrite ? 2 : PJ_FALSE;

    acc_cfg->contact_rewrite_method = PJSUA_CONTACT_REWRITE_ALWAYS_UPDATE | PJSUA_CONTACT_REWRITE_UNREGISTER;

    acc_cfg->publish_enabled = account->publish ? PJ_TRUE : PJ_FALSE;

    if (!account->voicemailNumber.IsEmpty()) {
        acc_cfg->mwi_enabled = PJ_TRUE;
    }

    if (account->transport == _T("udp") && transport_udp != -1) {
        acc_cfg->transport_id = transport_udp;
    }
    else if (account->transport == _T("tcp") && transport_tcp != -1) {
        if (isLocal) {
            acc_cfg->transport_id = transport_tcp;
        }
    }
    else if (account->transport == _T("tls") && transport_tls != -1) {
        if (isLocal) {
            acc_cfg->transport_id = transport_tls;
        }
    }

    acc_cfg->cred_count = 1;
    acc_cfg->cred_info[0].username = MSIP::StrToPjStr(!account->authID.IsEmpty() ? account->authID : (isLocal ? account->username : get_account_username()));
    acc_cfg->cred_info[0].realm = pj_str("*");
    acc_cfg->cred_info[0].scheme = pj_str("Digest");
    if (!account->digest.IsEmpty()) {
        acc_cfg->cred_info[0].data_type = PJSIP_CRED_DATA_DIGEST;
        acc_cfg->cred_info[0].data = MSIP::StrToPjStr(account->digest);
    }
    else {
        acc_cfg->cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
        acc_cfg->cred_info[0].data = MSIP::StrToPjStr((isLocal ? account->password : get_account_password()));
    }

    CStringList proxies;
    get_account_proxy(account, proxies);
    acc_cfg->proxy_cnt = proxies.GetCount();
    POSITION pos = proxies.GetHeadPosition();
    int i = 0;
    while (pos) {
        CString proxy = proxies.GetNext(pos);
        proxy = _T("sip:") + proxy;
        if (account->port > 0) {
            proxy.AppendFormat(_T(":%d"), account->port);
        }
        AddTransportSuffix(proxy, account);
        acc_cfg->proxy[i] = MSIP::StrToPjStr(proxy);
        i++;
    }
    if (isLocal) {
        acc_cfg->sip_stun_use = PJSUA_STUN_USE_DISABLED;
        acc_cfg->media_stun_use = PJSUA_STUN_USE_DISABLED;
    }
}

void CmainDlgCore::PJAccountAdd()
{
    if (!is_pjsua_running() || pjsua_acc_is_valid(account)) {
        return;
    }
    CString str;

    if (!accountSettings.accountId) {
        return;
    }
    if (accountSettings.account.username.IsEmpty()
        ) {
        OnAccount(0, 0);
        return;
    }
    PJAccountAddRaw();
}

void CmainDlgCore::PJAccountAddRaw()
{
    CString str;

    CString title = _T(_GLOBAL_NAME_VISIBLE);
    CString titleAdder;
    CString usernameLocal;
    usernameLocal = accountSettings.account.username;
    if (!accountSettings.account.label.IsEmpty())
    {
        titleAdder = accountSettings.account.label;
    }
    else if (!accountSettings.account.displayName.IsEmpty())
    {
        titleAdder = accountSettings.account.displayName;
    }
    else if (!usernameLocal.IsEmpty())
    {
        titleAdder = usernameLocal;
    }
    if (!titleAdder.IsEmpty()) {
        title.AppendFormat(_T(" - %s"), titleAdder);
    }
    SetPaneText2(accountSettings.account.username);
    SetWindowText(title);
    DialerSetName(); // MySimpleSIP: pageDialer->SetName()

    pjsua_acc_config acc_cfg;
    PJAccountConfig(&acc_cfg, &accountSettings.account);

    //-- port knocker
    MSIP::PortKnock();

    //--
    bool ok = false;
    pj_status_t status = -1;
    //--
    CString localURI;
    if (!get_account_display_name(&accountSettings.account).IsEmpty()) {
        localURI = _T("\"") + get_account_display_name(&accountSettings.account) + _T("\" ");
    }
    localURI += GetSIPURI(get_account_username());
    acc_cfg.id = MSIP::StrToPjStr(localURI);
    //--
    if (get_account_server().IsEmpty()) {
        acc_cfg.register_on_acc_add = PJ_FALSE;
    }
    else {
        CString regURI;
        regURI.Format(_T("sip:%s"), get_account_server());
        AddTransportSuffix(regURI, &accountSettings.account);
        acc_cfg.reg_uri = MSIP::StrToPjStr(regURI);
    }
    //--
    status = pjsua_acc_add(&acc_cfg, PJ_TRUE, &account);
    if (status == PJ_SUCCESS) {
        ok = true;
        if (acc_cfg.register_on_acc_add == PJ_FALSE) {
            Subscribe();
        }
    }
    if (!ok) {
        if (status != -1) {
            MSIP::ShowErrorMessage(this, status);
        }
        UpdateWindowText(_T(""), IDI_DEFAULT, true);
    }
    PublishStatus(true, acc_cfg.register_on_acc_add);
}

void CmainDlgCore::PJAccountAddLocal()
{
    if (MACRO_ENABLE_LOCAL_ACCOUNT) {
        pj_status_t status;
        pjsua_acc_config acc_cfg;
        PJAccountConfig(&acc_cfg, &accountSettings.accountLocal);

        CString localURI;
        if (!get_account_display_name(&accountSettings.accountLocal).IsEmpty()) {
            localURI = _T("\"") + get_account_display_name(&accountSettings.accountLocal) + _T("\" ");
        }
        CString domain;
        if (!accountSettings.accountLocal.domain.IsEmpty()) {
            domain = accountSettings.accountLocal.domain;
        }
        else {
            pjsua_transport_data* t = &pjsua_var.tpdata[0];
            domain = MSIP::PjToStr(&t->local_name.host);
        }
        if (!accountSettings.accountLocal.username.IsEmpty()) {
            localURI.AppendFormat(_T("<sip:%s@%s>"), accountSettings.accountLocal.username, domain);
        }
        else {
            localURI.AppendFormat(_T("<sip:%s>"), domain);
        }

        acc_cfg.id = MSIP::StrToPjStr(localURI);
        acc_cfg.priority--;
        pjsua_acc_add(&acc_cfg, PJ_TRUE, &account_local);
        acc_cfg.priority++;
    }
}

void CmainDlgCore::PJAccountDelete(bool deep, bool exit, CStringA code)
{
    Unsubscribe();
    if (pjsua_acc_is_valid(account)) {
        pjsua_acc_del(account);
        account = PJSUA_INVALID_ID;
    }

}

void CmainDlgCore::PJAccountDeleteLocal()
{
    if (pjsua_acc_is_valid(account_local)) {
        pjsua_acc_del(account_local);
        account_local = PJSUA_INVALID_ID;
    }
}

void CmainDlgCore::PublishStatus(bool online, bool init)
{
    if (!is_pjsua_running()) {
        return;
    }
    bool busy = (accountSettings.denyIncoming == _T("button") && accountSettings.DND);
    pjrpid_activity presenceStatusNew;
    pj_str_t note = pj_str("");
    if (m_PresenceStatus == PJRPID_ACTIVITY_BUSY) {
        if (!busy) {
            presenceStatusNew = PJRPID_ACTIVITY_UNKNOWN;
            note = pj_str("Idle");
        }
    }
    else {
        if (busy) {
            presenceStatusNew = PJRPID_ACTIVITY_BUSY;
            note = pj_str("Busy");
        }
        else {
            presenceStatusNew = online ? PJRPID_ACTIVITY_UNKNOWN : PJRPID_ACTIVITY_AWAY;
            note = online ? pj_str("Idle") : pj_str("Away");
        }
    }
    if (note.slen) {
        pjsua_acc_id ids[PJSUA_MAX_ACC];
        unsigned count = PJSUA_MAX_ACC;
        if (pjsua_enum_accs(ids, &count) == PJ_SUCCESS) {
            pjrpid_element pr;
            pr.type = PJRPID_ELEMENT_TYPE_PERSON;
            pr.id = pj_str(NULL);
            pr.note = pj_str(NULL);
            pr.note = note;
            pr.activity = presenceStatusNew;
            for (unsigned i = 0; i < count; i++) {
                pjsua_acc_set_online_status2(ids[i], PJ_TRUE, &pr);
            }
        }
        m_PresenceStatus = presenceStatusNew;
    }
    if (!init) {
        UpdateWindowText();
    }
}

static PJ_DEF(pj_status_t) on_pjsua_wav_file_end_callback(pjmedia_port * media_port, void* args)
{
    mainDlg->PostMessage(UM_ON_PLAYER_STOP, 0, 0);
    return -1;//Here it is important to return value other than PJ_SUCCESS
}

void CmainDlgCore::PlayerPlay(CString filename, bool noLoop, bool inCall, bool isAA)
{
    PlayerStop();
    bool stopCallback = false;
    if (!filename.IsEmpty()) {
        pj_str_t file = MSIP::StrToPjStr(filename);
        pjsua_player_id player_id;
        if (is_pjsua_running() && pjsua_player_create(&file, noLoop ? PJMEDIA_FILE_NO_LOOP : 0, &player_id) == PJ_SUCCESS) {
            pjmedia_port* player_media_port;
            if (pjsua_player_get_port(player_id, &player_media_port) == PJ_SUCCESS) {
                if (!player_eof_data) {
                    pj_pool_t* pool = pjsua_pool_create("microsip_eof_data", 512, 512);
                    player_eof_data = PJ_POOL_ZALLOC_T(pool, struct player_eof_data);
                    player_eof_data->pool = pool;
                }
                player_eof_data->player_id = player_id;
                if (noLoop) {
                    if (pjmedia_wav_player_set_eof_cb(player_media_port, player_eof_data, &on_pjsua_wav_file_end_callback) == PJ_SUCCESS) {
                        stopCallback = true;
                    }
                }
                if (
                    (!tone_gen && pjsua_conf_get_active_ports() <= 2)
                    ||
                    (tone_gen && pjsua_conf_get_active_ports() <= 3)
                    ) {
                    msip_set_sound_device(inCall ? msip_audio_output : msip_audio_ring);
                }
                pjsua_conf_port_id conf_port_id = pjsua_player_get_conf_port(player_id);
                if (inCall) {
                    pjsua_conf_adjust_rx_level(conf_port_id, 0.4);
                }
                else {
                    pjsua_conf_adjust_rx_level(conf_port_id, (float)accountSettings.volumeRing / 100);
                }
                pjsua_conf_connect(conf_port_id, 0);
            }
        }
        free(file.ptr);
    }
    if (noLoop && !stopCallback) {
        onPlayerStop(); // MySimpleSIP: handler de UM_ON_PLAYER_STOP
    }
}

void CmainDlgCore::PlayerStop()
{
    if (player_eof_data && player_eof_data->player_id != PJSUA_INVALID_ID) {
        if (is_pjsua_running()) {
            pjsua_conf_disconnect(pjsua_player_get_conf_port(player_eof_data->player_id), 0);
            pjsua_player_destroy(player_eof_data->player_id);
            player_eof_data->player_id = PJSUA_INVALID_ID;
        }
        else {
            player_eof_data->player_id = PJSUA_INVALID_ID;
        }
    }
}

#ifdef _GLOBAL_VIDEO
int CmainDlgCore::VideoCaptureDeviceId(CString name)
{
    unsigned count = PJMEDIA_VID_DEV_MAX_DEVS;
    pjmedia_vid_dev_info vid_dev_info[PJMEDIA_VID_DEV_MAX_DEVS];
    pjsua_vid_enum_devs(vid_dev_info, &count);
    for (unsigned i = 0; i < count; i++) {
        if (vid_dev_info[i].fmt_cnt && (vid_dev_info[i].dir == PJMEDIA_DIR_ENCODING || vid_dev_info[i].dir == PJMEDIA_DIR_ENCODING_DECODING)) {
            CString vidDevName = MSIP::Utf8DecodeUni(vid_dev_info[i].name);
            if ((!name.IsEmpty() && name == vidDevName)
                ||
                (name.IsEmpty() && accountSettings.videoCaptureDevice == vidDevName)) {
                return vid_dev_info[i].id;
            }
        }
    }
    return PJMEDIA_VID_DEFAULT_CAPTURE_DEV;
}
#endif

// MySimpleSIP: handlers de UM_CALL_ANSWER/UM_CALL_HANGUP do CmainDlg (lógica de pjsua; as
// chamadas de interface viraram ganchos virtuais)
LRESULT CmainDlgCore::onCallAnswer(WPARAM wParam, LPARAM lParam)
{
    if (is_pjsua_running()) {
        pjsua_call_id call_id = wParam;
        pjsua_call_info call_info;
        if (pjsua_call_get_info(call_id, &call_info) == PJ_SUCCESS) {
            if (call_info.role == PJSIP_ROLE_UAS && (call_info.state == PJSIP_INV_STATE_INCOMING || call_info.state == PJSIP_INV_STATE_EARLY)) {
                if (lParam < 0) {
                    pjsua_call_answer(call_id, -lParam, NULL, NULL);
                    return 0;
                }
                if (accountSettings.singleMode) {
                    msip_call_hangup_all_noincoming();
                }
                msip_set_sound_device(msip_audio_output);
                pjsua_call_setting call_setting;
                pjsua_call_setting_default(&call_setting);
#ifdef _GLOBAL_VIDEO
                if (lParam > 0 && !accountSettings.disableVideo) {
                    createPreviewWin();
                    call_setting.vid_cnt = 1;
                }
                else {
                    call_setting.vid_cnt = 0;
                }
#else
                call_setting.vid_cnt = 0;
#endif
                if (pjsua_call_answer2(call_id, &call_setting, 200, NULL, NULL) == PJ_SUCCESS) {
                    callIdIncomingIgnore = MSIP::PjToStr(&call_info.call_id);
                }
                PlayerStop();
                bool restore = true;
                call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(call_id);
                if (user_data) {
                    user_data->CS.Lock();
                    if (user_data->autoAnswer) {
                        if (!accountSettings.bringToFrontOnIncoming) {
                            restore = false;
                            if (!IsForegroundApp()) { // MySimpleSIP: GetForegroundWindow()->GetTopLevelParent() != this
                                SIPURI sipuri;
                                ParseCallSIPURI(&call_info, user_data, sipuri);
                                CString name = GetNameForCall(sipuri, user_data);
                                CString str;
                                if (name == sipuri.user) {
                                    str = sipuri.user;
                                }
                                else {
                                    str.Format(_T("%s – %s"), name, sipuri.user);
                                }
                                BaloonPopup(Translate(_T("Auto Answer")), str, NIIF_INFO);
                            }
                        }
                    }
                    user_data->CS.Unlock();
                }

                if (restore) {
                    RestoreWindow(); // MySimpleSIP: onTrayNotify(NULL, WM_LBUTTONUP)
                }
            }
        }
    }
    return 0;
}

LRESULT CmainDlgCore::onCallHangup(WPARAM wParam, LPARAM lParam)
{
    if (is_pjsua_running()) {
        pjsua_call_id call_id = wParam;
        msip_call_hangup_fast(call_id);
    }
    return 0;
}
