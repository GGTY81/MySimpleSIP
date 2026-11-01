/*
 * MySimpleSIP — base dos diálogos Exportar/Importar configuração.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// MySimpleSIP: ExportDlg e ImportDlg do MicroSIP repetem o mesmo código para as duas listas
// de contas (todas / selecionadas), as setas e as teclas Espaço/Delete; aqui esse trecho fica
// numa base comum, com os mesmos nomes (OnDeltaposSpinModify, OnDeltaposSpinOrder,
// OnVKeyToItem, InitCheckbox). Os IDs dos controles são <prefixo>_ACCOUNTS_ALL,
// _ACCOUNTS, _SPIN_MODIFY, _SPIN_ORDER, _SETTINGS e _SHORTCUTS (IDC_EXPORT/IDC_IMPORT).

#pragma once

#include "DialogLayout.h"
#include "afx.h"

#include <QWidget>

class QListWidget;

class AccountsListsDlg : public QWidget {
public:
    AccountsListsDlg(QWidget* pParent, const QString& idd, const QString& prefix);

    virtual void OnClose() = 0;
    void OnBnClickedCancel();
    virtual void OnBnClickedOk() = 0;
    void OnDeltaposSpinModify(int iDelta); // -1 = adicionar (→), 1 = remover (←)
    void OnDeltaposSpinOrder(int iDelta);  // -1 = subir, 1 = descer

    QListWidget* ListAll() const { return m_listAll; }        // _ACCOUNTS_ALL
    QListWidget* ListSelected() const { return m_listSelected; } // _ACCOUNTS
    CDialogLayout m_layout;

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override; // OnVKeyToItem
    QString Id(const char* suffix) const { return m_prefix + QLatin1String(suffix); }
    // lista de contas de um INI ("N: rótulo"), como no OnInitDialog/Init originais
    void FillAccounts(const CString& filename, bool skipLocal);
    void InitCheckbox(const CString& section, const char* suffix, const CString& filename);
    void Destroy(); // DestroyWindow: esconde e apaga depois

    bool m_destroyed = false;

private:
    QString m_prefix;
    QListWidget* m_listAll = nullptr;
    QListWidget* m_listSelected = nullptr;
};
