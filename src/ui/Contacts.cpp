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

// Port de Contacts.cpp (MicroSIP 3.22.16) sobre o IDD_CONTACTS original (CDialogLayout).
// CListCtrl_SortItemsEx → QTreeWidget ordenado à mão (como o CListCtrl_Sortable: clique
// no cabeçalho alterna a ordem; favoritos sempre no topo). Presença (assinaturas SIP)
// passa pelo CmainDlg (SubsribeNumber/UnsubscribeNumber). Diferenças com "MySimpleSIP:".

#include "Contacts.h"

#include "ContactsRepository.h"
#include "ContactsService.h"
#include "Database.h"
#include "Markup.h"
#include "MessagesDlg.h"
#include "StringConverter.h"
#include "UiCommon.h"
#include "langpack.h"
#include "mainDlg.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QTreeWidget>

#include <fstream>
#include <iterator>

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

namespace {

// Item da lista: aponta para o Contact (lParam do item; a lista "contacts" é a dona) e
// ordena como o SortFunc do original: favoritos primeiro (nas duas ordens), depois o
// texto da coluna, empate desfeito pelo nome.
class ContactItem : public QTreeWidgetItem {
public:
    explicit ContactItem(Contact* c) : contact(c) {}
    Contact* contact;
    bool operator<(const QTreeWidgetItem& other) const override
    {
        const auto& o = static_cast<const ContactItem&>(other);
        QTreeWidget* tree = treeWidget();
        const bool ascending = !tree || tree->header()->sortIndicatorOrder() == Qt::AscendingOrder;
        if (contact->starred != o.contact->starred) {
            // o Qt inverte o resultado na ordem decrescente: compensa para manter o topo
            return ascending ? contact->starred : !contact->starred;
        }
        const int col = tree ? tree->header()->sortIndicatorSection() : 0;
        const int ret = text(col).compare(other.text(col));
        if (ret == 0 && col > 0)
            return text(0) < other.text(0);
        return ret < 0;
    }
};

} // namespace

Contacts::Contacts(QWidget* parent) : QWidget(parent)
{
    m_layout.Build(this, QStringLiteral("IDD_CONTACTS"), false);
    m_list = static_cast<QTreeWidget*>(DLG_ITEM(m_layout, IDC_CONTACTS));
    m_filter = m_layout.Edit(QStringLiteral("IDC_FILER_VALUE"));
    OnInitDialog();
}

Contacts::~Contacts()
{
    m_list->clear();
    POSITION pos = contacts.GetHeadPosition();
    while (pos) {
        delete contacts.GetNext(pos);
    }
    contacts.RemoveAll();
}

void Contacts::OnInitDialog()
{
    m_layout.AutoMove(QStringLiteral("IDC_CONTACTS"), 0, 0, 100, 100);
    m_layout.AutoMove(QStringLiteral("IDC_SEARCH_PICTURE"), 0, 100, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_FILER_VALUE"), 0, 100, 100, 0);

    m_layout.TranslateDialog(this);

    addDlg = new AddDlg(this);

    // mainDlg->imageListStatus (mesma ordem de MSIP_CONTACT_ICON_*)
    for (int id : {IDI_BLANK, IDI_UNKNOWN, IDI_OFFLINE, IDI_AWAY, IDI_ONLINE, IDI_ON_THE_PHONE, IDI_BUSY, IDI_DEFAULT,
                   IDI_UNKNOWN_STARRED, IDI_OFFLINE_STARRED, IDI_AWAY_STARRED, IDI_ONLINE_STARRED,
                   IDI_ON_THE_PHONE_STARRED, IDI_BUSY_STARRED, IDI_DEFAULT_STARRED}) {
        m_imageListStatus.append(LoadImageIcon(id));
    }

    m_list->setRootIsDecorated(false);
    m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_list->setSelectionBehavior(QAbstractItemView::SelectRows); // LVS_EX_FULLROWSELECT
    m_list->setUniformRowHeights(true);
    m_list->setAllColumnsShowFocus(true);
    m_list->setIconSize(QSize(16, 16));
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list->installEventFilter(this);
    m_filter->installEventFilter(this);

    QFont font = m_list->font();
    font.setPixelSize(12);
    m_list->setFont(font);
    m_filter->setFont(font);
    m_filter->setClearButtonEnabled(true);

    m_list->setColumnCount(3);
    m_list->setHeaderLabels({TR(_T("Name")), TR(_T("Number")), TR(_T("Information"))});
    QHeaderView* header = m_list->header();
    header->setStretchLastSection(false);
    header->setSectionsMovable(false);
    header->setSectionsClickable(true);
    header->setSortIndicatorShown(true);
    header->resizeSection(0, accountSettings.contactsWidth0 > 0 ? accountSettings.contactsWidth0 : 160);
    header->resizeSection(1, accountSettings.contactsWidth1 > 0 ? accountSettings.contactsWidth1 : 100);
    header->resizeSection(2, accountSettings.contactsWidth2 > 0 ? accountSettings.contactsWidth2 : 120);
    // CListCtrl_Sortable::OnHeaderClick: o QHeaderView já inverte o indicador (mesma
    // coluna) antes de emitir sectionClicked; aqui só reordena
    QObject::connect(header, &QHeaderView::sectionClicked, this, [this](int) { SortList(); });
    // OnEndtrack
    QObject::connect(header, &QHeaderView::sectionResized, this, [this](int section, int, int width) {
        if (!isVisible()) {
            return;
        }
        switch (section) {
        case 0: accountSettings.contactsWidth0 = width; break;
        case 1: accountSettings.contactsWidth1 = width; break;
        case 2: accountSettings.contactsWidth2 = width; break;
        default: return;
        }
        MainDlg()->AccountSettingsPendingSave();
    });

    QObject::connect(m_filter, &QLineEdit::textChanged, this, [this] { OnFilterValueChange(); });
    QObject::connect(m_list, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item) {
        DefaultItemAction(m_list->indexOfTopLevelItem(item));
    });
    QObject::connect(m_list, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        OnContextMenu(m_list->viewport()->mapToGlobal(pos));
    });

    m_blinkTimer = new QTimer(this);
    QObject::connect(m_blinkTimer, &QTimer::timeout, this, [this] { OnTimerContactsBlink(); });

    // No 3.22.16 o contacts.db é criado, mas os contatos ficam no Contacts.xml
    CString filename = accountSettings.pathRoaming + _T("contacts.db");
    m_db = std::make_unique<Database>(CStringToUtf8(filename));
    m_db->InitContacts();
    m_contactsService = std::make_unique<ContactsService>(std::make_unique<ContactsRepository>(m_db->Get()));

    ContactsLoad();
}

void Contacts::OnCreated()
{
    // m_list.SetSortColumn(0, true)
    m_list->header()->setSortIndicator(0, Qt::AscendingOrder);
    SortList();
}

void Contacts::SortList()
{
    QHeaderView* header = m_list->header();
    m_list->sortItems(header->sortIndicatorSection(), header->sortIndicatorOrder());
}

void Contacts::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_layout.ApplyAutoMove(size());
}

bool Contacts::eventFilter(QObject* watched, QEvent* event)
{
    // PreTranslateMessage
    if (event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape) {
            if (watched == m_filter) {
                if (!m_filter->text().isEmpty()) {
                    FilterReset();
                }
                return true;
            }
            OnBnClickedCancel();
            return true;
        }
        if (ke->key() == Qt::Key_Delete && watched == m_list) {
            OnMenuDelete();
            return true;
        }
        if ((ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) && watched == m_list) {
            OnBnClickedOk();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

Contact* Contacts::ItemContact(int i) const
{
    auto* item = static_cast<ContactItem*>(m_list->topLevelItem(i));
    return item ? item->contact : nullptr;
}

int Contacts::FindItem(const Contact* contact) const
{
    const int count = m_list->topLevelItemCount();
    for (int i = 0; i < count; i++) {
        if (ItemContact(i) == contact) {
            return i;
        }
    }
    return -1;
}

void Contacts::SetItemImage(int i, int image)
{
    if (QTreeWidgetItem* item = m_list->topLevelItem(i)) {
        item->setIcon(0, image >= 0 && image < m_imageListStatus.size() ? m_imageListStatus.at(image) : QIcon());
    }
}

int Contacts::SelectedRow() const
{
    const int count = m_list->topLevelItemCount();
    for (int i = 0; i < count; i++) {
        if (m_list->topLevelItem(i)->isSelected()) {
            return i;
        }
    }
    return -1;
}

std::vector<Contact*> Contacts::ListContacts() const
{
    std::vector<Contact*> result;
    const int count = m_list->topLevelItemCount();
    for (int i = 0; i < count; i++) {
        result.push_back(ItemContact(i));
    }
    return result;
}

void Contacts::OnBnClickedOk()
{
    int i = SelectedRow();
    if (i != -1) {
        DefaultItemAction(i);
    }
}

void Contacts::DefaultItemAction(int i)
{
    Contact* contact = ItemContact(i);
    if (!contact) {
        return;
    }
    bool pickup = contact->ringing;
    pickup = false;
    if (pickup && MainDlg()->CommandCallPickup(contact->number)) {
    }
    else {
        MessagesContact* messagesContact = MainDlg()->messagesDlg->GetMessageContact();
        if (messagesContact && messagesContact->callId != -1) {
            MainDlg()->OpenTransferDlg(MainDlg(), MSIP_ACTION_TRANSFER, PJSUA_INVALID_ID, contact);
        }
        else {
            if (accountSettings.defaultAction.IsEmpty()) {
                MessageDlgOpen(accountSettings.singleMode);
            }
            else {
                if (accountSettings.defaultAction == _T("call")) {
                    OnMenuCall();
                }
#ifdef _GLOBAL_VIDEO
                else if (accountSettings.defaultAction == _T("video")) {
                    OnMenuCallVideo();
                }
#endif
                else {
                    OnMenuChat();
                }
            }
        }
    }
}

void Contacts::OnBnClickedCancel()
{
    MainDlg()->hide();
}

bool Contacts::IsFiltered()
{
    return !m_filter->text().isEmpty();
}

bool Contacts::IsFiltered(const Contact& contact)
{
    CString search = CS(m_filter->text());
    return m_contactsService->IsContactFiletered(contact, search);
}

void Contacts::FilterReset()
{
    m_filter->clear();
}

void Contacts::OnFilterValueChange()
{
    ContactsClear();
    POSITION pos = contacts.GetHeadPosition();
    while (pos) {
        Contact* contact = contacts.GetNext(pos);
        if (!IsFiltered(*contact)) {
            ListAppend(m_list, contact);
        }
    }
    SortList();
}

void Contacts::OnContextMenu(const QPoint& globalPos)
{
    int selectedItem = SelectedRow();
    // IDR_MENU_CONTACT + Exportar/Importar
    QMenu menu(this);
    QMenu numbersMenu(TR(_T("Call")), &menu);
    if (selectedItem != -1) {
        Contact* pContact = ItemContact(selectedItem);
        if (pContact->ringing) {
            if (accountSettings.enableFeatureCodeCP && !accountSettings.featureCodeCP.IsEmpty()) {
                menu.addAction(TR(_T("Call Pickup")), this, [this] { OnMenuCallPickup(); });
                menu.addSeparator();
            }
        }
        //--
        numbersMenu.addAction(QS(pContact->number), this, [this] { OnMenuCall(); });
        if (!pContact->phone.IsEmpty() && pContact->phone != pContact->number) {
            CString str;
            str.Format(_T("%s %s"), Translate(_T("Phone:")), pContact->phone);
            numbersMenu.addAction(QS(str), this, [this] { OnMenuCallPhone(); });
        }
        if (!pContact->mobile.IsEmpty() && pContact->mobile != pContact->number) {
            CString str;
            str.Format(_T("%s %s"), Translate(_T("Mobile:")), pContact->mobile);
            numbersMenu.addAction(QS(str), this, [this] { OnMenuCallMobile(); });
        }
    }
    const bool hasSelection = selectedItem != -1;
    QAction* actCall;
    if (numbersMenu.actions().size() > 1) {
        actCall = menu.addMenu(&numbersMenu);
    }
    else {
        actCall = menu.addAction(TR(_T("Call")), this, [this] { OnMenuCall(); });
    }
    QAction* actVideo = nullptr;
#ifdef _GLOBAL_VIDEO
    if (!accountSettings.disableVideo) {
        actVideo = menu.addAction(TR(_T("Video Call")), this, [this] { OnMenuCallVideo(); });
    }
#endif
    QAction* actChat = nullptr;
    if (!accountSettings.disableMessaging) {
        actChat = menu.addAction(TR(_T("Message")), this, [this] { OnMenuChat(); });
    }
    menu.addSeparator();
    menu.addAction(TR(_T("Add")), this, [this] { OnMenuAdd(); });
    QAction* actEdit = menu.addAction(TR(_T("Edit")), this, [this] { OnMenuEdit(); });
    QAction* actCopy = menu.addAction(TR(_T("Copy")), this, [this] { OnMenuCopy(); });
    QAction* actDelete = menu.addAction(TR(_T("Delete")), this, [this] { OnMenuDelete(); });
    menu.addSeparator();
    menu.addAction(TR(_T("Export")), this, [this] { OnMenuExport(); });
    menu.addAction(TR(_T("Import")), this, [this] { OnMenuImport(); });
    for (QAction* a : {actCall, actVideo, actChat, actCopy, actEdit, actDelete}) {
        if (a) {
            a->setEnabled(hasSelection);
        }
    }
    menu.exec(globalPos);
}

void Contacts::MessageDlgOpen(BOOL isCall, BOOL hasVideo, BYTE index)
{
    if (accountSettings.singleMode && MainDlg()->messagesDlg->GetCallsCount() && isCall) {
        MainDlg()->GotoTab(0);
        return;
    }
    int i = SelectedRow();
    if (i != -1) {
        Contact* pContact = ItemContact(i);
        CString number = pContact->number;
        if (index == 1 && !pContact->phone.IsEmpty()) {
            number = pContact->phone;
        }
        if (index == 2 && !pContact->mobile.IsEmpty()) {
            number = pContact->mobile;
        }
        if (isCall) {
            MainDlg()->MakeCall(number, hasVideo, false, false, pContact->name);
        }
        else {
            MainDlg()->MessagesOpen(number, false, false, pContact->name);
        }
    }
}

void Contacts::OnMenuCallPickup()
{
    int i = SelectedRow();
    if (i != -1) {
        Contact* pContact = ItemContact(i);
        if (pContact->ringing) {
            MainDlg()->CommandCallPickup(pContact->number);
        }
    }
}

void Contacts::OnMenuCall()
{
    MessageDlgOpen(TRUE);
}

void Contacts::OnMenuCallPhone()
{
    MessageDlgOpen(TRUE, 0, 1);
}

void Contacts::OnMenuCallMobile()
{
    MessageDlgOpen(TRUE, 0, 2);
}

void Contacts::OnMenuCallVideo()
{
    MessageDlgOpen(TRUE, TRUE);
}

void Contacts::OnMenuChat()
{
    if (!accountSettings.disableMessaging) {
        MessageDlgOpen();
    }
}

void Contacts::OnMenuAdd()
{
    if (!addDlg->isVisible()) {
        addDlg->show();
    }
    else {
        addDlg->raise();
        addDlg->activateWindow();
    }
    Contact contact;
    addDlg->Load(&contact);
}

void Contacts::OnMenuEdit()
{
    OnMenuAdd();
    int i = SelectedRow();
    if (i != -1) {
        addDlg->Load(ItemContact(i));
    }
}

void Contacts::OnMenuCopy()
{
    int i = SelectedRow();
    if (i != -1) {
        MainDlg()->CopyStringToClipboard(ItemContact(i)->number);
    }
}

void Contacts::OnMenuDelete()
{
    CList<CString, CString> contactsSelected;
    if (SelectedRow() != -1) {
        if (QMessageBox::question(this, TR(_T("Delete Contact")), TR(_T("Are you sure you want to delete the selected items?")),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        const int n = m_list->topLevelItemCount();
        for (int i = 0; i < n; i++) {
            if (m_list->topLevelItem(i)->isSelected()) {
                contactsSelected.AddTail(ItemContact(i)->number);
            }
        }
        if (IsFiltered()) {
            FilterReset();
        }
        int count = m_list->topLevelItemCount();
        bool deleted = false;
        for (int i = 0; i < count; i++) {
            Contact* pContact = ItemContact(i);
            if (contactsSelected.Find(pContact->number)) {
                bool allow = true;
                if (allow) {
                    ContactDelete(i);
                    count--;
                    i--;
                    deleted = true;
                }
            }
        }
        if (deleted) {
            ContactsSave();
        }
    }
}

bool Contacts::Import(CString filename, CArray<ContactWithFields*>& contactsWithFields, bool directory)
{
    auto error = m_contactsService->ImportContactsCSV(filename, contactsWithFields, directory);
    if (error.IsEmpty()) {
        return true;
    }
    else {
        return false;
    }
}

void Contacts::OnMenuExport()
{
    Export(true);
}

void Contacts::Export(bool bFilter)
{
    CString defaultFilename;
    defaultFilename.Format(_T("%s Contacts"), _T(_GLOBAL_NAME_NICE));
    CString search;
    if (bFilter) {
        search = CS(m_filter->text());
    }
    else {
        if (IsFiltered()) {
            FilterReset();
        }
    }
    if (!search.IsEmpty()) {
        defaultFilename.AppendFormat(_T(" %s"), search);
    }
    // CFileDialog (CSV | XML) → QFileDialog
    const QString csvFilter = QStringLiteral("CSV Files (*.csv)");
    const QString xmlFilter = QStringLiteral("XML Files (*.xml)");
    QString selected = csvFilter;
    QString file = QFileDialog::getSaveFileName(this, TR(_T("Export")), QS(MSIP::MakeFilenameFromString(defaultFilename)) + QStringLiteral(".csv"),
                                                csvFilter + QStringLiteral(";;") + xmlFilter, &selected);
    if (!file.isEmpty()) {
        CString filename = CS(file);
        const bool noExt = QFileInfo(file).suffix().isEmpty();
        if (selected == xmlFilter) {
            if (noExt) {
                filename.Append(_T(".xml"));
            }
            m_contactsService->ExportContactsXML(filename, search, ListContacts());
        }
        else {
            if (noExt) {
                filename.Append(_T(".csv"));
            }
            m_contactsService->ExportContactsCSV(filename, search, ListContacts());
        }
    }
}

ContactsService* Contacts::GetContactsService()
{
    return m_contactsService.get();
}

void Contacts::OnMenuImport()
{
    QString file = QFileDialog::getOpenFileName(this, TR(_T("Import")), QString(), QStringLiteral("CSV Files (*.csv)"));
    if (!file.isEmpty()) {
        if (IsFiltered()) {
            FilterReset();
        }
        CArray<ContactWithFields*> contactsWithFields;
        CString error = m_contactsService->ImportContactsCSV(CS(file), contactsWithFields);
        if (error.IsEmpty()) {
            if (contactsWithFields.GetCount()) {
                ContactsAdd(&contactsWithFields);
                for (int i = 0; i < contactsWithFields.GetCount(); i++) {
                    delete contactsWithFields.GetAt(i);
                }
                SortList();
            }
        }
        else {
            QMessageBox::warning(this, QStringLiteral(_GLOBAL_NAME_NICE), TR(error));
        }
    }
}

void Contacts::ContactCreate(QTreeWidget* list, Contact* pContact, bool subscribe)
{
    Contact* contact = new Contact();
    contacts.AddTail(contact);
    contact->image = MSIP_CONTACT_ICON_DEFAULT;
    contact->name = pContact->name;
    contact->number = pContact->number;
    contact->firstname = pContact->firstname;
    contact->lastname = pContact->lastname;
    contact->phone = pContact->phone;
    contact->mobile = pContact->mobile;
    contact->email = pContact->email;
    contact->address = pContact->address;
    contact->city = pContact->city;
    contact->state = pContact->state;
    contact->zip = pContact->zip;
    contact->comment = pContact->comment;
    contact->id = pContact->id;
    if (!contact->presence || contact->info.IsEmpty()) {
        contact->info = pContact->info;
    }
    contact->presence = pContact->presence;
    contact->directory = pContact->directory;
    contact->starred = pContact->starred;
    ListAppend(list, contact, subscribe);
}

void Contacts::ListAppend(QTreeWidget* list, Contact* contact, bool subscribe)
{
    auto* item = new ContactItem(contact);
    const int image = contact->image + (contact->starred ? 7 : 0);
    item->setIcon(0, image >= 0 && image < m_imageListStatus.size() ? m_imageListStatus.at(image) : QIcon());
    item->setText(0, QS(contact->name));
    CString number = contact->number;
    item->setText(1, QS(number));
    item->setText(2, QS(Translate(contact->info)));
    list->insertTopLevelItem(0, item);
    if (subscribe) {
        if (contact->presence) {
            MainDlg()->SubsribeNumber(contact->number);
        }
    }
}

bool Contacts::ContactUpdate(QTreeWidget* list, int i, Contact* contact, Contact* newContact, CStringList* fields)
{
    QTreeWidgetItem* item = list->topLevelItem(i);
    bool changed = false;
    if (!fields || fields->Find(_T("name"))) {
        if (contact->name != newContact->name) {
            item->setText(0, QS(newContact->name));
            contact->name = newContact->name;
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("number"))) {
        if (contact->number != newContact->number) {
            bool presenceOrig = contact->presence;
            if (contact->presence) {
                contact->presence = false;
                PresenceUnsubsribeOne(contact);
            }
            item->setText(1, QS(newContact->number));
            contact->number = newContact->number;
            if ((!fields || fields->Find(_T("presence")))) {
                contact->presence = newContact->presence;
            }
            else {
                contact->presence = presenceOrig;
            }
            if (contact->presence) {
                MainDlg()->SubsribeNumber(contact->number);
            }
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("firstname"))) {
        if (contact->firstname != newContact->firstname) {
            contact->firstname = newContact->firstname;
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("lastname"))) {
        if (contact->lastname != newContact->lastname) {
            contact->lastname = newContact->lastname;
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("phone"))) {
        if (contact->phone != newContact->phone) {
            contact->phone = newContact->phone;
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("mobile"))) {
        if (contact->mobile != newContact->mobile) {
            contact->mobile = newContact->mobile;
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("email"))) {
        if (contact->email != newContact->email) {
            contact->email = newContact->email;
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("address"))) {
        if (contact->address != newContact->address) {
            contact->address = newContact->address;
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("city"))) {
        if (contact->city != newContact->city) {
            contact->city = newContact->city;
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("state"))) {
        if (contact->state != newContact->state) {
            contact->state = newContact->state;
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("zip"))) {
        if (contact->zip != newContact->zip) {
            contact->zip = newContact->zip;
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("comment"))) {
        if (contact->comment != newContact->comment) {
            contact->comment = newContact->comment;
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("id"))) {
        if (contact->id != newContact->id) {
            contact->id = newContact->id;
            changed = true;
        }
    }

    if (!fields || fields->Find(_T("info"))) {
        if ((!contact->presence || contact->info.IsEmpty()) && contact->info != newContact->info) {
            item->setText(2, QS(Translate(newContact->info)));
            contact->info = newContact->info;
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("starred"))) {
        if (newContact->starred != contact->starred) {
            contact->starred = newContact->starred;
            SetItemImage(i, contact->image + (contact->starred ? 7 : 0));
            changed = true;
        }
    }
    if (!fields || fields->Find(_T("presence"))) {
        if (newContact->presence != contact->presence) {
            contact->presence = newContact->presence;
            if (contact->presence) {
                MainDlg()->SubsribeNumber(contact->number);
            }
            else {
                PresenceUnsubsribeOne(contact);
            }
            changed = true;
        }
    }
    return changed;
}

void Contacts::ContactsAdd(CArray<ContactWithFields*>* contactsWithFields, bool directory)
{
    if (IsFiltered()) {
        FilterReset();
    }
    QTreeWidget* list = m_list;
    bool changedAny = false;
    int count = list->topLevelItemCount();
    int countNew = contactsWithFields->GetCount();
    for (int i = 0; i < count; i++) {
        Contact* contact = ItemContact(i);
        bool found = false;
        for (int j = 0; j < countNew; j++) {
            ContactWithFields* contactWithFields = contactsWithFields->GetAt(j);
            if (contact->number == contactWithFields->contact.number
                && contact->name == contactWithFields->contact.name
                ) {
                contactWithFields->processed = true;
                found = true;
                if (ContactUpdate(list, i, contact, &contactWithFields->contact, &contactWithFields->fields)) {
                    changedAny = true;
                }
            }
        }
        if (directory && contact->directory && !found) {
            ContactDelete(i);
            changedAny = true;
            count--;
            i--;
        }
    }
    for (int j = 0; j < countNew; j++) {
        ContactWithFields* contactWithFields = contactsWithFields->GetAt(j);
        if (!contactWithFields->processed) {
            ContactCreate(list, &contactWithFields->contact);
            changedAny = true;
        }
    }
    if (changedAny) {
        ContactsSave();
    }
}

bool Contacts::ContactAdd(Contact contact, BOOL save, BOOL load, CStringList* fields, CString oldNumber, bool manual)
{
    if (!m_contactsService->ContactPrepare(contact)) {
        return false;
    }
    if (save) {
        if (IsFiltered()) {
            FilterReset();
        }
    }
    QTreeWidget* list = m_list;
    if (!load) {
        bool found = false;
        bool changedAny = false;
        int count = list->topLevelItemCount();
        for (int i = 0; i < count; i++) {
            Contact* pContact = ItemContact(i);
            CString compareNumber = !oldNumber.IsEmpty() ? oldNumber : contact.number;
            if (pContact->number == compareNumber) {
                found = true;
                pContact->candidate = false;
                bool changed = ContactUpdate(list, i, pContact, &contact, fields);
                if (changed) {
                    changedAny = true;
                }
            }
        }
        if (found) {
            if (save && changedAny) {
                ContactsSave();
            }
            if (manual && changedAny) {
                SortList();
            }
            return true;
        }
    }
    ContactCreate(list, &contact, !load);
    if (save) {
        ContactsSave();
    }
    if (manual) {
        SortList();
    }
    return true;
}

void Contacts::ContactDelete(int i)
{
    Contact* contact = ItemContact(i);
    delete m_list->takeTopLevelItem(i);
    ContactDeleteRaw(contact);
}

void Contacts::ContactDeleteRaw(Contact* contact)
{
    if (contact->presence) {
        contact->presence = false;
        PresenceUnsubsribeOne(contact);
    }
    POSITION pos = contacts.Find(contact);
    contacts.RemoveAt(pos);
    delete contact;
}

void Contacts::ContactsSave()
{
    if (IsFiltered()) {
        FilterReset();
    }
    CMarkup xml;
    xml.AddElem(_T("contacts"));
    xml.IntoElem();

    int count = m_list->topLevelItemCount();
    for (int i = 0; i < count; i++) {
        Contact* pContact = ItemContact(i);
        xml.AddElem(_T("contact"));
        xml.AddAttrib(_T("name"), pContact->name);
        xml.AddAttrib(_T("number"), pContact->number);
        xml.AddAttrib(_T("firstname"), pContact->firstname);
        xml.AddAttrib(_T("lastname"), pContact->lastname);
        xml.AddAttrib(_T("phone"), pContact->phone);
        xml.AddAttrib(_T("mobile"), pContact->mobile);
        xml.AddAttrib(_T("email"), pContact->email);
        xml.AddAttrib(_T("address"), pContact->address);
        xml.AddAttrib(_T("city"), pContact->city);
        xml.AddAttrib(_T("state"), pContact->state);
        xml.AddAttrib(_T("zip"), pContact->zip);
        xml.AddAttrib(_T("comment"), pContact->comment);
        xml.AddAttrib(_T("id"), pContact->id);
        xml.AddAttrib(_T("info"), pContact->info);
        xml.AddAttrib(_T("presence"), pContact->presence ? _T("1") : _T("0"));
        xml.AddAttrib(_T("starred"), pContact->starred ? _T("1") : _T("0"));
        xml.AddAttrib(_T("directory"), pContact->directory ? _T("1") : _T("0"));
    }

    CString filename = accountSettings.pathRoaming;
    filename.Append(_T("Contacts.xml"));
    // MySimpleSIP: CFile → std::ofstream (caminho UTF-8)
    std::ofstream file(CStringToUtf8(filename), std::ios::binary | std::ios::trunc);
    if (file) {
        CStringA str = "<?xml version=\"1.0\"?>\r\n";
        str.Append(MSIP::Utf8EncodeUni(xml.GetDoc()));
        file.write(str.GetString(), str.GetLength());
        file.close();
    }
}

void Contacts::ContactsClear()
{
    m_list->clear(); // os itens não são donos dos Contact (lista "contacts")
}

void Contacts::ContactsLoad()
{
    CString filename = accountSettings.pathRoaming;
    filename.Append(_T("Contacts.xml"));
    // MySimpleSIP: CFile → std::ifstream (caminho UTF-8)
    std::ifstream file(CStringToUtf8(filename), std::ios::binary);
    if (file) {
        std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        CMarkup xml;
        BOOL bResult = xml.SetDoc(MSIP::Utf8DecodeUni(data.c_str()));
        if (bResult) {
            if (xml.FindElem(_T("contacts"))) {
                while (xml.FindChildElem(_T("contact"))) {
                    xml.IntoElem();
                    Contact contact;
                    contact.name = xml.GetAttrib(_T("name"));
                    contact.number = xml.GetAttrib(_T("number"));
                    contact.firstname = xml.GetAttrib(_T("firstname"));
                    contact.lastname = xml.GetAttrib(_T("lastname"));
                    contact.phone = xml.GetAttrib(_T("phone"));
                    contact.mobile = xml.GetAttrib(_T("mobile"));
                    contact.email = xml.GetAttrib(_T("email"));
                    contact.address = xml.GetAttrib(_T("address"));
                    contact.city = xml.GetAttrib(_T("city"));
                    contact.state = xml.GetAttrib(_T("state"));
                    contact.zip = xml.GetAttrib(_T("zip"));
                    contact.comment = xml.GetAttrib(_T("comment"));
                    contact.id = xml.GetAttrib(_T("id"));
                    CString rab;
                    rab = xml.GetAttrib(_T("presence"));
                    contact.presence = rab == _T("1");
                    rab = xml.GetAttrib(_T("starred"));
                    contact.starred = rab == _T("1");
                    rab = xml.GetAttrib(_T("directory"));
                    contact.directory = rab == _T("1");
                    if (contact.presence || contact.directory) {
                        contact.info = xml.GetAttrib(_T("info"));
                    }
                    if (!contact.number.IsEmpty()) {
                        if (!IsFiltered(contact)) {
                            ContactAdd(contact, FALSE, TRUE);
                        }
                    }
                    xml.OutOfElem();
                }
            }
        }
    }
    else {
        // old
        CString key;
        CString val;
        LPTSTR ptr = val.GetBuffer(256);
        int i = 0;
        while (TRUE) {
            key.Format(_T("%d"), i);
            if (GetPrivateProfileString(_T("Contacts"), key, NULL, ptr, 256, accountSettings.iniFile)) {
                Contact contact;
                ContactDecode(ptr, contact);
                ContactAdd(contact, FALSE, TRUE);
            }
            else {
                break;
            }
            i++;
        }
        val.ReleaseBuffer();
        WritePrivateProfileSection(_T("Contacts"), NULL, accountSettings.iniFile);
        ContactsSave();
    }
    SortList();
}

void Contacts::ContactDecode(CString str, Contact& contact)
{
    CString rab;
    int begin;
    int end;
    begin = 0;
    end = str.Find(';', begin);
    if (end != -1) {
        contact.number = str.Mid(begin, end - begin);
        begin = end + 1;
        end = str.Find(';', begin);
        if (end != -1) {
            contact.name = str.Mid(begin, end - begin);
            begin = end + 1;
            end = str.Find(';', begin);
            if (end != -1) {
                rab = str.Mid(begin, end - begin);
                contact.presence = rab == _T("1");
                begin = end + 1;
                end = str.Find(';', begin);
                if (end != -1) {
                    rab = str.Mid(begin, end - begin);
                }
                else {
                    rab = str.Mid(begin);
                }
                contact.directory = rab == _T("1");
            }
            else {
                rab = str.Mid(begin);
                contact.presence = rab == _T("1");
            }
        }
        else {
            contact.name = str.Mid(begin);
        }
    }
    else {
        contact.number = str;
        contact.name = contact.number;
    }
}

Contact* Contacts::FindContact(CString number, bool subscribed)
{
    POSITION pos = contacts.GetHeadPosition();
    while (pos) {
        Contact* contact = contacts.GetNext(pos);
        if (subscribed) {
            if (contact->presence) {
                CString commands;
                CString numberFormated = FormatNumber(contact->number, &commands, true);
                if (number == numberFormated) {
                    return contact;
                }
            }
        }
        else {
            if (number == contact->number) {
                return contact;
            }
        }
    };
    return NULL;
}

CString Contacts::GetNameAltByNumber(const CString& numberRemote, const CString& numberLocal, const CString& name)
{
    CString nameAlt;
    if (MSIP::IsPSTNNnmber(numberRemote) && MSIP::IsPSTNNnmber(numberLocal)) {
        int offset1 = MSIP::GetNormalizedNumberOffset(numberRemote);
        int offset2 = MSIP::GetNormalizedNumberOffset(numberLocal);
        int len1 = numberRemote.GetLength();
        int len2 = numberLocal.GetLength();
        const CString* str1, * str2;
        if (len1 - offset1 > len2 - offset2) {
            str1 = &numberRemote;
            str2 = &numberLocal;
        }
        else {
            str1 = &numberLocal;
            str2 = &numberRemote;
            int tmp = len1;
            len1 = len2;
            len2 = tmp;
            tmp = offset1;
            offset1 = offset2;
            offset2 = tmp;
        }
        if ((len1 - offset1) - (len2 - offset2) <= 3) {
            if (wcsncmp(str1->GetString() + len1 - len2 + offset2, str2->GetString() + offset2, len2 - offset2) == 0) {
                nameAlt = name;
            }
        }
    }
    return nameAlt;
}

CString Contacts::GetNameByNumber(const CString& number)
{
    CString name;
    CString nameAlt;
    POSITION pos = contacts.GetHeadPosition();
    while (pos) {
        Contact* contact = contacts.GetNext(pos);
        CString stub;
        CString numberContact = FormatNumber(contact->number, &stub);
        SIPURI sipuri;
        MSIP::ParseSIPURI(numberContact, sipuri);
        numberContact = !sipuri.user.IsEmpty() ? sipuri.user : sipuri.domain;
        CString phoneContact;
        if (!contact->phone.IsEmpty()) {
            phoneContact = SanitizeNumber(contact->phone);
        }
        CString mobileContact;
        if (!contact->mobile.IsEmpty()) {
            mobileContact = SanitizeNumber(contact->mobile);
        }
        if (number == numberContact || number == phoneContact || number == mobileContact) {
            name = contact->name;
            break;
        }
        nameAlt = GetNameAltByNumber(number, numberContact, contact->name);
        if (nameAlt.IsEmpty() && !phoneContact.IsEmpty()) {
            nameAlt = GetNameAltByNumber(number, phoneContact, contact->name);
        }
        if (nameAlt.IsEmpty() && !mobileContact.IsEmpty()) {
            nameAlt = GetNameAltByNumber(number, mobileContact, contact->name);
        }
    };
    return !name.IsEmpty() ? name : nameAlt;
}

void Contacts::PresenceUnsubsribeOne(Contact* pContact)
{
    MainDlg()->UnsubscribeNumber(pContact->number);
    PresenceReset(pContact);
}

void Contacts::PresenceSubscribe()
{
    POSITION pos = contacts.GetHeadPosition();
    while (pos) {
        Contact* contact = contacts.GetNext(pos);
        if (contact->presence) {
            MainDlg()->SubsribeNumber(contact->number);
        }
    }
}

void Contacts::PresenceReset(Contact* pContact)
{
    if (IsFiltered()) {
        FilterReset();
    }
    int n = m_list->topLevelItemCount();
    for (int i = 0; i < n; i++) {
        Contact* contact = ItemContact(i);
        if (!pContact || pContact == contact) {
            if (contact->image != MSIP_CONTACT_ICON_DEFAULT) {
                contact->info.Empty();
                m_list->topLevelItem(i)->setText(2, QString());
            }
            contact->image = MSIP_CONTACT_ICON_DEFAULT;
            contact->ringing = false;
            SetItemImage(i, contact->image + (contact->starred ? 7 : 0));
        }
    }
}

void Contacts::PresenceReceived(CString* buddyNumber, int image, bool ringing, CString* info, bool fromUsersDirectory)
{
    bool blink = false;
    POSITION pos = contacts.GetHeadPosition();
    while (pos) {
        Contact* contact = contacts.GetNext(pos);
        if (contact->presence || fromUsersDirectory) {
            CString numberFormated;
            if (fromUsersDirectory) {
                numberFormated = contact->number;
            }
            else {
                CString commands;
                numberFormated = FormatNumber(contact->number, &commands, true);
            }
            if (*buddyNumber == numberFormated) {
                if (ringing) {
                    blink = true;
                }
                contact->image = image;
                contact->ringing = ringing;
                contact->info = *info;
                int i;
                if ((i = FindItem(contact)) != -1) {
                    SetItemImage(i, contact->image + (contact->starred ? 7 : 0));
                    m_list->topLevelItem(i)->setText(2, QS(Translate(contact->info)));
                }
            }
        }
    };
    if (blink) {
        if (!m_blinkTimer->isActive()) {
            m_blinkTimer->start(500);
            OnTimerContactsBlink();
        }
    }
}

void Contacts::OnTimerContactsBlink()
{
    if (!m_blinkTimer->isActive()) {
        return;
    }
    int n = m_list->topLevelItemCount();
    bool ringing = false;
    for (int i = 0; i < n; i++) {
        Contact* contact = ItemContact(i);
        if (contact->ringing) {
            SetItemImage(i, m_blinkState ? contact->image + (contact->starred ? 7 : 0) : MSIP_CONTACT_ICON_BLANK);
            ringing = true;
        }
    }
    if (!ringing) {
        m_blinkTimer->stop();
        m_blinkState = false;
    }
    else {
        m_blinkState = !m_blinkState;
    }
}

void Contacts::SetCanditates()
{
    if (IsFiltered()) {
        FilterReset();
    }
    m_filter->setEnabled(false);
    int count = m_list->topLevelItemCount();
    for (int i = 0; i < count; i++)
    {
        Contact* pContact = ItemContact(i);
        if (pContact->directory) {
            pContact->candidate = true;
        }
    }
}

int Contacts::DeleteCanditates()
{
    if (IsFiltered()) {
        FilterReset();
    }
    int count = m_list->topLevelItemCount();
    int deleted = 0;
    for (int i = 0; i < count; i++)
    {
        Contact* pContact = ItemContact(i);
        if (pContact->candidate) {
            ContactDelete(i);
            count--;
            i--;
            deleted++;
        }
    }
    m_filter->setEnabled(true);
    return deleted;
}
