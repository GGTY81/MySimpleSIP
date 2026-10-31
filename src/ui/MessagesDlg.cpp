/*
 * MySimpleSIP — janela de chamadas e mensagens (port Qt do MessagesDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de MessagesDlg.cpp (MicroSIP 3.22.16) sobre o IDD_MESSAGES original (CDialogLayout).
// RichEdit → QTextBrowser/QPlainTextEdit: o histórico de cada contato
// (MessagesContact::messages) é guardado em HTML em vez de RTF. Os menus de
// transferência/conferência (IDR_MENU_CALL_TRANSFER/CONFERENCE) são montados com QMenu
// a cada clique. Diferenças marcadas com "MySimpleSIP:".

#include "MessagesDlg.h"

#include "Calls.h"
#include "Dialer.h"
#include "DialerControls.h"
#include "UiCommon.h"
#include "langpack.h"
#include "mainDlg.h"

#include <QApplication>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QCursor>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextBrowser>

#include <vector>

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

// ---------------------------------------------------------------- CClosableTabCtrl

CClosableTabCtrl::CClosableTabCtrl(QWidget* parent) : QTabBar(parent)
{
    setTabsClosable(true);
    setExpanding(false);
    setUsesScrollButtons(true);
    setElideMode(Qt::ElideRight);
    setFocusPolicy(Qt::NoFocus);
    QObject::connect(this, &QTabBar::tabCloseRequested, this, [this](int i) {
        if (onCloseTab)
            onCloseTab(i);
    });
    QObject::connect(this, &QTabBar::currentChanged, this, [this](int) {
        if (!m_silent && onSelChange)
            onSelChange();
    });
}

void CClosableTabCtrl::SetCurSel(int i)
{
    m_silent = true;
    setCurrentIndex(i);
    m_silent = false;
}

MessagesContact* CClosableTabCtrl::GetItemParam(int i) const
{
    if (i < 0 || i >= count())
        return nullptr;
    return reinterpret_cast<MessagesContact*>(tabData(i).value<quintptr>());
}

int CClosableTabCtrl::InsertItem(int i, const QString& text, MessagesContact* param)
{
    m_silent = true;
    const int n = insertTab(i, text);
    setTabData(n, QVariant::fromValue(reinterpret_cast<quintptr>(param)));
    m_silent = false;
    return n;
}

void CClosableTabCtrl::DeleteItem(int i)
{
    m_silent = true;
    removeTab(i);
    m_silent = false;
}

void CClosableTabCtrl::SetImage(int i, int image)
{
    setTabIcon(i, image >= 0 && image < imageList.size() ? imageList.at(image) : QIcon());
}

void CClosableTabCtrl::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        const int i = tabAt(event->position().toPoint());
        if (i != -1 && onCloseTab) {
            onCloseTab(i);
            return;
        }
    }
    QTabBar::mouseReleaseEvent(event);
}

void CClosableTabCtrl::mouseDoubleClickEvent(QMouseEvent* event)
{
    const int i = tabAt(event->position().toPoint());
    if (i != -1 && onCloseTab) {
        onCloseTab(i);
        return;
    }
    QTabBar::mouseDoubleClickEvent(event);
}

// ---------------------------------------------------------------- MessagesDlg

MessagesDlg::MessagesDlg(QtMessageBackend* backend, QWidget* parent) : QWidget(parent, Qt::Window), m_backend(backend)
{
    m_hWnd = m_backend->Register(this);
    m_layout.customFactory = [this](const QString& id, const QString& text, QWidget* p) -> QWidget* {
        if (id == QLatin1String("IDC_MESSAGES_TAB"))
            return new CClosableTabCtrl(p);
        if (id == QLatin1String("IDC_CALL_END") || id == QLatin1String("IDC_END"))
            return new CButtonEx(text, p);
        if (id == QLatin1String("IDC_HOLD")) {
            // CHECKBOX BS_PUSHLIKE | BS_ICON
            auto* b = new QPushButton(p);
            b->setCheckable(true);
            b->setToolTip(text);
            return b;
        }
        if (id == QLatin1String("IDC_MESSAGE"))
            return new QPlainTextEdit(p);
        return nullptr;
    };
    m_layout.Build(this, QStringLiteral("IDD_MESSAGES"), false);
    m_messagesTab = static_cast<CClosableTabCtrl*>(DLG_ITEM(m_layout, IDC_MESSAGES_TAB));
    m_ButtonCall = static_cast<CButtonEx*>(DLG_ITEM(m_layout, IDC_CALL_END));
    m_ButtonEnd = static_cast<CButtonEx*>(DLG_ITEM(m_layout, IDC_END));
    m_buttonHold = static_cast<QPushButton*>(DLG_ITEM(m_layout, IDC_HOLD));
    m_list = static_cast<QTextBrowser*>(DLG_ITEM(m_layout, IDC_MESSAGES_LIST));
    m_message = static_cast<QPlainTextEdit*>(DLG_ITEM(m_layout, IDC_MESSAGE));
    OnInitDialog();
}

MessagesDlg::~MessagesDlg()
{
    for (int i = 0; i < m_messagesTab->GetItemCount(); i++) {
        delete GetMessageContact(i);
    }
    m_backend->Unregister(m_hWnd);
}

LRESULT MessagesDlg::WindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)lParam;
    switch (msg) {
    case UM_CLOSETAB:
        CloseTab((int)wParam);
        return TRUE;
    default:
        return 0;
    }
}

void MessagesDlg::OnInitDialog()
{
    if (langPack.rtl) {
        setLayoutDirection(Qt::RightToLeft);
    }
    // MySimpleSIP: o diálogo é redimensionável (WS_THICKFRAME); tamanho mínimo = o do .rc
    setMinimumSize(m_layout.BaseSize() / 2);

    m_layout.AutoMove(QStringLiteral("IDC_MESSAGES_TAB"), 0, 0, 100, 0);
    m_layout.AutoMove(QStringLiteral("IDC_LAST_CALL"), 100, 0, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_CLOSE_ALL"), 100, 0, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_CONFERENCE"), 100, 0, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_TRANSFER"), 100, 0, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_HOLD"), 100, 0, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_END"), 100, 0, 0, 0);
    m_layout.AutoMove(QStringLiteral("IDC_MESSAGES_LIST"), 0, 0, 100, 80);
    m_layout.AutoMove(QStringLiteral("IDC_MESSAGE"), 0, 80, 100, 20);
    lastCall = NULL;

    setWindowIcon(LoadImageIcon(IDI_MAINFRAME));

    m_messagesTab->imageList = {
        LoadImageIcon(IDI_CALL_OUT),
        LoadImageIcon(IDI_CALL_IN),
        LoadImageIcon(IDI_CALL_MISS),
        LoadImageIcon(IDI_ACTIVE),
        LoadImageIcon(IDI_ACTIVE_SECURE),
        LoadImageIcon(IDI_CONFERENCE),
        LoadImageIcon(IDI_CONFERENCE_SECURE),
        LoadImageIcon(IDI_MESSAGE_IN),
        LoadImageIcon(IDI_ON_HOLD),
        LoadImageIcon(IDI_ON_REMOTE_HOLD),
        LoadImageIcon(IDI_ON_REMOTE_HOLD_CONFERENCE),
    };
    m_messagesTab->onCloseTab = [this](int i) { PostMessage(UM_CLOSETAB, (WPARAM)i); };
    m_messagesTab->onSelChange = [this] { OnTcnSelchangeTab(); };

    m_layout.TranslateDialog(this);

#ifndef _GLOBAL_VIDEO
    DLG_ITEM(m_layout, IDC_VIDEO_CALL)->hide();
#else
    if (accountSettings.disableVideo) {
        DLG_ITEM(m_layout, IDC_VIDEO_CALL)->hide();
    }
#endif

    // fontes de 13 e 14 px (lfHeight = -MulDiv(13/14, dpiY, 96))
    QFont fontList = font();
    fontList.setPixelSize(13);
    QFont fontMessage = font();
    fontMessage.setPixelSize(14);

    // MySimpleSIP: SetAutoURLDetect + EN_LINK → links no HTML abertos por MSIP::OpenURL
    m_list->setOpenLinks(false);
    m_list->setFont(fontList);
    m_list->setContextMenuPolicy(Qt::NoContextMenu); // IDR_MENU_MESSAGES abaixo
    m_list->viewport()->installEventFilter(this);
    QObject::connect(m_list, &QTextBrowser::anchorClicked, this, [](const QUrl& url) {
        MSIP::OpenURL(CS(url.toString()));
    });
    m_message->setFont(fontMessage);
    m_message->installEventFilter(this);

    m_hIconHold = LoadImageIcon(IDI_HOLD);
    m_hIconResume = LoadImageIcon(IDI_RESUME);
    m_buttonHold->setIcon(m_hIconHold);

    m_ButtonCall->m_FaceColor = QColor(GetRValue(_GLOBAL_DIALER_CALL_COLOR), GetGValue(_GLOBAL_DIALER_CALL_COLOR), GetBValue(_GLOBAL_DIALER_CALL_COLOR));
    m_ButtonCall->m_TextColor = Qt::white;
    m_ButtonEnd->m_FaceColor = QColor(GetRValue(_GLOBAL_DIALER_END_COLOR), GetGValue(_GLOBAL_DIALER_END_COLOR), GetBValue(_GLOBAL_DIALER_END_COLOR));
    m_ButtonEnd->m_TextColor = Qt::white;

    auto click = [this](const char* id, std::function<void()> fn) {
        auto* b = qobject_cast<QPushButton*>(m_layout.Item(QString::fromLatin1(id)));
        if (b)
            QObject::connect(b, &QPushButton::clicked, this, [fn] { fn(); });
    };
    click("IDC_CALL_END", [this] { OnBnClickedCallEnd(); });
    click("IDC_VIDEO_CALL", [this] { OnBnClickedVideoCall(); });
    click("IDC_TRANSFER", [this] { OnBnClickedTransfer(); });
    click("IDC_CONFERENCE", [this] { OnBnClickedConference(); });
    click("IDC_HOLD", [this] {
        // MySimpleSIP: CHECKBOX (não automático): desfaz a alternância do QPushButton; o
        // estado vem do UpdateHoldButton
        m_buttonHold->setChecked(!m_buttonHold->isChecked());
        OnBnClickedHold();
    });
    click("IDC_END", [this] { OnBnClickedEnd(); });
    click("IDC_CLOSE_ALL", [this] { OnBnClickedCloseAll(); });
    click("IDC_LAST_CALL", [this] { OnBnClickedLastCall(); });
}

void MessagesDlg::closeEvent(QCloseEvent* event)
{
    // MySimpleSIP: no Qt 6, QApplication::quit() fecha as janelas e é cancelado se uma
    // delas recusar; no encerramento do app (CmainDlg::OnClose) o fechamento é aceito
    if (g_bClosingApp) {
        event->accept();
        return;
    }
    // OnSysCommand(SC_CLOSE) → OnCancel
    event->ignore();
    OnCancel();
}

bool MessagesDlg::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_message && event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if ((ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) && !(ke->modifiers() & Qt::ShiftModifier)) {
            OnEnMsgfilterMessage();
            return true;
        }
        if (ke->key() == Qt::Key_Escape) {
            OnCancel();
            return true;
        }
    }
    if (watched == m_list->viewport() && event->type() == QEvent::ContextMenu) {
        // IDR_MENU_MESSAGES
        auto* ce = static_cast<QContextMenuEvent*>(event);
        QMenu menu(this);
        menu.addAction(TR(_T("Copy")), this, [this] { OnCopy(); });
        menu.addAction(TR(_T("Select All")), this, [this] { OnSelectAll(); });
        menu.exec(ce->globalPos());
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void MessagesDlg::contextMenuEvent(QContextMenuEvent* event)
{
    // OnContextMenu: IDR_MENU_TABS
    QMenu menu(this);
    menu.addAction(TR(_T("Last Call")), this, [this] { OnGoToLastTab(); });
    menu.addAction(TR(_T("Close All")), this, [this] { OnCloseAllTabs(); });
    menu.exec(event->globalPos());
}

void MessagesDlg::OnClose()
{
    msip_call_hangup_all_noincoming();
    hide();
    MainDlg()->PostMessage(UM_NOTIFYICON, 0, WM_LBUTTONUP);
}

void MessagesDlg::moveEvent(QMoveEvent* event)
{
    QWidget::moveEvent(event);
    if (isVisible() && !isMaximized() && !isMinimized()) {
        accountSettings.messagesX = frameGeometry().left();
        accountSettings.messagesY = frameGeometry().top();
        MainDlg()->AccountSettingsPendingSave();
    }
}

void MessagesDlg::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_layout.ApplyAutoMove(size());
    if (isVisible() && !isMaximized() && !isMinimized()) {
        // MySimpleSIP: no Windows é o tamanho da janela com moldura; aqui, o da área útil
        accountSettings.messagesW = width();
        accountSettings.messagesH = height();
        MainDlg()->AccountSettingsPendingSave();
    }
}

void MessagesDlg::OnCancel()
{
    MessagesContact* messagesContact = GetMessageContact();
    if (messagesContact) {
        if (messagesContact->callId != -1) {
            OnBnClickedEnd();
            return;
        }
        OnGoToLastTab();
        MessagesContact* messagesContactLast = GetMessageContact();
        if (messagesContactLast) {
            if (messagesContactLast != messagesContact) {
                return;
            }
        }
    }
    OnClose();
}

void MessagesDlg::SetWindowTitle(const MessagesContact& messagesContact)
{
    if (messagesContact.name == messagesContact.numberShort) {
        setWindowTitle(QS(messagesContact.numberShort));
    }
    else {
        CString str;
        str.Format(_T("%s – %s"), messagesContact.name, messagesContact.numberShort);
        setWindowTitle(QS(str));
    }
}

MessagesContact* MessagesDlg::AddTab(CString address, BOOL activate, pjsua_call_info* call_info, call_user_data* user_data, BOOL notShowWindow, BOOL ifExists, CString numberOriginal, CString name)
{
    MessagesContact* messagesContact = nullptr;

    SIPURI sipuri;
    ParseCallSIPURI(address, user_data, sipuri);

    //-- incoming call
    if (call_info && call_info->role == PJSIP_ROLE_UAS) {
        //-- fix domain
        if (!sipuri.user.IsEmpty() && accountSettings.accountId) {
            if (MSIP::IsIP(MSIP::RemovePort(sipuri.domain))) {
                sipuri.domain = get_account_domain();
            }
        }
        //--
    }
    //--
    if (accountSettings.accountId && MSIP::RemovePort(get_account_domain()) == MSIP::RemovePort(sipuri.domain)) {
        sipuri.domain = get_account_domain();
    }

    CString aor = (!sipuri.user.IsEmpty() ? sipuri.user + _T("@") : _T("")) + sipuri.domain;
    CString numberShort = !sipuri.user.IsEmpty() ? sipuri.user : sipuri.domain;

    int nCallId = -1;
    int nNumber = -1;
    for (int i = 0; i < m_messagesTab->GetItemCount(); i++)
    {
        MessagesContact* pMessagesContact = GetMessageContact(i);
        if (call_info && call_info->id != -1 && call_info->id == pMessagesContact->callId) {
            nCallId = i;
            messagesContact = pMessagesContact;
        }
        if (pMessagesContact->aor == aor) {
            nNumber = i;
            if (nCallId == -1) {
                messagesContact = pMessagesContact;
            }
        }
    }
    int exists = nNumber;
    bool bNewCall = false;
    bool bUpdateNumber = false;
    if (nCallId != -1 || nNumber != -1) {
        if (nCallId != -1) {
            exists = nCallId;
            if (nNumber != -1 && nNumber != nCallId) {
                CloseTab(nNumber);
                if (nNumber < nCallId) {
                    exists--;
                }
            }
        }
        if (nNumber == -1 || nNumber != nCallId) {
            messagesContact->aor = aor;
            messagesContact->numberShort = numberShort;
            bUpdateNumber = true;
        }
        if (call_info) {
            if (messagesContact->callId != -1) {
                if (messagesContact->callId != call_info->id) {
                    if (call_info->role == PJSIP_ROLE_UAS && (call_info->state == PJSIP_INV_STATE_INCOMING || call_info->state == PJSIP_INV_STATE_EARLY)) {
                        MainDlg()->PostMessage(UM_CALL_ANSWER, (WPARAM)call_info->id, -486);
                    }
                    return NULL;
                }
            }
            else {
                bNewCall = true;
            }
        }
    }
    if (exists == -1 || bNewCall || bUpdateNumber) {
        if (name.IsEmpty()) {
            name = MainDlg()->GetNameForCall(sipuri, user_data, numberOriginal);
        }
    }
    if (exists == -1) {
        if (ifExists) {
            if (call_info && call_info->state == PJSIP_INV_STATE_DISCONNECTED) {
                if (call_info->role == PJSIP_ROLE_UAS && call_info->last_status == 486) {
                    MainDlg()->pageCalls->Add(call_info->call_id, aor + sipuri.suffix, name, L"", MSIP_CALL_TYPE_MISSED, user_data);
                }
            }
            return NULL;
        }
        messagesContact = new MessagesContact();
        messagesContact->aor = aor;
        messagesContact->numberShort = numberShort;
    }
    if (exists == -1 || bNewCall || bUpdateNumber || !name.IsEmpty()) {
        if (messagesContact->name != name) {
            messagesContact->name = name;
        }
        if (m_messagesTab->GetCurSel() == exists) {
            SetWindowTitle(*messagesContact);
        }
    }

    if (!call_info || call_info->state != PJSIP_INV_STATE_DISCONNECTED) {
        if (call_info) {
            messagesContact->callId = call_info->id;
            messagesContact->callIdStr = MSIP::PjToStr(&call_info->call_id);
        }
        if (messagesContact->callId == -1) {
            messagesContact->numberOriginal = numberOriginal;
            messagesContact->aorSuffix = sipuri.suffix;
        }
    }
    CString tabName = messagesContact->name;
    if (user_data) {
        user_data->CS.Lock();
        if (!user_data->diversion.IsEmpty()) {
            tabName = user_data->diversion + _T(" -> ") + tabName;
        }
        user_data->CS.Unlock();
    }
    // MySimpleSIP: sem os espaços de margem do original (o QTabBar já tem padding)
    if (exists == -1) {
        if (!m_messagesTab->GetItemCount()) {
            m_messagesTab->show();
            DLG_ITEM(m_layout, IDC_LAST_CALL)->setEnabled(true);
            DLG_ITEM(m_layout, IDC_CLOSE_ALL)->setEnabled(true);
            DLG_ITEM(m_layout, IDC_MESSAGE)->setEnabled(!accountSettings.disableMessaging);
        }
        exists = m_messagesTab->InsertItem(m_messagesTab->GetItemCount(), QS(tabName), messagesContact);
        if (m_messagesTab->GetCurSel() == exists) {
            OnChangeTab(call_info, user_data);
        }
    }
    else {
        //--
        m_messagesTab->setTabText(exists, QS(tabName));
        //--
        if (m_messagesTab->GetCurSel() == exists && call_info) {
            UpdateCallButton(messagesContact->callId != -1, call_info, user_data);
        }
    }
    //--update tab icon
    if (call_info) {
        UpdateTabIcon(messagesContact, exists, call_info, user_data);
    }

    if ((m_messagesTab->GetCurSel() != exists && activate)
        ) {
        OnTcnSelchangingTab();
        m_messagesTab->SetCurSel(exists);
        OnChangeTab(call_info, user_data);
    }
    if (!isVisible()) {
        bool show = false;
        if (!notShowWindow) {
            show = true;
        }
        if (show) {
            this->show();
            m_message->setFocus();
        }
    }
    return messagesContact;
}

void MessagesDlg::ShowMessages(const CString& messages)
{
    m_list->setHtml(QS(messages));
    m_list->verticalScrollBar()->setValue(m_list->verticalScrollBar()->maximum());
}

void MessagesDlg::OnChangeTab(pjsua_call_info* p_call_info, call_user_data* user_data, bool init)
{
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact) {
        return;
    }
    pjsua_call_info call_info;
    if (messagesContact->callId != -1) {
        if (!p_call_info) {
            if (is_pjsua_running() && pjsua_call_get_info(messagesContact->callId, &call_info) == PJ_SUCCESS) {
                p_call_info = &call_info;
            }
        }
    }
    if (messagesContact->hasNewMessages) {
        messagesContact->hasNewMessages = false;
        UpdateTabIcon(messagesContact, m_messagesTab->GetCurSel(), p_call_info, user_data);
    }

    SetWindowTitle(*messagesContact);

    if (messagesContact->callId != -1) {
        UpdateCallButton(TRUE, p_call_info, user_data);
        if (accountSettings.singleMode
            && p_call_info && (p_call_info->role == PJSIP_ROLE_UAC ||
                (p_call_info->role == PJSIP_ROLE_UAS &&
                    (p_call_info->state == PJSIP_INV_STATE_CONFIRMED
                        || p_call_info->state == PJSIP_INV_STATE_CONNECTING)
                    ))
            ) {
            if (messagesContact->name == messagesContact->numberShort) {
                MainDlg()->pageDialer->SetNumber(messagesContact->numberShort, 1);
            }
            else {
                CString str;
                str.Format(_T("%s – %s"), messagesContact->name, messagesContact->numberShort);
                MainDlg()->pageDialer->SetNumber(str, 1);
            }
        }
    }
    else {
        UpdateCallButton();
        if (accountSettings.singleMode) {
            // pageDialer->PostMessage(WM_COMMAND, IDC_CLEAR)
            QMetaObject::invokeMethod(MainDlg()->pageDialer, [] { MainDlg()->pageDialer->OnBnClickedClear(); }, Qt::QueuedConnection);
        }
    }
    if (init) {
        ShowMessages(messagesContact->messages);
        m_message->setPlainText(QS(messagesContact->message));
        m_message->moveCursor(QTextCursor::End);
    }
}

void MessagesDlg::OnTcnSelchangeTab()
{
    OnChangeTab();
    if (m_buttonHold->isVisibleTo(this) && m_buttonHold->isChecked()) {
        OnBnClickedHold();
    }
}

void MessagesDlg::OnTcnSelchangingTab()
{
    MessagesContact* messagesContact = GetMessageContact();
    if (messagesContact) {
        messagesContact->message = CS(m_message->toPlainText());
    }
}

BOOL MessagesDlg::CloseTab(int i, BOOL safe)
{
    MessagesContact* messagesContact = NULL;
    if (i == -1) {
        i = m_messagesTab->GetCurSel();
    }
    int count = m_messagesTab->GetItemCount();
    if (i < 0 || i >= count) {
        return TRUE;
    }
    messagesContact = GetMessageContact(i);
    if (!messagesContact) {
        return TRUE;
    }
    if (messagesContact->callId != -1) {
        if (safe) {
            return FALSE;
        }
        msip_call_hangup_fast(messagesContact->callId);
        if (count != m_messagesTab->GetItemCount()) {
            return TRUE;
        }
    }
    m_messagesTab->DeleteItem(i);
    count--;
    if (lastCall == messagesContact) {
        lastCall = nullptr; // MySimpleSIP: evita ponteiro pendente (o original só compara)
    }
    delete messagesContact;
    if (!count) {
        m_list->clear();
        m_message->clear();
        DLG_ITEM(m_layout, IDC_CALL_END)->hide();
        DLG_ITEM(m_layout, IDC_VIDEO_CALL)->hide();
        m_messagesTab->hide();
        DLG_ITEM(m_layout, IDC_LAST_CALL)->setEnabled(false);
        DLG_ITEM(m_layout, IDC_CLOSE_ALL)->setEnabled(false);
        DLG_ITEM(m_layout, IDC_MESSAGE)->setEnabled(false);
        if (isVisible()) {
            OnClose();
        }
    }
    else {
        int curSel = m_messagesTab->GetCurSel();
        m_messagesTab->SetCurSel(curSel < count && curSel != -1 ? curSel : count - 1);
        OnChangeTab();
    }

    return TRUE;
}

/*
 * Perform a call by string number.
 * You can pass initial call parameters via user_data.
 * Deletes user_data if the call fails.
 * Warning: it may end other calls (close tabs and delete related objects)
 */
pjsua_call_id MessagesDlg::PerformCall(CString number, bool hasVideo, pj_status_t* pStatus, call_user_data* user_data)
{
    pjsua_acc_id acc_id = PJSUA_INVALID_ID;
    pj_str_t pj_uri = {};
    do {
        if (!is_pjsua_running()) {
            if (pStatus) {
                *pStatus = PJSIP_ENOTINITIALIZED;
            }
            break;
        }
        if (!SelectSIPAccount(number, acc_id, &pj_uri)) {
            Account dummy;
            if (pStatus) {
                *pStatus = accountSettings.AccountLoad(1, &dummy) ? PJSIP_EAUTHACCDISABLED : PJSIP_EAUTHACCNOTFOUND;
            }
            break;
        }
        if (accountSettings.singleMode) {
            if (!user_data || (!user_data->inConference && !user_data->hidden)) {
                msip_call_hangup_all_noincoming();
            }
        }
#ifdef _GLOBAL_VIDEO
        if (hasVideo) {
            MainDlg()->createPreviewWin();
        }
#endif
        msip_set_sound_device(msip_audio_output);

        pjsua_call_setting call_setting;
        pjsua_call_setting_default(&call_setting);
        call_setting.flag = 0;
        call_setting.vid_cnt = hasVideo ? 1 : 0;

        pj_pool_t* pool = nullptr;
        pjsua_msg_data msg_data;
        msip_msg_data_init(pool, acc_id, msg_data);

        if (user_data && user_data->inConference) {
            pjsua_call_id call_ids[PJSUA_MAX_CALLS];
            unsigned calls_count = PJSUA_MAX_CALLS;
            if (pjsua_enum_calls(call_ids, &calls_count) == PJ_SUCCESS) {
                for (unsigned i = 0; i < calls_count; ++i) {
                    call_user_data* user_data_curr = (call_user_data*)pjsua_call_get_user_data(call_ids[i]);
                    if (user_data_curr) {
                        user_data_curr->CS.Lock();
                        if (user_data_curr->inConference) {
                            pjsua_call_info call_info_curr;
                            pjsua_call_get_info(call_ids[i], &call_info_curr);
                            pj_str_t hvalue, hname;
                            hname = pj_str((char*)"X-Conf-Call-ID");
                            hvalue = call_info_curr.call_id;
                            if (!pool) {
                                pool = pjsua_pool_create("msip_msg_data", 256, 256);
                            }
                            pjsip_generic_string_hdr* hdr = pjsip_generic_string_hdr_create(pool, &hname, &hvalue);
                            pj_list_push_back(&msg_data.hdr_list, hdr);
                        }
                        user_data_curr->CS.Unlock();
                    }
                }
            }
        }
        pjsua_call_id call_id = PJSUA_INVALID_ID;
        pj_status_t status = pjsua_call_make_call(
            acc_id,
            &pj_uri,
            &call_setting,
            user_data,
            &msg_data,
            &call_id);
        free(pj_uri.ptr);
        pj_pool_safe_release(&pool);
        if (pStatus) {
            *pStatus = status;
        }
        if (status != PJ_SUCCESS) {
            // MySimpleSIP: o original não libera user_data quando pjsua_call_make_call
            // falha (o StartCall o considera liberado); aqui ele é liberado
            break;
        }
        return call_id;
    } while (false);
    if (user_data) {
        delete user_data;
    }
    return PJSUA_INVALID_ID;
}

MessagesContact* MessagesDlg::StartCall(bool hasVideo, call_user_data* user_data)
{
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact || messagesContact->callId != -1) {
        if (user_data) {
            delete user_data;
        }
        return nullptr;
    }
    SIPURI sipuri;
    MSIP::ParseSIPURI(messagesContact->aor, sipuri);
    if (!sipuri.user.IsEmpty()) {
        accountSettings.lastCallNumber = sipuri.user;
        if (sipuri.domain != get_account_domain() || !messagesContact->aorSuffix.IsEmpty()) {
            accountSettings.lastCallNumber.AppendFormat(_T("@%s"), sipuri.domain);
        }
    }
    else {
        accountSettings.lastCallNumber = sipuri.domain;
    }
    accountSettings.lastCallNumber.Append(messagesContact->aorSuffix);
    accountSettings.lastCallNumber.Append(messagesContact->commands);
    accountSettings.lastCallHasVideo = hasVideo;

    if (!user_data) {
        user_data = new call_user_data(PJSUA_INVALID_ID);
    }
    else if (user_data->hidden) {
        // just in case, we must never get here
        delete user_data;
        return nullptr;
    }
    user_data->name = messagesContact->name;
    user_data->commands = messagesContact->commands.Mid(1);

    pj_status_t status = PJSIP_EINVALIDREQURI;

    CString address = messagesContact->aor + messagesContact->aorSuffix;
    pjsua_call_id call_id = PerformCall(address, hasVideo, &status, user_data);
    if (call_id != PJSUA_INVALID_ID) {
        messagesContact = nullptr;
        bool ok = false;
        pjsua_call_info call_info;
        if (pjsua_call_get_info(call_id, &call_info) == PJ_SUCCESS) {
            messagesContact = AddTab(address,
                accountSettings.singleMode,
                &call_info, user_data, true, false
            );
            if (messagesContact) {
                messagesContact->callId = call_id;
                UpdateCallButton(TRUE);
                MainDlg()->pageCalls->Add(call_info.call_id, messagesContact->numberOriginal, messagesContact->name, messagesContact->commands, MSIP_CALL_TYPE_OUTGOING, user_data);
                ok = true;
            }
        }
        if (!ok) {
            pjsua_call_hangup(call_id, 0, NULL, NULL);
        }
        return messagesContact;
    }
    else {
        user_data = nullptr; // the old object is deleted
        if (status != PJ_ERESOLVE) {
            CString descr;
            CString message = MSIP::GetErrorMessage(status, descr);
            // MySimpleSIP: sem os casos de privacidade do microfone do Windows
            // (ERROR_INVALID_PARAMETER, ms-settings:privacy-microphone)
            if (message.IsEmpty()) {
                message = Translate(_T("Microphone access is blocked. Please check your antivirus settings or security software."));
            }
            else if (status == PJMEDIA_EAUD_NODEFDEV) {
                message = Translate(_T("Microphone and speakers are required."));
                descr = Translate(_T("No microphone or audio output device was detected. Please connect them to make calls."));
            }
            // TaskDialog(TD_INFORMATION_ICON)
            QMessageBox box(QMessageBox::Information, QStringLiteral(_GLOBAL_NAME_NICE), QS(message), QMessageBox::Ok, MainDlg());
            box.setInformativeText(QS(descr));
            box.exec();
        }
    }
    return nullptr;
}

void MessagesDlg::OnBnClickedCallEnd()
{
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact) {
        return;
    }
    if (messagesContact->callId == -1) {
        messagesContact = StartCall();
    }
    else {
        MainDlg()->onCallAnswer((WPARAM)messagesContact->callId, (LPARAM)0);
    }
}

void MessagesDlg::OnEndCall(pjsua_call_info* call_info, call_user_data* user_data)
{
    SIPURI sipuri;
    ParseCallSIPURI(call_info, user_data, sipuri);

    CString numberLocal = sipuri.user;
    if (numberLocal.IsEmpty()) {
        numberLocal = sipuri.domain;
    }

    CString info;
    bool bError = true;
    bool elsewhere = false;
    if (call_info->state != PJSIP_INV_STATE_DISCONNECTED || call_info->last_status == 200) {
        info = _T("Call Ended");
        bError = false;
    }
    else {
        if (call_info->last_status == 404) {
            info = _T("Incorrect dialing or non-existent number.");
        }
        else if (call_info->last_status == 487) {
            info = _T("Canceled");
            if (user_data) {
                user_data->CS.Lock();
                if (!user_data->reason.IsEmpty()) {
                    info = user_data->reason;
                }
                user_data->CS.Unlock();
                if (info == _T("Call completed elsewhere")) {
                    elsewhere = true;
                }
            }
        }
        else if (call_info->last_status == 488) {
            info = _T("Unsupported codecs, encryption methods, or other media parameters.");
        }
        else {
            CString rab = MSIP::PjToStr(&call_info->last_status_text);
            if (rab.Find(_T("(PJ_ERESOLVE)")) != -1) {
                rab = _T("Cannot get IP address of the called host");
            }
            else if (rab.Find(_T("(WSAECONNREFUSED)")) != -1 || rab.Find(_T("(PJ_STATUS_FROM_OS(ECONNREFUSED))")) != -1) {
                // MySimpleSIP: no Linux o pjlib descreve os erros do sistema sem o prefixo WSA
                rab = _T("Connection refused");
            }
            else if (rab.Find(_T("(WSAETIMEDOUT)")) != -1 || rab.Find(_T("(PJ_STATUS_FROM_OS(ETIMEDOUT))")) != -1) {
                rab = _T("Connection timed out");
            }
            if (call_info->acc_id && call_info->last_status >= 500 && call_info->last_status < 600) {
                info.Format(_T("%s (%d). %s"), Translate(_T("Server error")), call_info->last_status, Translate(rab));
            }
            else {
                info = rab;
            }
        }
    }

    MessagesContact* messagesContact = NULL;
    int messagesContactNum = -1;
    for (int i = 0; i < m_messagesTab->GetItemCount(); i++)
    {
        messagesContact = GetMessageContact(i);
        messagesContactNum = i;
        if (messagesContact->callId == call_info->id)
        {
            lastCall = messagesContact;
            messagesContact->callId = -1;
            UpdateTabIcon(messagesContact);
            if (m_messagesTab->GetCurSel() == i) {
                UpdateCallButton(FALSE, call_info, user_data);
            }
            break;
        }
        messagesContact = NULL;
        messagesContactNum = -1;
    }

    CString aor;
    CString name;
    if (messagesContact) {
        aor = messagesContact->aor;
        name = messagesContact->name;
    }
    else {
        aor = (!sipuri.user.IsEmpty() ? sipuri.user + _T("@") : _T("")) + sipuri.domain;
        name = MainDlg()->GetNameForCall(sipuri, user_data);
    }
    if (call_info->role == PJSIP_ROLE_UAS && call_info->connect_duration.sec == 0 && call_info->connect_duration.msec == 0) {
        bool ignore = false;
        bool declined = false;
        if (user_data) {
            user_data->CS.Lock();
            ignore = user_data->hidden;
            declined = user_data->hangup;
            user_data->CS.Unlock();
        }
        if (!ignore) {
            MainDlg()->pageCalls->Add(call_info->call_id, aor, name, L"", elsewhere ? MSIP_CALL_TYPE_AE : MSIP_CALL_TYPE_MISSED, user_data);
            if (declined) {
                info = _T("Declined");
            }
            else if (elsewhere) {
                info = _T("Answered Elsewhere");
            }
            else if (info.IsEmpty()) {
                info = _T("Missed Call");
            }
        }
    }
    info = Translate(info);

    MainDlg()->pageCalls->SetDuration(call_info->call_id, msip_get_duration(&call_info->connect_duration), msip_get_duration(&call_info->total_duration));
    MainDlg()->pageCalls->SetInfo(call_info->call_id, info);
    if (user_data) {
        user_data->CS.Lock();
        user_data->duration = msip_get_duration(&call_info->connect_duration);
        user_data->CS.Unlock();
    }

    if (!messagesContact) {
        return;
    }

    if (MainDlg()->autoAnswerTimerCallId != PJSUA_INVALID_ID) {
        KillTimer(IDT_TIMER_AUTOANSWER);
        MainDlg()->autoAnswerTimerCallId = PJSUA_INVALID_ID;
    }
    if (MainDlg()->forwardingTimerCallId != PJSUA_INVALID_ID) {
        KillTimer(IDT_TIMER_FORWARDING);
        MainDlg()->forwardingTimerCallId = PJSUA_INVALID_ID;
    }

    msip_conference_leave(call_info, user_data);

    if (user_data) {
        user_data->CS.Lock();
        msip_call_recording_stop(user_data);
        /* Cancel duration timer, if any */
        if (user_data->auto_hangup_timer.id != PJSUA_INVALID_ID) {
            if (is_pjsua_running()) {
                pjsua_cancel_timer(&user_data->auto_hangup_timer);
            }
            user_data->auto_hangup_timer.id = PJSUA_INVALID_ID;
        }
        user_data->CS.Unlock();
    }
    msip_call_deinit_tonegen(call_info->id, user_data);

    if (accountSettings.localDTMF) {
        if (call_info->state != PJSIP_INV_STATE_DISCONNECTED || call_info->last_status == 200) {
            MainDlg()->onPlayerPlay(MSIP_SOUND_HANGUP, 0);
        }
    }

    CString message;
    if (bError) {
        // MySimpleSIP: o original declara outro "message" neste bloco e a mensagem de falha
        // fica vazia (o AddMessage abaixo é descartado); aqui ela é exibida
        message.Format(_T("%s %s"), Translate(_T("Call failed:")), info);
    }
    else {
        message = info;
    }
    AddMessage(messagesContact, message, MSIP_MESSAGE_TYPE_SYSTEM, TRUE);

    if (accountSettings.singleMode) {
        pjsua_call_id current_call_id = MainDlg()->CurrentCallId();
        if (current_call_id == call_info->id || current_call_id == -1) {
            MainDlg()->pageDialer->Clear(false);
            MainDlg()->pageDialer->UpdateCallButton(FALSE, 0);
        }
    }
    else {
        MainDlg()->pageDialer->Clear();
    }
    MainDlg()->pageDialer->SetName();

    MainDlg()->UpdateWindowText(_T("-"));

    if (call_info->state == PJSIP_INV_STATE_DISCONNECTED && call_info->last_status != 200) {
        if (accountSettings.singleMode) {
            if (call_info->last_status == 487 || (call_info->role == PJSIP_ROLE_UAS && (call_info->last_status == 486 || call_info->last_status == 600 || call_info->last_status == 603))) {
                //don't show
            }
            else {
                CString title;
                title.Format(_T("%s %s"), Translate(_T("Call failed:")), name);
                MainDlg()->BaloonPopup(title, info, NIIF_INFO);
            }
        }
    }

    if (call_info->state == PJSIP_INV_STATE_DISCONNECTED && (call_info->last_status == 486 || call_info->last_status == 600 || call_info->last_status == 603)) {
        //--
        if (!accountSettings.cmdCallBusy.IsEmpty()) {
            CString params = numberLocal;
            MSIP::RunCmd(accountSettings.cmdCallBusy, params);
        }
        //--
    }
    else {
        //--
        if (!accountSettings.cmdCallEnd.IsEmpty()) {
            CString params = numberLocal;
            MSIP::RunCmd(accountSettings.cmdCallEnd, params);
        }
        //--
    }

    if (accountSettings.singleMode) {
        if (messagesContactNum != -1) {
            bool isActive = m_messagesTab->GetCurSel() == messagesContactNum;
            CloseTab(messagesContactNum, TRUE);
            if (isActive) {
                OnGoToLastTab();
            }
        }
        else {
            OnGoToLastTab();
        }
    }
}

void MessagesDlg::UpdateCallButton(BOOL active, pjsua_call_info* call_info, call_user_data* user_data)
{
    if (active) {
        if (call_info && call_info->role == PJSIP_ROLE_UAS &&
            (call_info->state == PJSIP_INV_STATE_INCOMING || call_info->state == PJSIP_INV_STATE_EARLY)) {
            m_ButtonCall->show();
            m_ButtonCall->setText(TR(_T("Answer")));
        }
        else {
            m_ButtonCall->hide();
        }
    }
    else {
        m_ButtonCall->setText(TR(_T("Call")));
        m_ButtonCall->show();
    }
#ifdef _GLOBAL_VIDEO
    if (accountSettings.disableVideo) {
        DLG_ITEM(m_layout, IDC_VIDEO_CALL)->hide();
    }
    else {
        DLG_ITEM(m_layout, IDC_VIDEO_CALL)->setVisible(!active);
    }
#endif

    m_ButtonEnd->setVisible(active);
    UpdateHoldButton(call_info);
    UpdateRecButton(user_data);
    if (!active) {
        if (MainDlg()->transferDlg) {
            MainDlg()->transferDlg->OnClose();
        }
        // MySimpleSIP: WM_CANCELMODE (fecha menus abertos) não se aplica
    }
}

void MessagesDlg::UpdateHoldButton(pjsua_call_info* call_info)
{
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact) {
        return;
    }
    bool hasActions = false;
    bool hasHold = false;
    bool onHold = false;
    QWidget* buttonTransfer = DLG_ITEM(m_layout, IDC_TRANSFER);
    QWidget* buttonConference = DLG_ITEM(m_layout, IDC_CONFERENCE);
    QPushButton* buttonHold = m_buttonHold;
    QPushButton* buttonHoldDialer = MainDlg()->pageDialer->DlgButton(QStringLiteral("IDC_HOLD"));
    QPushButton* buttonTransferDialer = MainDlg()->pageDialer->DlgButton(QStringLiteral("IDC_TRANSFER"));
    if (messagesContact->callId != -1 && call_info) {
        if (messagesContact->callId != call_info->id) {
            return;
        }
        if (call_info->state == PJSIP_INV_STATE_EARLY ||
            call_info->state == PJSIP_INV_STATE_CONNECTING ||
            call_info->state == PJSIP_INV_STATE_CONFIRMED) {
            hasActions = true;
            if (call_info->state == PJSIP_INV_STATE_CONFIRMED) {
                hasHold = true;
                if (call_info->media_cnt > 0) {
                    if (call_info->media_status == PJSUA_CALL_MEDIA_LOCAL_HOLD
                        || call_info->media_status == PJSUA_CALL_MEDIA_NONE) {
                        onHold = true;
                    }
                }
            }
        }
    }
    //--
    buttonTransfer->setVisible(hasActions);
    buttonConference->setVisible(hasActions);
    buttonTransferDialer->setEnabled(hasActions);
    MainDlg()->pageDialer->EnableButtonCONF(hasActions);
    //--
    buttonHold->setVisible(hasHold);
    buttonHoldDialer->setEnabled(hasHold);
    //--
    if (onHold) {
        buttonHold->setChecked(true);
        buttonHold->setIcon(m_hIconResume);
        buttonHoldDialer->setChecked(true);
        buttonHoldDialer->setIcon(MainDlg()->pageDialer->m_hIconResume);
    }
    else {
        buttonHold->setChecked(false);
        buttonHold->setIcon(m_hIconHold);
        buttonHoldDialer->setChecked(false);
        buttonHoldDialer->setIcon(MainDlg()->pageDialer->m_hIconHold);
    }
    //--
}

void MessagesDlg::UpdateRecButton(call_user_data* user_data)
{
    bool state = false;
    MessagesContact* messagesContact = GetMessageContact();
    if (messagesContact) {
        if (messagesContact->callId != -1) {
            if (!user_data) {
                if (is_pjsua_running()) {
                    user_data = (call_user_data*)pjsua_call_get_user_data(messagesContact->callId);
                }
            }
            if (user_data) {
                user_data->CS.Lock();
                if (user_data->recorder_id != PJSUA_INVALID_ID) {
                    state = true;
                }
                user_data->CS.Unlock();
            }
        }
    }
    MainDlg()->pageDialer->SetCheckREC(state);
}

bool MessagesDlg::CallCheck()
{
    if (!accountSettings.singleMode || !GetCallsCount())
    {
        MessagesContact* messagesContact = GetMessageContact();
        if (!messagesContact || messagesContact->callId == -1)
        {
            return true;
        }
    }
    else {
        MainDlg()->GotoTab(0);
    }
    return false;
}

pjsua_call_id MessagesDlg::CurrentCallId()
{
    // declarado no MessagesDlg.h original, sem implementação; equivale a CmainDlg::CurrentCallId
    MessagesContact* messagesContact = GetMessageContact();
    if (messagesContact) {
        return messagesContact->callId;
    }
    return -1;
}

void MessagesDlg::Call(BOOL hasVideo)
{
    if (CallCheck()) {
        MessagesContact* messagesContact = GetMessageContact();
        call_user_data* user_data = new call_user_data(PJSUA_INVALID_ID);
        messagesContact = StartCall(hasVideo, user_data);
        (void)messagesContact;
    }
}

// MySimpleSIP: texto → HTML com os links detectados (SetAutoURLDetect do RichEdit)
static QString MessageHtml(const CString& text)
{
    static const QRegularExpression re(QStringLiteral(R"((https?://|ftp://|www\.)[^\s<]+)"));
    const QString s = QS(text);
    QString out;
    qsizetype last = 0;
    auto it = re.globalMatch(s);
    while (it.hasNext()) {
        const auto m = it.next();
        out += s.mid(last, m.capturedStart() - last).toHtmlEscaped();
        QString url = m.captured();
        QString href = url.startsWith(QLatin1String("www.")) ? QStringLiteral("http://") + url : url;
        out += QStringLiteral("<a href=\"%1\">%2</a>").arg(href.toHtmlEscaped(), url.toHtmlEscaped());
        last = m.capturedEnd();
    }
    out += s.mid(last).toHtmlEscaped();
    out.replace(QLatin1Char('\n'), QLatin1String("<br>"));
    return out;
}

void MessagesDlg::AddMessage(MessagesContact* messagesContact, CString message, int type, BOOL blockForeground, CTime* pTime)
{
    CTime tm;
    if (pTime) {
        tm = *pTime;
    }
    else {
        tm = CTime::GetCurrentTime();
    }
    if (type == MSIP_MESSAGE_TYPE_SYSTEM) {
        if (messagesContact->lastSystemMessage == message && messagesContact->lastSystemMessageTime > tm.GetTime() - 2) {
            messagesContact->lastSystemMessageTime = tm;
            return;
        }
        messagesContact->lastSystemMessage = message;
        messagesContact->lastSystemMessageTime = tm;
    }
    else if (!messagesContact->lastSystemMessage.IsEmpty()) {
        messagesContact->lastSystemMessage.Empty();
    }

    if (isVisible() && !blockForeground) {
        raise();
        activateWindow();
    }
    MessagesContact* messagesContactSelected = GetMessageContact();

    // cores e tamanhos do original: hora cinza 8 pt; nome em negrito; texto 10 pt
    // (mensagens do sistema em cinza 8 pt)
    QString color;
    CString name;
    if (type == MSIP_MESSAGE_TYPE_LOCAL) {
        color = QStringLiteral("#000000");
        if (!accountSettings.account.displayName.IsEmpty()) {
            name = accountSettings.account.displayName;
        }
    }
    else if (type == MSIP_MESSAGE_TYPE_REMOTE) {
        color = QStringLiteral("#1565ce");
        name = messagesContact->name;
        int pos = name.Find(_T(" ("));
        if (pos == -1) {
            pos = name.Find(_T("@"));
        }
        if (pos != -1) {
            name = name.Mid(0, pos);
        }
    }

    CString time = MSIP::FormatDateTime(&tm);
    const QString size = type != MSIP_MESSAGE_TYPE_SYSTEM ? QStringLiteral("10pt") : QStringLiteral("8pt");
    QString html = QStringLiteral("<p style=\"margin-left:5px; margin-top:5px; margin-bottom:0px\">"
                                  "<span style=\"color:#838383; font-size:8pt\">[%1]&nbsp;&nbsp;</span>")
                       .arg(QS(time).toHtmlEscaped());
    if (name.GetLength()) {
        html += QStringLiteral("<span style=\"color:%1; font-size:%2; font-weight:bold\">%3: </span>")
                    .arg(color, size, QS(name).toHtmlEscaped());
    }
    html += QStringLiteral("<span style=\"color:%1; font-size:%2\">%3</span></p>")
                .arg(type == MSIP_MESSAGE_TYPE_SYSTEM ? QStringLiteral("#838383") : color, size, MessageHtml(message));
    messagesContact->messages += CS(html);

    if (messagesContactSelected == messagesContact) {
        ShowMessages(messagesContact->messages);
    }
    else {
        if (type == MSIP_MESSAGE_TYPE_REMOTE) {
            messagesContact->hasNewMessages = true;
            UpdateTabIcon(messagesContact);
        }
    }
}

void MessagesDlg::OnEnMsgfilterMessage()
{
    CString message = CS(m_message->toPlainText());
    message.Trim();
    if (message.GetLength()) {
        MessagesContact* messagesContact = GetMessageContact();
        if (SendInstantMessage(messagesContact, message)) {
            m_message->clear();
            m_message->setFocus();
            AddMessage(messagesContact, message, MSIP_MESSAGE_TYPE_LOCAL);
            if (accountSettings.localDTMF) {
                MainDlg()->onPlayerPlay(MSIP_SOUND_MESSAGE_OUT, 0);
            }
        }
    }
}

BOOL MessagesDlg::SendInstantMessage(MessagesContact* messagesContact, CString message, CString number)
{
    message.Trim();
    if (message.GetLength()) {
        pjsua_acc_id acc_id;
        pj_str_t pj_uri;
        pj_status_t status;
        if (SelectSIPAccount(messagesContact ? messagesContact->aor + messagesContact->aorSuffix : number, acc_id, &pj_uri)) {
            pj_pool_t* pool = nullptr;
            pjsua_msg_data msg_data;
            msip_msg_data_init(pool, acc_id, msg_data);
            char* buf = MSIP::WideCharToPjStr(message);
            pj_str_t content = pj_str(buf); // MySimpleSIP: &pj_str(...) não compila no GCC
            status = pjsua_im_send(acc_id, &pj_uri, NULL, &content, &msg_data, NULL);
            free(pj_uri.ptr);
            free(buf);
            pj_pool_safe_release(&pool);
        }
        else {
            Account dummy;
            status = accountSettings.AccountLoad(1, &dummy) ? PJSIP_EAUTHACCDISABLED : PJSIP_EAUTHACCNOTFOUND;
        }
        if (status != PJ_SUCCESS) {
            if (messagesContact) {
                CString descr;
                CString message = MSIP::GetErrorMessage(status, descr);
                AddMessage(messagesContact, message);
            }
        }
        else {
            return TRUE;
        }
    }
    return FALSE;
}

MessagesContact* MessagesDlg::GetMessageContact(int i)
{
    if (i == -1) {
        i = m_messagesTab->GetCurSel();
    }
    if (i != -1) {
        return m_messagesTab->GetItemParam(i);
    }
    return NULL;
}

MessagesContact* MessagesDlg::GetMessageContactInCall()
{
    MessagesContact* messagesContactActive = NULL;
    for (int i = 0; i < m_messagesTab->GetItemCount(); i++) {
        MessagesContact* messagesContact = GetMessageContact(i);
        if (messagesContact->callId != -1) {
            if (messagesContact->mediaStatus == PJSUA_CALL_MEDIA_ACTIVE || messagesContact->mediaStatus == PJSUA_CALL_MEDIA_REMOTE_HOLD) {
                messagesContactActive = messagesContact;
                break;
            }
            if (messagesContact->mediaStatus == PJSUA_CALL_MEDIA_LOCAL_HOLD || messagesContact->mediaStatus == PJSUA_CALL_MEDIA_NONE) {
                messagesContactActive = messagesContact;
            }
        }
    }
    return messagesContactActive;
}

void MessagesDlg::OnBnClickedVideoCall()
{
    StartCall(true);
}

void MessagesDlg::Merge(pjsua_call_id call_id)
{
    if (!is_pjsua_running()) {
        return;
    }
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact || messagesContact->callId == -1) {
        return;
    }
    if (messagesContact->callId == call_id || !pjsua_call_is_active(call_id)) {
        return;
    }
    call_user_data* user_data;
    user_data = (call_user_data*)pjsua_call_get_user_data(messagesContact->callId);
    if (!user_data) {
        user_data = new call_user_data(messagesContact->callId);
        pjsua_call_set_user_data(messagesContact->callId, user_data);
    }
    user_data->CS.Lock();
    user_data->inConference = true;
    user_data->CS.Unlock();

    user_data = (call_user_data*)pjsua_call_get_user_data(call_id);
    if (!user_data) {
        user_data = new call_user_data(messagesContact->callId);
        pjsua_call_set_user_data(messagesContact->callId, user_data);
    }
    user_data->CS.Lock();
    user_data->inConference = true;
    user_data->CS.Unlock();

    pjsua_call_info call_info;
    if (pjsua_call_get_info(call_id, &call_info) != PJ_SUCCESS) {
        return;
    }
    msip_conference_join(&call_info);
    msip_call_unhold(&call_info);
}

void MessagesDlg::Separate(pjsua_call_id call_id)
{
    if (!is_pjsua_running()) {
        return;
    }
    pjsua_call_info call_info;
    if (pjsua_call_get_info(call_id, &call_info) != PJ_SUCCESS) {
        return;
    }
    msip_conference_leave(&call_info);
    msip_call_unhold(&call_info);
}

bool MessagesDlg::CallAction(int action, CString number, pjsua_call_id forward_call_id)
{
    number.Trim();
    if (action == MSIP_ACTION_FORWARD) {
        if (forward_call_id != PJSUA_INVALID_ID) {
            if (number.IsEmpty()) {
                number = accountSettings.forwardingNumber;
            }
            if (!number.IsEmpty()) {
                CString commands;
                CString numberFormated = FormatNumber(number, &commands);
                pjsua_msg_data msg_data;
                pjsua_msg_data_init(&msg_data);
                pjsip_generic_string_hdr subject;
                pj_str_t hvalue, hname;
                hname = pj_str((char*)"Contact");
                char* buf = MSIP::WideCharToPjStr(numberFormated);
                pj_strset2(&hvalue, buf);
                pjsip_generic_string_hdr_init2(&subject, &hname, &hvalue);
                pj_list_push_back(&msg_data.hdr_list, &subject);
                pjsua_call_info call_info;
                pjsip_generic_string_hdr diversion;
                if (pjsua_call_get_info(forward_call_id, &call_info) == PJ_SUCCESS) {
                    hname = pj_str((char*)"Diversion");
                    hvalue = call_info.local_info;
                    pjsip_generic_string_hdr_init2(&diversion, &hname, &hvalue);
                    pj_list_push_back(&msg_data.hdr_list, &diversion);
                }
                pj_status_t status = pjsua_call_answer(forward_call_id, 302, NULL, &msg_data);
                free(buf);
                if (status == PJ_SUCCESS) {
                    return true;
                }
            }
        }
        return false;
    }
    if (!number.IsEmpty()) {
        MessagesContact* messagesContactSelected = GetMessageContact();
        if (!messagesContactSelected || messagesContactSelected->callId == -1) {
            return false;
        }
        CString commands;
        CString numberFormated = FormatNumber(number, &commands);
        SIPURI sipuri;
        MSIP::ParseSIPURI(numberFormated, sipuri);
        if (messagesContactSelected->aor == sipuri.user + _T("@") + sipuri.domain || (sipuri.user.IsEmpty() && messagesContactSelected->aor == sipuri.domain)) {
            CString title;
            if (action == MSIP_ACTION_TRANSFER) {
                title = Translate(_T("Blind Transfer"));
            }
            else if (action == MSIP_ACTION_ATTENDED_TRANSFER) {
                title = Translate(_T("Attended Transfer"));
            }
            else if (action == MSIP_ACTION_INVITE) {
                title = Translate(_T("Invite to Conference"));
            }
            CString message = Translate(_T("You already have an active call with this number."));
            MainDlg()->BaloonPopup(title, message, NIIF_ERROR);
            return false;
        }
        call_user_data* user_data;
        user_data = (call_user_data*)pjsua_call_get_user_data(messagesContactSelected->callId);
        if (action == MSIP_ACTION_TRANSFER || action == MSIP_ACTION_ATTENDED_TRANSFER) {
            bool xfer;
            if (user_data) {
                user_data->CS.Lock();
                xfer = !user_data->inConference;
                user_data->CS.Unlock();
            }
            else {
                xfer = true;
            }
            number = sipuri.user;
            if (xfer) {
                if (action == MSIP_ACTION_ATTENDED_TRANSFER) {
                    bool attendedTransferFC = accountSettings.enableFeatureCodeAT && !accountSettings.featureCodeAT.IsEmpty();
                    CString str = accountSettings.featureCodeAT;
                    if (attendedTransferFC) {
                        str.Append(number);
                        msip_call_dial_dtmf(messagesContactSelected->callId, str, true);
                    }
                    else {
                        if (!accountSettings.singleMode) {
                            MainDlg()->MakeCall(number);
                        }
                        else {
                            QMessageBox::information(this, QStringLiteral(_GLOBAL_NAME_NICE), TR(_T("Attended transfer is not available in single-call mode. Disable this mode or use PBX feature codes.")));
                        }
                    }
                    return true;
                }
                else {
                    bool blindTransferFC = accountSettings.enableFeatureCodeBT && !accountSettings.featureCodeBT.IsEmpty();
                    CString str = accountSettings.featureCodeBT;
                    if (blindTransferFC) {
                        str.Append(number);
                        msip_call_dial_dtmf(messagesContactSelected->callId, str, true);
                    }
                    else {
                        char* buf = MSIP::WideCharToPjStr(numberFormated);
                        pj_str_t dest = pj_str(buf);
                        pjsua_call_xfer(messagesContactSelected->callId, &dest, NULL);
                        free(buf);
                    }
                }
                return true;
            }
        }
        if (action == MSIP_ACTION_INVITE) {
            if (MainDlg()->MessagesOpen(number, true)) {
                MessagesContact* messagesContact = GetMessageContact();
                if (messagesContact && messagesContact->callId == -1) {
                    pjsua_call_info call_info;
                    pjsua_call_get_info(messagesContactSelected->callId, &call_info);
                    msip_call_unhold(&call_info);
                    if (!user_data) {
                        user_data = new call_user_data(messagesContactSelected->callId);
                        pjsua_call_set_user_data(messagesContactSelected->callId, user_data);
                    }
                    call_user_data* user_data_new = new call_user_data(PJSUA_INVALID_ID);
                    user_data_new->inConference = true;
                    messagesContact = StartCall(false, user_data_new);
                    messagesContactSelected = nullptr; // invalidate pointer after StartCall
                    if (messagesContact) {
                        user_data->CS.Lock();
                        user_data->inConference = true;
                        user_data->CS.Unlock();
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void MessagesDlg::OnBnClickedHold()
{
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact || messagesContact->callId == -1) {
        return;
    }
    pjsua_call_info call_info;
    if (is_pjsua_running() && pjsua_call_get_info(messagesContact->callId, &call_info) == PJ_SUCCESS) {
        if (call_info.state == PJSIP_INV_STATE_CONFIRMED) {
            if (call_info.media_status == PJSUA_CALL_MEDIA_LOCAL_HOLD || call_info.media_status == PJSUA_CALL_MEDIA_NONE) {
                msip_call_unhold(&call_info);
            }
            else {
                msip_call_hold(&call_info);
            }
        }
    }
}

void MessagesDlg::OnBnClickedTransfer()
{
    if (accountSettings.enableFeatureCodeAT
        && !accountSettings.featureCodeAT.IsEmpty()) {
        MainDlg()->OpenTransferDlg(this, MSIP_ACTION_TRANSFER);
    }
    else {
        OnBnClickedActions();
    }
}

void MessagesDlg::OnBnClickedConference()
{
    OnBnClickedActions(true);
}

void MessagesDlg::OnBnClickedActions(bool isConference)
{
    if (!is_pjsua_running()) {
        return;
    }
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact || messagesContact->callId == -1) {
        return;
    }
    // MySimpleSIP: menuTransfer/menuConference (recursos) → QMenu montado aqui
    QMenu tracker(this);
    QAction* actTransfer = nullptr;
    QAction* actAttended = nullptr;
    QAction* actConference = nullptr;
    QAction* actMerge = nullptr;
    QAction* actMergeAll = nullptr;
    QAction* actSeparate = nullptr;
    QAction* actSeparateAll = nullptr;
    QAction* actDisconnect = nullptr;
    QMenu menuAttendedTransfer(TR(_T("Transfer Call to")), &tracker);
    QMenu menuMerge(TR(_T("Merge")), &tracker);
    pjsua_call_info call_info;
    if (pjsua_call_get_info(messagesContact->callId, &call_info) != PJ_SUCCESS) {
        return;
    }
    call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(messagesContact->callId);
    bool inConference = false;
    if (user_data) {
        user_data->CS.Lock();
        if (user_data->inConference) {
            inConference = true;
        }
        user_data->CS.Unlock();
    }
    pjsua_call_id call_ids[PJSUA_MAX_CALLS];
    unsigned calls_count = PJSUA_MAX_CALLS;
    std::vector<std::pair<CString, pjsua_call_id>> attended;
    std::vector<std::pair<CString, pjsua_call_id>> merge;
    pjsua_call_id mergeConferenceAddedId = PJSUA_INVALID_ID;
    if (pjsua_enum_calls(call_ids, &calls_count) == PJ_SUCCESS) {
        for (unsigned i = 0; i < calls_count; ++i) {
            if (call_ids[i] != messagesContact->callId) {
                pjsua_call_info call_info_curr;
                pjsua_call_get_info(call_ids[i], &call_info_curr);
                call_user_data* user_data_curr = (call_user_data*)pjsua_call_get_user_data(call_ids[i]);
                bool inConferenceCurr = false;
                if (user_data_curr) {
                    user_data_curr->CS.Lock();
                    if (user_data_curr->inConference) {
                        inConferenceCurr = true;
                    }
                    user_data_curr->CS.Unlock();
                }
                CString str;
                //--attended transfer
                if (call_info_curr.role == PJSIP_ROLE_UAS || call_info_curr.state == PJSIP_INV_STATE_CONFIRMED) {
                    SIPURI sipuri_curr;
                    ParseCallSIPURI(&call_info_curr, user_data_curr, sipuri_curr);
                    str = !sipuri_curr.name.IsEmpty() ? sipuri_curr.name : (!sipuri_curr.user.IsEmpty() ? sipuri_curr.user : (sipuri_curr.domain));
                    if (!isConference) {
                        if (!inConference && !inConferenceCurr) {
                            attended.emplace_back(str, call_ids[i]);
                        }
                    }
                }
                //--merge
                if (call_info.state == PJSIP_INV_STATE_CONFIRMED && call_info_curr.state == PJSIP_INV_STATE_CONFIRMED) {
                    if (!inConference || !inConferenceCurr) {
                        if (!inConferenceCurr || mergeConferenceAddedId == PJSUA_INVALID_ID) {
                            if (inConferenceCurr) {
                                str = Translate(_T("Conference"));
                                mergeConferenceAddedId = call_ids[i];
                            }
                            merge.emplace_back(str, call_ids[i]);
                        }
                    }
                }
                //--
            }
        }
    }
    if (!inConference && mergeConferenceAddedId != PJSUA_INVALID_ID && merge.size() > 1) {
        // only 1 conference allowed, remove all regular calls
        merge.clear();
        merge.emplace_back(Translate(_T("Conference")), mergeConferenceAddedId);
    }
    for (const auto& [name, id] : attended) {
        menuAttendedTransfer.addAction(QS(name), this, [this, id = id] { OnAttendedTransferRange(id); });
    }
    for (const auto& [name, id] : merge) {
        menuMerge.addAction(QS(name), this, [this, id = id] { OnMerge(id); });
    }
    if (isConference) {
        // IDR_MENU_CALL_CONFERENCE
        actConference = tracker.addAction(TR(_T("Add")), this, [this] { OnConference(); });
        actMerge = tracker.addMenu(&menuMerge);
        actMergeAll = tracker.addAction(TR(_T("Merge All")), this, [this] { OnMergeAll(); });
        CString str;
        CString format = _T("%s (%s)");
        str.Format(format, Translate(_T("Separate")), messagesContact->name);
        actSeparate = tracker.addAction(QS(str), this, [this] { OnSeparate(); });
        actSeparateAll = tracker.addAction(TR(_T("Separate All")), this, [this] { OnSeparateAll(); });
        str.Format(format, Translate(_T("Disconnect")), messagesContact->name);
        actDisconnect = tracker.addAction(QS(str), this, [this] { OnDisconnect(); });
    }
    else {
        // IDR_MENU_CALL_TRANSFER (+ "Transfer Call to" no topo)
        if (!attended.empty()) {
            tracker.addMenu(&menuAttendedTransfer);
        }
        actTransfer = tracker.addAction(TR(_T("Blind Transfer")), this, [this] { OnTransfer(); });
        actAttended = tracker.addAction(TR(_T("Attended Transfer")), this, [this] { OnAttendedTransfer(); });
    }
    //-- transfer
    if (actTransfer) {
        actTransfer->setEnabled(!inConference);
        actAttended->setEnabled(!inConference);
    }
    if (actMerge) {
        actMerge->setEnabled(!merge.empty());
        actMergeAll->setEnabled(!merge.empty());
        //-- invite in conference
        actConference->setEnabled(call_info.state == PJSIP_INV_STATE_CONFIRMED && (inConference || mergeConferenceAddedId == PJSUA_INVALID_ID));
        //-- separate & disconnect
        actSeparate->setEnabled(inConference);
        actSeparateAll->setEnabled(inConference);
        actDisconnect->setEnabled(inConference);
    }
    tracker.exec(QCursor::pos());
}

void MessagesDlg::OnTransfer()
{
    MainDlg()->OpenTransferDlg(this, MSIP_ACTION_TRANSFER);
}

void MessagesDlg::OnAttendedTransfer()
{
    MainDlg()->OpenTransferDlg(this, MSIP_ACTION_ATTENDED_TRANSFER);
}

void MessagesDlg::OnAttendedTransferRange(pjsua_call_id call_id)
{
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact || messagesContact->callId == -1) {
        return;
    }
    if (!pjsua_call_is_active(call_id)) {
        return;
    }
    pjsua_call_xfer_replaces(messagesContact->callId, call_id, 0, 0);
}

void MessagesDlg::OnConference()
{
    MainDlg()->OpenTransferDlg(this, MSIP_ACTION_INVITE);
}

void MessagesDlg::OnMerge(pjsua_call_id call_id)
{
    Merge(call_id);
}

void MessagesDlg::OnMergeAll()
{
    if (!is_pjsua_running()) {
        return;
    }
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact || messagesContact->callId == -1) {
        return;
    }
    pjsua_call_id call_ids[PJSUA_MAX_CALLS];
    unsigned calls_count = PJSUA_MAX_CALLS;
    if (pjsua_enum_calls(call_ids, &calls_count) == PJ_SUCCESS) {
        for (unsigned i = 0; i < calls_count; ++i) {
            if (call_ids[i] != messagesContact->callId) {
                Merge(call_ids[i]);
            }
        }
    }
}

void MessagesDlg::OnSeparate()
{
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact || messagesContact->callId == -1) {
        return;
    }
    Separate(messagesContact->callId);
}

void MessagesDlg::OnSeparateAll()
{
    if (!is_pjsua_running()) {
        return;
    }
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact || messagesContact->callId == -1) {
        return;
    }
    pjsua_call_info call_info;
    pjsua_call_get_info(messagesContact->callId, &call_info);
    msip_call_hold(&call_info);
    pjsua_call_id call_ids[PJSUA_MAX_CALLS];
    unsigned calls_count = PJSUA_MAX_CALLS;
    if (pjsua_enum_calls(call_ids, &calls_count) == PJ_SUCCESS) {
        for (unsigned i = 0; i < calls_count; ++i) {
            if (call_ids[i] != messagesContact->callId) {
                pjsua_call_info call_info;
                if (pjsua_call_get_info(call_ids[i], &call_info) == PJ_SUCCESS) {
                    msip_conference_leave(&call_info);
                }
            }
        }
    }
}

void MessagesDlg::OnDisconnect()
{
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact || messagesContact->callId == -1) {
        return;
    }
    msip_call_hangup_fast(messagesContact->callId);
}

void MessagesDlg::OnBnClickedEnd()
{
    MessagesContact* messagesContact = GetMessageContact();
    if (!messagesContact || messagesContact->callId == -1) {
        return;
    }
    msip_call_end(messagesContact->callId);
}

void MessagesDlg::OnCloseAllTabs()
{
    int i = 0;
    while (i < m_messagesTab->GetItemCount()) {
        if (CloseTab(i, TRUE)) {
            i = 0;
        }
        else {
            i++;
        }
    }
}

void MessagesDlg::OnGoToLastTab()
{
    int i = 0;
    BOOL found = FALSE;
    int lastCallIndex = -1;
    while (i < m_messagesTab->GetItemCount()) {
        MessagesContact* messagesContact = GetMessageContact(i);
        if (messagesContact->callId != -1) {
            found = TRUE;
            if (m_messagesTab->GetCurSel() != i) {
                OnTcnSelchangingTab();
                m_messagesTab->SetCurSel(i);
                OnChangeTab();
                break;
            }
        }
        if (messagesContact == lastCall) {
            lastCallIndex = i;
        }
        i++;
    }
    if (!found && lastCallIndex != -1) {
        if (m_messagesTab->GetCurSel() != lastCallIndex) {
            OnTcnSelchangingTab();
            m_messagesTab->SetCurSel(lastCallIndex);
            OnChangeTab();
        }
    }
}

int MessagesDlg::GetCallDuration(pjsua_call_id* call_id)
{
    int duration = -1;
    pjsua_call_info call_info;
    int i = 0;
    int count = 0;
    while (i < m_messagesTab->GetItemCount()) {
        MessagesContact* messagesContact = GetMessageContact(i);
        if (messagesContact->callId != -1) {
            if (is_pjsua_running() && pjsua_call_get_info(messagesContact->callId, &call_info) == PJ_SUCCESS) {
                if (call_info.state == PJSIP_INV_STATE_CONFIRMED) {
                    duration = msip_get_duration(&call_info.connect_duration);
                    *call_id = messagesContact->callId;
                    count++;
                }
            }
        }
        i++;
    }
    if (count > 1) {
        *call_id = PJSUA_INVALID_ID;
        duration = count;
    }
    return duration;
}

int MessagesDlg::GetCallsCount(bool withIncoming, bool onlyActive)
{
    int count = 0;
    int i = 0;
    while (i < m_messagesTab->GetItemCount()) {
        MessagesContact* messagesContact = GetMessageContact(i);
        if (messagesContact->callId != -1) {
            if (onlyActive) {
                if (messagesContact->mediaStatus == PJSUA_CALL_MEDIA_ACTIVE) {
                    count++;
                }
            }
            else {
                if (withIncoming) {
                    count++;
                }
                else {
                    pjsua_call_info call_info;
                    if (is_pjsua_running() && pjsua_call_get_info(messagesContact->callId, &call_info) == PJ_SUCCESS) {
                        if (call_info.role != PJSIP_ROLE_UAS || (call_info.state != PJSIP_INV_STATE_INCOMING && call_info.state != PJSIP_INV_STATE_EARLY)) {
                            count++;
                        }
                    }
                }
            }
        }
        i++;
    }
    return count;
}

void MessagesDlg::OnCopy()
{
    m_list->copy();
}

void MessagesDlg::OnSelectAll()
{
    m_list->selectAll();
}

void MessagesDlg::OnBnClickedCloseAll()
{
    OnCloseAllTabs();
}

void MessagesDlg::OnBnClickedLastCall()
{
    OnGoToLastTab();
}

void MessagesDlg::UpdateTabIcon(MessagesContact* messagesContact, int tabIndex, pjsua_call_info* p_call_info, call_user_data* user_data)
{
    if (tabIndex == -1) {
        for (int i = 0; i < m_messagesTab->GetItemCount(); i++) {
            if (messagesContact == GetMessageContact(i)) {
                tabIndex = i;
                break;
            }
        }
    }
    if (tabIndex == -1) {
        return;
    }
    int icon = -1;
    if (messagesContact->hasNewMessages) {
        icon = MSIP_TAB_ICON_MESSAGE_IN;
    }
    else if (p_call_info) {
        //-----------------
        switch (p_call_info->state) {
        case PJSIP_INV_STATE_NULL:
        case PJSIP_INV_STATE_DISCONNECTED:
            if (p_call_info->role == PJSIP_ROLE_UAS && !p_call_info->connect_duration.sec && !p_call_info->connect_duration.msec) {
                icon = MSIP_TAB_ICON_CALL_MISS;
            }
            break;
        case PJSIP_INV_STATE_CONFIRMED:
            if (p_call_info->media_status == PJSUA_CALL_MEDIA_LOCAL_HOLD || p_call_info->media_status == PJSUA_CALL_MEDIA_NONE) {
                icon = MSIP_TAB_ICON_ON_HOLD;
            }
            else {
                if (!user_data) {
                    if (is_pjsua_running()) {
                        user_data = (call_user_data*)pjsua_call_get_user_data(p_call_info->id);
                    }
                }
                if (user_data) {
                    user_data->CS.Lock();
                    if (p_call_info->media_status == PJSUA_CALL_MEDIA_REMOTE_HOLD) {
                        if (user_data && user_data->inConference) {
                            icon = MSIP_TAB_ICON_ON_REMOTE_HOLD_CONFERENCE;
                        }
                        else {
                            icon = MSIP_TAB_ICON_ON_REMOTE_HOLD;
                        }
                    }
                    else {
                        if (user_data->inConference) {
                            if (user_data->srtp == MSIP_SRTP) {
                                icon = MSIP_TAB_ICON_CONFERENCE_SECURE;
                            }
                            else {
                                icon = MSIP_TAB_ICON_CONFERENCE;
                            }
                        }
                        else {
                            if (user_data->srtp == MSIP_SRTP) {
                                icon = MSIP_TAB_ICON_ACTIVE_SECURE;
                            }
                            else {
                                icon = MSIP_TAB_ICON_ACTIVE;
                            }
                        }
                    }
                    user_data->CS.Unlock();
                }
                else {
                    if (p_call_info->media_status == PJSUA_CALL_MEDIA_REMOTE_HOLD) {
                        icon = MSIP_TAB_ICON_ON_REMOTE_HOLD;
                    }
                    else {
                        icon = MSIP_TAB_ICON_ACTIVE;
                    }
                }
            }
            break;
        default:
            if (p_call_info->role == PJSIP_ROLE_UAS) {
                icon = MSIP_TAB_ICON_CALL_IN;
            }
            else {
                icon = MSIP_TAB_ICON_CALL_OUT;
            }
            break;
        }
        //-----------------
    }
    m_messagesTab->SetImage(tabIndex, icon);
}

bool MessagesDlg::GotoTab(int i)
{
    int nTab = m_messagesTab->GetCurSel();
    if (i < 0) {
        int max = m_messagesTab->GetItemCount() - 1;
        if (i == -1) {
            i = nTab < max ? nTab + 1 : 0;
        }
        else {
            i = nTab == 0 ? max : nTab - 1;
        }
    }
    if (nTab != i) {
        if (i >= m_messagesTab->GetItemCount()) {
            i = 0;
        }
        if (nTab != i) {
            OnTcnSelchangingTab();
            m_messagesTab->SetCurSel(i);
            OnTcnSelchangeTab();
            return true;
        }
    }
    return false;
}

void MessagesDlg::TabIconUpdate(pjsua_call_id call_id)
{
    for (int i = 0; i < m_messagesTab->GetItemCount(); i++) {
        MessagesContact* messagesContact = GetMessageContact(i);
        if (messagesContact->callId == call_id) {
            pjsua_call_info call_info;
            if (pjsua_call_get_info(call_id, &call_info) == PJ_SUCCESS) {
                UpdateTabIcon(messagesContact, i, &call_info);
            }
            break;
        }
    }
}
