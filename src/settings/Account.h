/*
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// MySimpleSIP: partes de global.h do MicroSIP 3.22.16 sem dependência do pjsua
// (struct Account e tipos de atalho), para a camada de configurações.
// O restante de global.h será portado em src/core/ na Fase 2.

#pragma once

#include "afx.h"

#define MSIP_SHORTCUT_CALL _T("call")
#define MSIP_SHORTCUT_VIDEOCALL _T("video")
#define MSIP_SHORTCUT_MESSAGE _T("message")
#define MSIP_SHORTCUT_DTMF _T("dtmf")
#define MSIP_SHORTCUT_BT _T("transfer")
#define MSIP_SHORTCUT_AT _T("attendedTransfer")
#define MSIP_SHORTCUT_CALL_BT _T("callOrBT")
#define MSIP_SHORTCUT_CALL_AT _T("callOrAT")
#define MSIP_SHORTCUT_CONF _T("conference")
#define MSIP_SHORTCUT_CALL_CONF _T("callOrConference")

struct Account {
    CString label;
    CString server;
    CString proxy;
    CString username;
    CString domain;
    int port;
    CString authID;
    CString password;
    CString digest;
    bool rememberPassword;
    CString displayName;
    CString dialingPrefix;
    CString dialPlan;
    CString hideCID;
    CString voicemailNumber;
    CString srtp;
    CString transport;
    CString publicAddr;
    int registerRefresh;
    int keepAlive;
    bool publish;
    bool ice;
    bool allowRewrite;
    bool disableSessionTimer;
    bool operator==(const Account& a) const {
        if (
            label == a.label
            && server == a.server
            && proxy == a.proxy
            && username == a.username
            && domain == a.domain
            && port == a.port
            && authID == a.authID
            && password == a.password
            && digest == a.digest
            && displayName == a.displayName
            && dialingPrefix == a.dialingPrefix
            && dialPlan == a.dialPlan
            && hideCID == a.hideCID
            && voicemailNumber == a.voicemailNumber
            && srtp == a.srtp
            && transport == a.transport
            && publicAddr == a.publicAddr
            && registerRefresh == a.registerRefresh
            && keepAlive == a.keepAlive
            && publish == a.publish
            && ice == a.ice
            && allowRewrite == a.allowRewrite
            && disableSessionTimer == a.disableSessionTimer
            )
            return true;
        return false;
    };
    bool operator!=(const Account& a) const {
        return !(*this == a);
    }
    void operator=(const Account& a)
    {
        label = a.label;
        server = a.server;
        proxy = a.proxy;
        username = a.username;
        domain = a.domain;
        port = a.port;
        authID = a.authID;
        password = a.password;
        digest = a.digest;
        displayName = a.displayName;
        dialingPrefix = a.dialingPrefix;
        dialPlan = a.dialPlan;
        hideCID = a.hideCID;
        voicemailNumber = a.voicemailNumber;
        srtp = a.srtp;
        transport = a.transport;
        publicAddr = a.publicAddr;
        registerRefresh = a.registerRefresh;
        keepAlive = a.keepAlive;
        publish = a.publish;
        ice = a.ice;
        allowRewrite = a.allowRewrite;
        disableSessionTimer = a.disableSessionTimer;
    };
    Account() : port(0)
        , rememberPassword(false)
        , registerRefresh(0)
        , keepAlive(0)
        , publish(false)
        , ice(false)
        , allowRewrite(false)
        , disableSessionTimer(false)
    {
    }
};
