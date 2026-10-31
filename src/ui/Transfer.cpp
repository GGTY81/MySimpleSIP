/*
 * MySimpleSIP — diálogo de transferência/conferência (port Qt do Transfer do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de Transfer.cpp (MicroSIP 3.22.16) sobre o IDD_TRANSFER original (CDialogLayout).
// O número de cada item da lista fica no dado do item (no original, CString* no
// ItemData). Diferenças marcadas com "MySimpleSIP:".

#include "Transfer.h"

#include "Contacts.h"
#include "MessagesDlg.h"
#include "UiCommon.h"
#include "langpack.h"
#include "mainDlg.h"

#include <QAbstractItemView>
#include <QCloseEvent>
#include <QComboBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>

#include <vector>

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

Transfer::Transfer(QWidget* pParent) : QWidget(pParent, Qt::Dialog)
{
    if (langPack.rtl) {
        setLayoutDirection(Qt::RightToLeft);
    }
    m_layout.Build(this, QStringLiteral("IDD_TRANSFER"));
    m_number = m_layout.Combo(QStringLiteral("IDC_NUMBER"));
    OnInitDialog();
    show(); // WS_VISIBLE
}

Transfer::~Transfer()
{
}

void Transfer::OnInitDialog()
{
    setWindowIcon(LoadImageIcon(IDI_MAINFRAME));
    m_layout.TranslateDialog(this);

    // lfHeight = MulDiv(16, dpiY, 96)
    QFont font = this->font();
    font.setPixelSize(16);
    m_number->setFont(font);
    m_number->setInsertPolicy(QComboBox::NoInsert);
    m_number->setGeometry(m_number->x(), m_number->y(), m_number->width(), DluY(14));
    for (const char* id : {"IDC_KEY_1", "IDC_KEY_2", "IDC_KEY_3", "IDC_KEY_4", "IDC_KEY_5", "IDC_KEY_6",
                           "IDC_KEY_7", "IDC_KEY_8", "IDC_KEY_9", "IDC_KEY_0", "IDC_KEY_STAR", "IDC_KEY_GRATE"}) {
        QPushButton* key = m_layout.Button(QString::fromLatin1(id));
        key->setFont(font);
        const QString digit = key->text();
        QObject::connect(key, &QPushButton::clicked, this, [this, digit] { Input(CS(digit)); });
    }
    QObject::connect(m_layout.Button(QStringLiteral("IDOK")), &QPushButton::clicked, this, [this] { OnBnClickedOk(); });
    QObject::connect(m_layout.Button(QStringLiteral("IDCANCEL")), &QPushButton::clicked, this, [this] { OnBnClickedCancel(); });
    QObject::connect(m_layout.Button(QStringLiteral("IDC_TRANSFER_ATTENDED")), &QPushButton::clicked, this, [this] { OnBnClickedAttnded(); });
    QObject::connect(m_layout.Button(QStringLiteral("IDC_TRANSFER_BLIND")), &QPushButton::clicked, this, [this] { OnBnClickedBlind(); });
    if (QLineEdit* edit = m_number->lineEdit()) {
        QObject::connect(edit, &QLineEdit::returnPressed, this, [this] { OnBnClickedOk(); }); // IDOK
    }
}

void Transfer::SetDropDownWidth(QComboBox* combo)
{
    int maxWidth = 0;
    const QFontMetrics fm(combo->font());
    for (int i = 0; i < combo->count(); ++i) {
        maxWidth = std::max(maxWidth, fm.horizontalAdvance(combo->itemText(i)));
    }
    maxWidth += 20 + 10; // SM_CXVSCROLL + 10
    combo->view()->setMinimumWidth(std::max(maxWidth, combo->width()));
}

void Transfer::LoadFromContacts(Contact* selectedContact)
{
    ClearDropdown();
    QComboBox* combobox = m_number;
    int selectedIndex = -1;
    bool hasStarred = false;
    POSITION pos = MainDlg()->pageContacts->contacts.GetHeadPosition();
    while (pos) {
        Contact* contact = MainDlg()->pageContacts->contacts.GetNext(pos);
        if (contact->starred) {
            hasStarred = true;
            break;
        }
    };
    pos = MainDlg()->pageContacts->contacts.GetHeadPosition();
    int i = 0;
    while (pos) {
        Contact* contact = MainDlg()->pageContacts->contacts.GetNext(pos);
        if (!hasStarred || contact->starred) {
            if (selectedContact == contact) {
                selectedIndex = i;
            }
            std::vector<CString> itemsNumber;
            std::vector<CString> itemsType;
            itemsNumber.push_back(contact->number);
            itemsType.push_back(_T(""));
            if (!contact->phone.IsEmpty()) {
                itemsNumber.push_back(contact->phone);
                itemsType.push_back(Translate(_T("Phone:")));
            }
            if (!contact->mobile.IsEmpty()) {
                itemsNumber.push_back(contact->mobile);
                itemsType.push_back(Translate(_T("Mobile:")));
            }
            for (size_t j = 0; j < itemsNumber.size(); ++j)
            {
                CString& number = itemsNumber[j];
                CString& type = itemsType[j];
                CString str;
                if (type.IsEmpty()) {
                    str.Format(_T("%s – %s"), contact->name, number);
                }
                else {
                    str.Format(_T("%s – %s %s"), contact->name, type, number);
                }
                combobox->addItem(QS(str), QS(number));
            }
            i++;
        }
    }
    // CComboBox sem seleção inicial
    combobox->setCurrentIndex(selectedIndex);
    if (selectedIndex == -1) {
        combobox->setEditText(QString());
    }
    SetDropDownWidth(combobox);
}

void Transfer::ClearDropdown()
{
    m_number->clear();
}

void Transfer::closeEvent(QCloseEvent* event)
{
    if (!m_destroyed && !g_bClosingApp) {
        OnClose();
    }
    event->accept();
}

void Transfer::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        OnBnClickedCancel();
        return;
    }
    QWidget::keyPressEvent(event);
}

void Transfer::OnClose()
{
    // DestroyWindow → OnDestroy (mainDlg->transferDlg = nullptr) → PostNcDestroy (delete)
    if (m_destroyed) {
        return;
    }
    m_destroyed = true;
    if (MainDlg()->transferDlg == this) {
        MainDlg()->transferDlg = nullptr;
    }
    ClearDropdown();
    hide();
    deleteLater();
}

void Transfer::OnBnClickedOk()
{
    if (!m_layout.Button(QStringLiteral("IDOK"))->isVisible()) {
        return;
    }
    if (Action(action)) {
        OnClose();
    }
}

void Transfer::OnBnClickedCancel()
{
    OnClose();
}

void Transfer::OnBnClickedAttnded()
{
    if (Action(MSIP_ACTION_ATTENDED_TRANSFER)) {
        OnClose();
    }
}

void Transfer::OnBnClickedBlind()
{
    if (Action(MSIP_ACTION_TRANSFER)) {
        OnClose();
    }
}

void Transfer::SetAction(msip_action action, pjsua_call_id call_id)
{
    this->action = action;
    callId = call_id;
    bool buttons = false;
    if (action == MSIP_ACTION_TRANSFER || action == MSIP_ACTION_ATTENDED_TRANSFER || action == MSIP_ACTION_FORWARD) {
        if (action == MSIP_ACTION_TRANSFER || action == MSIP_ACTION_ATTENDED_TRANSFER) {
            if (accountSettings.enableFeatureCodeAT
                && !accountSettings.featureCodeAT.IsEmpty()
                ) {
                buttons = true;
            }
        }
        if (buttons || action == MSIP_ACTION_FORWARD) {
            setWindowTitle(TR(_T("Call Transfer")));
        }
        else {
            if (action == MSIP_ACTION_ATTENDED_TRANSFER) {
                setWindowTitle(TR(_T("Attended Transfer")));
            }
            else {
                setWindowTitle(TR(_T("Blind Transfer")));
            }
        }
    }
    if (action == MSIP_ACTION_INVITE) {
        setWindowTitle(TR(_T("Invite to Conference")));
    }
    m_layout.Button(QStringLiteral("IDOK"))->setVisible(!buttons);
    m_layout.Button(QStringLiteral("IDC_TRANSFER_ATTENDED"))->setVisible(buttons);
    m_layout.Button(QStringLiteral("IDC_TRANSFER_BLIND"))->setVisible(buttons);
}

bool Transfer::Action(msip_action action)
{
    CString number;
    QComboBox* combobox = m_number;
    // GetCurSel: -1 quando o texto digitado não é o de um item da lista
    int i = combobox->currentIndex();
    if (i != -1 && combobox->currentText() != combobox->itemText(i)) {
        i = -1;
    }
    if (i == -1) {
        number = CS(combobox->currentText());
        number.Trim();
    }
    else {
        number = CS(combobox->itemData(i).toString());
    }
    if (!number.IsEmpty()) {
        MainDlg()->messagesDlg->CallAction(action, number, callId);
        return true;
    }
    return false;
}

void Transfer::Input(CString digits)
{
    if (QLineEdit* edit = m_number->lineEdit()) {
        edit->end(false);
        edit->insert(QS(digits));
    }
}
