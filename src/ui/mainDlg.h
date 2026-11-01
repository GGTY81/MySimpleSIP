/*
 * MySimpleSIP — janela principal (port Qt do CmainDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// A parte pjsua do CmainDlg está em core/mainDlgCore (CmainDlgCore); aqui fica a janela:
// abas (Discador, Histórico, Contatos), botão de menu, barra de status, ícone da bandeja,
// menu principal e os handlers das mensagens UM_* que chegam pelo QtMessageBackend.
// Mesmos nomes de métodos do original (OnInitDialog, UpdateWindowText, MainPopupMenu,
// onTrayNotify, OnClose...). As páginas e diálogos ainda não portados aparecem como
// marcadores e serão substituídos nas próximas etapas da Fase 3.

#pragma once

#include "mainDlgCore.h"
#include "Contact.h"
#include "Transfer.h" // msip_action
#include "QtMessageBackend.h"

#include <QHash>
#include <QIcon>
#include <QList>
#include <QWidget>

class AccountDlg;
class Calls;
class Contacts;
class Dialer;
class MessagesDlg;
class RinginDlg;
class Transfer;
class SettingsDlg;
class ShortcutsDlg;
class ExportDlg;
class ImportDlg;
class Shortcut;
class QKeyEvent;
class QMessageBox;
class QLabel;
class QMenu;
class QStackedWidget;
class QSystemTrayIcon;
class QTabBar;
class QToolButton;

class CmainDlg : public QWidget, public CmainDlgCore, public IMessageTarget {
public:
    explicit CmainDlg(QtMessageBackend* backend, bool startMinimized);
    ~CmainDlg() override;

    // IMessageTarget: mensagens UM_* e WM_TIMER (equivale ao mapa de mensagens do MFC)
    LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

    // --- ganchos de CmainDlgCore implementados pela janela ---
    void UpdateWindowText(CString text = CString(), int icon = IDI_DEFAULT, bool afterRegister = false) override;
    void SetPaneText2(CString str = _T("")) override;
    void SetWindowText(const CString& title) override;
    void BaloonPopup(CString title, CString message, DWORD flags = NIIF_WARNING, UINT id = MSIP_BALOON_DEFAULT) override;
    void OnAccount(WPARAM wParam, LPARAM lParam) override;
    void InitUI() override;
    bool AskRetryNetwork() override;
    bool IsForegroundApp() override;
    void RestoreWindow() override;
    HWND DialerHwnd() override;
    void DialerSetName() override;

    // --- portados do CmainDlg ---
    void OnInitDialog();
    LRESULT OnCreated(WPARAM wParam, LPARAM lParam);
    void ShowTrayIcon();
    void TrayIconUpdateTip();
    void MainPopupMenu(bool isMenuButton = false);
    LRESULT onTrayNotify(WPARAM wParam, LPARAM lParam);
    void OnMenuAccountAdd();
    void OnMenuAccountEdit(int index);
    void OnMenuAccountLocalEdit();
    void OnMenuAccountChange(int index);
    void OnMenuAlwaysOnTop();
    void OnMenuLog();
    void OnMenuExit();
    void OnClose();
    void AccountSettingsPendingSave();
    void OnAccountChanged(bool init = false);
    LRESULT onRegState2(WPARAM wParam, LPARAM lParam);
    LRESULT OnUpdateWindowText(WPARAM wParam, LPARAM lParam);
    void TabFocusSet();
    void SwitchDND(int state = -1, bool update = false);
    bool GotoTab(int i);
    bool MakeCall(CString number, bool hasVideo = false, bool fromCommandLine = false, bool noTransform = false, CString name = _T(""));
    void Redial();
    bool MessagesOpen(CString number, bool forCall = false, bool noTransform = false, CString name = _T(""));
    pjsua_call_id CurrentCallId();
    void OpenTransferDlg(QWidget* pParent, msip_action action, pjsua_call_id call_id = PJSUA_INVALID_ID, Contact* selectedContact = NULL);
    LRESULT onCallTransferStatus(WPARAM wParam, LPARAM lParam);
    LRESULT onMWIInfo(WPARAM wParam, LPARAM lParam);
    void OnMenuSettings();
    LRESULT OnRecreate(WPARAM wParam, LPARAM lParam);
    bool CommandCallPickup(CString number);
    LRESULT onRefreshLevels(WPARAM wParam, LPARAM lParam);
    LRESULT onCallState(WPARAM wParam, LPARAM lParam);
    LRESULT onCallMediaState(WPARAM wParam, LPARAM lParam);
    LRESULT onIncomingCall(WPARAM wParam, LPARAM lParam);
    LRESULT onCreateRingingDlg(WPARAM wParam, LPARAM lParam);
    LRESULT onPlayerPlay(WPARAM wParam, LPARAM lParam);
    void onPlayerStop() override;
    bool AutoAnswer(pjsua_call_id call_id, bool force = false);
    CString GetNameForCall(SIPURI& sipuri, call_user_data* user_data, const CString& numberOriginal = L"") override;
    void OnTimer(UINT_PTR TimerVal);
    void OnTimerCall();
    BOOL CopyStringToClipboard(const CString& str);
    // presença/BLF e diretório de usuários
    void SubsribeNumber(CString number);
    void UnsubscribeNumber(CString number);
    void Subscribe() override;
    void Unsubscribe() override;
    LRESULT onBuddyState(WPARAM wParam, LPARAM lParam);
    void UsersDirectoryLoad(bool update = false) override;
    LRESULT onUsersDirectoryLoaded(WPARAM wParam, LPARAM lParam);
    // atalhos do discador
    void OnMenuShortcuts();
    void OnMenuExport();
    // mensagens SIP (MESSAGE)
    LRESULT onPager(WPARAM wParam, LPARAM lParam);
    LRESULT onPagerStatus(WPARAM wParam, LPARAM lParam);
    void MessagesIncoming(CString* number, CString* message, CTime* pTime = NULL);
    void OnMenuImport();
    void ShortcutAction(Shortcut* shortcut, bool block = false, bool second = false);
    void ShortcutsRemoveAll();
    LRESULT OnRestart(WPARAM wParam, LPARAM lParam);
    void OnMuteInput();
    void OnMuteOutput();

    AccountDlg* accountDlg = nullptr;
    MessagesDlg* messagesDlg = nullptr;
    Dialer* pageDialer = nullptr;
    Calls* pageCalls = nullptr;
    Contacts* pageContacts = nullptr;
    Transfer* transferDlg = nullptr;
    SettingsDlg* settingsDlg = nullptr;
    ShortcutsDlg* shortcutsDlg = nullptr;
    ExportDlg* exportDlg = nullptr;
    ImportDlg* importDlg = nullptr;
    bool newMessages = false;
    // estado dos atalhos na criação da janela (mudar posição/quantidade exige reiniciar)
    bool shortcutsEnabled = false;
    bool shortcutsBottom = false;
    int shortcutsCount = 0;
    int widthAdd = 0;
    int heightAdd = 0;
    QList<QIcon> imageListStatus; // mesma ordem de MSIP_CONTACT_ICON_*
    CArray<RinginDlg*> ringinDlgs;
    bool m_startMinimized = false;
    bool missed = false;

protected:
    void closeEvent(QCloseEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    // CBaseDialog::PreTranslateMessage: teclas de atalho da janela principal, da janela de
    // chamadas e das de chamada recebida (filtro instalado no QApplication)
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    bool PreTranslateKey(QWidget* window, QKeyEvent* event);
    QMenu* BuildMainMenu();
    void SetTrayIcon(const QIcon& icon, int state);

    QtMessageBackend* m_backend;
    QTabBar* m_mainTab = nullptr;
    QToolButton* m_ButtonMenu = nullptr;
    QStackedWidget* m_pages = nullptr;
    QLabel* m_barIcon = nullptr;
    QLabel* m_barText = nullptr;
    QLabel* m_barText2 = nullptr;
    QSystemTrayIcon* m_tray = nullptr;
    QIcon iconSmall;
    QIcon iconInactive;
    QIcon iconMissed;
    int m_trayState = -1; // 0 inativo, 1 normal, 2 perdida
    bool m_created = false;
    bool m_closing = false;
};

// MySimpleSIP: OnRestart pede o reinício; main.cpp relança o executável depois de liberar o
// servidor de instância única (no original, ShellExecute logo após OnClose)
extern bool msip_restart_requested;
