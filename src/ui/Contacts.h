/*
 * MySimpleSIP — aba Contatos (port Qt do Contacts do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Agenda (Contacts.xml em pathRoaming) com busca, favoritos no topo, ícone de presença,
// menu de contexto, diálogo AddDlg, importação/exportação CSV/XML e a busca de nome
// por número usada nas chamadas (GetNameByNumber).

#pragma once

#include "AddDlg.h"
#include "Contact.h"
#include "DialogLayout.h"
#include "global.h"

#include <QWidget>

#include <memory>
#include <vector>

class ContactsService;
class Database;
class QLineEdit;
class QTimer;
class QTreeWidget;

class Contacts : public QWidget {
public:
    explicit Contacts(QWidget* parent = nullptr);
    ~Contacts() override;

    AddDlg* addDlg = nullptr;
    CList<Contact*> contacts;
    void ContactCreate(QTreeWidget* list, Contact* pContact, bool subscribe = true);
    void ListAppend(QTreeWidget* list, Contact* contact, bool subscribe = true);
    bool ContactUpdate(QTreeWidget* list, int i, Contact* contact, Contact* newContact, CStringList* fields);
    void ContactsAdd(CArray<ContactWithFields*>* contacts, bool directory = false);
    bool ContactAdd(Contact contact, BOOL save = FALSE, BOOL load = FALSE, CStringList* fields = NULL, CString oldNumber = _T(""), bool manual = false);
    void ContactDelete(int i);
    void ContactDeleteRaw(Contact* contact);
    void ContactsSave();
    void ContactsClear();
    void ContactsLoad();
    void SetCanditates();
    int DeleteCanditates();
    Contact* FindContact(CString number, bool subscribed = false);
    CString GetNameByNumber(const CString& number);
    void PresenceUnsubsribeOne(Contact* pContact);
    void PresenceSubscribe();
    void PresenceReset(Contact* pContact = NULL);
    void PresenceReceived(CString* buddyNumber, int image, bool ringing, CString* info, bool fromUsersDirectory = false);
    void OnTimerContactsBlink();
    void OnCreated();
    bool Import(CString filename, CArray<ContactWithFields*>& contacts, bool directory = false);
    void Export(bool bFilter = false);
    ContactsService* GetContactsService();

    void OnBnClickedOk();
    void OnBnClickedCancel();
    void OnFilterValueChange();
    void OnMenuCallPickup();
    void OnMenuCall();
    void OnMenuCallPhone();
    void OnMenuCallMobile();
    void OnMenuCallVideo();
    void OnMenuChat();
    void OnMenuAdd();
    void OnMenuEdit();
    void OnMenuCopy();
    void OnMenuDelete();
    void OnMenuExport();
    void OnMenuImport();

    QTreeWidget* List() const { return m_list; } // IDC_CONTACTS (testes)
    void SortList();                             // m_list.SortColumn(GetSortColumn(), IsAscending())
    CDialogLayout m_layout;

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void OnInitDialog();
    void OnContextMenu(const QPoint& globalPos);
    bool IsFiltered();
    bool IsFiltered(const Contact& contact);
    void FilterReset();
    CString GetNameAltByNumber(const CString& numberRemote, const CString& numberLocal, const CString& name);
    void ContactDecode(CString str, Contact& contact);
    void MessageDlgOpen(BOOL isCall = FALSE, BOOL hasVideo = FALSE, BYTE index = 0);
    void DefaultItemAction(int i);
    Contact* ItemContact(int i) const;        // GetItemData
    int FindItem(const Contact* contact) const; // FindItem(LVFI_PARAM)
    void SetItemImage(int i, int image);       // SetItem(LVIF_IMAGE)
    int SelectedRow() const;                   // primeiro selecionado (-1 = nenhum)
    std::vector<Contact*> ListContacts() const; // itens na ordem da lista

    // MySimpleSIP: no original, objetos static em OnInitDialog; aqui pertencem à aba
    std::unique_ptr<Database> m_db;
    std::unique_ptr<ContactsService> m_contactsService;
    QTreeWidget* m_list = nullptr;
    QLineEdit* m_filter = nullptr;
    QTimer* m_blinkTimer = nullptr; // blinkTimer (IDT_TIMER_CONTACTS_BLINK)
    bool m_blinkState = false;
    QList<QIcon> m_imageListStatus; // mainDlg->imageListStatus
};
