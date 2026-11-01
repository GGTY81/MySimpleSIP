/*
 * MySimpleSIP — Exportar configuração (port Qt do ExportDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de ExportDlg.cpp (MicroSIP 3.22.16) sobre o IDD_EXPORT original.

#include "ExportDlg.h"

#include "UiCommon.h"
#include "mainDlg.h"

#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QListWidget>

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

ExportDlg::ExportDlg(QWidget* pParent) : AccountsListsDlg(pParent, QStringLiteral("IDD_EXPORT"), QStringLiteral("IDC_EXPORT"))
{
    OnInitDialog();
    show();
}

ExportDlg::~ExportDlg()
{
    if (MainDlg() && MainDlg()->exportDlg == this) {
        MainDlg()->exportDlg = nullptr;
    }
}

void ExportDlg::OnInitDialog()
{
    // o original preenche IDC_IMPORT_ACCOUNTS, que tem o mesmo valor de IDC_EXPORT_ACCOUNTS:
    // todas as contas já começam na lista das selecionadas
    FillAccounts(accountSettings.iniFile, !accountSettings.enableLocalAccount);
    InitCheckbox(_T("Settings"), "_SETTINGS", accountSettings.iniFile);
    InitCheckbox(_T("Shortcuts"), "_SHORTCUTS", accountSettings.iniFile);
}

void ExportDlg::OnClose()
{
    // DestroyWindow → OnDestroy (mainDlg->exportDlg = NULL) → delete
    if (m_destroyed) {
        return;
    }
    if (MainDlg() && MainDlg()->exportDlg == this) {
        MainDlg()->exportDlg = nullptr;
    }
    Destroy();
}

void ExportDlg::OnBnClickedOk()
{
    bool hasAccounts = ListSelected()->count();
    bool hasSettings = m_layout.Check(QStringLiteral("IDC_EXPORT_SETTINGS"))->isChecked();
    bool hasShortcuts = m_layout.Check(QStringLiteral("IDC_EXPORT_SHORTCUTS"))->isChecked();
    if (hasAccounts || hasSettings || hasShortcuts) {
        // CFileDialog (Ini Files) → QFileDialog (confirma a sobrescrita)
        QString file = QFileDialog::getSaveFileName(this, TR(_T("Export Configuration")), QFileInfo(QS(accountSettings.iniFile)).fileName(),
                                                    QStringLiteral("Ini Files (*.ini)"));
        if (!file.isEmpty()) {
            if (QFileInfo(file).suffix().isEmpty()) {
                file.append(QStringLiteral(".ini"));
            }
            Export(CS(file));
        }
    }
}

void ExportDlg::Export(CString filename)
{
    bool hasAccounts = ListSelected()->count();
    bool hasSettings = m_layout.Check(QStringLiteral("IDC_EXPORT_SETTINGS"))->isChecked();
    bool hasShortcuts = m_layout.Check(QStringLiteral("IDC_EXPORT_SHORTCUTS"))->isChecked();
    CString sectionGlobal = _T("Global");
    // MySimpleSIP: o original cria o arquivo com BOM UTF-16 para o INI do Windows ser Unicode;
    // aqui o INI é UTF-8, basta recriar o arquivo vazio com a seção [Global]
    QFile file(QS(filename));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QStringLiteral("[%1]\n").arg(QS(sectionGlobal)).toUtf8());
        file.close();
    }
    WritePrivateProfileString(sectionGlobal, _T("version"), _T(_GLOBAL_VERSION), filename);
    if (hasSettings) {
        accountSettings.SettingsSave(filename);
    }
    if (hasShortcuts) {
        ShortcutsSave(filename);
    }
    if (hasAccounts) {
        int idCurr = 1;
        Account dummy;
        for (int i = 0; i < ListSelected()->count(); i++) {
            CString value = CS(ListSelected()->item(i)->text());
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
            accountSettings.AccountLoad(idRead, &dummy);
            accountSettings.AccountSave(idWrite, &dummy, filename);
        }
    }
    OnClose();
}
