/*
 * MySimpleSIP — diálogo de contato (port Qt do AddDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de AddDlg.cpp (MicroSIP 3.22.16) sobre o IDD_ADD original (CDialogLayout).

#include "AddDlg.h"

#include "Contacts.h"
#include "UiCommon.h"
#include "langpack.h"
#include "mainDlg.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

AddDlg::AddDlg(QWidget* pParent) : QWidget(pParent, Qt::Dialog)
{
    if (langPack.rtl) {
        setLayoutDirection(Qt::RightToLeft);
    }
    loading = false;
    m_layout.Build(this, QStringLiteral("IDD_ADD"));
    OnInitDialog();
}

AddDlg::~AddDlg()
{
}

void AddDlg::OnInitDialog()
{
    setWindowIcon(LoadImageIcon(IDI_MAINFRAME));
    m_layout.TranslateDialog(this);
    QObject::connect(m_layout.Button(QStringLiteral("IDOK")), &QPushButton::clicked, this, [this] { OnBnClickedOk(); });
    QObject::connect(m_layout.Button(QStringLiteral("IDCANCEL")), &QPushButton::clicked, this, [this] { OnBnClickedCancel(); });
    m_layout.Button(QStringLiteral("IDOK"))->setDefault(true);
    for (const char* id : {"IDC_ADD_FIRSTNAME", "IDC_ADD_LASTNAME"}) {
        QObject::connect(m_layout.Edit(QString::fromLatin1(id)), &QLineEdit::textChanged, this, [this] { OnChangeFirstLastName(); });
    }
    QObject::connect(m_layout.Edit(QStringLiteral("IDC_ADD_PHONE")), &QLineEdit::textChanged, this, [this] { OnChangePhoneNumber(); });
    // Enter em qualquer campo = OK (DEFPUSHBUTTON)
    const QList<QLineEdit*> edits = findChildren<QLineEdit*>();
    for (QLineEdit* e : edits) {
        QObject::connect(e, &QLineEdit::returnPressed, this, [this] { OnBnClickedOk(); });
    }
}

CString AddDlg::Text(const char* id) const
{
    return CS(m_layout.Edit(QString::fromLatin1(id))->text());
}

void AddDlg::SetText(const char* id, const CString& s)
{
    m_layout.Edit(QString::fromLatin1(id))->setText(QS(s));
}

void AddDlg::closeEvent(QCloseEvent* event)
{
    // OnClose: só esconde (no encerramento do app, aceita)
    if (g_bClosingApp) {
        event->accept();
        return;
    }
    event->ignore();
    OnClose();
}

void AddDlg::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        OnBnClickedCancel(); // IDCANCEL
        return;
    }
    QWidget::keyPressEvent(event);
}

void AddDlg::OnClose()
{
    hide();
}

void AddDlg::Load(Contact* pContact)
{
    loading = true;
    number = pContact->number;
    firstname = pContact->firstname;
    lastname = pContact->lastname;
    phone = pContact->phone;
    SetText("IDC_ADD_NAME", pContact->name);
    CString numberLocal = pContact->number;
    SetText("IDC_ADD_NUMBER", numberLocal);
    SetText("IDC_ADD_FIRSTNAME", pContact->firstname);
    SetText("IDC_ADD_LASTNAME", pContact->lastname);
    SetText("IDC_ADD_PHONE", pContact->phone);
    SetText("IDC_ADD_MOBILE", pContact->mobile);
    SetText("IDC_ADD_EMAIL", pContact->email);
    SetText("IDC_ADD_ADDRESS", pContact->address);
    SetText("IDC_ADD_CITY", pContact->city);
    SetText("IDC_ADD_STATE", pContact->state);
    SetText("IDC_ADD_ZIP", pContact->zip);
    SetText("IDC_ADD_COMMENT", pContact->comment);
    m_layout.Check(QStringLiteral("IDC_ADD_PRESENCE"))->setChecked(pContact->presence);
    m_layout.Check(QStringLiteral("IDC_ADD_STARRED"))->setChecked(pContact->starred);
    m_layout.Edit(QStringLiteral("IDC_ADD_NAME"))->setFocus();
    loading = false;
}

void AddDlg::OnBnClickedOk()
{
    CStringList fields;
    Contact contact;
    fields.AddTail(_T("name"));
    contact.name = Text("IDC_ADD_NAME");
    contact.name.Trim();
    fields.AddTail(_T("number"));
    contact.number = Text("IDC_ADD_NUMBER");
    contact.number.Trim();
    fields.AddTail(_T("firstname"));
    contact.firstname = Text("IDC_ADD_FIRSTNAME");
    contact.firstname.Trim();
    fields.AddTail(_T("lastname"));
    contact.lastname = Text("IDC_ADD_LASTNAME");
    contact.lastname.Trim();
    fields.AddTail(_T("phone"));
    contact.phone = Text("IDC_ADD_PHONE");
    contact.phone.Trim();
    fields.AddTail(_T("mobile"));
    contact.mobile = Text("IDC_ADD_MOBILE");
    contact.mobile.Trim();
    fields.AddTail(_T("email"));
    contact.email = Text("IDC_ADD_EMAIL");
    contact.email.Trim();
    fields.AddTail(_T("address"));
    contact.address = Text("IDC_ADD_ADDRESS");
    contact.address.Trim();
    fields.AddTail(_T("city"));
    contact.city = Text("IDC_ADD_CITY");
    contact.city.Trim();
    fields.AddTail(_T("state"));
    contact.state = Text("IDC_ADD_STATE");
    contact.state.Trim();
    fields.AddTail(_T("zip"));
    contact.zip = Text("IDC_ADD_ZIP");
    contact.zip.Trim();
    fields.AddTail(_T("comment"));
    contact.comment = Text("IDC_ADD_COMMENT");
    contact.comment.Trim();
    fields.AddTail(_T("presence"));
    contact.presence = m_layout.Check(QStringLiteral("IDC_ADD_PRESENCE"))->isChecked();
    fields.AddTail(_T("starred"));
    contact.starred = m_layout.Check(QStringLiteral("IDC_ADD_STARRED"))->isChecked();
    if (!contact.number.IsEmpty()) {
        if (number.IsEmpty() || number != contact.number) {
            if (MainDlg()->pageContacts->FindContact(contact.number)) {
                if (QMessageBox::question(this, TR(_T("Contact already exists")),
                                          TR(_T("A contact with this phone number already exists. Do you want to replace it?")),
                                          QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
                    return;
                }
            }
        }
        if (MainDlg()->pageContacts->ContactAdd(contact, TRUE, FALSE, &fields, number, true)) {
            OnClose();
        }
    }
    else {
        m_layout.Edit(QStringLiteral("IDC_ADD_NUMBER"))->setFocus();
    }
}

void AddDlg::OnBnClickedCancel()
{
    OnClose();
}

void AddDlg::OnChangeFirstLastName()
{
    if (loading) {
        return;
    }
    CString str = Text("IDC_ADD_NAME");
    str.Trim();
    CString fname;
    CString lname;
    int pos = -1;
    if (!firstname.IsEmpty()) {
        if (str.Find(firstname) == 0) {
            pos = firstname.GetLength();
        }
    }
    else if (!lastname.IsEmpty()) {
        if (str.Find(lastname) == 0) {
            pos = 0;
        }
    }
    if (pos == -1) {
        pos = str.Find(' ');
    }
    if (pos != -1) {
        fname = str.Left(pos);
        lname = str.Mid(pos);
        lname.Trim();
    }
    else {
        fname = str;
    }
    //--
    CString str1 = Text("IDC_ADD_FIRSTNAME");
    str1.Trim();
    CString str2 = Text("IDC_ADD_LASTNAME");
    str2.Trim();
    if ((fname == firstname && lname == lastname) || (lname.IsEmpty() && fname == lastname)) {
        str.Format(_T("%s %s"), str1, str2);
        str.Trim();
        SetText("IDC_ADD_NAME", str);
    }
    firstname = str1;
    lastname = str2;
}

void AddDlg::OnChangePhoneNumber()
{
    CString str = Text("IDC_ADD_NUMBER");
    CString str1 = Text("IDC_ADD_PHONE");
    if (phone == str) {
        SetText("IDC_ADD_NUMBER", str1);
    }
    phone = str1;
}
