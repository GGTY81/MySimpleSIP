/*
 * MySimpleSIP — parte pjsua do CmainDlg do MicroSIP (sem interface gráfica).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// No MicroSIP, CmainDlg (mainDlg.cpp, 6.100 linhas) mistura a janela principal com o
// controle do pjsua. Aqui fica a parte pjsua, portada com os mesmos nomes de membros e
// métodos: PJCreate/PJCreateRaw/PJDestroy, codecs, dispositivos de som, contas,
// PublishStatus e o player de sons; os callbacks on_* do pjsua estão em callbacks.cpp.
//
// Onde o original chama a interface (UpdateWindowText, pageDialer->SetName, TaskDialog,
// BaloonPopup...), há um método virtual com implementação vazia: a janela Qt (Fase 3)
// deriva desta classe e os sobrescreve; a CLI de teste também.
//
// Os handlers das mensagens UM_ON_* (onCallState, onIncomingCall... — executados na
// thread da UI e cheios de chamadas de interface) não estão aqui: chegam pela fila de
// mensagens (compat/Win32Window.h) e são tratados por quem deriva desta classe.

#pragma once

#include "global.h"
#include "settings.h"
#include "resource.h"
#include "Statistics.h"

class CmainDlgCore : public CWnd {
public:
    CmainDlgCore();
    virtual ~CmainDlgCore();

    // --- estado usado pelos callbacks (mesmos nomes do CmainDlg) ---
    pjsua_call_id autoAnswerTimerCallId = PJSUA_INVALID_ID;
    pjsua_call_id autoAnswerPlayCallId = PJSUA_INVALID_ID;
    pjsua_call_id forwardingTimerCallId = PJSUA_INVALID_ID;
    ::player_eof_data* player_eof_data = nullptr; // MySimpleSIP: "::" (GCC: -Wchanges-meaning)
    CString callIdIncomingIgnore;
    CList<int, int> toneCalls;
    CList<int, int> attendedCalls;
    CList<CString> audioCodecList;
    CList<int> confernceCalls;
    bool usersDirectoryLoaded = false;
    bool shortcutsURLLoaded = false;
    bool isSubscribed = false;
    pjrpid_activity m_PresenceStatus = PJRPID_ACTIVITY_UNKNOWN;

    // --- pjsua (portado do CmainDlg) ---
    void PJCreate();
    void PJDestroy(bool exit = false);
    void PJAccountAdd();
    void PJAccountAddRaw();
    void PJAccountAddLocal();
    void PJAccountDelete(bool deep = false, bool exit = false, CStringA code = "");
    void PJAccountDeleteLocal();
    void PJAccountConfig(pjsua_acc_config* acc_cfg, Account* account);
    void PJAudioCodecs();
#ifdef _GLOBAL_VIDEO
    void PJVideoCodecs();
    int VideoCaptureDeviceId(CString name = _T(""));
#endif
    void UpdateSoundDevicesIds();
    void PublishStatus(bool online = true, bool init = false);
    void PlayerPlay(CString filename, bool noLoop = false, bool inCall = false, bool isAA = false);
    void PlayerStop();
    // Handlers de UM_CALL_ANSWER / UM_CALL_HANGUP (msip_call_answer e afins postam estas
    // mensagens para a janela principal); quem deriva desta classe deve encaminhá-las.
    LRESULT onCallAnswer(WPARAM wParam, LPARAM lParam);
    LRESULT onCallHangup(WPARAM wParam, LPARAM lParam);

    // MySimpleSIP: usa o dispositivo de som nulo do pjsua (testes, CLI sem áudio)
    bool nullSoundDevice = false;

    // --- ganchos para a interface (no original: chamadas diretas à UI) ---
    virtual void UpdateWindowText(CString /*text*/ = CString(), int /*icon*/ = IDI_DEFAULT, bool /*afterRegister*/ = false) {}
    virtual void SetPaneText2(CString /*str*/ = _T("")) {}
    virtual void SetWindowText(const CString& /*title*/) {}
    virtual void DialerSetName() {}                         // pageDialer->SetName()
    virtual HWND DialerHwnd() { return m_hWnd; }            // pageDialer->m_hWnd (timer do VU-meter)
    virtual void BaloonPopup(CString /*title*/, CString /*message*/, DWORD /*flags*/ = NIIF_WARNING, UINT /*id*/ = MSIP_BALOON_DEFAULT) {}
    virtual void OnAccount(WPARAM /*wParam*/, LPARAM /*lParam*/) {} // abrir diálogo de conta
    virtual void InitUI() {}
    virtual void CheckUpdates() {}
    virtual void UsersDirectoryLoad(bool /*update*/ = false) {}
    virtual void Subscribe() {}
    virtual void Unsubscribe() {}
    virtual void onPlayerStop() { PlayerStop(); }           // CmainDlg::onPlayerStop
    virtual void createPreviewWin() {}                      // preview de vídeo local
    virtual bool IsForegroundApp() { return true; }         // janela do app em primeiro plano?
    virtual void RestoreWindow() {}                         // mostrar/trazer a janela principal
    // Nome a exibir para a chamada (contatos/diretório); padrão: nome da URI ou usuário
    virtual CString GetNameForCall(SIPURI& sipuri, call_user_data* /*user_data*/, const CString& /*numberOriginal*/ = L"")
    {
        return sipuri.name.IsEmpty() ? sipuri.user : sipuri.name;
    }
    // TaskDialog "Network connection is not available" em PJCreate: true = tentar de novo
    virtual bool AskRetryNetwork() { return false; }
    // MySimpleSIP: substituem WTSRegisterSessionNotification, CMMNotificationClient,
    // NotifyUnicastIpAddressChange/NotifyRouteChange2 e Hid (monitores da Fase 7)
    virtual void PlatformMonitorsStart() {}
    virtual void PlatformMonitorsStop() {}

protected:
    void PJCreateRaw();
};

// No MicroSIP: CmainDlg* mainDlg. Aqui aponta para o objeto que deriva de CmainDlgCore.
extern CmainDlgCore* mainDlg;
extern Statistics statistics;

// MySimpleSIP: procura o identificador real de um codec de vídeo pelo nome ("H264" →
// "H264/97"); os payload types do pjproject upstream diferem dos do MicroSIP.
CString msip_vid_codec_id(const CString& name);

// Registra os callbacks on_* (callbacks.cpp) na configuração do pjsua.
void msip_set_pjsua_callbacks(pjsua_config* ua_cfg);
