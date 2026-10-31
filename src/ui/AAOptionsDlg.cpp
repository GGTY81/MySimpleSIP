/*
 * MySimpleSIP — opções do atendimento automático (port Qt do AAOptionsDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de AAOptionsDlg.cpp (MicroSIP 3.22.16) sobre o IDD_AA_OPTIONS original.

#include "AAOptionsDlg.h"

#include "SettingsDlg.h"
#include "UiCommon.h"
#include "langpack.h"
#include "mainDlg.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QCursor>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

static CString callsItems[] = {
_T(""),
_T("hold"),
_T("all"),
};
static CString callsValues[] = {
_T("No"),
_T("Hold"),
_T("All Calls"),
};

AAOptionsDlg::AAOptionsDlg(QWidget* pParent) : QWidget(pParent, Qt::Dialog)
{
    if (langPack.rtl) {
        setLayoutDirection(Qt::RightToLeft);
    }
    m_layout.Build(this, QStringLiteral("IDD_AA_OPTIONS"));
    // OnCreate: perto do cursor (no Wayland, o compositor decide)
    const QPoint p = QCursor::pos();
    move(p.x() - 250, p.y() + 10);
    OnInitDialog();
    show();
}

void AAOptionsDlg::OnInitDialog()
{
    m_layout.TranslateDialog(this);
    CString str;
    str.Format(_T("%d"), accountSettings.autoAnswerDelay);
    m_layout.Edit(QStringLiteral("IDC_AA_OPTIONS_DELAY"))->setText(QS(str));
    m_layout.Edit(QStringLiteral("IDC_AA_OPTIONS_NUMBER"))->setText(QS(accountSettings.autoAnswerNumber));
    QComboBox* combobox = m_layout.Combo(QStringLiteral("IDC_AA_OPTIONS_CALLS"));
    int n = sizeof(callsItems) / sizeof(callsItems[0]);
    bool found = false;
    for (int i = 0; i < n; i++) {
        combobox->addItem(TR(callsValues[i]));
        if (accountSettings.autoAnswerCalls == callsItems[i]) {
            combobox->setCurrentIndex(i);
            found = true;
        }
    }
    if (!found) {
        combobox->setCurrentIndex(0);
    }
    QObject::connect(m_layout.Button(QStringLiteral("IDOK")), &QPushButton::clicked, this, [this] { OnBnClickedOk(); });
    QObject::connect(m_layout.Button(QStringLiteral("IDCANCEL")), &QPushButton::clicked, this, [this] { OnBnClickedCancel(); });
    m_layout.Button(QStringLiteral("IDOK"))->setDefault(true);
}

void AAOptionsDlg::closeEvent(QCloseEvent* event)
{
    if (!m_destroyed && !g_bClosingApp) {
        OnClose();
    }
    event->accept();
}

void AAOptionsDlg::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        OnBnClickedCancel();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        OnBnClickedOk();
        return;
    }
    QWidget::keyPressEvent(event);
}

void AAOptionsDlg::OnClose()
{
    // DestroyWindow → OnDestroy (settingsDlg->aaOptionsDlg = nullptr) → delete
    if (m_destroyed) {
        return;
    }
    m_destroyed = true;
    if (MainDlg()->settingsDlg && MainDlg()->settingsDlg->aaOptionsDlg == this) {
        MainDlg()->settingsDlg->aaOptionsDlg = nullptr;
    }
    hide();
    deleteLater();
}

void AAOptionsDlg::OnBnClickedOk()
{
    CString str = CS(m_layout.Edit(QStringLiteral("IDC_AA_OPTIONS_DELAY"))->text());
    accountSettings.autoAnswerDelay = _wtoi(str);
    accountSettings.autoAnswerNumber = CS(m_layout.Edit(QStringLiteral("IDC_AA_OPTIONS_NUMBER"))->text());
    accountSettings.autoAnswerNumber.Trim();
    QComboBox* combobox = m_layout.Combo(QStringLiteral("IDC_AA_OPTIONS_CALLS"));
    accountSettings.autoAnswerCalls = callsItems[combobox->currentIndex() < 0 ? 0 : combobox->currentIndex()];
    accountSettings.SettingsSave();
    OnClose();
}

void AAOptionsDlg::OnBnClickedCancel()
{
    OnClose();
}
