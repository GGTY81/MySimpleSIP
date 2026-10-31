/*
 * MySimpleSIP — diálogo de transferência/conferência (port Qt do Transfer do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// IDD_TRANSFER: número (lista com os contatos — só os favoritos, se houver), teclado e
// OK, ou Transferência assistida/cega quando há código de função de transferência
// assistida. A ação vai para MessagesDlg::CallAction. Uma instância por vez
// (mainDlg->transferDlg), destruída ao fechar.

#pragma once

#include "Contact.h"
#include "DialogLayout.h"
#include "global.h"

#include <QWidget>

class QComboBox;

enum msip_action {
    MSIP_ACTION_TRANSFER,
    MSIP_ACTION_ATTENDED_TRANSFER,
    MSIP_ACTION_INVITE,
    MSIP_ACTION_FORWARD,
};

class Transfer : public QWidget {
public:
    explicit Transfer(QWidget* pParent = nullptr);
    ~Transfer() override;

    msip_action action = MSIP_ACTION_TRANSFER;
    pjsua_call_id callId = PJSUA_INVALID_ID;
    void SetAction(msip_action action, pjsua_call_id call_id = PJSUA_INVALID_ID);
    bool Action(msip_action action);
    void LoadFromContacts(Contact* selectedContact = NULL);

    void OnClose();
    void Input(CString digits);
    void OnBnClickedOk();
    void OnBnClickedCancel();
    void OnBnClickedAttnded();
    void OnBnClickedBlind();

    CDialogLayout m_layout;

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void OnInitDialog();
    void ClearDropdown();
    void SetDropDownWidth(QComboBox* combo);
    QComboBox* m_number = nullptr;
    bool m_destroyed = false;
};
