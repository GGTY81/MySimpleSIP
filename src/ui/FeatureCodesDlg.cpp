/*
 * MySimpleSIP — códigos de função (port Qt do FeatureCodesDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de FeatureCodesDlg.cpp (MicroSIP 3.22.16) sobre o IDD_FEATURE_CODES original.

#include "FeatureCodesDlg.h"

#include "SettingsDlg.h"
#include "UiCommon.h"
#include "langpack.h"
#include "mainDlg.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QCursor>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

FeatureCodesDlg::FeatureCodesDlg(QWidget* pParent) : QWidget(pParent, Qt::Dialog)
{
    if (langPack.rtl) {
        setLayoutDirection(Qt::RightToLeft);
    }
    m_layout.Build(this, QStringLiteral("IDD_FEATURE_CODES"));
    const QPoint p = QCursor::pos();
    move(p.x() - 250, p.y() + 10);
    OnInitDialog();
    show();
}

void FeatureCodesDlg::OnInitDialog()
{
    m_layout.TranslateDialog(this);
    m_layout.Edit(QStringLiteral("IDC_FEATURE_CODES_CP"))->setText(QS(accountSettings.featureCodeCP));
    m_layout.Check(QStringLiteral("IDC_FEATURE_CODES_ENABLE_CP"))->setChecked(accountSettings.enableFeatureCodeCP);
    m_layout.Edit(QStringLiteral("IDC_FEATURE_CODES_BT"))->setText(QS(accountSettings.featureCodeBT));
    m_layout.Check(QStringLiteral("IDC_FEATURE_CODES_ENABLE_BT"))->setChecked(accountSettings.enableFeatureCodeBT);
    m_layout.Edit(QStringLiteral("IDC_FEATURE_CODES_AT"))->setText(QS(accountSettings.featureCodeAT));
    m_layout.Check(QStringLiteral("IDC_FEATURE_CODES_ENABLE_AT"))->setChecked(accountSettings.enableFeatureCodeAT);
    QObject::connect(m_layout.Button(QStringLiteral("IDOK")), &QPushButton::clicked, this, [this] { OnBnClickedOk(); });
    QObject::connect(m_layout.Button(QStringLiteral("IDCANCEL")), &QPushButton::clicked, this, [this] { OnBnClickedCancel(); });
    m_layout.Button(QStringLiteral("IDOK"))->setDefault(true);
}

void FeatureCodesDlg::closeEvent(QCloseEvent* event)
{
    if (!m_destroyed && !g_bClosingApp) {
        OnClose();
    }
    event->accept();
}

void FeatureCodesDlg::keyPressEvent(QKeyEvent* event)
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

void FeatureCodesDlg::OnClose()
{
    if (m_destroyed) {
        return;
    }
    m_destroyed = true;
    if (MainDlg()->settingsDlg && MainDlg()->settingsDlg->featureCodesDlg == this) {
        MainDlg()->settingsDlg->featureCodesDlg = nullptr;
    }
    hide();
    deleteLater();
}

void FeatureCodesDlg::OnBnClickedOk()
{
    accountSettings.featureCodeCP = CS(m_layout.Edit(QStringLiteral("IDC_FEATURE_CODES_CP"))->text());
    accountSettings.enableFeatureCodeCP = m_layout.Check(QStringLiteral("IDC_FEATURE_CODES_ENABLE_CP"))->isChecked();
    accountSettings.featureCodeBT = CS(m_layout.Edit(QStringLiteral("IDC_FEATURE_CODES_BT"))->text());
    accountSettings.enableFeatureCodeBT = m_layout.Check(QStringLiteral("IDC_FEATURE_CODES_ENABLE_BT"))->isChecked();
    accountSettings.featureCodeAT = CS(m_layout.Edit(QStringLiteral("IDC_FEATURE_CODES_AT"))->text());
    accountSettings.enableFeatureCodeAT = m_layout.Check(QStringLiteral("IDC_FEATURE_CODES_ENABLE_AT"))->isChecked();
    accountSettings.SettingsSave();
    OnClose();
}

void FeatureCodesDlg::OnBnClickedCancel()
{
    OnClose();
}
