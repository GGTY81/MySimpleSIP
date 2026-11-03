/*
 * MySimpleSIP — janela de atalhos do discador (port Qt do ShortcutsDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de ShortcutsDlg.cpp (MicroSIP 3.22.16) sobre o IDD_SHORTCUTS original. Os controles
// de cada linha têm IDs base + i (ex.: IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL+3).

#include "ShortcutsDlg.h"

#include "Dialer.h"
#include "UiCommon.h"
#include "langpack.h"
#include "mainDlg.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

// GetDlgItem(IDC_... + i)
static QString ItemId(const char* base, int i)
{
    return QStringLiteral("%1+%2").arg(QLatin1String(base)).arg(i);
}

static int numberWidth = -1;
static int numberWidth2 = -1;
static int numberHeight = -1;

static CString defaultActionItems[] = {
    MSIP_SHORTCUT_CALL,
#ifdef _GLOBAL_VIDEO
    MSIP_SHORTCUT_VIDEOCALL,
#endif
    MSIP_SHORTCUT_MESSAGE,
    MSIP_SHORTCUT_DTMF,
    MSIP_SHORTCUT_BT,
    MSIP_SHORTCUT_AT,
    MSIP_SHORTCUT_CONF,
    MSIP_SHORTCUT_CALL_BT,
    MSIP_SHORTCUT_CALL_AT,
    MSIP_SHORTCUT_CALL_CONF,
};
static CString defaultActionValues[] = {
    _T("Call"),
#ifdef _GLOBAL_VIDEO
    _T("Video Call"),
#endif
    _T("Message"),
    _T("DTMF"),
    _T("Blind Transfer"),
    _T("Attended Transfer"),
    _T("Conference"),
    _T("Call/Blind Transfer"),
    _T("Call/Attended Transfer"),
    _T("Call/Conference"),

};

ShortcutsDlg::ShortcutsDlg(QWidget* pParent) : QWidget(pParent, Qt::Dialog)
{
    if (langPack.rtl) {
        setLayoutDirection(Qt::RightToLeft);
    }
    m_layout.Build(this, QStringLiteral("IDD_SHORTCUTS"));
    OnInitDialog();
    show();
}

ShortcutsDlg::~ShortcutsDlg()
{
    if (MainDlg() && MainDlg()->shortcutsDlg == this) {
        MainDlg()->shortcutsDlg = nullptr;
    }
}

void ShortcutsDlg::OnInitDialog()
{
    m_layout.TranslateDialog(this);

    QLabel* label = m_layout.Label(QStringLiteral("IDC_SHORTCUTS_LABEL2"));
    label->setText(label->text() + QStringLiteral(" 2"));
    label = m_layout.Label(QStringLiteral("IDC_SHORTCUTS_NUMBER2"));
    label->setText(label->text() + QStringLiteral(" 2"));

    m_layout.Check(QStringLiteral("IDC_SHORTCUTS_ENABLE"))->setChecked(accountSettings.enableShortcuts);
    m_layout.Check(QStringLiteral("IDC_SHORTCUTS_BOTTOM"))->setChecked(accountSettings.shortcutsBottom);

    // OnNMClickSyslinkType/Toggle/BLF: links "?" para o manual
    static const struct {
        const char* id;
        LPCTSTR code;
    } helpLinks[] = {
        {"IDC_SHORTCUTS_SYSLINK_TYPE", _T("shortcutType")},
        {"IDC_SHORTCUTS_SYSLINK_TOGGLE", _T("toggle")},
        {"IDC_SHORTCUTS_SYSLINK_BLF", _T("BLF")},
    };
    for (const auto& h : helpLinks) {
        if (QLabel* link = m_layout.Label(QString::fromLatin1(h.id))) {
            LPCTSTR code = h.code;
            link->setProperty("msipHelpCode", QS(code));
            QObject::connect(link, &QLabel::linkActivated, this, [code] { OpenHelp(code); });
        }
    }

    QWidget* number = m_layout.Item(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER", 0));
    numberWidth = number->width();
    numberHeight = number->height();
    numberWidth2 = m_layout.Item(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2", 0))->width();

    for (int i = 0; i < _GLOBAL_SHORTCUTS_QTY; i++) {
        QObject::connect(m_layout.Check(ItemId("IDC_SHORTCUTS_TOGGLE", i)), &QCheckBox::clicked, this,
                         [this, i] { OnBnClickedToggle(i); });
    }
    QObject::connect(m_layout.Button(QStringLiteral("IDOK")), &QPushButton::clicked, this, [this] { OnBnClickedOk(); });
    QObject::connect(m_layout.Button(QStringLiteral("IDCANCEL")), &QPushButton::clicked, this, [this] { OnBnClickedCancel(); });
    m_layout.Button(QStringLiteral("IDOK"))->setDefault(true);

    // MySimpleSIP: o original posta WM_USER + 1 e preenche as linhas aos poucos (com o
    // Salvar desabilitado até o fim); aqui o preenchimento é imediato
    OnInitialied();
}

void ShortcutsDlg::OnInitialied()
{
    QStringList listActions;
    int n = sizeof(defaultActionItems) / sizeof(defaultActionItems[0]);
    for (int j = 0; j < n; j++) {
        CString str = defaultActionValues[j];
        int pos = str.Find('/');
        if (pos != -1) {
            CString str1 = Translate(str.Left(pos));
            CString str2 = Translate(str.Mid(pos + 1));
            listActions.append(QS(str1 + _T(" / ") + str2));
        }
        else {
            listActions.append(TR(str));
        }
    }

    int c = shortcuts.GetCount();
    for (int i = 0; i < _GLOBAL_SHORTCUTS_QTY; i++) {
        QComboBox* combobox = m_layout.Combo(ItemId("IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE", i));
        combobox->addItems(listActions);
        if (i < c) {
            Shortcut shortcut;
            shortcut = shortcuts.GetAt(i);
            m_layout.Edit(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL", i))->setText(QS(shortcut.label));
            m_layout.Edit(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER", i))->setText(QS(shortcut.number));
            m_layout.Edit(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2", i))->setText(QS(shortcut.label2));
            m_layout.Edit(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2", i))->setText(QS(shortcut.number2));
            int m = 0;
            for (int j = 0; j < n; j++) {
                if (shortcut.type == defaultActionItems[j]) {
                    m = j;
                }
            }
            combobox->setCurrentIndex(m);
            if (!shortcut.number2.IsEmpty()) {
                m_layout.Check(ItemId("IDC_SHORTCUTS_TOGGLE", i))->setChecked(true);
                UpdateToggle(true, i);
            }
            m_layout.Check(ItemId("IDC_SHORTCUTS_PRESENCE", i))->setChecked(shortcut.presence);
        }
        else {
            combobox->setCurrentIndex(0);
        }
    }
}

void ShortcutsDlg::closeEvent(QCloseEvent* event)
{
    if (!m_destroyed && !g_bClosingApp) {
        OnClose();
    }
    event->accept();
}

void ShortcutsDlg::keyPressEvent(QKeyEvent* event)
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

void ShortcutsDlg::OnClose()
{
    // DestroyWindow → OnDestroy (mainDlg->shortcutsDlg = nullptr) → delete
    if (m_destroyed) {
        return;
    }
    m_destroyed = true;
    if (MainDlg() && MainDlg()->shortcutsDlg == this) {
        MainDlg()->shortcutsDlg = nullptr;
    }
    hide();
    deleteLater();
}

void ShortcutsDlg::OnBnClickedCancel()
{
    OnClose();
}

void ShortcutsDlg::OnBnClickedOk()
{
    hide();
    MainDlg()->ShortcutsRemoveAll();
    for (int i = 0; i < _GLOBAL_SHORTCUTS_QTY; i++) {
        Shortcut shortcut;
        shortcut.label = CS(m_layout.Edit(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL", i))->text());
        shortcut.number = CS(m_layout.Edit(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER", i))->text());
        shortcut.label2 = CS(m_layout.Edit(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2", i))->text());
        shortcut.number2 = CS(m_layout.Edit(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2", i))->text());
        if (!m_layout.Check(ItemId("IDC_SHORTCUTS_TOGGLE", i))->isChecked()) {
            shortcut.label2.Empty();
            shortcut.number2.Empty();
        }
        shortcut.presence = m_layout.Check(ItemId("IDC_SHORTCUTS_PRESENCE", i))->isChecked();
        int n = m_layout.Combo(ItemId("IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE", i))->currentIndex();
        if (n >= 0 && !shortcut.label.IsEmpty() &&
            (!shortcut.number.IsEmpty() ||
                defaultActionItems[n] == MSIP_SHORTCUT_BT ||
                defaultActionItems[n] == MSIP_SHORTCUT_AT)
            ) {
            if (shortcut.label2.IsEmpty()) {
                shortcut.label2 = shortcut.label;
            }
            shortcut.type = defaultActionItems[n];
            shortcuts.Add(shortcut);
        }
    }
    ShortcutsSave();

    accountSettings.enableShortcuts = m_layout.Check(QStringLiteral("IDC_SHORTCUTS_ENABLE"))->isChecked();
    accountSettings.shortcutsBottom = m_layout.Check(QStringLiteral("IDC_SHORTCUTS_BOTTOM"))->isChecked();

    MainDlg()->pageDialer->RebuildShortcutsRestart();

    OnClose();
}

void ShortcutsDlg::OnBnClickedToggle(int i)
{
    bool checked = m_layout.Check(ItemId("IDC_SHORTCUTS_TOGGLE", i))->isChecked();
    UpdateToggle(checked, i);
    if (!checked) {
        bool hasChecked = false;
        for (int j = 0; j < _GLOBAL_SHORTCUTS_QTY; j++) {
            if (m_layout.Check(ItemId("IDC_SHORTCUTS_TOGGLE", j))->isChecked()) {
                hasChecked = true;
                break;
            }
        }
        if (!hasChecked) {
            m_layout.Item(QStringLiteral("IDC_SHORTCUTS_LABEL2"))->hide();
            m_layout.Item(QStringLiteral("IDC_SHORTCUTS_NUMBER2"))->hide();
        }
    }
}

void ShortcutsDlg::UpdateToggle(bool check, int i)
{
    QWidget* number = m_layout.Item(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER", i));
    if (check) {
        number->resize(numberWidth2, numberHeight);
        m_layout.Item(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2", i))->show();
        m_layout.Item(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2", i))->show();
        m_layout.Item(QStringLiteral("IDC_SHORTCUTS_LABEL2"))->show();
        m_layout.Item(QStringLiteral("IDC_SHORTCUTS_NUMBER2"))->show();
    }
    else {
        number->resize(numberWidth, numberHeight);
        m_layout.Item(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2", i))->hide();
        m_layout.Item(ItemId("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2", i))->hide();
    }
}
