/*
 * MySimpleSIP — diálogo de contato (port Qt do AddDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Janela não modal (IDD_ADD) para adicionar ou editar um contato: nome, número,
// presença, favorito e dados extras. OK grava pela aba Contatos (ContactAdd).

#pragma once

#include "Contact.h"
#include "DialogLayout.h"

#include <QWidget>

class AddDlg : public QWidget {
public:
    explicit AddDlg(QWidget* pParent = nullptr);
    ~AddDlg() override;

    void Load(Contact* pContact);

    void OnClose();
    void OnBnClickedOk();
    void OnBnClickedCancel();
    void OnChangeFirstLastName();
    void OnChangePhoneNumber();

    CDialogLayout m_layout;

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    CString number;
    CString firstname;
    CString lastname;
    CString phone;
    bool loading = false;

private:
    void OnInitDialog();
    CString Text(const char* id) const;             // GetDlgItem(id)->GetWindowText
    void SetText(const char* id, const CString& s); // GetDlgItem(id)->SetWindowText
};
