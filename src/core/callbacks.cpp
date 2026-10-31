/*
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// MySimpleSIP: callbacks do pjsua (on_*) de mainDlg.cpp do MicroSIP 3.22.16, copiados
// na ordem original. Executam nas threads do pjsua e avisam a UI com PostMessage(UM_ON_*).
// Os handlers da UI que ficavam entre eles (CmainDlg::onRegState2, onCallState,
// onCallMediaState, onIncomingCall, onBuddyState, onCallTransferStatus, onMWIInfo,
// onDTMFDigit) foram retirados: pertencem à janela (Fase 3).
// Alterações marcadas com "MySimpleSIP:".

#define THIS_FILENAME "mainDlg.cpp"

#include "mainDlgCore.h"
#include "CMask.h"
#include "DialPlanRegex.h"
#include "langpack.h"

static void on_reg_started2(pjsua_acc_id acc_id, pjsua_reg_info* info)
{
    statistics.cbRegStarted++;
    statistics.cbRegStartedTime = CTime::GetCurrentTime().GetTime();
    if (info->renew) {
        PostMessage(mainDlg->m_hWnd, UM_UPDATEWINDOWTEXT, 1, 0);
    }
}

static void on_reg_state2(pjsua_acc_id acc_id, pjsua_reg_info* info)
{
    statistics.cbRegState++;
    statistics.cbRegStateTime = CTime::GetCurrentTime().GetTime();
    if (!IsWindow(mainDlg->m_hWnd)) {
        return;
    }
    CString* str = nullptr;
    if (info->cbparam->code >= 400 && info->cbparam->rdata) {
        pjsip_generic_string_hdr* hsr;
        const pj_str_t headerError = { "P-Registrar-Error",17 };
        hsr = (pjsip_generic_string_hdr*)pjsip_msg_find_hdr_by_name(info->cbparam->rdata->msg_info.msg, &headerError, NULL);
        if (hsr) {
            str = new CString();
            str->SetString(MSIP::PjToStr(&hsr->hvalue, true));
        }
    }
    PostMessage(mainDlg->m_hWnd, UM_ON_REG_STATE2, (WPARAM)info->cbparam->code, (LPARAM)str);
}


/* Callback from timer when the maximum call duration has been
 * exceeded.
 */
static void call_timeout_callback(pj_timer_heap_t* timer_heap,
    struct pj_timer_entry* entry)
{
    pjsua_call_id call_id = entry->id;
    pjsua_msg_data msg_data_;
    pjsip_generic_string_hdr warn;
    pj_str_t hname = pj_str("Warning");
    pj_str_t hvalue = pj_str("399 localhost \"Call duration exceeded\"");

    PJ_UNUSED_ARG(timer_heap);

    if (call_id == PJSUA_INVALID_ID) {
        PJ_LOG(1, (THIS_FILENAME, "Invalid call ID in timer callback"));
        return;
    }

    /* Add warning header */
    pjsua_msg_data_init(&msg_data_);
    pjsip_generic_string_hdr_init2(&warn, &hname, &hvalue);
    pj_list_push_back(&msg_data_.hdr_list, &warn);

    /* Call duration has been exceeded; disconnect the call */
    PJ_LOG(3, (THIS_FILENAME, "Duration (%d seconds) has been exceeded "
        "for call %d, disconnecting the call",
        accountSettings.autoHangUpTime, call_id));
    entry->id = PJSUA_INVALID_ID;
    pjsua_call_hangup(call_id, 200, NULL, &msg_data_);
}

static void on_call_state(pjsua_call_id call_id, pjsip_event* e)
{
    statistics.cbCallState++;
    statistics.cbCallStateTime = CTime::GetCurrentTime().GetTime();
    statistics.cbCallStateRole = -1;
    statistics.cbCallStateState = -1;
    statistics.postCallState = 0;
    if (!IsWindow(mainDlg->m_hWnd)) {
        return;
    }
    pjsua_call_info* call_info = new pjsua_call_info();
    if (pjsua_call_get_info(call_id, call_info) != PJ_SUCCESS) {
        return;
    }
    statistics.cbCallStateRole = call_info->role;
    statistics.cbCallStateState = call_info->state;
    if (call_info->state == PJSIP_INV_STATE_NULL) {
        return;
    }
    if (call_info->state == PJSIP_INV_STATE_DISCONNECTED && call_info->last_status == 481) {
        return;
    }
    call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(call_info->id);
    // reset user_data after call transfer
    if (user_data) {
        bool callIdMissmatch = false;
        user_data->CS.Lock();
        if (user_data->call_id == PJSUA_INVALID_ID) {
            user_data->call_id = call_info->id;
        }
        else if (user_data->call_id != call_info->id) {
            callIdMissmatch = true;
        }
        bool hidden = user_data->hidden;
        user_data->CS.Unlock();
        if (callIdMissmatch) {
            user_data = new call_user_data(call_info->id);
            pjsua_call_set_user_data(call_info->id, user_data);
        }
        else {
            if (hidden) {
                if (call_info->state == PJSIP_INV_STATE_DISCONNECTED) {
                    pjsua_call_set_user_data(call_info->id, NULL);
                    delete user_data;
                }
                return;
            }
        }
    }
    if (!user_data) {
        user_data = new call_user_data(call_info->id);
        pjsua_call_set_user_data(call_info->id, user_data);
    }

    user_data->CS.Lock();

    switch (call_info->state) {
    case PJSIP_INV_STATE_CALLING:
        msip_call_unhold(call_info);
        break;
    case PJSIP_INV_STATE_CONNECTING:
        msip_call_unhold(call_info);
        break;
    case PJSIP_INV_STATE_CONFIRMED:
        if (accountSettings.autoRecording) {
            msip_call_recording_start(user_data, call_info);
        }
        if (accountSettings.autoHangUpTime > 0) {
            /* Schedule timer to hangup call after the specified duration */
            pj_time_val delay;
            user_data->auto_hangup_timer.id = call_info->id;
            user_data->auto_hangup_timer.cb = &call_timeout_callback;
            delay.sec = accountSettings.autoHangUpTime;
            delay.msec = 0;
            pjsua_schedule_timer(&user_data->auto_hangup_timer, &delay);
        }
        break;
    case PJSIP_INV_STATE_DISCONNECTED:
        pjsua_call_set_user_data(call_info->id, NULL);
        break;
    }

    user_data->CS.Unlock();

    statistics.postCallState = 1;
    statistics.postCallState0 = 0;
    statistics.postCallState1 = 0;
    statistics.postCallState2 = 0;
    statistics.postCallState3 = 0;
    statistics.postCallState4 = 0;
    statistics.postCallState5 = 0;
    statistics.postCallState6 = 0;
    statistics.postCallState7 = 0;
    statistics.postCallState8 = 0;
    statistics.postCallState9 = 0;

    if (call_info->state == PJSIP_INV_STATE_DISCONNECTED) {
        PostMessage(mainDlg->m_hWnd, UM_ON_CALL_STATE, (WPARAM)call_info, (LPARAM)user_data);
    }
    else {
        PostMessage(mainDlg->m_hWnd, UM_ON_CALL_STATE, (WPARAM)call_info, (LPARAM)user_data);
    }
}


static void on_call_media_state(pjsua_call_id call_id)
{
    statistics.cbCallMediaState++;
    statistics.cbCallMediaStateTime = CTime::GetCurrentTime().GetTime();
    pjsua_call_info* call_info = new pjsua_call_info();
    if (pjsua_call_get_info(call_id, call_info) != PJ_SUCCESS || call_info->state == PJSIP_INV_STATE_NULL) {
        return;
    }

    call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(call_info->id);
    if (!user_data) {
        user_data = new call_user_data(call_info->id);
        pjsua_call_set_user_data(call_info->id, user_data);
    }

    if (call_info->media_status == PJSUA_CALL_MEDIA_ACTIVE
        || call_info->media_status == PJSUA_CALL_MEDIA_REMOTE_HOLD
        ) {
        msip_conference_join(call_info);
        pjsua_conf_connect(call_info->conf_slot, 0);
        pjsua_conf_connect(0, call_info->conf_slot);
        //--
        user_data->CS.Lock();
        user_data->holdFrom = -1;
        if (user_data->recorder_id != PJSUA_INVALID_ID) {
            pjsua_conf_port_id rec_conf_port_id = pjsua_recorder_get_conf_port(user_data->recorder_id);
            pjsua_conf_connect(call_info->conf_slot, rec_conf_port_id);
            pjsua_conf_adjust_tx_level(rec_conf_port_id, 1);
        }
        user_data->CS.Unlock();

        //--
        ::SetTimer(mainDlg->DialerHwnd(), IDT_TIMER_VU_METER, 100, NULL); // MySimpleSIP: pageDialer->m_hWnd
        //--
    }
    else {
        if (user_data->recorder_id != PJSUA_INVALID_ID) {
            pjsua_conf_port_id rec_conf_port_id = pjsua_recorder_get_conf_port(user_data->recorder_id);
            pjsua_conf_adjust_tx_level(rec_conf_port_id, 0);
        }
        msip_conference_leave(call_info, user_data, true);
        pjsua_conf_disconnect(call_info->conf_slot, 0);
        pjsua_conf_disconnect(0, call_info->conf_slot);
        msip_call_deinit_tonegen(call_info->id, user_data);
        //--
        user_data->CS.Lock();
        user_data->holdFrom = msip_get_duration(&call_info->connect_duration);
        user_data->CS.Unlock();
        //--
    }

    PostMessage(mainDlg->m_hWnd, UM_ON_CALL_MEDIA_STATE, (WPARAM)call_info, (LPARAM)user_data);
}


static void on_call_media_event(pjsua_call_id call_id,
    unsigned med_idx,
    pjmedia_event* event)
{
    statistics.cbCallMediaEvent++;
    statistics.cbCallMediaEventTime = CTime::GetCurrentTime().GetTime();
    //char event_name[5];

    //PJ_LOG(5, (THIS_FILENAME, "Event %s",
        //pjmedia_fourcc_name(event->type, event_name)));

    //#if PJSUA_HAS_VIDEO
        //if (event->type == PJMEDIA_EVENT_FMT_CHANGED) {
        //	pjsua_call_info ci;
        //	pjsua_call_get_info(call_id, &ci);
        //	if ((ci.media[med_idx].type == PJMEDIA_TYPE_VIDEO) &&
        //		(ci.media[med_idx].dir & PJMEDIA_DIR_DECODING)) {
        //		pjsua_vid_win_id wid;
        //		pjmedia_rect_size size;
        //		pjsua_vid_win_info win_info;

        //		wid = ci.media[med_idx].stream.vid.win_in;
        //		pjsua_vid_win_get_info(wid, &win_info);

        //		size = event->data.fmt_changed.new_fmt.det.vid.size;
        //		if (size.w != win_info.size.w || size.h != win_info.size.h) {
        //			pjsua_vid_win_set_size(wid, &size);
        //			/* Re-arrange video windows */
        //			arrange_window(PJSUA_INVALID_ID);
        //		}
        //	}
        //}
    //#else
    //	PJ_UNUSED_ARG(call_id);
    //	PJ_UNUSED_ARG(med_idx);
    //	PJ_UNUSED_ARG(event);
    //#endif
}

static void on_incoming_call(pjsua_acc_id acc, pjsua_call_id call_id,
    pjsip_rx_data* rdata)
{
    statistics.cbIncomingCall++;
    statistics.cbIncomingCallTime = CTime::GetCurrentTime().GetTime();
    statistics.postIncomingCall = 0;

    pjsua_call_info* call_info = new pjsua_call_info();
    if (pjsua_call_get_info(call_id, call_info) != PJ_SUCCESS) {
        return;
    }

    call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(call_info->id);
    if (!user_data) {
        user_data = new call_user_data(call_info->id);
        pjsua_call_set_user_data(call_info->id, user_data);
    }

    user_data->CS.Lock();

    if (accountSettings.forceCodec) {
        pjsua_call* call;
        pjsip_dialog* dlg;
        pj_status_t status;
        status = acquire_call("on_incoming_call()", call_id, &call, &dlg);
        if (status == PJ_SUCCESS) {
            pjmedia_sdp_neg_set_prefer_remote_codec_order(call->inv->neg, PJ_FALSE);
            pjsip_dlg_dec_lock(dlg);
        }
    }
    pjsip_generic_string_hdr* hsr;
    // -- diversion
    const pj_str_t headerDiversion = { "Diversion",9 };
    hsr = (pjsip_generic_string_hdr*)pjsip_msg_find_hdr_by_name(rdata->msg_info.msg, &headerDiversion, NULL);
    if (hsr) {
        CString str = MSIP::PjToStr(&hsr->hvalue, true);
        SIPURI sipuriDiversion;
        MSIP::ParseSIPURI(str, sipuriDiversion);
        user_data->diversion = !sipuriDiversion.user.IsEmpty() ? sipuriDiversion.user : sipuriDiversion.domain;
    }
    // -- end diversion
    // -- caller id
    user_data->callerID = GetPAI(rdata);
    if (!user_data->callerID.IsEmpty()) {
        user_data->name.Empty();
    }
    // -- end caller id
    // -- user agent
    const pj_str_t headerUserAgent = { "User-Agent",10 };
    hsr = (pjsip_generic_string_hdr*)pjsip_msg_find_hdr_by_name(rdata->msg_info.msg, &headerUserAgent, NULL);
    if (hsr) {
        user_data->userAgent = MSIP::PjToStr(&hsr->hvalue, true);
        int pos = user_data->userAgent.FindOneOf(_T("~+-"));
        if (pos) {
            user_data->userAgent = user_data->userAgent.Left(pos);
        }
    }
    // -- end user agent

    SIPURI sipuri;
    ParseCallSIPURI(call_info, user_data, sipuri);

    if (!accountSettings.cmdIncomingCall.IsEmpty()) {
        CString params = sipuri.user;
        MSIP::RunCmd(accountSettings.cmdIncomingCall, params);
    }
    //--
    //--
    bool busy = false;
    pjsua_call_id call_ids[PJSUA_MAX_CALLS];
    unsigned calls_count = PJSUA_MAX_CALLS;
    unsigned calls_count_cmp = 0;
    if (pjsua_enum_calls(call_ids, &calls_count) == PJ_SUCCESS) {
        for (unsigned i = 0; i < calls_count; ++i) {
            pjsua_call_info call_info_curr;
            if (pjsua_call_get_info(call_ids[i], &call_info_curr) == PJ_SUCCESS) {
                call_user_data* user_data_curr = (call_user_data*)pjsua_call_get_user_data(call_info_curr.id);
                SIPURI sipuri_curr;
                ParseCallSIPURI(&call_info_curr, user_data_curr, sipuri_curr);
                if (call_info_curr.id != call_info->id &&
                    sipuri.user + _T("@") + sipuri.domain == sipuri_curr.user + _T("@") + sipuri_curr.domain
                    ) {
                    busy = true;
                    break;
                }
                if (user_data_curr) {
                    user_data_curr->CS.Lock();
                    if (!user_data_curr->hangup && call_info_curr.state != PJSIP_INV_STATE_DISCONNECTED) {
                        calls_count_cmp++;
                    }
                    user_data_curr->CS.Unlock();
                }
                else {
                    if (call_info_curr.state != PJSIP_INV_STATE_DISCONNECTED) {
                        calls_count_cmp++;
                    }
                }
            }
        }
    }
    if (busy) {
        msip_call_busy(call_info->id, _T("Already in call with this contact"));
        user_data->hidden = true;
    }
    else if ((!accountSettings.callWaiting && calls_count_cmp > 1) || (accountSettings.maxConcurrentCalls > 0 && calls_count_cmp > accountSettings.maxConcurrentCalls)) {
        msip_call_busy(call_info->id, _T("Maximum concurrent calls reached"));
        user_data->hidden = true;
    }
    else if (!mainDlg->callIdIncomingIgnore.IsEmpty() && mainDlg->callIdIncomingIgnore == MSIP::PjToStr(&call_info->call_id)) {
        pjsua_call_answer(call_info->id, 487, NULL, NULL);
        user_data->hidden = true;
    }
    else {
        bool reject = false;
        CString reason;
        if (accountSettings.denyIncoming == _T("all")) {
            reject = true;
        }
        else if (accountSettings.denyIncoming == _T("button")) {
            reject = accountSettings.DND;
            reason = _T("Do Not Disturb");
        }
        else if (accountSettings.denyIncoming == _T("user")) {
            SIPURI sipuri_curr;
            MSIP::ParseSIPURI(MSIP::PjToStr(&call_info->local_info, TRUE), sipuri_curr);
            if (sipuri_curr.user != get_account_username()) {
                reject = true;
            }
        }
        else if (accountSettings.denyIncoming == _T("domain")) {
            SIPURI sipuri_curr;
            MSIP::ParseSIPURI(MSIP::PjToStr(&call_info->local_info, TRUE), sipuri_curr);
            if (accountSettings.accountId) {
                if (sipuri_curr.domain != get_account_domain()) {
                    reject = true;
                }
            }
        }
        else if (accountSettings.denyIncoming == _T("remotedomain")) {
            if (accountSettings.accountId) {
                if (sipuri.domain != get_account_domain()) {
                    reject = true;
                }
            }
        }
        else if (accountSettings.denyIncoming == _T("userdomain")) {
            SIPURI sipuri_curr;
            MSIP::ParseSIPURI(MSIP::PjToStr(&call_info->local_info, TRUE), sipuri_curr);
            if (sipuri_curr.user != get_account_username()) {
                reject = true;
            }
            else {
                CString domain = get_account_domain();
                if (domain != _T("") && sipuri_curr.domain != domain) {
                    reject = true;
                }
            }
        }
        if (reject) {
            if (reason.IsEmpty()) {
                reason = _T("Incoming calls blocked by user");
            }
            msip_call_busy(call_info->id, reason);
            user_data->hidden = true;
        }
        else {
            bool autoAnswer = false;
            int autoAnswerDelay = accountSettings.autoAnswerDelay;
            if (accountSettings.autoAnswer == _T("all")) {
                autoAnswer = true;
            }
            else if (accountSettings.autoAnswer == _T("button")) {
                autoAnswer = accountSettings.AA;
            }
            else if (accountSettings.autoAnswer == _T("header")) {
                //--
                pjsip_generic_string_hdr* hsr = NULL;
                const pj_str_t header = pj_str("X-AUTOANSWER");
                hsr = (pjsip_generic_string_hdr*)pjsip_msg_find_hdr_by_name(rdata->msg_info.msg, &header, NULL);
                if (hsr) {
                    CString autoAnswerValue = MSIP::PjToStr(&hsr->hvalue, TRUE);
                    autoAnswerValue.MakeLower();
                    if (autoAnswerValue == _T("true") || autoAnswerValue == _T("1")) {
                        autoAnswer = true;
                    }
                }
                //--
                if (!autoAnswer) {
                    pjsip_generic_string_hdr* hsr = NULL;
                    const pj_str_t header = pj_str("Call-Info");
                    hsr = (pjsip_generic_string_hdr*)pjsip_msg_find_hdr_by_name(rdata->msg_info.msg, &header, NULL);
                    if (hsr) {
                        CString callInfoValue = MSIP::PjToStr(&hsr->hvalue, TRUE);
                        callInfoValue.MakeLower();
                        if (callInfoValue.Find(_T("auto answer")) != -1) {
                            autoAnswer = true;
                        }
                        else {
                            // MySimpleSIP: CAtlRegExp → DialPlanRegexMatch (dialeto ATL traduzido)
                            DialPlanMatch mc;
                            if (DialPlanRegexMatch(_T("answer-after={[0-9]+}"), callInfoValue, mc) && mc.groups.size() == 1) {
                                CStringA text(callInfoValue.Mid(mc.groups[0].first, mc.groups[0].second - mc.groups[0].first));
                                autoAnswerDelay = atoi(text);
                                autoAnswer = true;
                            }
                        }
                    }
                }
            }

            if (autoAnswer && !accountSettings.autoAnswerNumber.IsEmpty()) {
                bool found = false;
                int pos = 0;
                CString resToken = accountSettings.autoAnswerNumber.Tokenize(_T(";|"), pos);
                while (!resToken.IsEmpty()) {
                    resToken.Trim();
                    if (!resToken.IsEmpty()) {
                        CMask mask;
                        if (mask.WildMatch(resToken, sipuri.user, _T(""))) {
                            found = true;
                            break;
                        }
                    }
                    resToken = accountSettings.autoAnswerNumber.Tokenize(_T(";|"), pos);
                }
                if (!found) {
                    autoAnswer = false;
                }
            }
            bool forwarding = false;
            if (!accountSettings.forwardingNumber.IsEmpty()) {
                if (accountSettings.forwarding == _T("all") ||
                    (accountSettings.forwarding == _T("button") && accountSettings.FWD)
                    ) {
                    forwarding = true;
                }
            }
            if (forwarding) {
                if (accountSettings.forwardingDelay > 0) {
                    if (autoAnswer && autoAnswerDelay > 0 && mainDlg->autoAnswerTimerCallId == PJSUA_INVALID_ID && autoAnswerDelay < accountSettings.forwardingDelay) {
                        //
                    }
                    else {
                        if (mainDlg->forwardingTimerCallId == PJSUA_INVALID_ID) {
                            mainDlg->forwardingTimerCallId = call_info->id;
                            mainDlg->SetTimer(IDT_TIMER_FORWARDING, accountSettings.forwardingDelay * 1000, NULL);
                        }
                    }
                }
                else {
                    user_data->forwarding = true;
                }
            }
            if (autoAnswer) {
                if (autoAnswerDelay > 0) {
                    if (mainDlg->autoAnswerTimerCallId == PJSUA_INVALID_ID) {
                        mainDlg->autoAnswerTimerCallId = call_info->id;
                        mainDlg->SetTimer(IDT_TIMER_AUTOANSWER, autoAnswerDelay * 1000, NULL);
                    }
                }
                else {
                    user_data->autoAnswer = true;
                }
            }
            statistics.postIncomingCall = 1;
            statistics.postIncomingCall0 = 0;
            statistics.postIncomingCall1 = 0;
            statistics.postIncomingCall2 = 0;
            statistics.postIncomingCall3 = 0;
            statistics.postIncomingCall4 = 0;
            statistics.postIncomingCall5 = 0;
            statistics.postIncomingCall6 = 0;
            statistics.postIncomingCall7 = 0;
            statistics.postIncomingCall8 = 0;
            statistics.postIncomingCall9 = 0;
            PostMessage(mainDlg->m_hWnd, UM_ON_INCOMING_CALL, (WPARAM)call_info, (LPARAM)user_data);
        }
    }
    user_data->CS.Unlock();
}


static void on_nat_detect(const pj_stun_nat_detect_result * res)
{
    statistics.cbNatDetect++;
    statistics.cbNatDetectTime = CTime::GetCurrentTime().GetTime();
    if (res->status != PJ_SUCCESS) {
        pjsua_perror(THIS_FILENAME, "NAT detection failed", res->status);
    }
    else {
        if (res->nat_type == PJ_STUN_NAT_TYPE_SYMMETRIC) {
            if (IsWindow(mainDlg->m_hWnd)) {
                CString message;
                //				pjsua_acc_config acc_cfg;
                //				pj_pool_t *pool;
                //				pool = pjsua_pool_create("acc_cfg-pjsua", 1000, 1000);
                //				if (pool) {
                //					pjsua_acc_id ids[PJSUA_MAX_ACC];
                //					unsigned count = PJSUA_MAX_ACC;
                //					if (pjsua_enum_accs(ids, &count) == PJ_SUCCESS) {
                //						for (unsigned i = 0; i < count; i++) {
                //							if (pjsua_acc_get_config(ids[i], pool, &acc_cfg) == PJ_SUCCESS) {
                //								acc_cfg.sip_stun_use = PJSUA_STUN_USE_DISABLED;
                //								acc_cfg.media_stun_use = PJSUA_STUN_USE_DISABLED;
                //								if (pjsua_acc_modify(ids[i], &acc_cfg) == PJ_SUCCESS) {
                //									message = _T("STUN was automatically disabled.");
//									message.Append(_T(" For more info visit MicroSIP website, help page."));
//
//								}
//							}
//						}
//					}
//					pj_pool_release(pool);
//				}
                message = _T("The softphpne may not work properly with enabled STUN and your internet connection.");
                mainDlg->BaloonPopup(Translate(_T("Symmetric NAT detected!")), Translate(message));
            }
        }
        PJ_LOG(3, (THIS_FILENAME, "NAT detected as %s", res->nat_type_name));
    }
}

void on_buddy_state(pjsua_buddy_id buddy_id)
{
    statistics.cbBuddyState++;
    statistics.cbBuddyStateTime = CTime::GetCurrentTime().GetTime();
    if (!IsWindow(mainDlg->m_hWnd)) {
        return;
    }
    mainDlg->PostMessage(UM_ON_BUDDY_STATE, (WPARAM)buddy_id);
}


static void on_pager2(pjsua_call_id call_id, const pj_str_t * from, const pj_str_t * to, const pj_str_t * contact, const pj_str_t * mime_type, const pj_str_t * body, pjsip_rx_data * rdata, pjsua_acc_id acc_id)
{
    statistics.cbPager++;
    statistics.cbPagerTime = CTime::GetCurrentTime().GetTime();
    if (pj_strcmp2(mime_type, "text/plain") != 0 || accountSettings.disableMessaging) {
        return;
    }
    if (IsWindow(mainDlg->m_hWnd)) {
        CString* number = new CString();
        CString* message = new CString();
        number->SetString(MSIP::PjToStr(from, TRUE));
        message->SetString(MSIP::PjToStr(body, TRUE));
        message->Trim();
        call_user_data user_data(PJSUA_INVALID_ID);
        user_data.callerID = GetPAI(rdata);
        SIPURI sipuri;
        ParseCallSIPURI(*number, &user_data, sipuri);
        //-- fix domain
        if (accountSettings.accountId) {
            if (MSIP::IsIP(sipuri.domain)) {
                sipuri.domain = get_account_domain();
            }
        }
        //--
        number->SetString(MSIP::BuildSIPURI(sipuri));
        mainDlg->PostMessage(UM_ON_PAGER, (WPARAM)number, (LPARAM)message);
    }
}

static void on_pager_status2(pjsua_call_id call_id, const pj_str_t * to, const pj_str_t * body, void* user_data, pjsip_status_code status, const pj_str_t * reason, pjsip_tx_data * tdata, pjsip_rx_data * rdata, pjsua_acc_id acc_id)
{
    statistics.cbPagerStatus++;
    statistics.cbPagerStatusTime = CTime::GetCurrentTime().GetTime();
    if (status >= 300) {
        if (IsWindow(mainDlg->m_hWnd)) {
            CString* number = new CString();
            CString* message = new CString();
            number->SetString(MSIP::PjToStr(to, TRUE));
            message->SetString(MSIP::PjToStr(reason, TRUE));
            message->Trim();
            call_user_data user_data(PJSUA_INVALID_ID);
            user_data.callerID = GetPAI(rdata);
            SIPURI sipuri;
            ParseCallSIPURI(*number, &user_data, sipuri);
            //-- fix domain
            if (accountSettings.accountId) {
                if (MSIP::IsIP(sipuri.domain)) {
                    sipuri.domain = get_account_domain();
                }
            }
            //--
            number->SetString(MSIP::BuildSIPURI(sipuri));
            mainDlg->PostMessage(UM_ON_PAGER_STATUS, (WPARAM)number, (LPARAM)message);
        }
    }
}

static void on_call_transfer_status(pjsua_call_id call_id,
    int status_code,
    const pj_str_t * status_text,
    pj_bool_t final,
    pj_bool_t * p_cont)
{
    statistics.cbCallTransferStatus++;
    statistics.cbCallTransferStatusTime = CTime::GetCurrentTime().GetTime();
    pjsua_call_info* call_info = new pjsua_call_info();
    if (pjsua_call_get_info(call_id, call_info) != PJ_SUCCESS || call_info->state == PJSIP_INV_STATE_NULL) {
        return;
    }

    call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(call_info->id);
    if (!user_data) {
        user_data = new call_user_data(call_info->id);
        pjsua_call_set_user_data(call_info->id, user_data);
    }

    CString* str = new CString();
    str->Format(_T("%s: %s"),
        Translate(_T("Call Transfer")),
        MSIP::PjToStr(status_text, TRUE)
    );
    if (final) {
        str->AppendFormat(_T(" [%s]"), Translate(_T("Final")));
    }

    if (status_code / 100 == 2) {
        *p_cont = PJ_FALSE;
    }

    call_info->last_status = (pjsip_status_code)status_code;

    call_info->call_id.ptr = (char*)user_data;
    call_info->call_id.slen = 0;

    PostMessage(mainDlg->m_hWnd, UM_ON_CALL_TRANSFER_STATUS, (WPARAM)call_info, (LPARAM)str);
}


static void on_call_transfer_request2(pjsua_call_id call_id, const pj_str_t * dst, pjsip_status_code * code, pjsua_call_setting * opt)
{
    statistics.cbCallTransferRequest++;
    statistics.cbCallTransferRequestTime = CTime::GetCurrentTime().GetTime();
    SIPURI sipuri;
    MSIP::ParseSIPURI(MSIP::PjToStr(dst, TRUE), sipuri);
    pj_bool_t cont;
    CString number = sipuri.user;
    if (number.IsEmpty()) {
        number = sipuri.domain;
    }
    else if (!accountSettings.accountId || sipuri.domain != get_account_domain()) {
        number.Append(_T("@") + sipuri.domain);
    }
    char* buf = MSIP::WideCharToPjStr(number);
    pj_str_t transfer_text = pj_str(buf); // MySimpleSIP: GCC não aceita &pj_str(...)
    on_call_transfer_status(call_id,
        0,
        &transfer_text,
        PJ_FALSE,
        &cont);
    free(buf);
    //--
    if (!code) {
        // if our function call
        return;
    }
    pjsua_call_info call_info;
    if (pjsua_call_get_info(call_id, &call_info) != PJ_SUCCESS || call_info.state != PJSIP_INV_STATE_CONFIRMED) {
        *code = PJSIP_SC_DECLINE;
    }
    if (*code != PJSIP_SC_DECLINE) {
        // deny transfer if we already have a call with same dest address
        pjsua_call_id call_ids[PJSUA_MAX_CALLS];
        unsigned calls_count = PJSUA_MAX_CALLS;
        if (pjsua_enum_calls(call_ids, &calls_count) == PJ_SUCCESS) {
            for (unsigned i = 0; i < calls_count; ++i) {
                pjsua_call_info call_info_curr;
                if (pjsua_call_get_info(call_ids[i], &call_info_curr) == PJ_SUCCESS) {
                    call_user_data* user_data_curr = (call_user_data*)pjsua_call_get_user_data(call_info_curr.id);
                    SIPURI sipuri_curr;
                    ParseCallSIPURI(&call_info_curr, user_data_curr, sipuri_curr);
                    if (sipuri.user + _T("@") + sipuri.domain == sipuri_curr.user + _T("@") + sipuri_curr.domain
                        ) {
                        *code = PJSIP_SC_DECLINE;
                        break;
                    }
                }
            }
        }
    }
}

static void on_call_replace_request2(pjsua_call_id call_id, pjsip_rx_data * rdata, int* st_code, pj_str_t * st_text, pjsua_call_setting * opt)
{
    statistics.cbCallReplaceRequest++;
    statistics.cbCallReplaceRequestTime = CTime::GetCurrentTime().GetTime();
    pjsua_call_info call_info;
    if (pjsua_call_get_info(call_id, &call_info) == PJ_SUCCESS) {
        if (!call_info.rem_vid_cnt) {
            opt->vid_cnt = 0;
        }
    }
    else {
        opt->vid_cnt = 0;
    }
}

static void on_call_replaced(pjsua_call_id old_call_id, pjsua_call_id new_call_id)
{
    statistics.cbCallReplaced++;
    statistics.cbCallReplacedTime = CTime::GetCurrentTime().GetTime();
    pjsua_call_info call_info;
    if (pjsua_call_get_info(new_call_id, &call_info) == PJ_SUCCESS) {
        on_call_transfer_request2(old_call_id, &call_info.remote_info, NULL, NULL);
    }
}

static void on_mwi_info(pjsua_acc_id acc_id, pjsua_mwi_info * mwi_info)
{
    statistics.cbMWIInfo++;
    statistics.cbMWIInfoTime = CTime::GetCurrentTime().GetTime();
    bool hasMail = false;
    if (mwi_info->rdata->msg_info.ctype) {
        const pjsip_ctype_hdr* ctype = mwi_info->rdata->msg_info.ctype;
        if (pj_strcmp2(&ctype->media.type, "application") != 0 || pj_strcmp2(&ctype->media.subtype, "simple-message-summary") != 0) {
            return;
        }
    }
    if (!mwi_info->rdata->msg_info.msg->body || !mwi_info->rdata->msg_info.msg->body->len) {
        return;
    }
    pjsip_msg_body* body = mwi_info->rdata->msg_info.msg->body;
    LPARAM lParam = 0;
    pj_scanner scanner;
    pj_scan_init(&scanner, (char*)body->data, body->len, PJ_SCAN_AUTOSKIP_WS, 0);
    while (!pj_scan_is_eof(&scanner)) {
        pj_str_t key;
        pj_scan_get_until_chr(&scanner, ":", &key);
        pj_strtrim(&key);
        if (key.slen && !pj_scan_is_eof(&scanner)) {
            scanner.curptr++;
            pj_str_t value;
            pj_scan_get_until_chr(&scanner, "\r\n", &value);
            pj_strtrim(&value);
            if (pj_stricmp2(&key, "Messages-Waiting") == 0) {
                hasMail = pj_stricmp2(&value, "yes") == 0;
                break;
            }
        }
    }
    pj_scan_fini(&scanner);
    PostMessage(mainDlg->m_hWnd, UM_ON_MWI_INFO, (WPARAM)hasMail, lParam);
}


static void on_dtmf_digit(pjsua_call_id call_id, int digit)
{
    statistics.cbDtmfDigit++;
    statistics.cbDtmfDigitTime = CTime::GetCurrentTime().GetTime();
    if (!IsWindow(mainDlg->m_hWnd)) {
        return;
    }
    mainDlg->PostMessage(UM_ON_DTMF_DIGIT, (WPARAM)call_id, (LPARAM)digit);
}


static void on_call_tsx_state(pjsua_call_id call_id, pjsip_transaction * tsx, pjsip_event * e)
{
    statistics.cbCallTsxState++;
    statistics.cbCallTsxStateTime = CTime::GetCurrentTime().GetTime();
    const pjsip_method update_method = {
        PJSIP_OTHER_METHOD,
        { "UPDATE", 6 }
    };
    if (tsx->method.id == PJSIP_INVITE_METHOD || pjsip_method_cmp(&tsx->method, &update_method) == 0) {
        /*
        * Handle INVITE/UPDATE method.
        */
        if (e->body.tsx_state.type == PJSIP_EVENT_RX_MSG) {
            pjsip_rx_data* rdata = e->body.rx_msg.rdata;
            // --
            CString pai = GetPAI(rdata);
            if (!pai.IsEmpty()) {
                call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(call_id);
                if (user_data) {
                    user_data->CS.Lock();
                    user_data->callerID = pai;
                    user_data->name.Empty();
                    user_data->CS.Unlock();
                }
            }
        }
        return;
    }
    const pjsip_method info_method = {
        PJSIP_OTHER_METHOD,
        { "INFO", 4 }
    };
    if (pjsip_method_cmp(&tsx->method, &info_method) == 0) {
        /*
        * Handle INFO method.
        */
        if (tsx->role == PJSIP_ROLE_UAS && tsx->state == PJSIP_TSX_STATE_TRYING) {
            if (e->body.tsx_state.type == PJSIP_EVENT_RX_MSG) {
                pjsip_rx_data* rdata = e->body.tsx_state.src.rdata;
                pjsip_msg_body* body = rdata->msg_info.msg->body;
                int code = 0;
                if (body && body->len
                    && pj_strcmp2(&body->content_type.type, "application") == 0
                    && pj_strcmp2(&body->content_type.subtype, "dtmf-relay") == 0) {
                    code = 400;
                    pj_scanner scanner;
                    pj_scan_init(&scanner, (char*)body->data, body->len, PJ_SCAN_AUTOSKIP_WS, 0);
                    char digit;
                    int duration = 250;
                    while (!pj_scan_is_eof(&scanner)) {
                        pj_str_t key;
                        pj_scan_get_until_chr(&scanner, "=", &key);
                        pj_strtrim(&key);
                        if (key.slen && !pj_scan_is_eof(&scanner)) {
                            scanner.curptr++;
                            pj_str_t value;
                            pj_scan_get_until_chr(&scanner, "\r\n", &value);
                            pj_strtrim(&value);
                            if (pj_stricmp2(&key, "Signal") == 0) {
                                if (value.slen == 1) {
                                    digit = *value.ptr;
                                    code = 200;
                                }
                            }
                            else if (pj_stricmp2(&key, "Duration") == 0) {
                                int res = 0;
                                for (int i = 0; i < (unsigned)value.slen; ++i) {
                                    res = res * 10 + (value.ptr[i] - '0');
                                    res = res;
                                }
                                if (res >= 100 || res <= 5000) {
                                    duration = res;
                                }
                            }
                        }
                    }
                    pj_scan_fini(&scanner);
                    if (code == 200) {
                        on_dtmf_digit(-1, digit);
                    }
                }
                else if (!body || !body->len) {
                    /* 200/OK */
                    code = 200;
                }
                if (code) {
                    /* Answer incoming INFO */
                    pjsip_tx_data* tdata;
                    if (pjsip_endpt_create_response(tsx->endpt, rdata,
                        code, NULL, &tdata) == PJ_SUCCESS
                        ) {
                        pjsip_tsx_send_msg(tsx, tdata);
                    }
                }
            }
        }
        return;
    }
    const pjsip_method cancel_method = {
        PJSIP_CANCEL_METHOD,
        { "CANCEL", 6 }
    };
    if (pjsip_method_cmp(&tsx->method, &cancel_method) == 0) {
        /*
        * Handle CANCEL method.
        */
        if (e->body.tsx_state.type == PJSIP_EVENT_RX_MSG) {
            pjsip_rx_data* rdata = e->body.rx_msg.rdata;
            // -- reason
            const pj_str_t headerReason = { "Reason",6 };
            pjsip_generic_string_hdr* hsr;
            hsr = (pjsip_generic_string_hdr*)pjsip_msg_find_hdr_by_name(rdata->msg_info.msg, &headerReason, NULL);
            if (hsr) {
                CString str = MSIP::PjToStr(&hsr->hvalue, true);
                int pos = str.Find(_T("text=\""));
                if (pos != -1) {
                    str = str.Mid(pos + 6);
                    pos = str.Find(_T("\""));
                    if (pos != -1) {
                        str = str.Left(pos);
                        call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(call_id);
                        if (user_data) {
                            user_data->CS.Lock();
                            user_data->reason = str;
                            user_data->CS.Unlock();
                        }
                    }
                }
            }
            // -- end reason
        }
        return;
    }
    if (tsx->state == PJSIP_TSX_STATE_COMPLETED) {
        // display declined REFER status
        const pjsip_method refer_method = {
            PJSIP_OTHER_METHOD,
            { "REFER", 5 }
        };
        if (pjsip_method_cmp(&tsx->method, &refer_method) == 0 && tsx->status_code / 100 != 2) {
            pj_bool_t cont;
            on_call_transfer_status(call_id,
                tsx->status_code,
                &tsx->status_text,
                PJ_FALSE,
                &cont);
        }
    }
}

static pjsip_redirect_op on_call_redirected(pjsua_call_id call_id,
    const pjsip_uri * target,
    const pjsip_event * e)
{
    statistics.cbCallRedirected++;
    statistics.cbCallRedirectedTime = CTime::GetCurrentTime().GetTime();
    return PJSIP_REDIRECT_ACCEPT_REPLACE;
}

// MySimpleSIP: UnicastIpChangeCallback/RouteChangeCallback (NotifyUnicastIpAddressChange,
// NotifyRouteChange2 do Windows) removidos; o monitor de rede do Linux (Fase 7) envia a
// mesma mensagem UM_NETWORK_CHANGE para a janela principal.


// MySimpleSIP: no original estas atribuições ficavam em CmainDlg::PJCreateRaw
void msip_set_pjsua_callbacks(pjsua_config* ua_cfg)
{
    ua_cfg->cb.on_reg_started2 = &on_reg_started2;
    ua_cfg->cb.on_reg_state2 = &on_reg_state2;
    ua_cfg->cb.on_call_state = &on_call_state;
    ua_cfg->cb.on_dtmf_digit = &on_dtmf_digit;
    ua_cfg->cb.on_call_tsx_state = &on_call_tsx_state;

    ua_cfg->cb.on_call_redirected = &on_call_redirected;

    ua_cfg->cb.on_call_media_state = &on_call_media_state;
    ua_cfg->cb.on_call_media_event = &on_call_media_event;
    ua_cfg->cb.on_incoming_call = &on_incoming_call;
    ua_cfg->cb.on_nat_detect = &on_nat_detect;
    ua_cfg->cb.on_buddy_state = &on_buddy_state;
    ua_cfg->cb.on_pager2 = &on_pager2;
    ua_cfg->cb.on_pager_status2 = &on_pager_status2;
    ua_cfg->cb.on_call_transfer_request2 = &on_call_transfer_request2;
    ua_cfg->cb.on_call_transfer_status = &on_call_transfer_status;

    ua_cfg->cb.on_call_replace_request2 = &on_call_replace_request2;
    ua_cfg->cb.on_call_replaced = &on_call_replaced;

    ua_cfg->cb.on_mwi_info = &on_mwi_info;
}
