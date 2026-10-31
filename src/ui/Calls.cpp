/*
 * MySimpleSIP — aba Histórico (port Qt do Calls do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de Calls.cpp (MicroSIP 3.22.16) sobre o IDD_CALLS original (CDialogLayout).
// CListCtrl_SortItemsEx → QTreeWidget com itens que ordenam hora e duração pelo valor
// (como o SortFunc do original).
// Diferenças marcadas com "MySimpleSIP:".

#include "Calls.h"

#include "CallsRepository.h"
#include "CallsService.h"
#include "Contacts.h"
#include "Database.h"
#include "MessagesDlg.h"
#include "StringConverter.h"
#include "UiCommon.h"
#include "langpack.h"
#include "mainDlg.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QTreeWidget>

enum {
    MSIP_CALLS_COL_NAME,
    MSIP_CALLS_COL_NUMBER,
    MSIP_CALLS_COL_TIME,
    MSIP_CALLS_COL_DURATION,
    MSIP_CALLS_COL_INFO,
};

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

namespace {

// Item da lista: guarda o Call (lParam do item) e ordena como o SortFunc do original —
// hora e duração pelo valor; demais colunas pelo texto, empate desfeito pelo nome.
class CallItem : public QTreeWidgetItem {
public:
    explicit CallItem(std::unique_ptr<Call> c) : call(std::move(c)) {}
    std::unique_ptr<Call> call;
    bool operator<(const QTreeWidgetItem& other) const override
    {
        const auto& o = static_cast<const CallItem&>(other);
        const int col = treeWidget() ? treeWidget()->sortColumn() : 0;
        if (col == MSIP_CALLS_COL_TIME)
            return call->time < o.call->time;
        if (col == MSIP_CALLS_COL_DURATION)
            return call->duration < o.call->duration;
        const int ret = text(col).compare(other.text(col));
        if (ret == 0 && col > 0)
            return text(0) < other.text(0);
        return ret < 0;
    }
};

} // namespace

Calls::Calls(QWidget* parent) : QWidget(parent)
{
    m_layout.Build(this, QStringLiteral("IDD_CALLS"), false);
    m_list = static_cast<QTreeWidget*>(DLG_ITEM(m_layout, IDC_CALLS));
    m_filter = m_layout.Edit(QStringLiteral("IDC_FILER_VALUE"));
    OnInitDialog();
}

Calls::~Calls()
{
    CallsClear();
}

void Calls::OnInitDialog()
{
    m_layout.AutoMove(QStringLiteral("IDC_CALLS"), 0, 0, 100, 100);
    m_layout.AutoMove(QStringLiteral("IDC_SEARCH_PICTURE"), 0, 100, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_FILER_VALUE"), 0, 100, 100, 0);

    m_layout.TranslateDialog(this);

    m_nLastDay = 0;

    m_list->setRootIsDecorated(false);
    m_list->setSelectionMode(QAbstractItemView::ExtendedSelection); // LVS_REPORT sem LVS_SINGLESEL
    m_list->setSelectionBehavior(QAbstractItemView::SelectRows);     // LVS_EX_FULLROWSELECT
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

    m_list->setColumnCount(5);
    m_list->setHeaderLabels({TR(_T("Name")), TR(_T("Number")), TR(_T("Time")), TR(_T("Duration")), TR(_T("Information"))});
    QHeaderView* header = m_list->header();
    header->setStretchLastSection(false);
    header->setSectionsMovable(false);
    header->resizeSection(MSIP_CALLS_COL_NAME, accountSettings.callsWidth0 > 0 ? accountSettings.callsWidth0 : 160);
    header->resizeSection(MSIP_CALLS_COL_NUMBER, accountSettings.callsWidth1 > 0 ? accountSettings.callsWidth1 : 100);
    header->resizeSection(MSIP_CALLS_COL_TIME, accountSettings.callsWidth2 > 0 ? accountSettings.callsWidth2 : 135);
    header->resizeSection(MSIP_CALLS_COL_DURATION, accountSettings.callsWidth3 > 0 ? accountSettings.callsWidth3 : 70);
    header->resizeSection(MSIP_CALLS_COL_INFO, accountSettings.callsWidth4 > 0 ? accountSettings.callsWidth4 : 120);
    // OnEndtrack: larguras salvas no INI
    QObject::connect(header, &QHeaderView::sectionResized, this, [this](int section, int, int width) {
        if (!isVisible()) {
            return;
        }
        switch (section) {
        case 0: accountSettings.callsWidth0 = width; break;
        case 1: accountSettings.callsWidth1 = width; break;
        case 2: accountSettings.callsWidth2 = width; break;
        case 3: accountSettings.callsWidth3 = width; break;
        case 4: accountSettings.callsWidth4 = width; break;
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

    CString filename = accountSettings.pathRoaming + _T("call_log.db");
    m_db = std::make_unique<Database>(CStringToUtf8(filename));
    m_db->InitCalls();
    m_callsService = std::make_unique<CallsService>(std::make_unique<CallsRepository>(m_db->Get()));

    CallsLoad();

    // SetTimer(IDT_TIMER_CALLS, 300 * 1000) → OnTimer → ReloadTime
    m_timer = new QTimer(this);
    QObject::connect(m_timer, &QTimer::timeout, this, [this] { ReloadTime(); });
    m_timer->start(300 * 1000);
}

void Calls::OnCreated()
{
    // m_list.SetSortColumn(2, false)
    m_list->setSortingEnabled(true);
    m_list->sortByColumn(MSIP_CALLS_COL_TIME, Qt::DescendingOrder);
}

void Calls::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_layout.ApplyAutoMove(size());
}

bool Calls::eventFilter(QObject* watched, QEvent* event)
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
            OnBnClickedOk(); // IDOK
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

Call* Calls::ItemCall(int i) const
{
    auto* item = static_cast<CallItem*>(m_list->topLevelItem(i));
    return item ? item->call.get() : nullptr;
}

QList<int> Calls::SelectedRows() const
{
    QList<int> rows;
    const int count = m_list->topLevelItemCount();
    for (int i = 0; i < count; i++) {
        if (m_list->topLevelItem(i)->isSelected()) {
            rows.append(i);
        }
    }
    return rows;
}

void Calls::OnBnClickedOk()
{
    const QList<int> rows = SelectedRows();
    if (!rows.isEmpty()) {
        DefaultItemAction(rows.first());
    }
}

void Calls::DefaultItemAction(int i)
{
    (void)i;
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

void Calls::OnBnClickedCancel()
{
    MainDlg()->hide();
}

void Calls::OnFilterValueChange()
{
    CallsClear();
    CallsLoad();
}

bool Calls::IsFiltered(const Call& call)
{
    CString search = CS(m_filter->text());
    return m_callsService->IsCallFiletered(call, search);
}

void Calls::FilterReset()
{
    m_filter->clear();
}

void Calls::OnContextMenu(const QPoint& globalPos)
{
    const QList<int> rows = SelectedRows();
    int selectedItem = rows.isEmpty() ? -1 : rows.at(0);
    int selectedItem2 = rows.size() > 1 ? rows.at(1) : -1;

    // IDR_MENU_CONTACT (sem "Edit") + Exportar/Importar
    QMenu menu(this);
    QAction* actCall = menu.addAction(TR(_T("Call")), this, [this] { OnMenuCall(); });
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
    QAction* actAdd = menu.addAction(TR(_T("Add Contact")), this, [this] { OnMenuAdd(); });
    QAction* actCopy = menu.addAction(TR(_T("Copy")), this, [this] { OnMenuCopy(); });
    QAction* actDelete = menu.addAction(TR(_T("Delete")), this, [this] { OnMenuDelete(); });
    menu.addSeparator();
    menu.addAction(TR(_T("Export")), this, [this] { OnMenuExport(); });
    menu.addAction(TR(_T("Import")), this, [this] { OnMenuImport(); });

    bool disabled = selectedItem == -1;
    if (!disabled) {
        Call* pCall = ItemCall(selectedItem);
        if (MainDlg()->pageContacts->FindContact(pCall->number)) {
            disabled = true;
        }
    }
    actAdd->setEnabled(!disabled);
    // um item selecionado: tudo habilitado; vários: só Apagar; nenhum: nada
    const bool single = selectedItem != -1 && selectedItem2 == -1;
    for (QAction* a : {actCall, actVideo, actChat, actCopy}) {
        if (a) {
            a->setEnabled(single);
        }
    }
    actDelete->setEnabled(selectedItem != -1);
    menu.exec(globalPos);
}

void Calls::MessageDlgOpen(BOOL isCall, BOOL hasVideo)
{
    if (accountSettings.singleMode && MainDlg()->messagesDlg->GetCallsCount() && isCall) {
        MainDlg()->GotoTab(0);
        return;
    }
    const QList<int> rows = SelectedRows();
    if (!rows.isEmpty()) {
        Call* pCall = ItemCall(rows.first());
        CString number = pCall->number;
        if (isCall) {
            MainDlg()->MakeCall(number, hasVideo, false, pCall->type != MSIP_CALL_TYPE_OUTGOING, pCall->name);
        }
        else {
            MainDlg()->MessagesOpen(number, false, pCall->type != MSIP_CALL_TYPE_OUTGOING, pCall->name);
        }
    }
}

void Calls::OnMenuCall()
{
    MessageDlgOpen(TRUE);
}

void Calls::OnMenuCallVideo()
{
    MessageDlgOpen(TRUE, TRUE);
}

void Calls::OnMenuChat()
{
    if (!accountSettings.disableMessaging) {
        MessageDlgOpen();
    }
}

void Calls::OnMenuAdd()
{
    MainDlg()->pageContacts->OnMenuAdd();
    const QList<int> rows = SelectedRows();
    if (rows.isEmpty()) {
        return;
    }
    Call* pCall = ItemCall(rows.first());
    Contact contact;
    contact.number = pCall->number;
    contact.name = pCall->name;
    MainDlg()->pageContacts->addDlg->Load(&contact);
}

void Calls::OnMenuCopy()
{
    const QList<int> rows = SelectedRows();
    if (!rows.isEmpty()) {
        Call* pCall = ItemCall(rows.first());
        MainDlg()->CopyStringToClipboard(pCall->number);
    }
}

void Calls::OnMenuDelete()
{
    QList<int> rows = SelectedRows();
    if (rows.isEmpty()) {
        return;
    }
    if (QMessageBox::question(this, TR(_T("Delete Call Log")), TR(_T("Are you sure you want to delete the selected items?")),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    if (rows.size() < 100) {
        for (int k = rows.size() - 1; k >= 0; --k) {
            int i = rows.at(k);
            Call* pCall = ItemCall(i);
            m_callsService->DeleteCall(pCall->id);
            delete m_list->takeTopLevelItem(i);
        }
    }
    else {
        std::vector<int> ids;
        for (int i : rows) {
            ids.push_back(ItemCall(i)->id);
        }
        m_callsService->DeleteCalls(ids);
        OnFilterValueChange();
    }
}

void Calls::OnMenuExport()
{
    Export(true);
}

void Calls::Export(bool bFilter)
{
    CString defaultFilename;
    defaultFilename.Format(_T("%s Call Log"), _T(_GLOBAL_NAME_NICE));
    CString search;
    if (bFilter) {
        search = CS(m_filter->text());
    }
    if (!search.IsEmpty()) {
        defaultFilename.AppendFormat(_T(" %s"), search);
    }
    // CFileDialog → QFileDialog
    QString file = QFileDialog::getSaveFileName(this, TR(_T("Export")), QS(MSIP::MakeFilenameFromString(defaultFilename)) + QStringLiteral(".csv"),
                                                QStringLiteral("CSV Files (*.csv)"));
    if (!file.isEmpty()) {
        CString filename = CS(file);
        if (QFileInfo(file).suffix().isEmpty()) {
            filename.Append(_T(".csv"));
        }
        m_callsService->ExportCallsCSV(filename, search);
    }
}

void Calls::OnMenuImport()
{
    QString file = QFileDialog::getOpenFileName(this, TR(_T("Import")), QString(), QStringLiteral("CSV Files (*.csv)"));
    if (!file.isEmpty()) {
        std::string error = m_callsService->ImportCallsCSV(CS(file));
        if (error.empty()) {
            OnFilterValueChange();
        }
        else {
            QMessageBox::warning(this, QStringLiteral(_GLOBAL_NAME_NICE), QS(Utf8ToCString(error)));
        }
    }
}

int Calls::FindListItem(int id)
{
    int count = m_list->topLevelItemCount();
    for (int i = 0; i < count; i++)
    {
        Call* pCall = ItemCall(i);
        if (pCall->id == id) {
            return i;
        }
    }
    return -1;
}

void Calls::Add(pj_str_t id, CString address, CString name, CString commands, int type, call_user_data* user_data)
{
    (void)user_data;
    SIPURI sipuri;
    MSIP::ParseSIPURI(address, sipuri);

    CString numberLocal;
    CString numberLocalSuffix;
    if (sipuri.user.IsEmpty()) {
        numberLocal = sipuri.domain;
        numberLocalSuffix.Append(sipuri.suffix);
        numberLocalSuffix.Append(commands);
    }
    else {
        numberLocal = sipuri.user;
        if (!accountSettings.account.dialingPrefix.IsEmpty()) {
            if (numberLocal.Find(accountSettings.account.dialingPrefix) == 0) {
                numberLocal = numberLocal.Mid(accountSettings.account.dialingPrefix.GetLength());
            }
        }
        if (sipuri.suffix.IsEmpty() && (get_account_domain() == sipuri.domain || sipuri.domain.IsEmpty())) {
            numberLocalSuffix.Append(commands);
        }
        else {
            numberLocalSuffix.Append(_T("@"));
            numberLocalSuffix.Append(sipuri.domain);
            numberLocalSuffix.Append(sipuri.suffix);
            numberLocalSuffix.Append(commands);
        }
    }
    numberLocal.Append(numberLocalSuffix);

    CString callId = MSIP::PjToStr(&id);

    auto call = m_callsService->GetCallByCallId(callId);
    if (!call) {
        ReloadTime();
        call = std::make_unique<Call>();
        call->callId = callId;
        call->number = numberLocal;
        call->name = name;
        call->type = type;
        if (m_callsService->AddCall(*call)) {
            if (!IsFiltered(*call)) {
                AddListItem(std::move(call));
            }
        }
    }
    else {
        int nItem = FindListItem(call->id);
        bool changed = false;
        if (call->number != (numberLocal)) {
            call->number = (numberLocal);
            changed = true;
        }
        if (call->name != (name)) {
            call->name = (name);
            changed = true;
        }
        if (call->type != type) {
            call->type = type;
            changed = true;
        }
        if (changed) {
            m_callsService->UpdateCall(*call);
            if (nItem != -1) {
                // SetItemText/SetItem(LVIF_IMAGE) + SetItemData(novo Call)
                auto* item = static_cast<CallItem*>(m_list->topLevelItem(nItem));
                item->setText(MSIP_CALLS_COL_NUMBER, QS(call->number));
                item->setText(MSIP_CALLS_COL_NAME, QS(call->name));
                static const int icons[] = {IDI_CALL_OUT, IDI_CALL_IN, IDI_CALL_MISS, IDI_CALL_MISS_1};
                item->setIcon(MSIP_CALLS_COL_NAME, LoadImageIcon(icons[type >= 0 && type <= 3 ? type : 0]));
                item->call = std::move(call);
            }
        }
    }
}

void Calls::DeleteAll()
{
    m_callsService->DeleteAllCalls();
    CallsClear();
}

void Calls::SetName(pj_str_t id, CString name)
{
    CString callId = MSIP::PjToStr(&id);
    std::unique_ptr<Call> call = m_callsService->GetCallByCallId((callId));
    if (call) {
        call->name = (name);
        m_callsService->UpdateCall(*call);
        int nItem = FindListItem(call->id);
        if (nItem != -1) {
            ItemCall(nItem)->name = call->name;
            m_list->topLevelItem(nItem)->setText(MSIP_CALLS_COL_NAME, QS(name));
        }
    }
}

void Calls::SetDuration(pj_str_t id, int sec, int total)
{
    (void)total;
    CString callId = MSIP::PjToStr(&id);
    std::unique_ptr<Call> call = m_callsService->GetCallByCallId((callId));
    if (call) {
        call->duration = sec;
        m_callsService->UpdateCall(*call);
        int nItem = FindListItem(call->id);
        if (nItem != -1) {
            ItemCall(nItem)->duration = call->duration;
            m_list->topLevelItem(nItem)->setText(MSIP_CALLS_COL_DURATION, QS(MSIP::GetDuration(call->duration)));
        }
    }
}

void Calls::SetInfo(pj_str_t id, CString info)
{
    CString callId = MSIP::PjToStr(&id);
    std::unique_ptr<Call> call = m_callsService->GetCallByCallId((callId));
    if (call) {
        call->info = (info);
        m_callsService->UpdateCall(*call);
        int nItem = FindListItem(call->id);
        if (nItem != -1) {
            ItemCall(nItem)->info = call->info;
            m_list->topLevelItem(nItem)->setText(MSIP_CALLS_COL_INFO, QS(info));
        }
    }
}

void Calls::AddListItem(std::unique_ptr<Call> call, int pos)
{
    static const int icons[] = {IDI_CALL_OUT, IDI_CALL_IN, IDI_CALL_MISS, IDI_CALL_MISS_1};
    Call* pCall = call.get();
    auto* item = new CallItem(std::move(call));
    item->setIcon(MSIP_CALLS_COL_NAME, LoadImageIcon(icons[pCall->type >= 0 && pCall->type <= 3 ? pCall->type : 0]));
    item->setText(MSIP_CALLS_COL_NAME, QS(pCall->name));
    item->setText(MSIP_CALLS_COL_NUMBER, QS(pCall->number));
    item->setText(MSIP_CALLS_COL_TIME, QS(FormatTime(pCall->time)));
    item->setText(MSIP_CALLS_COL_DURATION, QS(MSIP::GetDuration(pCall->duration)));
    item->setText(MSIP_CALLS_COL_INFO, QS(pCall->info));
    // MySimpleSIP: com a ordenação ligada o QTreeWidget reposiciona o item pela coluna
    // atual (no original, InsertItem na posição pos sem reordenar)
    m_list->insertTopLevelItem(pos, item);
}

CString Calls::FormatTime(int64_t time, CTime* pTimeNow)
{
    CTime timeNow;
    if (!pTimeNow) {
        timeNow = CTime::GetCurrentTime();
        pTimeNow = &timeNow;
    }
    if (!m_nLastDay) {
        m_nLastDay = pTimeNow->GetDay();
    }
    CTime timeCall((time_t)time);
    return timeCall.Format(
        pTimeNow->GetYear() == timeCall.GetYear() &&
        pTimeNow->GetMonth() == timeCall.GetMonth() &&
        pTimeNow->GetDay() == timeCall.GetDay()
        ? _T("%X") : _T("%c")
    );
}

void Calls::ReloadTime()
{
    CTime timeNow = CTime::GetCurrentTime();
    if (m_nLastDay && m_nLastDay != timeNow.GetDay()) {
        m_nLastDay = timeNow.GetDay();
        int count = m_list->topLevelItemCount();
        for (int i = 0; i < count; i++) {
            Call* pCall = ItemCall(i);
            m_list->topLevelItem(i)->setText(MSIP_CALLS_COL_TIME, QS(FormatTime(pCall->time, &timeNow)));
        }
    }
}

void Calls::CallsClear()
{
    m_list->clear(); // os itens guardam (e liberam) o Call
}

void Calls::CallsLoad()
{
    CString search = CS(m_filter->text());
    auto calls = m_callsService->GetAllCalls();
    const bool sorting = m_list->isSortingEnabled();
    m_list->setSortingEnabled(false);
    for (auto& call : calls) {
        if (!m_callsService->IsCallFiletered(*call, search)) {
            AddListItem(std::move(call));
        }
    }
    // m_list.SortColumn(m_list.GetSortColumn(), m_list.IsAscending())
    m_list->setSortingEnabled(sorting);
}
