/*
 * MySimpleSIP — base dos diálogos Exportar/Importar configuração.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Código comum de ExportDlg.cpp e ImportDlg.cpp (MicroSIP 3.22.16).

#include "AccountsListsDlg.h"

#include "UiCommon.h"
#include "global.h"
#include "langpack.h"
#include "settings.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>

AccountsListsDlg::AccountsListsDlg(QWidget* pParent, const QString& idd, const QString& prefix)
    : QWidget(pParent, Qt::Dialog), m_prefix(prefix)
{
    if (langPack.rtl) {
        setLayoutDirection(Qt::RightToLeft);
    }
    m_layout.customFactory = [this](const QString& id, const QString&, QWidget* p) -> QWidget* {
        if (id == Id("_ACCOUNTS_ALL") || id == Id("_ACCOUNTS")) {
            return new QListWidget(p);
        }
        if (id == Id("_SPIN_MODIFY") || id == Id("_SPIN_ORDER")) {
            return CreateUpDown(p, id == Id("_SPIN_MODIFY"));
        }
        return nullptr;
    };
    m_layout.Build(this, idd);
    m_layout.TranslateDialog(this);
    m_listAll = static_cast<QListWidget*>(m_layout.Item(Id("_ACCOUNTS_ALL")));
    m_listSelected = static_cast<QListWidget*>(m_layout.Item(Id("_ACCOUNTS")));
    m_listAll->installEventFilter(this);
    m_listSelected->installEventFilter(this);

    QWidget* spinModify = m_layout.Item(Id("_SPIN_MODIFY"));
    QObject::connect(spinModify->findChild<QToolButton*>(QStringLiteral("second")), &QToolButton::clicked, this,
                     [this] { OnDeltaposSpinModify(-1); }); // → adicionar
    QObject::connect(spinModify->findChild<QToolButton*>(QStringLiteral("first")), &QToolButton::clicked, this,
                     [this] { OnDeltaposSpinModify(1); });  // ← remover
    QWidget* spinOrder = m_layout.Item(Id("_SPIN_ORDER"));
    QObject::connect(spinOrder->findChild<QToolButton*>(QStringLiteral("first")), &QToolButton::clicked, this,
                     [this] { OnDeltaposSpinOrder(-1); });
    QObject::connect(spinOrder->findChild<QToolButton*>(QStringLiteral("second")), &QToolButton::clicked, this,
                     [this] { OnDeltaposSpinOrder(1); });
    QObject::connect(m_layout.Button(QStringLiteral("IDOK")), &QPushButton::clicked, this, [this] { OnBnClickedOk(); });
    QObject::connect(m_layout.Button(QStringLiteral("IDCANCEL")), &QPushButton::clicked, this, [this] { OnBnClickedCancel(); });
    m_layout.Button(QStringLiteral("IDOK"))->setDefault(true);
}

void AccountsListsDlg::FillAccounts(const CString& filename, bool skipLocal)
{
    Account acc;
    int i = skipLocal ? 1 : 0;
    CString str;
    while (true) {
        if (!accountSettings.AccountLoad(i, &acc, filename)) {
            if (i > 0) {
                break;
            }
        }
        else {
            str.Format(_T("%d: %s"), i, GetAccountLabel(&acc, i));
            m_listSelected->addItem(QS(str));
        }
        i++;
    }
}

void AccountsListsDlg::InitCheckbox(const CString& section, const char* suffix, const CString& filename)
{
    QCheckBox* checkbox = m_layout.Check(Id(suffix));
    TCHAR buffer[3];
    if (GetPrivateProfileString(section, NULL, NULL, buffer, 3, filename)) {
        checkbox->setEnabled(true);
        checkbox->setChecked(true);
    }
    else {
        checkbox->setEnabled(false);
        checkbox->setChecked(false);
    }
}

void AccountsListsDlg::closeEvent(QCloseEvent* event)
{
    if (!m_destroyed && !g_bClosingApp) {
        OnClose();
    }
    event->accept();
}

void AccountsListsDlg::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        OnBnClickedCancel();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool AccountsListsDlg::eventFilter(QObject* watched, QEvent* event)
{
    // OnVKeyToItem: Espaço adiciona a conta selecionada; Delete a retira
    if (event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (watched == m_listAll && m_listAll->currentRow() != -1 && ke->key() == Qt::Key_Space) {
            OnDeltaposSpinModify(-1);
            return true;
        }
        if (watched == m_listSelected && m_listSelected->currentRow() != -1 && ke->key() == Qt::Key_Delete) {
            OnDeltaposSpinModify(1);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void AccountsListsDlg::Destroy()
{
    m_destroyed = true;
    hide();
    deleteLater();
}

void AccountsListsDlg::OnBnClickedCancel()
{
    OnClose();
}

void AccountsListsDlg::OnDeltaposSpinModify(int iDelta)
{
    QListWidget* listbox = m_listAll;
    QListWidget* listbox2 = m_listSelected;
    if (iDelta == -1) {
        //add
        int selected = listbox->currentRow();
        if (selected != -1) {
            listbox2->addItem(listbox->takeItem(selected));
            listbox->setCurrentRow(selected < listbox->count() ? selected : selected - 1);
        }
    }
    else {
        //remove
        int selected = listbox2->currentRow();
        if (selected != -1) {
            listbox->addItem(listbox2->takeItem(selected));
            listbox2->setCurrentRow(selected < listbox2->count() ? selected : selected - 1);
        }
    }
}

void AccountsListsDlg::OnDeltaposSpinOrder(int iDelta)
{
    QListWidget* listbox2 = m_listSelected;
    int selected = listbox2->currentRow();
    if (selected != -1) {
        if (iDelta == -1) {
            //up
            if (selected > 0) {
                listbox2->insertItem(selected - 1, listbox2->takeItem(selected));
                listbox2->setCurrentRow(selected - 1);
            }
        }
        else {
            //down
            if (selected < listbox2->count() - 1) {
                listbox2->insertItem(selected + 1, listbox2->takeItem(selected));
                listbox2->setCurrentRow(selected + 1);
            }
        }
    }
}
