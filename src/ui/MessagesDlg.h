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

// Uma aba por contato/chamada (MessagesContact no lParam da aba), botões de chamada
// (Chamar/Atender, Vídeo, Espera, Transferir, Conferência, Encerrar), histórico da
// conversa e campo de mensagem. Mesmos nomes de métodos do original.

#pragma once

#include "DialogLayout.h"
#include "QtMessageBackend.h"
#include "global.h"

#include <QIcon>
#include <QTabBar>
#include <QWidget>

class CButtonEx;
class QPlainTextEdit;
class QPushButton;
class QTextBrowser;

enum {
    MSIP_TAB_ICON_CALL_OUT,
    MSIP_TAB_ICON_CALL_IN,
    MSIP_TAB_ICON_CALL_MISS,
    MSIP_TAB_ICON_ACTIVE,
    MSIP_TAB_ICON_ACTIVE_SECURE,
    MSIP_TAB_ICON_CONFERENCE,
    MSIP_TAB_ICON_CONFERENCE_SECURE,
    MSIP_TAB_ICON_MESSAGE_IN,
    MSIP_TAB_ICON_ON_HOLD,
    MSIP_TAB_ICON_ON_REMOTE_HOLD,
    MSIP_TAB_ICON_ON_REMOTE_HOLD_CONFERENCE
};

#include "Transfer.h" // msip_action

// CClosableTabCtrl: abas com botão de fechar; clique do meio e duplo clique fecham
// (UM_CLOSETAB). O item guarda o MessagesContact* (lParam do TCITEM).
class CClosableTabCtrl : public QTabBar {
public:
    explicit CClosableTabCtrl(QWidget* parent = nullptr);
    int GetItemCount() const { return count(); }
    int GetCurSel() const { return currentIndex(); }
    void SetCurSel(int i);
    MessagesContact* GetItemParam(int i) const;
    int InsertItem(int i, const QString& text, MessagesContact* param);
    void DeleteItem(int i);
    void SetImage(int i, int image);
    std::function<void(int)> onCloseTab;      // UM_CLOSETAB
    std::function<void()> onSelChanging;      // TCN_SELCHANGING
    std::function<void()> onSelChange;        // TCN_SELCHANGE
    QList<QIcon> imageList;
    bool m_silent = false;                    // SetCurSel não gera TCN_SELCHANGE (como no Win32)

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
};

class MessagesDlg : public QWidget, public CWnd, public IMessageTarget {
public:
    MessagesDlg(QtMessageBackend* backend, QWidget* parent);
    ~MessagesDlg() override;

    LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

    bool GotoTab(int i);
    void SetWindowTitle(const MessagesContact& messagesContact);
    MessagesContact* AddTab(CString address, BOOL activate = FALSE, pjsua_call_info* call_info = NULL, call_user_data* user_data = NULL, BOOL notShowWindow = FALSE, BOOL ifExists = FALSE, CString numberOriginal = _T(""), CString name = _T(""));
    void OnChangeTab(pjsua_call_info* p_call_info = NULL, call_user_data* user_data = NULL, bool init = true);
    void OnEndCall(pjsua_call_info* call_info, call_user_data* user_data);
    bool CallCheck();
    pjsua_call_id CurrentCallId();
    void Call(BOOL hasVideo = FALSE);
    pjsua_call_id PerformCall(CString number, bool hasVideo = false, pj_status_t* pStatus = nullptr, call_user_data* user_data = nullptr);
    void AddMessage(MessagesContact* messagesContact, CString message, int type = MSIP_MESSAGE_TYPE_SYSTEM, BOOL blockForeground = FALSE, CTime* pTime = NULL);
    MessagesContact* GetMessageContact(int i = -1);
    MessagesContact* GetMessageContactInCall();
    int GetCallDuration(pjsua_call_id* call_id = NULL);
    int GetCallsCount(bool withIncoming = false, bool onlyActive = false);
    int GetCount() { return m_messagesTab->GetItemCount(); }
    BOOL SendInstantMessage(MessagesContact* messagesContact, CString message, CString number = _T(""));
    void UpdateHoldButton(pjsua_call_info* call_info);
    void UpdateRecButton(call_user_data* user_data = NULL);
    void UpdateTabIcon(MessagesContact* messagesContact, int tabIndex = -1, pjsua_call_info* call_info = NULL, call_user_data* user_data = NULL);
    bool CallAction(int action, CString number, pjsua_call_id forward_call_id = PJSUA_INVALID_ID);
    void Merge(pjsua_call_id call_id);
    void Separate(pjsua_call_id call_id);
    void UpdateCallButton(BOOL active = FALSE, pjsua_call_info* call_info = NULL, call_user_data* user_data = NULL);
    void TabIconUpdate(pjsua_call_id call_id);

    MessagesContact* lastCall = nullptr;
    QIcon m_hIconHold;
    QIcon m_hIconResume;
    CButtonEx* m_ButtonCall = nullptr;
    CButtonEx* m_ButtonEnd = nullptr;

    void OnClose();
    void OnCancel();
    void OnTcnSelchangeTab();
    void OnTcnSelchangingTab();
    void OnBnClickedCallEnd();
    void OnBnClickedVideoCall();
    void OnBnClickedTransfer();
    void OnBnClickedConference();
    void OnBnClickedActions(bool isConference = false);
    void OnTransfer();
    void OnAttendedTransfer();
    void OnAttendedTransferRange(pjsua_call_id call_id); // MySimpleSIP: ID do menu → call_id
    void OnConference();
    void OnMerge(pjsua_call_id call_id);                  // MySimpleSIP: ID do menu → call_id
    void OnBnClickedHold();
    void OnBnClickedEnd();
    void OnCloseAllTabs();
    void OnGoToLastTab();
    void OnBnClickedCloseAll();
    void OnBnClickedLastCall();
    void OnCopy();
    void OnSelectAll();
    void OnMergeAll();
    void OnSeparate();
    void OnSeparateAll();
    void OnDisconnect();

protected:
    void closeEvent(QCloseEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void OnInitDialog();
    BOOL CloseTab(int i, BOOL safe = FALSE);
    MessagesContact* StartCall(bool hasVideo = false, call_user_data* user_data = NULL);
    void OnEnMsgfilterMessage(); // Enter no campo de mensagem
    void ShowMessages(const CString& messages);

    QtMessageBackend* m_backend;
    CDialogLayout m_layout;
    CClosableTabCtrl* m_messagesTab = nullptr;
    QTextBrowser* m_list = nullptr;     // IDC_MESSAGES_LIST
    QPlainTextEdit* m_message = nullptr; // IDC_MESSAGE
    QPushButton* m_buttonHold = nullptr; // IDC_HOLD (BS_PUSHLIKE | BS_ICON)
};
