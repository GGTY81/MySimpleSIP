/*
 * MySimpleSIP — diálogo de conta SIP (port Qt do AccountDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "DialogLayout.h"
#include "settings.h"

#include <QWidget>

class AccountDlg : public QWidget {
public:
    explicit AccountDlg(QWidget* parent = nullptr);
    ~AccountDlg() override;

    // id > 0: conta existente; 0: conta local; -1: nova conta
    void Load(int id);
    // Foco no campo de senha (OnAccount com wParam, após senha incorreta)
    void FocusPassword();

private:
    void OnInitDialog();
    void OnBnClickedOk();
    void OnNMClickSyslinkDelete();
    void OnNMClickSyslinkDisplayPasswod();
    void OnChangePassword();

    CDialogLayout m_layout;
    int accountId = -1;
    Account m_Account;
};
