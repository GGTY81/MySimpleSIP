/*
 * MySimpleSIP — janela de chamada recebida (port Qt do RinginDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Uma janela por chamada tocando (mainDlg->ringinDlgs): nome e endereço de quem liga,
// Atender, Atender com vídeo, Recusar e Desviar. Fechar a janela ignora a chamada
// (ou recusa, com noIgnoreCall). Mesmos nomes de métodos do original.

#pragma once

#include "DialogLayout.h"
#include "afx.h"

#include <pjsua-lib/pjsua.h>

#include <QWidget>

class RinginDlg : public QWidget, public CWnd {
public:
    RinginDlg(QWidget* pParent = nullptr, bool bVideo = false);
    ~RinginDlg() override;

    void CallAccept(BOOL hasVideo);
    void CallAcceptDefault();
    pjsua_call_id GetCallId();
    void SetCallId(pjsua_call_id nCallId);
    void OnAnswer();
    void DestroyWindow(); // MySimpleSIP: CWnd::DestroyWindow → deleteLater

    // GetDlgItem(IDC_CALLER_NAME / IDC_RINGIN_NAME_BLIND / IDC_CALLER_ADDR)->SetWindowText
    void SetCallerName(const CString& name);
    void SetCallerAddr(const CString& addr); // vazio → EnableWindow(FALSE)

    void OnClose();
    void OnBnClickedCancel();
    void OnBnClickedAudio();
    void OnBnClickedVideo();
    void OnBnClickedDecline();
    void OnBnClickedTransfer();

    CDialogLayout m_layout;

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void moveEvent(QMoveEvent* event) override;

private:
    void OnInitDialog();
    void Close(BOOL accept = FALSE);

    bool m_bVideo;
    bool m_bAnswered = false;
    pjsua_call_id m_nCallId = PJSUA_INVALID_ID;
    bool m_destroyed = false;
};
