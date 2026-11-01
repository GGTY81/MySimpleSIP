/*
 * MySimpleSIP — Importar configuração (port Qt do ImportDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de ImportDlg.cpp (MicroSIP 3.22.16) sobre o IDD_IMPORT original.

#include "ImportDlg.h"

#include "Dialer.h"
#include "UiCommon.h"
#include "mainDlg.h"

#include <QCheckBox>
#include <QListWidget>

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

ImportDlg::ImportDlg(QWidget* pParent) : AccountsListsDlg(pParent, QStringLiteral("IDD_IMPORT"), QStringLiteral("IDC_IMPORT"))
{
    show();
}

ImportDlg::~ImportDlg()
{
    if (MainDlg() && MainDlg()->importDlg == this) {
        MainDlg()->importDlg = nullptr;
    }
}

void ImportDlg::Init(CString filename)
{
    inFilename = filename;
    FillAccounts(inFilename, false);
    // IDC_EXPORT_SETTINGS/SHORTCUTS no original: mesmos valores de IDC_IMPORT_*
    InitCheckbox(_T("Settings"), "_SETTINGS", inFilename);
    InitCheckbox(_T("Shortcuts"), "_SHORTCUTS", inFilename);
}

void ImportDlg::OnClose()
{
    // DestroyWindow → OnDestroy (mainDlg->importDlg = NULL) → delete
    if (m_destroyed) {
        return;
    }
    if (MainDlg() && MainDlg()->importDlg == this) {
        MainDlg()->importDlg = nullptr;
    }
    Destroy();
}

void ImportDlg::OnBnClickedOk()
{
    bool restart = false;
    bool activate = false;
    QListWidget* listbox2 = ListSelected();
    bool hasAccounts = listbox2->count();
    bool hasSettings = m_layout.Check(QStringLiteral("IDC_IMPORT_SETTINGS"))->isChecked();
    bool hasShortcuts = m_layout.Check(QStringLiteral("IDC_IMPORT_SHORTCUTS"))->isChecked();
    if (hasAccounts || hasSettings || hasShortcuts) {
        if (hasSettings) {
            accountSettings.SettingsLoad(inFilename);
            restart = true;
        }
        if (hasShortcuts) {
            MainDlg()->ShortcutsRemoveAll();
            ShortcutsLoad(inFilename);
            ShortcutsSave();
            if (!hasSettings) {
                if (!accountSettings.enableShortcuts) {
                    accountSettings.enableShortcuts = true;
                    restart = true;
                }
                else {
                    MainDlg()->pageDialer->RebuildShortcuts();
                }
            }
        }
        if (hasAccounts) {
            Account dummy;
            int i = 1;
            while (true) {
                if (!accountSettings.AccountLoad(i, &dummy)) {
                    break;
                }
                i++;
            }
            int idCurr = i;
            bool hadAccounts = false;
            if (idCurr > 1) {
                hadAccounts = true;
            }
            for (int i = 0; i < listbox2->count(); i++) {
                CString value = CS(listbox2->item(i)->text());
                int pos = value.Find(':');
                int idRead = _wtoi(value.Left(pos));
                int idWrite;
                if (idRead == 0) {
                    idWrite = idRead;
                }
                else {
                    idWrite = idCurr;
                    idCurr++;
                }
                accountSettings.AccountLoad(idRead, &dummy, inFilename);
                accountSettings.AccountSave(idWrite, &dummy);
                if (!hadAccounts) {
                    activate = true;
                }
            }
        }
        if (restart) {
            if (activate) {
                accountSettings.accountId = 1;
                accountSettings.SettingsSave();
            }
            MainDlg()->PostMessage(UM_RESTART, 0, 0);
        }
        else if (activate) {
            MainDlg()->OnMenuAccountChange(0); // ID_ACCOUNT_CHANGE_RANGE → conta 1
        }
        OnClose();
    }
}
