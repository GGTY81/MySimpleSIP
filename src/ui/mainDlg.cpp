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

// Port de trechos de mainDlg.cpp (MicroSIP 3.22.16): OnInitDialog, InitUI, ShowTrayIcon,
// OnCreated, TrayIconUpdateTip, BaloonPopup, OnMenuAccountChange, OnMenuAlwaysOnTop,
// OnMenuExit, onTrayNotify, MainPopupMenu, onRegState2, OnUpdateWindowText,
// UpdateWindowText, OnClose, OnMove, OnSize, AccountSettingsPendingSave, OnAccountChanged.
// Diferenças de plataforma marcadas com "MySimpleSIP:".

#include "mainDlg.h"

#include "AccountDlg.h"
#include "Calls.h"
#include "Contacts.h"
#include "ContactsService.h"
#include "Markup.h"
#include "Mp3Encode.h"
#include "PulseAudioDev.h"
#include "SystemMonitors.h"
#include "MediaKeys.h"
#include "Hid.h"
#include "Preview.h"
#include "VideoWindow.h"

#include <json/json.h>
#include <sstream>
#include <vector>
#include "CoreHost.h"
#include "Dialer.h"
#include "MessagesDlg.h"
#include "RinginDlg.h"
#include "Transfer.h"
#include "SettingsDlg.h"
#include "ShortcutsDlg.h"
#include "ExportDlg.h"
#include "ImportDlg.h"
#include "UiCommon.h"
#include "langpack.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QKeyEvent>
#include <QTemporaryFile>
#include <QTreeWidget>
#include <QCloseEvent>
#include <QFileInfo>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QScreen>
#include <QStackedWidget>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <arpa/inet.h>
#include <netdb.h>

#include <chrono>
#include <functional>

#define THIS_FILENAME "mainDlg.cpp"
#include <condition_variable>
#include <mutex>
#include <thread>

CmainDlg::CmainDlg(QtMessageBackend* backend, bool startMinimized)
    : QWidget(nullptr), m_backend(backend), m_startMinimized(startMinimized)
{
    statistics.createTime = CTime::GetCurrentTime().GetTime();
    mainDlg = this;
    m_hWnd = m_backend->Register(this);
    AfxGetApp()->m_pMainWnd = this;

    // lista de nomes amigáveis dos codecs, como no construtor original
    CString audioCodecsCaptions = _T("opus/48000/2;Opus 24 kHz;\
PCMA/8000/1;G.711 A-law;\
PCMU/8000/1;G.711 u-law;\
G722/16000/1;G.722 16 kHz;\
G7221/16000/1;G.722.1 16 kHz;\
G7221/32000/1;G.722.1 32 kHz;\
G723/8000/1;G.723 8 kHz;\
G729/8000/1;G.729 8 kHz;\
GSM/8000/1;GSM 8 kHz;\
GSM-EFR/8000/1;GSM-EFR 8 kHz;\
AMR/8000/1;AMR 8 kHz;\
AMR-WB/16000/1;AMR-WB 16 kHz;\
iLBC/8000/1;iLBC 8 kHz;\
speex/32000/1;Speex 32 kHz;\
speex/16000/1;Speex 16 kHz;\
speex/8000/1;Speex 8 kHz;\
SILK/24000/1;SILK 24 kHz;\
SILK/16000/1;SILK 16 kHz;\
SILK/12000/1;SILK 12 kHz;\
SILK/8000/1;SILK 8 kHz;\
L16/8000/1;LPCM 8 kHz;\
L16/8000/2;LPCM 8 kHz Stereo;\
L16/16000/1;LPCM 16 kHz;\
L16/16000/2;LPCM 16 kHz Stereo;\
L16/44100/1;LPCM 44 kHz;\
L16/44100/2;LPCM 44 kHz Stereo;\
L16/48000/1;LPCM 48 kHz;\
L16/48000/2;LPCM 48 kHz Stereo");
    int pos = 0;
    CString resToken = audioCodecsCaptions.Tokenize(_T(";"), pos);
    while (!resToken.IsEmpty()) {
        audioCodecList.AddTail(resToken);
        resToken = audioCodecsCaptions.Tokenize(_T(";"), pos);
    }

    OnInitDialog();
}

CmainDlg::~CmainDlg()
{
    qApp->removeEventFilter(this);
    msip_host.onEndCall = nullptr;
    m_backend->Unregister(m_hWnd);
    if (AfxGetApp()->m_pMainWnd == this)
        AfxGetApp()->m_pMainWnd = nullptr;
    if (mainDlg == this)
        mainDlg = nullptr;
}

void CmainDlg::OnInitDialog()
{
    messagesDlg = new MessagesDlg(m_backend, this);
    msip_host.onEndCall = [this](pjsua_call_info* call_info, call_user_data* user_data) {
        messagesDlg->OnEndCall(call_info, user_data);
    };

    iconSmall = LoadImageIcon(IDI_MAINFRAME);
    iconInactive = LoadImageIcon(IDI_INACTIVE);
    iconMissed = LoadImageIcon(IDI_MISSED);
    setWindowIcon(iconSmall);
    QApplication::setWindowIcon(iconSmall); // MySimpleSIP: demais janelas (ex.: vídeo)
    if (langPack.rtl)
        setLayoutDirection(Qt::RightToLeft);

    // --- layout do IDD_MAIN: aba (SysTabControl32) + botão de menu, páginas, status ---
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* top = new QHBoxLayout();
    top->setContentsMargins(0, DluY(3), DluX(3), 0);
    top->setSpacing(0);
    m_mainTab = new QTabBar(this);
    m_mainTab->setExpanding(false);
    m_mainTab->setDrawBase(true);
    // como m_mainTab.SetItemSize(_GLOBAL_TAB_WIDTH): abas de largura fixa
    m_mainTab->setUsesScrollButtons(false);
    m_mainTab->setElideMode(Qt::ElideRight);
    m_mainTab->setStyleSheet(QStringLiteral("QTabBar::tab { width: %1px; padding: 3px 0px; }").arg(DluX(_GLOBAL_TAB_WIDTH) - 4));
    top->addWidget(m_mainTab, 1);
    m_ButtonMenu = new QToolButton(this);
    // MySimpleSIP: glifo preto; no tema escuro, tingido (e refeito ao trocar o tema)
    m_ButtonMenu->setIcon(ThemedGlyphIcon(IDI_DROPDOWN));
    m_ButtonMenu->setAutoRaise(true);
    m_ButtonMenu->setFixedSize(DluX(16) + 4, DluY(11) + 4);
    m_ButtonMenu->setToolTip(TR(_T("Menu")));
    top->addWidget(m_ButtonMenu, 0, Qt::AlignVCenter);
    root->addLayout(top);

    // mainDlg->imageListStatus (presença dos atalhos do discador)
    for (int id : {IDI_BLANK, IDI_UNKNOWN, IDI_OFFLINE, IDI_AWAY, IDI_ONLINE, IDI_ON_THE_PHONE, IDI_BUSY, IDI_DEFAULT}) {
        imageListStatus.append(LoadImageIcon(id));
    }

    // OnCreate: atalhos e o espaço extra que eles ocupam no discador
    ShortcutsLoad();
    shortcutsEnabled = accountSettings.enableShortcuts;
    shortcutsBottom = accountSettings.shortcutsBottom;
    shortcutsCount = shortcuts.GetCount();
    if (accountSettings.enableShortcuts) {
        if (shortcutsBottom) {
            if (shortcutsCount) {
                if (
                    shortcuts.GetCount() > _GLOBAL_SHORTCUTS_QTY / 2 || shortcuts.GetCount() > 6
                    ) {
                    heightAdd += 10 + (shortcutsCount + shortcutsCount % 2) * 25 / 2;
                }
                else {
                    heightAdd += 10 + shortcutsCount * 25;
                }
            }
        }
        else {
            widthAdd += 180;
        }
    }

    m_pages = new QStackedWidget(this);
    pageDialer = new Dialer(m_backend, m_pages);
    m_mainTab->addTab(TR(_T("Phone")));
    m_pages->addWidget(pageDialer);
    m_mainTab->addTab(TR(_T("Logs")));
    pageCalls = new Calls(m_pages);
    pageCalls->OnCreated();
    m_pages->addWidget(pageCalls);
    m_mainTab->addTab(TR(_T("Contacts")));
    pageContacts = new Contacts(m_pages);
    pageContacts->OnCreated();
    m_pages->addWidget(pageContacts);
    root->addWidget(m_pages, 1);

    auto* bar = new QStatusBar(this);
    bar->setSizeGripEnabled(!accountSettings.noResize);
    m_barIcon = new QLabel(bar);
    m_barText = new QLabel(bar);
    m_barText2 = new QLabel(bar);
    bar->addWidget(m_barIcon);
    bar->addWidget(m_barText, 1);
    bar->addPermanentWidget(m_barText2);
    root->addWidget(bar);

    QObject::connect(m_mainTab, &QTabBar::currentChanged, this, [this](int i) {
        m_pages->setCurrentIndex(i);
        accountSettings.activeTab = i;
        AccountSettingsPendingSave();
        if (m_pages->currentWidget() == pageCalls && missed) {
            missed = false;
            UpdateWindowText();
        }
    });
    QObject::connect(m_ButtonMenu, &QToolButton::clicked, this, [this] { MainPopupMenu(true); });
    qApp->installEventFilter(this); // CBaseDialog::PreTranslateMessage

    // ON_WM_POWERBROADCAST: suspender encerra o pjsua; ao voltar, recria a sessão
    m_powerMonitor = new PowerMonitor(this);
    QObject::connect(m_powerMonitor, &PowerMonitor::aboutToSleep, this, [this] { OnPowerBroadcast(PBT_APMSUSPEND, 0); });
    QObject::connect(m_powerMonitor, &PowerMonitor::resumed, this, [this] { OnPowerBroadcast(PBT_APMRESUMEAUTOMATIC, 0); });
    // ON_WM_WTSSESSION_CHANGE (WTSRegisterSessionNotification)
    QObject::connect(m_powerMonitor, &PowerMonitor::sessionActivated, this, [this] { OnSessionChange(WTS_CONSOLE_CONNECT, 0); });
    // RegisterShellHookWindow (teclas de mídia) → player MPRIS; como no PreTranslateMessage,
    // os comandos só valem com enableMediaButtons
    m_mediaKeys = new MediaKeys(this);
    QObject::connect(m_mediaKeys, &MediaKeys::appCommand, this, [this](int nCmd) {
        if (accountSettings.enableMediaButtons) {
            onShellHookMessage(HSHELL_APPCOMMAND, (LPARAM)((DWORD)nCmd << 16)); // MAKELPARAM(0, nCmd)
        }
    });
    QObject::connect(m_mediaKeys, &MediaKeys::raiseRequested, this, [this] { onTrayNotify(0, 0); });
    m_mediaKeys->setEnabled(accountSettings.enableMediaButtons);

    // --- posição e tamanho (mesma regra do OnInitDialog original) ---
    // MySimpleSIP: no Wayland o compositor decide a posição; a regra vale no X11/XWayland.
    const QRect work = QGuiApplication::primaryScreen()->availableGeometry();
    QRect screenRect = work;
    if (accountSettings.multiMonitor) {
        CRect r;
        MSIP::GetScreenRect(&r);
        if (r.Width() > 0)
            screenRect = QRect(r.left, r.top, r.Width(), r.Height());
    }
    const int defW = DluX(_GLOBAL_WIDTH) + widthAdd;
    const int defH = DluY(_GLOBAL_HEIGHT + 31) + DluY(12) + heightAdd;
    int mW = accountSettings.mainW > 0 ? accountSettings.mainW : defW;
    int mH = accountSettings.mainH > 0 ? accountSettings.mainH : defH;
    int mx, my;
    if (!accountSettings.mainX && !accountSettings.mainY) {
        mx = work.right() - mW;
        my = work.bottom() - mH;
    }
    else {
        int maxLeft = screenRect.right() - mW;
        mx = accountSettings.mainX > maxLeft ? maxLeft : (accountSettings.mainX < screenRect.left() ? screenRect.left() : accountSettings.mainX);
        int maxTop = screenRect.bottom() - mH;
        my = accountSettings.mainY > maxTop ? maxTop : (accountSettings.mainY < screenRect.top() ? screenRect.top() : accountSettings.mainY);
    }
    //--set messages window pos/size
    int messagesX;
    int messagesY;
    int messagesW = accountSettings.messagesW > 0 ? accountSettings.messagesW : 550;
    int messagesH = accountSettings.messagesH > 0 ? accountSettings.messagesH : mH;
    // coors not specified, first run
    if (!accountSettings.messagesX && !accountSettings.messagesY) {
        accountSettings.messagesX = mx - messagesW;
        accountSettings.messagesY = my;
    }
    int maxLeft = screenRect.right() - messagesW;
    if (accountSettings.messagesX > maxLeft) {
        messagesX = maxLeft;
    }
    else {
        messagesX = accountSettings.messagesX < screenRect.left() ? screenRect.left() : accountSettings.messagesX;
    }
    int maxTop = screenRect.bottom() - messagesH;
    if (accountSettings.messagesY > maxTop) {
        messagesY = maxTop;
    }
    else {
        messagesY = accountSettings.messagesY < screenRect.top() ? screenRect.top() : accountSettings.messagesY;
    }
    messagesDlg->resize(messagesW, messagesH);
    messagesDlg->move(messagesX, messagesY);
    resize(mW, mH);
    move(mx, my);
    if (accountSettings.noResize)
        setFixedSize(mW, mH);
    setWindowFlag(Qt::WindowStaysOnTopHint, accountSettings.alwaysOnTop);

    m_mainTab->setCurrentIndex(accountSettings.activeTab >= 0 && accountSettings.activeTab < m_mainTab->count()
                                   ? accountSettings.activeTab : 0);

    // BOOL minimized = !lstrcmp(theApp.m_lpCmdLine, _T("/minimized"))
    if (msip_command_line == _T("/minimized")) {
        msip_command_line = _T("");
        m_startMinimized = true;
    }
    m_startMinimized = (!firstRun && m_startMinimized) || accountSettings.minimized;

    ShowTrayIcon();
    InitUI();
    OnAccountChanged(true);

    PostMessage(UM_CREATED);
}

void CmainDlg::InitUI()
{
    SetPaneText2();
    SetWindowText(_T(_GLOBAL_NAME_VISIBLE));
    UpdateWindowText();
    DialerSetName();
}

void CmainDlg::ShowTrayIcon()
{
    // MySimpleSIP: Shell_NotifyIcon → QSystemTrayIcon (StatusNotifierItem no GNOME com a
    // extensão AppIndicator do Ubuntu)
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;
    m_tray = new QSystemTrayIcon(this);
    SetTrayIcon(iconInactive, 0);
    QObject::connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick)
            onTrayNotify(1, 0); // WM_LBUTTONUP vindo da bandeja
    });
    // NIN_BALLOONUSERCLICK
    QObject::connect(m_tray, &QSystemTrayIcon::messageClicked, this, [this] {
        if (tndId == MSIP_BALOON_WEBSITE) {
            OnMenuWebsite();
        }
        else {
            onTrayNotify(0, 0);
        }
    });
    // o menu da bandeja é reconstruído a cada abertura, como o MainPopupMenu original
    auto* trayMenu = new QMenu(this);
    QObject::connect(trayMenu, &QMenu::aboutToShow, this, [this, trayMenu] {
        trayMenu->clear();
        // MySimpleSIP: no GNOME (AppIndicator) o clique esquerdo no ícone abre este menu em
        // vez de ativar a janela (WM_LBUTTONUP no Windows); este item faz esse papel.
        QAction* showAction = trayMenu->addAction(iconSmall, QStringLiteral(_GLOBAL_NAME_VISIBLE), this,
                                                  [this] { onTrayNotify(0, 0); });
        QFont bold = showAction->font();
        bold.setBold(true);
        showAction->setFont(bold);
        trayMenu->setDefaultAction(showAction);
        trayMenu->addSeparator();
        QMenu* m = BuildMainMenu();
        for (QAction* a : m->actions())
            trayMenu->addAction(a);
        m->setParent(trayMenu);
    });
    m_tray->setContextMenu(trayMenu);
    m_tray->show();
}

void CmainDlg::SetTrayIcon(const QIcon& icon, int state)
{
    if (m_tray && m_trayState != state) {
        m_tray->setIcon(icon);
        m_trayState = state;
    }
}

LRESULT CmainDlg::OnCreated(WPARAM, LPARAM)
{
    if (m_created) {
        statistics.createdTimeCancel = CTime::GetCurrentTime().GetTime();
        return 0;
    }
    m_created = true;
    statistics.createdTimeOk = CTime::GetCurrentTime().GetTime();

    if (!m_startMinimized || !m_tray) {
        show();
        TabFocusSet();
    }

    PJCreate();

    if (msip_command_line.GetLength()) {
        CString params = msip_command_line;
        msip_command_line.Empty();
        CommandLine(params);
    }
    PJAccountAdd();
    // MySimpleSIP: RegisterShellHookWindow (WM_SHELLHOOKMESSAGE) não tem equivalente no Linux
    return 0;
}

void CmainDlg::TrayIconUpdateTip()
{
    if (m_tray) {
        CString tip;
        tip = _T(_GLOBAL_NAME_VISIBLE);
        if (accountSettings.accountId) {
            if (!accountSettings.account.label.IsEmpty()) {
                tip.AppendFormat(_T("\n%s"), accountSettings.account.label);
            }
            else if (!accountSettings.account.username.IsEmpty()) {
                tip.AppendFormat(_T("\n%s"), accountSettings.account.username);
            }
            if (!accountSettings.account.displayName.IsEmpty()) {
                tip.AppendFormat(_T("\n%s"), accountSettings.account.displayName);
            }
        }
        m_tray->setToolTip(QS(tip));
    }
}

void CmainDlg::BaloonPopup(CString title, CString message, DWORD flags, UINT id)
{
    // MySimpleSIP: balão do Shell_NotifyIcon → notificação da bandeja
    tndId = id; // NIN_BALLOONUSERCLICK decide pelo último balão
    if (m_tray) {
        QSystemTrayIcon::MessageIcon icon = flags == NIIF_INFO ? QSystemTrayIcon::Information
            : flags == NIIF_ERROR                             ? QSystemTrayIcon::Critical
            : flags == NIIF_NONE                              ? QSystemTrayIcon::NoIcon
                                                              : QSystemTrayIcon::Warning;
        m_tray->showMessage(QS(title.IsEmpty() ? CString(_T(_GLOBAL_NAME_VISIBLE)) : title), QS(message), icon);
    }
}

// MySimpleSIP: no original os diálogos são filhos da janela principal; aqui são janelas
// independentes (Qt::Dialog sem pai), para não sumirem quando a principal se esconde.
static AccountDlg* OpenAccountDlg(CmainDlg* dlg)
{
    if (!dlg->accountDlg) {
        dlg->accountDlg = new AccountDlg();
    }
    dlg->accountDlg->show();
    dlg->accountDlg->raise();
    dlg->accountDlg->activateWindow(); // SetForegroundWindow
    return dlg->accountDlg;
}

void CmainDlg::OnAccount(WPARAM wParam, LPARAM lParam)
{
    (void)lParam;
    AccountDlg* accountDlg = OpenAccountDlg(this);
    accountDlg->Load(accountSettings.accountId ? accountSettings.accountId : -1);
    if (wParam) {
        accountDlg->FocusPassword();
    }
}

void CmainDlg::OnMenuAccountAdd()
{
    OpenAccountDlg(this)->Load(-1);
}

void CmainDlg::OnMenuAccountEdit(int index)
{
    int id = accountSettings.accountId > 0 ? accountSettings.accountId : index + 1;
    OpenAccountDlg(this)->Load(id ? id : -1);
}

void CmainDlg::OnMenuAccountLocalEdit()
{
    if (MACRO_ENABLE_LOCAL_ACCOUNT) {
        OpenAccountDlg(this)->Load(0);
    }
}

void CmainDlg::OnMenuAccountChange(int index)
{
    int idNew = index + 1;
    if (accountSettings.accountId != idNew) {
        if (accountSettings.accountId) {
            PJAccountDelete(true);
        }
        accountSettings.accountId = idNew;
        accountSettings.AccountLoad(accountSettings.accountId, &accountSettings.account);
    }
    else {
        if (accountSettings.accountId) {
            CString str;
            if (!accountSettings.account.server.IsEmpty()) {
                str = L"This will disconnect you from the PBX server. You will no longer be able to make or receive calls. Are you sure?";
            }
            else {
                str = L"You will no longer be able to make outgoing calls using this account. Are you sure?";
            }
            if (QMessageBox::question(this, QStringLiteral(_GLOBAL_NAME_NICE), TR(str), QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok) {
                return;
            }
            PJAccountDelete(true);
        }
        accountSettings.accountId = 0;
        InitUI();
    }
    OnAccountChanged();
    accountSettings.SettingsSave();
    PJAccountAdd();
}

void CmainDlg::OnMenuAlwaysOnTop()
{
    accountSettings.alwaysOnTop = 1 - accountSettings.alwaysOnTop;
    AccountSettingsPendingSave();
    // MySimpleSIP: SetWindowPos(wndTopMost) → flag do Qt (no Wayland depende do compositor)
    const bool visible = isVisible();
    setWindowFlag(Qt::WindowStaysOnTopHint, accountSettings.alwaysOnTop);
    if (visible)
        show();
}

void CmainDlg::OnMenuLog()
{
    MSIP::OpenFile(accountSettings.logFile);
}

void CmainDlg::OnMenuExit()
{
    hide();
    OnClose();
}

LRESULT CmainDlg::onTrayNotify(WPARAM wParam, LPARAM)
{
    statistics.trayNotifyTime = CTime::GetCurrentTime().GetTime();
    if (isVisible() && !isMinimized()) {
        if (wParam) {
            hide();
        }
        else {
            activateWindow();
            raise();
        }
    }
    else {
        if (isMinimized()) {
            showNormal();
        }
        else {
            show();
        }
        activateWindow();
        raise();
        if (missed) {
            m_mainTab->setCurrentIndex(1); // GotoTabLParam(pageCalls)
            missed = false;
            UpdateWindowText();
        }
        // -- show ringing dialogs
        int count = ringinDlgs.GetCount();
        for (int i = 0; i < count; i++) {
            RinginDlg* ringinDlg = ringinDlgs.GetAt(i);
            ringinDlg->showNormal();
        }
        // -- show messages dialog
        bool showMessagesDialog = (!accountSettings.singleMode && messagesDlg->GetCallsCount()) || newMessages;
        if (showMessagesDialog) {
            newMessages = false;
            messagesDlg->show();
        }
        //--
        TabFocusSet();
    }
    return TRUE;
}

void CmainDlg::RestoreWindow()
{
    onTrayNotify(0, 0);
}

bool CmainDlg::IsForegroundApp()
{
    return QApplication::activeWindow() != nullptr;
}

QMenu* CmainDlg::BuildMainMenu()
{
    auto* menu = new QMenu(this);
    CString str;
    Account acc;
    int i = 0;
    bool checked = false;
    QList<QAction*> accountActions;
    auto* editMenu = new QMenu(TR(_T("Edit Account")), menu);
    while (accountSettings.AccountLoad(i + 1, &acc)) {
        str = GetAccountLabel(&acc, i);
        QAction* a = new QAction(QS(str), menu);
        a->setCheckable(true);
        a->setChecked(accountSettings.accountId == i + 1);
        QObject::connect(a, &QAction::triggered, this, [this, i] { OnMenuAccountChange(i); });
        accountActions << a;
        editMenu->addAction(QS(str), this, [this, i] { OnMenuAccountEdit(i); });
        if (!checked) {
            checked = accountSettings.accountId == i + 1;
        }
        i++;
    }
    if (i == 1) {
        accountActions[0]->setText(TR(_T("Make Active")));
    }
    for (QAction* a : accountActions)
        menu->addAction(a);
    if (i == 1) {
        menu->addAction(TR(_T("Edit Account")) + QStringLiteral("\tCtrl+M"), this, [this] { OnMenuAccountEdit(0); });
    }
    else if (i > 1) {
        menu->addSeparator();
        if (checked)
            menu->addAction(TR(_T("Edit Account")) + QStringLiteral("\tCtrl+M"), this, [this] { OnMenuAccountEdit(0); });
        else
            menu->addMenu(editMenu);
    }
    menu->addAction(TR(_T("Add Account...")), this, [this] { OnMenuAccountAdd(); });
    if (accountSettings.enableLocalAccount && MACRO_ENABLE_LOCAL_ACCOUNT) {
        menu->addAction(TR(_T("Edit Local Account")) + QStringLiteral("\tCtrl+L"), this,
                        [this] { OnMenuAccountLocalEdit(); });
    }
    menu->addAction(TR(_T("Settings")) + QStringLiteral("\tCtrl+P"), this, [this] { OnMenuSettings(); });
    menu->addSeparator();
    menu->addAction(TR(_T("Shortcuts")) + QStringLiteral("\tCtrl+S"), this, [this] { OnMenuShortcuts(); });

    menu->addSeparator();
    QAction* top = menu->addAction(TR(_T("Always on Top")), this, [this] { OnMenuAlwaysOnTop(); });
    top->setCheckable(true);
    top->setChecked(accountSettings.alwaysOnTop);

    QMenu* importExportMenu = menu->addMenu(TR(_T("Export/Import")));
    for (LPCTSTR item : {_T("Export Configuration"), _T("Import Configuration"), _T(""), _T("Export Call Log"),
                         _T("Import Call Log"), _T(""), _T("Export Contacts"), _T("Import Contacts")}) {
        if (!*item)
            importExportMenu->addSeparator();
        else
            if (CString(item) == _T("Export Call Log")) {
                importExportMenu->addAction(TR(item), this, [this] { pageCalls->Export(); }); // OnMenuExportCalls
            }
            else if (CString(item) == _T("Import Call Log")) {
                importExportMenu->addAction(TR(item), this, [this] { pageCalls->OnMenuImport(); }); // OnMenuImportCalls
            }
            else if (CString(item) == _T("Export Contacts")) {
                importExportMenu->addAction(TR(item), this, [this] { pageContacts->Export(); }); // OnMenuExportContacts
            }
            else if (CString(item) == _T("Export Configuration")) {
                importExportMenu->addAction(TR(item), this, [this] { OnMenuExport(); });
            }
            else if (CString(item) == _T("Import Configuration")) {
                importExportMenu->addAction(TR(item), this, [this] { OnMenuImport(); });
            }
            else if (CString(item) == _T("Import Contacts")) {
                importExportMenu->addAction(TR(item), this, [this] { pageContacts->OnMenuImport(); }); // OnMenuImportContacts
            }
            else {
                importExportMenu->addAction(TR(item), this, [this, item] { NotYetPorted(this, TR(item)); });
            }
    }
    if (accountSettings.enableLog) {
        menu->addAction(TR(_T("View Log File")), this, [this] { OnMenuLog(); });
    }

    menu->addSeparator();
    menu->addAction(TR(_T("Visit Website")) + QStringLiteral("\tCtrl+W"), this, [this] { OnMenuWebsite(); });
    menu->addSeparator();
    menu->addAction(TR(_T("Help")) + QStringLiteral("\tv") + QStringLiteral(_GLOBAL_VERSION), this, [this] { OnMenuHelp(); });

    menu->addSeparator();
    menu->addAction(TR(_T("Exit")) + QStringLiteral("\tCtrl+Q"), this, [this] { OnMenuExit(); });
    return menu;
}

void CmainDlg::MainPopupMenu(bool isMenuButton)
{
    QMenu* menu = BuildMainMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);
    QPoint point = isMenuButton ? m_ButtonMenu->mapToGlobal(QPoint(0, m_ButtonMenu->height())) : QCursor::pos();
    menu->popup(point);
}

void CmainDlg::changeEvent(QEvent* event)
{
    // CThemeDialog::OnSettingChange("ImmersiveColorSet"): o Qt já troca a paleta; só o
    // glifo do menu precisa ser refeito
    if (event->type() == QEvent::PaletteChange && m_ButtonMenu) {
        m_ButtonMenu->setIcon(ThemedGlyphIcon(IDI_DROPDOWN));
    }
    QWidget::changeEvent(event);
}

void CmainDlg::contextMenuEvent(QContextMenuEvent* event)
{
    // como OnContextMenu: botão direito na faixa da barra de status abre o menu principal
    if (height() - event->pos().y() <= DluY(16)) {
        MainPopupMenu();
        return;
    }
    QWidget::contextMenuEvent(event);
}

LRESULT CmainDlg::onRegState2(WPARAM wParam, LPARAM lParam)
{
    int code = wParam;
    CString headerError;
    if (lParam) {
        CString* str = (CString*)lParam;
        headerError = *str;
        delete str;
    }

    if (code == 200) {
        Subscribe();
        if (accountSettings.usersDirectory.Find(_T("%s")) != -1 || accountSettings.usersDirectory.Find(_T("{")) != -1) {
            UsersDirectoryLoad();
        }
    }

    UpdateWindowText(headerError, IDI_DEFAULT, true);

    return 0;
}

LRESULT CmainDlg::OnUpdateWindowText(WPARAM wParam, LPARAM)
{
    if (wParam == 1) {
        bool show = !messagesDlg->GetCallsCount();
        if (show) {
            CString str;
            str.Format(_T("%s..."), Translate(_T("Connecting")));
            UpdateWindowText(str);
        }
    }
    else {
        UpdateWindowText(_T("-"));
    }
    return TRUE;
}

void CmainDlg::TabFocusSet()
{
    if (QWidget* page = m_pages->currentWidget())
        page->setFocus();
}

void CmainDlg::UpdateWindowText(CString text, int icon, bool afterRegister)
{
    if (text.IsEmpty() && is_pjsua_running() && messagesDlg->GetCallsCount()) {
        return;
    }
    CString str;
    bool showAccountDlg = false;
    bool noReg = false;
    bool isOffline = false;
    if (!is_pjsua_running()) {
        isOffline = true;
    }
    else if (text.IsEmpty() || text == _T("-")) {
        pjsua_acc_id acc_id = account;
        if (is_pjsua_running() && pjsua_acc_is_valid(acc_id)) {
            pjsua_acc_info info;
            pjsua_acc_get_info(acc_id, &info);
            str = MSIP::PjToStr(&info.status_text);
            if (str != _T("Default status message")) {
                if (!info.has_registration) {
                    icon = IDI_DEFAULT;
                    str = Translate(_T("Idle"));
                    noReg = true;
                }
                else if (str == _T("OK")) {
                    if (m_PresenceStatus == PJRPID_ACTIVITY_BUSY) {
                        icon = IDI_BUSY;
                        str = Translate(_T("Do Not Disturb"));
                    }
                    else {
                        if (m_PresenceStatus == PJRPID_ACTIVITY_AWAY) {
                            icon = IDI_AWAY;
                            str = Translate(_T("Away"));
                        }
                        else {
                            if (accountSettings.account.transport == _T("tls") && transport_tls != -1) {
                                icon = IDI_SECURE;
                            }
                            else {
                                icon = IDI_ONLINE;
                            }
                            str = Translate(_T("Online"));
                        }
                        if (accountSettings.forwarding == _T("button") && accountSettings.FWD) {
                            icon = IDI_FORWARDING;
                            str = Translate(_T("Call Forwarding"));
                        }
                        else {
                            if (!accountSettings.singleMode && accountSettings.AC) {
                                str.AppendFormat(_T(" (%s)"), Translate(_T("Auto Conference")));
                            }
                            else if (accountSettings.autoAnswer == _T("button") && accountSettings.AA) {
                                str.AppendFormat(_T(" (%s)"), Translate(_T("Auto Answer")));
                            }
                        }
                    }
                    if (!dialNumberDelayed.IsEmpty()) {
                        DialNumber(dialNumberDelayed);
                        dialNumberDelayed = _T("");
                    }
                }
                else if (str == _T("In Progress")) {
                    str.Format(_T("%s..."), Translate(_T("Connecting")));
                }
                else if (info.status == 401 || info.status == 403) {
                    icon = IDI_OFFLINE;
                    str = Translate(_T("Incorrect Password"));
                    if (afterRegister) {
                        if (isVisible() && !isMinimized()) {
                            showAccountDlg = true;
                        }
                        else {
                            BaloonPopup(_T(""), str);
                        }
                    }
                }
                else {
                    if (info.status == 502) {
                        str = _T("Connection Failed");
                        icon = IDI_OFFLINE;
                    }
                    str = Translate(str);
                }
            }
            else {
                str.Format(_T("%s %d"), Translate(_T("The server returned an error code:")), info.status);
            }
        }
        else {
            if (afterRegister) {
                showAccountDlg = true;
            }
            isOffline = true;
        }
    }
    else {
        str = text;
    }
    if (isOffline) {
        icon = IDI_DEFAULT;
        if (MACRO_ENABLE_LOCAL_ACCOUNT) {
            str = _T(_GLOBAL_NAME_VISIBLE);
        }
        else {
            str = Translate(_T("Offline"));
            icon = IDI_OFFLINE;
        }
    }

    m_barText->setText(QS(str));

    if (icon != -1) {
        m_barIcon->setPixmap(LoadImageIcon(icon).pixmap(16, 16));
        //--
        if ((is_pjsua_running() && !pjsua_acc_is_valid(account) && MACRO_ENABLE_LOCAL_ACCOUNT) || ((icon != IDI_DEFAULT || noReg) && icon != IDI_OFFLINE)) {
            if (missed) {
                SetTrayIcon(iconMissed, 2);
            }
            else {
                SetTrayIcon(iconSmall, 1);
            }
        }
        else {
            SetTrayIcon(iconInactive, 0);
        }
        //--
    }
    if (showAccountDlg) {
        PostMessage(UM_ON_ACCOUNT, 1);
    }
}

void CmainDlg::SetPaneText2(CString str)
{
    m_barText2->setText(QS(str));
}

void CmainDlg::SetWindowText(const CString& title)
{
    setWindowTitle(QS(title));
}

bool CmainDlg::AskRetryNetwork()
{
    QMessageBox box(QMessageBox::Critical, QStringLiteral(_GLOBAL_NAME_NICE),
                    TR(_T("Network connection is not available.")), QMessageBox::Retry | QMessageBox::Cancel, this);
    box.setInformativeText(TR(_T("Please check that your firewall or antivirus is not blocking this application.")));
    return box.exec() == QMessageBox::Retry;
}

void CmainDlg::AccountSettingsPendingSave()
{
    KillTimer(IDT_TIMER_SAVE);
    SetTimer(IDT_TIMER_SAVE, 5000, NULL);
}

void CmainDlg::OnAccountChanged(bool init)
{
    TrayIconUpdateTip();
    if (!init) {
        pageDialer->RebuildButtons();
    }
}

void CmainDlg::OnClose()
{
    if (m_closing)
        return;
    m_closing = true;
    statistics.closeTime = CTime::GetCurrentTime().GetTime();
    g_bClosingApp = true;

    PJDestroy(true);
    msip_wav_to_mp3_wait(); // MySimpleSIP: termina as conversões de gravação para MP3

    accountSettings.SettingsSave();

    if (m_tray) {
        m_tray->hide();
    }
    QApplication::quit();
}

void CmainDlg::closeEvent(QCloseEvent* event)
{
    // como OnSysCommand(SC_CLOSE): fechar a janela só esconde (o app continua na bandeja);
    // sem bandeja disponível, fechar encerra o aplicativo.
    if (m_closing || !m_tray) {
        event->accept();
        if (!m_closing)
            OnClose();
        return;
    }
    event->ignore();
    hide();
}

void CmainDlg::moveEvent(QMoveEvent* event)
{
    QWidget::moveEvent(event);
    if (isVisible() && !isMaximized() && !isMinimized()) {
        accountSettings.mainX = frameGeometry().left();
        accountSettings.mainY = frameGeometry().top();
        AccountSettingsPendingSave();
    }
}

void CmainDlg::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (isVisible() && !isMaximized() && !isMinimized()) {
        accountSettings.mainW = width();
        accountSettings.mainH = height();
        AccountSettingsPendingSave();
    }
}

HWND CmainDlg::DialerHwnd()
{
    return pageDialer ? pageDialer->m_hWnd : m_hWnd;
}

void CmainDlg::DialerSetName()
{
    if (pageDialer)
        pageDialer->SetName();
}

void CmainDlg::SwitchDND(int state, bool update)
{
    if (state == -1) {
        accountSettings.DND = !accountSettings.DND;
    }
    else {
        accountSettings.DND = state;
    }
    pageDialer->SetCheckDND(accountSettings.DND);
    AccountSettingsPendingSave();
    PublishStatus();
    if (update) {
        return;
    }
}

bool CmainDlg::GotoTab(int i)
{
    int nTab = m_mainTab->currentIndex();
    if (i < 0) {
        int max = m_mainTab->count() - 1;
        if (i == -1) {
            i = nTab < max ? nTab + 1 : 0;
        }
        else {
            i = nTab == 0 ? max : nTab - 1;
        }
    }
    if (nTab != i) {
        m_mainTab->setCurrentIndex(i);
        return true;
    }
    return false;
}

bool CmainDlg::MakeCall(CString number, bool hasVideo, bool fromCommandLine, bool noTransform, CString name)
{
    if (accountSettings.singleMode && messagesDlg->GetCallsCount()) {
        GotoTab(0);
        return false;
    }
    if (!pjsua_acc_is_valid(account) && !accountSettings.enableLocalAccount && MSIP::IsPSTNNnmber(number) && !MSIP::IsIP(number)) {
        Account dummy;
        bool found = accountSettings.AccountLoad(1, &dummy);
        if (found) {
            OnMenuAccountChange(0);
        }
        else {
            MSIP::ShowErrorMessage(this, PJSIP_EAUTHACCNOTFOUND);
            OnAccount(0, 0);
            return false;
        }
    }
    if (MessagesOpen(number, true, noTransform, name)) {
        MessagesContact* messagesContact = messagesDlg->GetMessageContact();
        messagesContact->fromCommandLine = fromCommandLine;
        messagesDlg->Call(hasVideo);
        return true;
    }
    return false;
}

void CmainDlg::Redial()
{
    if (!accountSettings.lastCallNumber.IsEmpty()) {
        MakeCall(accountSettings.lastCallNumber, accountSettings.lastCallHasVideo, false, true);
    }
}

bool CmainDlg::MessagesOpen(CString number, bool forCall, bool noTransform, CString name)
{
    CString commands;
    CString address = FormatNumber(number, &commands, noTransform);
    pj_status_t pj_status = msip_verify_sip_url(address);
    if (pj_status == PJ_SUCCESS) {
        bool doNotShowMessagesWindow = false;
        if (forCall) {
            doNotShowMessagesWindow = accountSettings.singleMode;
        }
        MessagesContact* messagesContact = messagesDlg->AddTab(address, TRUE, NULL, NULL, doNotShowMessagesWindow, FALSE, number, name);
        if (messagesContact) {
            messagesContact->commands = commands;
            return true;
        }
    }
    else {
        MSIP::ShowErrorMessage(this, pj_status);
    }
    return false;
}

bool CmainDlg::AutoAnswer(pjsua_call_id call_id, bool force)
{
    bool allow = false;
    if (accountSettings.autoAnswerCalls == _T("all")) {
        allow = true;
    }
    else if (accountSettings.autoAnswerCalls == _T("hold")) {
        allow = !messagesDlg->GetCallsCount(false, true);
    }
    else {
        allow = !messagesDlg->GetCallsCount();
    }
    if (allow) {
        bool play = false;
        if (!force) {
            if (accountSettings.localDTMF) {
                autoAnswerPlayCallId = call_id;
                onPlayerPlay(MSIP_SOUND_RINGIN2, 0);
                play = true;
            }
        }
        if (!play) {
            pjsua_call_info call_info;
            if (!is_pjsua_running() || pjsua_call_get_info(call_id, &call_info) != PJ_SUCCESS || (call_info.state != PJSIP_INV_STATE_INCOMING && call_info.state != PJSIP_INV_STATE_EARLY)) {
                return false;
            }
            call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(call_id);
            if (user_data) {
                user_data->CS.Lock();
                user_data->autoAnswer = true;
                user_data->CS.Unlock();
            }
            mainDlg->PostMessage(UM_CALL_ANSWER, (WPARAM)call_id, (LPARAM)call_info.rem_vid_cnt);
        }
    }
    return allow;
}

// Presença/BLF e diretório de usuários (port de mainDlg.cpp)
static int usersDirectorySequence;
static int usersDirectoryRefresh;
static int usersDirectorySilent;
static int usersDirectoryReconnect;

void CmainDlg::SubsribeNumber(CString number)
{
    if (!isSubscribed) {
        return;
    }
    if (!is_pjsua_running()) {
        return;
    }
    CString commands;
    CString numberFormated = FormatNumber(number, &commands, true);

    pjsua_buddy_id ids[PJSUA_MAX_BUDDIES];
    unsigned count = PJSUA_MAX_BUDDIES;
    pjsua_enum_buddies(ids, &count);
    for (unsigned i = 0; i < count; i++) {
        CString* buddyNumber = (CString*)pjsua_buddy_get_user_data(ids[i]);
        if (buddyNumber && *buddyNumber == numberFormated) {
            onBuddyState(ids[i], 0);
            return;
        }
    }
    CString numberFormatedPresence = numberFormated;
    pj_status_t status = msip_verify_sip_url(numberFormatedPresence);
    if (status == PJ_SUCCESS) {
        pjsua_acc_id acc_id;
        pj_str_t pj_uri;
        if (SelectSIPAccount(numberFormatedPresence, acc_id, &pj_uri)) {
            pjsua_buddy_id p_buddy_id;
            pjsua_buddy_config buddy_cfg;
            pjsua_buddy_config_default(&buddy_cfg);
            buddy_cfg.subscribe = PJ_TRUE;
            buddy_cfg.uri = pj_uri;
            buddy_cfg.user_data = (void*)(new CString(numberFormated));
            status = pjsua_buddy_add(&buddy_cfg, &p_buddy_id);
            free(pj_uri.ptr);
        }
    }
    if (status != PJ_SUCCESS) {
        CString str;
        str.Format(_T("%s\r\n%s"), Translate(_T("Presence Subscription")), number);
        CString descr;
        CString message = MSIP::GetErrorMessage(status, descr);
        BaloonPopup(str, message, NIIF_INFO);
    }
}

void CmainDlg::UnsubscribeNumber(CString number)
{
    if (!isSubscribed) {
        return;
    }
    if (!is_pjsua_running()) {
        return;
    }
    CString commands;
    CString numberFormated = FormatNumber(number, &commands, true);

    pjsua_buddy_id ids[PJSUA_MAX_BUDDIES];
    unsigned count = PJSUA_MAX_BUDDIES;
    pjsua_enum_buddies(ids, &count);
    for (unsigned i = 0; i < count; i++) {
        CString* buddyNumber = (CString*)pjsua_buddy_get_user_data(ids[i]);
        if (buddyNumber && *buddyNumber == numberFormated) {
            bool found1 = false;
            bool found2 = false;
            if (pageContacts->FindContact(numberFormated, true)) {
                found1 = true;
            }
            if (!found1) {
                // atalhos com presença (shortcuts, settings.h)
                for (int i = 0; i < shortcuts.GetCount(); i++) {
                    Shortcut* shortcut = &shortcuts.GetAt(i);
                    if (shortcut->presence) {
                        CString commands;
                        if (numberFormated == FormatNumber(shortcut->number, &commands, true)) {
                            found2 = true;
                            break;
                        }
                    }
                }
            }
            if (!found1 && !found2) {
                pjsua_buddy_del(ids[i]);
                delete buddyNumber;
            }
            return;
        }
    }
}

void CmainDlg::Subscribe()
{
    if (isSubscribed) {
        return;
    }
    isSubscribed = true;
    pageContacts->PresenceSubscribe();
    pageDialer->PresenceSubscribe();
}

void CmainDlg::Unsubscribe()
{
    if (!isSubscribed) {
        return;
    }
    if (is_pjsua_running()) {
        pjsua_buddy_id ids[PJSUA_MAX_BUDDIES];
        unsigned count = PJSUA_MAX_BUDDIES;
        pjsua_enum_buddies(ids, &count);
        for (unsigned i = 0; i < count; i++) {
            CString* buddyNumber = (CString*)pjsua_buddy_get_user_data(ids[i]);
            pjsua_buddy_del(ids[i]);
            if (buddyNumber) {
                delete buddyNumber;
            }
        }
    }
    pageContacts->PresenceReset();
    pageDialer->PresenceReset();
    isSubscribed = false;
}

LRESULT CmainDlg::onBuddyState(WPARAM wParam, LPARAM /*lParam*/)
{
    if (isSubscribed && is_pjsua_running()) {
        pjsua_buddy_id buddy_id = wParam;
        pjsua_buddy_info buddy_info;
        if (pjsua_buddy_is_valid(buddy_id) && pjsua_buddy_get_info(buddy_id, &buddy_info) == PJ_SUCCESS) {
            int image;
            bool ringing = false;
            CString info;
            switch (buddy_info.status)
            {
            case PJSUA_BUDDY_STATUS_OFFLINE:
                image = MSIP_CONTACT_ICON_OFFLINE;
                break;
            case PJSUA_BUDDY_STATUS_ONLINE:
                if (PJRPID_ACTIVITY_UNKNOWN && !buddy_info.rpid.activity) {
                    image = MSIP_CONTACT_ICON_ON_THE_PHONE;
                }
                else if (buddy_info.rpid.activity == PJRPID_ACTIVITY_AWAY)
                {
                    image = MSIP_CONTACT_ICON_AWAY;
                }
                else if (buddy_info.rpid.activity == PJRPID_ACTIVITY_BUSY) {
                    image = MSIP_CONTACT_ICON_BUSY;
                }
                else {
                    image = MSIP_CONTACT_ICON_ONLINE;
                }
                break;
            default:
                image = MSIP_CONTACT_ICON_UNKNOWN;
            }
            info = MSIP::PjToStr(&buddy_info.status_text);
            if (buddy_info.status == PJSUA_BUDDY_STATUS_ONLINE) {
                if (info == _T("On the phone")) {
                    image = MSIP_CONTACT_ICON_ON_THE_PHONE;
                }
                else if (MSIP::PjToStr(&buddy_info.status_text).Left(4) == _T("Ring")) {
                    image = MSIP_CONTACT_ICON_ON_THE_PHONE;
                    ringing = true;
                }
            }
            CString* buddyNumber = (CString*)pjsua_buddy_get_user_data(buddy_id);
            if (buddyNumber) {
                //--
                pageContacts->PresenceReceived(buddyNumber, image, ringing, &info);
                pageDialer->PresenceReceived(buddyNumber, image, ringing);
            }
        }
    }
    return 0;
}

LRESULT CmainDlg::onUsersDirectoryLoaded(WPARAM wParam, LPARAM /*lParam*/)
{
    CString message;
    //PJ_LOG(3, (THIS_FILENAME, "Users directory loaded"));
    URLGetAsyncData* response = (URLGetAsyncData*)wParam;
    if (response->statusCode == 0) {
        if (usersDirectorySequence == 1) {
            message = Translate(_T("Connection Failed"));
        }
        usersDirectoryReconnect++;
    }
    else {
        usersDirectoryReconnect = 0;
        if (response->statusCode >= 300) {
            if (usersDirectorySequence == 1) {
                message.Format(_T("%s %d"), Translate(_T("The server returned an error code:")), response->statusCode);
            }
        }
        else if (response->statusCode == 200 && !response->body.IsEmpty()) {
            CArray<ContactWithFields*> contacts;
            ContactWithFields* contactWithFields;
            CList<Prensence> prensences;
            BOOL ok = FALSE;
            if (response->headers.Find(_T("Content-Type: text/csv")) != -1) {
                // MySimpleSIP: GetTempPath/GetTempFileName/CFile/DeleteFile → QTemporaryFile
                QTemporaryFile tmp(QDir::tempPath() + QStringLiteral("/csvXXXXXX"));
                if (tmp.open()) {
                    tmp.write(response->body.GetString(), response->body.GetLength());
                    tmp.close();
                    if (pageContacts->Import(CS(tmp.fileName()), contacts)) {
                        ok = TRUE;
                    }
                }
            }
            else {
                // JSON
                Json::Value root;
                Json::Value items;
                Json::Value presence;
                Json::Reader reader;
                bool parsedSuccess = reader.parse((LPCSTR)response->body,
                    root,
                    false);
                if (parsedSuccess) {
                    //PJ_LOG(3, (THIS_FILENAME, "JSON foramt detected"));
                    try {
                        if (root.isObject()) {
                            if (root.isMember("refresh") && root["refresh"].isInt()) {
                                usersDirectoryRefresh = root["refresh"].asInt();
                            }
                            if (root.isMember("silent") && root["silent"].isInt()) {
                                usersDirectorySilent = root["silent"].asInt();
                            }
                            if (root.isMember("items") && root["items"].isArray()) {
                                items = root["items"];
                                ok = true;
                            }
                            if (root.isMember("presence") && root["presence"].isArray()) {
                                presence = root["presence"];
                            }
                        }
                        else if (root.isArray()) {
                            items = root;
                            ok = true;
                        }
                        if (items.isArray()) {
                            for (Json::Value::ArrayIndex i = 0; i != items.size(); i++) {
                                contactWithFields = new ContactWithFields();
                                contactWithFields->contact.directory = true;
                                if (items[i]["name"].isString()) {
                                    contactWithFields->fields.AddTail(_T("name"));
                                    contactWithFields->contact.name = MSIP::Utf8DecodeUni(items[i]["name"].asCString());
                                }
                                if (items[i]["number"].isString()) {
                                    contactWithFields->fields.AddTail(_T("number"));
                                    contactWithFields->contact.number = MSIP::Utf8DecodeUni(items[i]["number"].asCString());
                                }
                                else if (items[i]["phone"].isString()) {
                                    contactWithFields->fields.AddTail(_T("number"));
                                    contactWithFields->contact.number = MSIP::Utf8DecodeUni(items[i]["phone"].asCString());
                                }
                                else if (items[i]["telephone"].isString()) {
                                    contactWithFields->fields.AddTail(_T("number"));
                                    contactWithFields->contact.number = MSIP::Utf8DecodeUni(items[i]["telephone"].asCString());
                                }
                                if (items[i]["firstname"].isString()) {
                                    contactWithFields->fields.AddTail(_T("firstname"));
                                    contactWithFields->contact.firstname = MSIP::Utf8DecodeUni(items[i]["firstname"].asCString());
                                }
                                if (items[i]["lastname"].isString()) {
                                    contactWithFields->fields.AddTail(_T("lastname"));
                                    contactWithFields->contact.lastname = MSIP::Utf8DecodeUni(items[i]["lastname"].asCString());
                                }
                                if (items[i]["phone"].isString()) {
                                    contactWithFields->fields.AddTail(_T("phone"));
                                    contactWithFields->contact.phone = MSIP::Utf8DecodeUni(items[i]["phone"].asCString());
                                }
                                if (items[i]["mobile"].isString()) {
                                    contactWithFields->fields.AddTail(_T("mobile"));
                                    contactWithFields->contact.mobile = MSIP::Utf8DecodeUni(items[i]["mobile"].asCString());
                                }
                                if (items[i]["email"].isString()) {
                                    contactWithFields->fields.AddTail(_T("email"));
                                    contactWithFields->contact.email = MSIP::Utf8DecodeUni(items[i]["email"].asCString());
                                }
                                if (items[i]["address"].isString()) {
                                    contactWithFields->fields.AddTail(_T("address"));
                                    contactWithFields->contact.address = MSIP::Utf8DecodeUni(items[i]["address"].asCString());
                                }
                                if (items[i]["city"].isString()) {
                                    contactWithFields->fields.AddTail(_T("city"));
                                    contactWithFields->contact.city = MSIP::Utf8DecodeUni(items[i]["city"].asCString());
                                }
                                if (items[i]["state"].isString()) {
                                    contactWithFields->fields.AddTail(_T("state"));
                                    contactWithFields->contact.state = MSIP::Utf8DecodeUni(items[i]["state"].asCString());
                                }
                                if (items[i]["zip"].isString()) {
                                    contactWithFields->fields.AddTail(_T("zip"));
                                    contactWithFields->contact.zip = MSIP::Utf8DecodeUni(items[i]["zip"].asCString());
                                }
                                if (items[i]["comment"].isString()) {
                                    contactWithFields->fields.AddTail(_T("comment"));
                                    contactWithFields->contact.comment = MSIP::Utf8DecodeUni(items[i]["comment"].asCString());
                                }
                                if (items[i]["id"].isString()) {
                                    contactWithFields->fields.AddTail(_T("id"));
                                    contactWithFields->contact.id = MSIP::Utf8DecodeUni(items[i]["id"].asCString());
                                }
                                if (items[i]["info"].isString()) {
                                    contactWithFields->fields.AddTail(_T("info"));
                                    contactWithFields->contact.info = MSIP::Utf8DecodeUni(items[i]["info"].asCString());
                                }
                                if (items[i]["presence"].isInt()) {
                                    contactWithFields->fields.AddTail(_T("presence"));
                                    contactWithFields->contact.presence = items[i]["presence"].asInt() ? 1 : 0;
                                }
                                if (items[i]["starred"].isInt()) {
                                    contactWithFields->fields.AddTail(_T("starred"));
                                    contactWithFields->contact.starred = items[i]["starred"].asInt() ? 1 : 0;
                                }
                                if (pageContacts->GetContactsService()->ContactPrepare(contactWithFields->contact)) {
                                    contacts.Add(contactWithFields);
                                }
                                else {
                                    delete contactWithFields;
                                }
                            }
                        }
                        if (presence.isArray()) {
                            for (Json::Value::ArrayIndex i = 0; i != presence.size(); i++) {
                                if (presence[i]["number"].isString() && presence[i]["status"].isString()) {
                                    CString number = MSIP::Utf8DecodeUni(presence[i]["number"].asCString());
                                    CString status = MSIP::Utf8DecodeUni(presence[i]["status"].asCString());
                                    CString info;
                                    if (presence[i]["info"].isString()) {
                                        info = MSIP::Utf8DecodeUni(presence[i]["info"].asCString());
                                    }
                                    int image;
                                    bool ringing = false;
                                    if (status == _T("offline")) {
                                        image = MSIP_CONTACT_ICON_OFFLINE;
                                    }
                                    else if (status == _T("online")) {
                                        image = MSIP_CONTACT_ICON_ONLINE;
                                    }
                                    else if (status == _T("away")) {
                                        image = MSIP_CONTACT_ICON_AWAY;
                                    }
                                    else if (status == _T("busy")) {
                                        image = MSIP_CONTACT_ICON_BUSY;
                                    }
                                    else if (status == _T("ring")) {
                                        image = MSIP_CONTACT_ICON_ON_THE_PHONE;
                                        ringing = true;
                                    }
                                    else if (status == _T("phone")) {
                                        image = MSIP_CONTACT_ICON_ON_THE_PHONE;
                                    }
                                    else {
                                        image = MSIP_CONTACT_ICON_UNKNOWN;
                                    }
                                    Prensence prensence;
                                    prensence.number = number;
                                    prensence.image = image;
                                    prensence.ringing = ringing;
                                    prensence.info = info;
                                    prensences.AddTail(prensence);
                                }
                            }
                        }
                    }
                    catch (std::exception const&) {
                    }
                }
                else {
                    // XML
                    //PJ_LOG(3, (THIS_FILENAME, "XML foramt detected"));
                    CMarkup xml;
                    BOOL bResult = xml.SetDoc(MSIP::Utf8DecodeUni(response->body));
                    if (bResult) {
                        ok = true;
                        if (xml.FindElem(_T("contacts"))) {
                            if (xml.FindAttrib(_T("refresh"))) {
                                usersDirectoryRefresh = _wtoi(xml.GetAttrib(_T("refresh")));
                            }
                            if (xml.FindAttrib(_T("silent"))) {
                                usersDirectorySilent = _wtoi(xml.GetAttrib(_T("silent")));
                            }
                            while (xml.FindChildElem(_T("contact"))) {
                                xml.IntoElem();
                                contactWithFields = new ContactWithFields();
                                contactWithFields->contact.directory = true;
                                if (xml.FindAttrib(_T("name"))) {
                                    contactWithFields->fields.AddTail(_T("name"));
                                    contactWithFields->contact.name = xml.GetAttrib(_T("name"));
                                }
                                if (xml.FindAttrib(_T("number"))) {
                                    contactWithFields->fields.AddTail(_T("number"));
                                    contactWithFields->contact.number = xml.GetAttrib(_T("number"));
                                }
                                if (xml.FindAttrib(_T("firstname"))) {
                                    contactWithFields->fields.AddTail(_T("firstname"));
                                    contactWithFields->contact.firstname = xml.GetAttrib(_T("firstname"));
                                }
                                if (xml.FindAttrib(_T("lastname"))) {
                                    contactWithFields->fields.AddTail(_T("lastname"));
                                    contactWithFields->contact.lastname = xml.GetAttrib(_T("lastname"));
                                }
                                if (xml.FindAttrib(_T("phone"))) {
                                    contactWithFields->fields.AddTail(_T("phone"));
                                    contactWithFields->contact.phone = xml.GetAttrib(_T("phone"));
                                }
                                if (xml.FindAttrib(_T("mobile"))) {
                                    contactWithFields->fields.AddTail(_T("mobile"));
                                    contactWithFields->contact.mobile = xml.GetAttrib(_T("mobile"));
                                }
                                if (xml.FindAttrib(_T("email"))) {
                                    contactWithFields->fields.AddTail(_T("email"));
                                    contactWithFields->contact.email = xml.GetAttrib(_T("email"));
                                }
                                if (xml.FindAttrib(_T("address"))) {
                                    contactWithFields->fields.AddTail(_T("address"));
                                    contactWithFields->contact.address = xml.GetAttrib(_T("address"));
                                }
                                if (xml.FindAttrib(_T("city"))) {
                                    contactWithFields->fields.AddTail(_T("city"));
                                    contactWithFields->contact.city = xml.GetAttrib(_T("city"));
                                }
                                if (xml.FindAttrib(_T("state"))) {
                                    contactWithFields->fields.AddTail(_T("state"));
                                    contactWithFields->contact.state = xml.GetAttrib(_T("state"));
                                }
                                if (xml.FindAttrib(_T("zip"))) {
                                    contactWithFields->fields.AddTail(_T("zip"));
                                    contactWithFields->contact.zip = xml.GetAttrib(_T("zip"));
                                }
                                if (xml.FindAttrib(_T("comment"))) {
                                    contactWithFields->fields.AddTail(_T("comment"));
                                    contactWithFields->contact.comment = xml.GetAttrib(_T("comment"));
                                }
                                if (xml.FindAttrib(_T("id"))) {
                                    contactWithFields->fields.AddTail(_T("id"));
                                    contactWithFields->contact.id = xml.GetAttrib(_T("id"));
                                }
                                if (xml.FindAttrib(_T("info"))) {
                                    contactWithFields->fields.AddTail(_T("info"));
                                    contactWithFields->contact.info = xml.GetAttrib(_T("info"));
                                }
                                if (xml.FindAttrib(_T("presence"))) {
                                    contactWithFields->fields.AddTail(_T("presence"));
                                    CString rab = xml.GetAttrib(_T("presence"));
                                    contactWithFields->contact.presence = rab == _T("1");
                                }
                                if (xml.FindAttrib(_T("starred"))) {
                                    contactWithFields->fields.AddTail(_T("starred"));
                                    CString rab = xml.GetAttrib(_T("starred"));
                                    contactWithFields->contact.starred = rab == _T("1");
                                }
                                if (pageContacts->GetContactsService()->ContactPrepare(contactWithFields->contact)) {
                                    contacts.Add(contactWithFields);
                                }
                                else {
                                    delete contactWithFields;
                                }
                                xml.OutOfElem();
                            }
                        }
                        else if (xml.FindElem(_T("YealinkIPPhoneBook"))) {
                            while (xml.FindChildElem(_T("Menu"))) {
                                xml.IntoElem();
                                while (xml.FindChildElem(_T("Unit"))) {
                                    xml.IntoElem();
                                    contactWithFields = new ContactWithFields();
                                    contactWithFields->contact.directory = true;
                                    if (xml.FindAttrib(_T("Name"))) {
                                        contactWithFields->fields.AddTail(_T("name"));
                                        contactWithFields->contact.name = xml.GetAttrib(_T("Name"));
                                    }
                                    if (xml.FindAttrib(_T("Phone1"))) {
                                        contactWithFields->fields.AddTail(_T("number"));
                                        contactWithFields->contact.number = xml.GetAttrib(_T("Phone1"));
                                    }
                                    if (xml.FindAttrib(_T("Phone2"))) {
                                        contactWithFields->fields.AddTail(_T("phone"));
                                        contactWithFields->contact.phone = xml.GetAttrib(_T("Phone2"));
                                    }
                                    if (xml.FindAttrib(_T("Phone3"))) {
                                        contactWithFields->fields.AddTail(_T("mobile"));
                                        contactWithFields->contact.mobile = xml.GetAttrib(_T("Phone3"));
                                    }
                                    if (pageContacts->GetContactsService()->ContactPrepare(contactWithFields->contact)) {
                                        contacts.Add(contactWithFields);
                                    }
                                    else {
                                        delete contactWithFields;
                                    }
                                    xml.OutOfElem();
                                }
                                xml.OutOfElem();
                            }
                        }
                        else {
                            while (xml.FindChildElem(_T("entry")) || xml.FindChildElem(_T("DirectoryEntry"))) {
                                xml.IntoElem();
                                contactWithFields = new ContactWithFields();
                                contactWithFields->contact.directory = true;
                                if (xml.FindChildElem(_T("extension")) || xml.FindChildElem(_T("Telephone"))) {
                                    contactWithFields->fields.AddTail(_T("number"));
                                    contactWithFields->contact.number = xml.GetChildData();
                                    if (xml.FindChildElem(_T("extension")) || xml.FindChildElem(_T("Telephone"))) {
                                        contactWithFields->fields.AddTail(_T("phone"));
                                        contactWithFields->contact.phone = xml.GetChildData();
                                    }
                                    if (xml.FindChildElem(_T("extension")) || xml.FindChildElem(_T("Telephone"))) {
                                        contactWithFields->fields.AddTail(_T("mobile"));
                                        contactWithFields->contact.mobile = xml.GetChildData();
                                    }
                                    xml.ResetChildPos();
                                }
                                if (xml.FindChildElem(_T("name")) || xml.FindChildElem(_T("Name"))) {
                                    contactWithFields->fields.AddTail(_T("name"));
                                    contactWithFields->contact.name = xml.GetChildData();
                                    xml.ResetChildPos();
                                }
                                if (xml.FindChildElem(_T("info"))) {
                                    contactWithFields->fields.AddTail(_T("info"));
                                    contactWithFields->contact.info = xml.GetChildData();
                                    xml.ResetChildPos();
                                }
                                if (xml.FindChildElem(_T("presence"))) {
                                    contactWithFields->fields.AddTail(_T("presence"));
                                    contactWithFields->contact.presence = xml.GetChildData() == _T("1");
                                    xml.ResetChildPos();
                                }
                                if (xml.FindChildElem(_T("starred"))) {
                                    contactWithFields->fields.AddTail(_T("starred"));
                                    contactWithFields->contact.starred = xml.GetChildData() == _T("1");
                                    xml.ResetChildPos();
                                }
                                if (pageContacts->GetContactsService()->ContactPrepare(contactWithFields->contact)) {
                                    contacts.Add(contactWithFields);
                                }
                                else {
                                    delete contactWithFields;
                                }
                                xml.OutOfElem();
                            }
                        }
                    }
                }
            }
            bool sort = false;
            if (ok) {
                pageContacts->ContactsAdd(&contacts, true);
                for (int i = 0; i < contacts.GetCount(); i++) {
                    contactWithFields = contacts.GetAt(i);
                    delete contactWithFields;
                }
                usersDirectoryLoaded = true;
                sort = true;
            }

            POSITION pos = prensences.GetHeadPosition();
            if (pos) {
                while (pos) {
                    Prensence prensence = prensences.GetNext(pos);
                    pageContacts->PresenceReceived(&prensence.number, prensence.image, prensence.ringing, &prensence.info, true);
                    pageDialer->PresenceReceived(&prensence.number, prensence.image, prensence.ringing, true);
                };
                if (!sort && pageContacts->List()->header()->sortIndicatorSection() == 2) {
                    sort = true;
                }
            }
            if (sort) {
                pageContacts->SortList();
            }
            if (usersDirectoryRefresh == -1) {
                response->headers.MakeLower();
                CString search = _T("\r\ncache-control:");
                int n = response->headers.Find(search);
                if (n > 0) {
                    n = n + search.GetLength();
                    int l = response->headers.Find(_T("\r\n"), n);
                    if (l > 0) {
                        response->headers = response->headers.Mid(n, l - n);
                        search = _T("max-age=");
                        n = response->headers.Find(search);
                        if (n != -1) {
                            response->headers = response->headers.Mid(n + search.GetLength());
                            usersDirectoryRefresh = atoi(CStringA(response->headers));
                        }
                    }
                }
                if (usersDirectoryRefresh > 0) {
                    if (usersDirectoryRefresh < 60) {
                        usersDirectoryRefresh = 60;
                    }
                    if (usersDirectoryRefresh > 86400) {
                        usersDirectoryRefresh = 86400;
                    }
                }
            }
            if (usersDirectorySequence == 1) {
                if (!ok) {
                    message = Translate(_T("The received data cannot be recognized"));
                }
            }
        }
    }
    if (usersDirectoryReconnect) {
        SetTimer(IDT_TIMER_DIRECTORY, 1000 * 10 * usersDirectoryReconnect * usersDirectoryReconnect * usersDirectoryReconnect, NULL);
    }
    else {
        if (usersDirectoryRefresh > 0) {
            SetTimer(IDT_TIMER_DIRECTORY, 1000 * usersDirectoryRefresh, NULL);
        }
    }

    //PJ_LOG(3, (THIS_FILENAME, "End UsersDirectoryLoad"));
    delete response;

    // MySimpleSIP: "if (message && ...)" é sempre verdadeiro no MFC, mas o balão do
    // Windows com texto vazio não aparece; aqui a notificação vazia apareceria
    if (!message.IsEmpty() && !usersDirectorySilent) {
        BaloonPopup(Translate(_T("Directory of Users")), message, NIIF_INFO);
    }

    return 0;
}

void CmainDlg::UsersDirectoryLoad(bool update)
{
    KillTimer(IDT_TIMER_DIRECTORY);
    if (!update) {
        usersDirectorySequence = 0;
        usersDirectoryRefresh = -1;
        usersDirectorySilent = 0;
        usersDirectoryReconnect = 0;
    }
    if (!accountSettings.usersDirectory.IsEmpty()) {
        //PJ_LOG(3, (THIS_FILENAME, "Users directory load"));
        CString postData, headers;
        CString url = accountSettings.usersDirectory;
        url.Replace(_T("%"), _T("*"));
        url.Replace(_T("*s"), _T("%s"));
        url.Format(url, accountSettings.account.username, accountSettings.account.password, get_account_server());
        url.Replace(_T("*"), _T("%"));
        url = msip_url_mask(url);
        url.AppendFormat(_T("%ssequence=%d"), url.Find('?') == -1 ? _T("?") : _T("&"), usersDirectorySequence);
        usersDirectorySequence++;
        //PJ_LOG(3, (THIS_FILENAME, "Begin UsersDirectoryLoad"));
        URLGetAsync(url, m_hWnd, UM_USERS_DIRECTORY
            , false
        );
    }
}

BOOL CmainDlg::CopyStringToClipboard(const CString& str)
{
    // MySimpleSIP: OpenClipboard/SetClipboardData(CF_UNICODETEXT) → QClipboard
    QGuiApplication::clipboard()->setText(QS(str));
    return TRUE;
}

CString CmainDlg::GetNameForCall(SIPURI& sipuri, call_user_data* user_data, const CString& numberOriginal)
{
    CString name;
    if (user_data) {
        user_data->CS.Lock();
        if (!user_data->name.IsEmpty()) {
            name = user_data->name;
        }
        user_data->CS.Unlock();
    }
    if (name.IsEmpty()) {
        if (!accountSettings.disableNameLookup) {
            name = pageContacts->GetNameByNumber(!sipuri.user.IsEmpty() ? sipuri.user : sipuri.domain);
        }
        if (name.IsEmpty()) {
            if (!sipuri.name.IsEmpty()) {
                name = sipuri.name;
            }
        }
        if (name.IsEmpty() && !numberOriginal.IsEmpty()) {
            name = numberOriginal;
        }
        if (name.IsEmpty()) {
            if (!sipuri.user.IsEmpty()) {
                name = sipuri.user;
            }
            else if (!sipuri.domain.IsEmpty()) {
                name = sipuri.domain;
            }
        }
        if (user_data) {
            user_data->CS.Lock();
            user_data->name = name;
            user_data->CS.Unlock();
        }
    }
    return name;
}

pjsua_call_id CmainDlg::CurrentCallId()
{
    MessagesContact* messagesContact = messagesDlg->GetMessageContact();
    if (messagesContact) {
        return messagesContact->callId;
    }
    return -1;
}

void CmainDlg::OpenTransferDlg(QWidget* pParent, msip_action action, pjsua_call_id call_id, Contact* selectedContact)
{
    if (mainDlg && transferDlg) {
        transferDlg->OnClose();
    }
    transferDlg = new Transfer(pParent);
    transferDlg->SetAction(action, call_id);
    transferDlg->LoadFromContacts(selectedContact);
    // SetForegroundWindow
    transferDlg->raise();
    transferDlg->activateWindow();
}

LRESULT CmainDlg::onCallTransferStatus(WPARAM wParam, LPARAM lParam)
{
    pjsua_call_info* call_info = (pjsua_call_info*)wParam;
    call_user_data* user_data = (call_user_data*)call_info->call_id.ptr;
    CString* str = (CString*)lParam;


    MessagesContact* messagesContact = NULL;
    CString number = MSIP::PjToStr(&call_info->remote_info, TRUE);
    messagesContact = messagesDlg->AddTab(number, FALSE, call_info, user_data, TRUE, TRUE);
    if (messagesContact) {
        messagesDlg->AddMessage(messagesContact, *str);
    }
    if (call_info->last_status / 100 == 2) {
        if (messagesContact) {
            messagesDlg->AddMessage(messagesContact, Translate(_T("Call transfered successfully, disconnecting call")));
        }
        msip_call_hangup_fast(call_info->id);
    }
    delete call_info;
    delete str;
    return 0;
}

void CmainDlg::OnMenuSettings()
{
    if (!settingsDlg) {
        bool showDlg = true;
        if (showDlg) {
            settingsDlg = new SettingsDlg(this);
        }
    }
    else {
        if (!settingsDlg->isVisible()) {
            settingsDlg->show();
        }
        settingsDlg->raise();
        settingsDlg->activateWindow();
    }
}

void CmainDlg::OnMenuShortcuts()
{
    if (!shortcutsDlg) {
        shortcutsDlg = new ShortcutsDlg(this);
    }
    else {
        shortcutsDlg->raise(); // SetForegroundWindow
        shortcutsDlg->activateWindow();
    }
}

void CmainDlg::OnMenuExport()
{
    if (!exportDlg) {
        exportDlg = new ExportDlg(this);
    }
    else {
        exportDlg->raise(); // SetForegroundWindow
        exportDlg->activateWindow();
    }
}

void CmainDlg::OnMenuImport()
{
    // CFileDialog (Ini Files) → QFileDialog
    QString file = QFileDialog::getOpenFileName(this, TR(_T("Import Configuration")), QString(), QStringLiteral("Ini Files (*.ini)"));
    if (!file.isEmpty()) {
        if (!importDlg) {
            importDlg = new ImportDlg(this);
        }
        else {
            importDlg->raise();
            importDlg->activateWindow();
        }
        importDlg->Init(CS(file));
    }
}

void CmainDlg::OnMuteInput()
{
    pageDialer->OnBnClickedMuteInput();
}

void CmainDlg::OnMuteOutput()
{
    pageDialer->OnBnClickedMuteOutput();
}

void CmainDlg::ShortcutAction(Shortcut* shortcut, bool /*block*/, bool second)
{
    CString params;
    CString number = second && !shortcut->number2.IsEmpty() ? shortcut->number2 : shortcut->number;
    bool hasCalls = messagesDlg->GetCallsCount();
    if (shortcut->type == MSIP_SHORTCUT_CALL_BT) {
        shortcut->type = hasCalls ? MSIP_SHORTCUT_BT : MSIP_SHORTCUT_CALL;
    }
    if (shortcut->type == MSIP_SHORTCUT_CALL_AT) {
        shortcut->type = hasCalls ? MSIP_SHORTCUT_AT : MSIP_SHORTCUT_CALL;
    }
    if (shortcut->type == MSIP_SHORTCUT_CALL_CONF) {
        shortcut->type = hasCalls ? MSIP_SHORTCUT_CONF : MSIP_SHORTCUT_CALL;
    }
    if (shortcut->type == MSIP_SHORTCUT_CALL) {
        if (shortcut->ringing && CommandCallPickup(number)) {
        }
        else {
            MakeCall(number);
        }
    }
    else if (shortcut->type == MSIP_SHORTCUT_VIDEOCALL) {
#ifdef _GLOBAL_VIDEO
        MakeCall(number, true);
#else
        MakeCall(number);
#endif
    }
    else if (shortcut->type == MSIP_SHORTCUT_MESSAGE) {
        MessagesOpen(number);
    }
    else if (shortcut->type == MSIP_SHORTCUT_DTMF) {
        pageDialer->DTMF(number);
    }
    else if (shortcut->type == MSIP_SHORTCUT_BT) {
        if (number.IsEmpty()) {
            OpenTransferDlg(this, MSIP_ACTION_TRANSFER);
        }
        else {
            messagesDlg->CallAction(MSIP_ACTION_TRANSFER, number);
        }
    }
    else if (shortcut->type == MSIP_SHORTCUT_AT) {
        if (number.IsEmpty()) {
            OpenTransferDlg(this, MSIP_ACTION_ATTENDED_TRANSFER);
        }
        else {
            messagesDlg->CallAction(MSIP_ACTION_ATTENDED_TRANSFER, number);
        }
    }
    else if (shortcut->type == MSIP_SHORTCUT_CONF) {
        if (number.IsEmpty()) {
            OpenTransferDlg(this, MSIP_ACTION_INVITE);
        }
        else {
            messagesDlg->CallAction(MSIP_ACTION_INVITE, number);
        }
    }
}

void CmainDlg::ShortcutsRemoveAll()
{
    for (int i = 0; i < shortcuts.GetCount(); i++) {
        Shortcut* shortcut = &shortcuts.GetAt(i);
        if (shortcut->presence) {
            shortcut->presence = false;
            CString number = shortcut->number;
            UnsubscribeNumber(number);
        }
    }
    shortcuts.RemoveAll();
}

bool msip_restart_requested = false;

LRESULT CmainDlg::OnRestart(WPARAM, LPARAM)
{
    OnClose();
    // MySimpleSIP: ShellExecute(accountSettings.exeFile) → relançado em main.cpp, depois que
    // a instância atual libera o servidor de instância única
    msip_restart_requested = true;
    return TRUE;
}

bool CmainDlg::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress && watched->isWidgetType() && !m_closing) {
        QWidget* window = static_cast<QWidget*>(watched)->window();
        bool baseDialog = window == this || window == messagesDlg;
        for (int i = 0; !baseDialog && i < ringinDlgs.GetCount(); i++) {
            baseDialog = window == ringinDlgs.GetAt(i);
        }
        if (baseDialog && PreTranslateKey(window, static_cast<QKeyEvent*>(event))) {
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool CmainDlg::PreTranslateKey(QWidget* window, QKeyEvent* event)
{
    bool catched = false;
    const Qt::KeyboardModifiers mods = event->modifiers();
    bool controlState = mods & Qt::ControlModifier;
    bool altState = mods & Qt::AltModifier;
    bool shiftState = mods & Qt::ShiftModifier;
    const int key = event->key();
    std::function<void()> postCommand;
    if (controlState && !altState) {
        if (key == Qt::Key_Tab || key == Qt::Key_Backtab) {
            // MySimpleSIP: só na janela principal (no Windows, GetParent da janela de chamadas
            // também devolve a principal, que fica escondida atrás dela)
            if (window == this) {
                if (!shiftState) {
                    GotoTab(-1);
                }
                else {
                    GotoTab(-2);
                }
                catched = true;
            }
        }
        if (key == Qt::Key_M) {
            postCommand = [this] { OnMenuAccountEdit(0); }; // ID_ACCOUNT_EDIT_RANGE
        }
        if (key == Qt::Key_L) {
            postCommand = [this] { OnMenuAccountLocalEdit(); };
        }
        if (key == Qt::Key_P) {
            postCommand = [this] { OnMenuSettings(); };
        }
        if (key == Qt::Key_S) {
            postCommand = [this] { OnMenuShortcuts(); };
        }
        if (key == Qt::Key_Q) {
            postCommand = [this] { OnMenuExit(); };
        }
        if (key == Qt::Key_QuoteLeft || key == Qt::Key_I) {
            postCommand = [this] { OnMuteInput(); };
        }
        if (key == Qt::Key_O) {
            postCommand = [this] { OnMuteOutput(); };
        }
        if (key == Qt::Key_W) {
            postCommand = [this] { OnMenuWebsite(); };
        }
    }
    else {
        if (key == Qt::Key_F2) {
            //answer a call
            msip_call_answer();
            catched = true;
        }
        if (key == Qt::Key_F4) {
            //hangup
            msip_call_hangup_all();
            catched = true;
        }
        if (key == Qt::Key_Escape) {
            if (window == this && focusWidget() == m_ButtonMenu) {
                TabFocusSet();
                catched = true;
            }
        }
        // WM_SYSKEYDOWN VK_F10: foco no botão de menu (ou de volta para a aba)
        if (key == Qt::Key_F10 && !controlState && window == this) {
            if (focusWidget() == m_ButtonMenu) {
                TabFocusSet();
            }
            else {
                m_ButtonMenu->setFocus();
            }
            catched = true;
        }
    }
    if (postCommand) {
        // PostMessage(WM_COMMAND): executa fora do tratamento da tecla
        QTimer::singleShot(0, this, postCommand);
        catched = true;
    }
    return catched;
}

LRESULT CmainDlg::OnRecreate(WPARAM, LPARAM)
{
    if (!m_created) {
        statistics.postRecreateTimeCancel = CTime::GetCurrentTime().GetTime();
    }
    else {
        statistics.postRecreateTimeOk = CTime::GetCurrentTime().GetTime();
        PJCreate();
        OnAccountChanged();
        PJAccountAdd();
    }
    return 0;
}

LRESULT CmainDlg::onMWIInfo(WPARAM wParam, LPARAM /*lParam*/)
{
    bool hasMail = (bool)wParam;
    pageDialer->UpdateVoicemailButton(hasMail);
    return 0;
}

void CmainDlg::OnMenuWebsite()
{
    CString url = _T(_GLOBAL_MENU_WEBSITE);
    MSIP::OpenURL(url);
}

void CmainDlg::OnMenuHelp()
{
    // MySimpleSIP: OpenHelp() abre a ajuda do site do MicroSIP; aqui, o guia do usuário
    MSIP::OpenURL(_T(_GLOBAL_MENU_HELP));
}

void CmainDlg::OnSessionChange(UINT nSessionState, UINT /*nId*/)
{
    if (nSessionState == WTS_REMOTE_CONNECT || nSessionState == WTS_CONSOLE_CONNECT) {
        KillTimer(IDT_TIMER_SWITCH_DEVICES);
        if (is_pjsua_running()) {
            PJ_LOG(3, (THIS_FILENAME, "WM_WTSSESSION_CHANGE"));
            SetTimer(IDT_TIMER_SWITCH_DEVICES, 1500, NULL);
        }
    }
}

// MySimpleSIP: o servidor do MicroSIP responde em texto (URL, versão, novidades); aqui a
// resposta é a última Release do repositório (API do GitHub, JSON: tag_name, html_url, body)
void CmainDlg::OnCheckUpdates()
{
    updateCheckerShow = true;
    CheckUpdates();
}

void CmainDlg::CheckUpdates()
{
    CString url = _T(_GLOBAL_URL_CHECK_VERSION);
    // a API do GitHub exige User-Agent
    CString headers;
    headers.Format(_T("User-Agent: %s/%s\r\nAccept: application/vnd.github+json\r\n"), _T(_GLOBAL_NAME_VISIBLE), _T(_GLOBAL_VERSION));
    URLGetAsync(url, m_hWnd, UM_UPDATE_CHECKER_LOADED, 0, _T(""), headers);
}

// "1.2.3" (ou "v1.2.3") é mais nova que a nossa? Como o original, completa até 4 partes.
bool IsNewerVersion(const CString& version, const CString& our)
{
    auto parts = [](CString v) {
        v.Trim();
        if (!v.IsEmpty() && (v[0] == 'v' || v[0] == 'V')) {
            v = v.Mid(1);
        }
        std::vector<int> out;
        int pos = 0;
        CString token = v.Tokenize(_T("."), pos);
        while (!token.IsEmpty() && out.size() < 4) {
            for (int i = 0; i < token.GetLength(); i++) {
                if (token[i] < '0' || token[i] > '9') {
                    return std::vector<int>();
                }
            }
            out.push_back(_wtoi(token));
            token = v.Tokenize(_T("."), pos);
        }
        out.resize(4, 0);
        return out;
    };
    const std::vector<int> a = parts(version);
    const std::vector<int> b = parts(our);
    if (a.empty() || b.empty()) {
        return false;
    }
    return a > b;
}

LRESULT CmainDlg::OnUpdateCheckerLoaded(WPARAM wParam, LPARAM /*lParam*/)
{
    bool found = false;
    URLGetAsyncData* response = (URLGetAsyncData*)wParam;
    if (response->statusCode == 200 && !response->body.IsEmpty()) {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;
        std::istringstream in(std::string(response->body.GetString(), response->body.GetLength()));
        if (Json::parseFromStream(builder, in, &root, &errs) && root.isObject()) {
            CString version = MSIP::Utf8DecodeUni(CStringA(root.get("tag_name", "").asString().c_str()));
            CString url = MSIP::Utf8DecodeUni(CStringA(root.get("html_url", "").asString().c_str()));
            CString info = MSIP::Utf8DecodeUni(CStringA(root.get("body", "").asString().c_str()));
            info.Trim();
            if (info.GetLength() > 600) {
                info = info.Left(600) + _T("…");
            }
            // como o original só aceitava https em *.microsip.org: só a página do projeto
            bool allowed = url.Find(_T(_GLOBAL_MENU_WEBSITE "/")) == 0;
            if (allowed && IsNewerVersion(version, _T(_GLOBAL_VERSION))) {
                CString caption;
                caption.Format(_T("%s %s"), _T(_GLOBAL_NAME_VISIBLE), Translate(_T("Update Available")));
                CString message = Translate(_T("Do you want to update now?"));
                if (!info.IsEmpty()) {
                    message.AppendFormat(_T("\r\n\r\n%s"), info);
                }
                found = true;
                lastUpdateFound = version;
                if (!updateCheckerSilent && QMessageBox::question(this, QS(caption), QS(message)) == QMessageBox::Yes) {
                    MSIP::OpenURL(url);
                }
            }
        }
    }
    delete response;
    if (updateCheckerShow && !found && !updateCheckerSilent) {
        QMessageBox::information(this, QString(), TR(_T("No new version found")));
    }
    updateCheckerShow = false;
    updateCheckerDone = true;
    return 0;
}

void CmainDlg::createPreviewWin()
{
    if (!previewWin) {
        previewWin = new Preview(this);
    }
    previewWin->Start(VideoCaptureDeviceId());
}

void CmainDlg::InstallVideoRenderer()
{
    // MySimpleSIP: renderizador do pjmedia com janelas Qt (VideoWindow) no lugar do SDL
    msip_qt_video_install();
}

bool CmainDlg::CommandCallAnswer() {
    if (ringinDlgs.GetCount()) {
        RinginDlg* ringinDlg = ringinDlgs.GetAt(0);
        mainDlg->PostMessage(UM_CALL_ANSWER, (WPARAM)ringinDlg->GetCallId(), (LPARAM)0);
        return true;
    }
    return false;
}

bool CmainDlg::CommandCallReject()
{
    if (ringinDlgs.GetCount()) {
        RinginDlg* ringinDlg = ringinDlgs.GetAt(ringinDlgs.GetCount() - 1);
        ringinDlg->OnBnClickedDecline();
        return true;
    }
    return false;
}

bool CmainDlg::CommandCallPickup(CString number)
{
    if (accountSettings.enableFeatureCodeCP && !accountSettings.featureCodeCP.IsEmpty()) {
        CString commands;
        CString numberFormated = FormatNumber(number, &commands);
        SIPURI sipuri;
        MSIP::ParseSIPURI(numberFormated, sipuri);
        CString str = accountSettings.featureCodeCP;
        str.Append(sipuri.user);
        sipuri.user = str;
        numberFormated = MSIP::BuildSIPURI(sipuri);
        messagesDlg->PerformCall(numberFormated);
        return true;
    }
    return false;
}

LRESULT CmainDlg::onShellHookMessage(WPARAM wParam, LPARAM lParam)
{
    if (wParam == HSHELL_APPCOMMAND) {
        int nCmd = GET_APPCOMMAND_LPARAM(lParam);
        if (nCmd == APPCOMMAND_MEDIA_PLAY ||
            nCmd == APPCOMMAND_MEDIA_PLAY_PAUSE ||
            nCmd == APPCOMMAND_MEDIA_STOP) {
            if (ringinDlgs.GetCount()) {
                RinginDlg* ringinDlg = ringinDlgs.GetAt(0);
                if (nCmd == APPCOMMAND_MEDIA_STOP) {
                    ringinDlg->OnBnClickedDecline();
                }
                else {
                    ringinDlg->CallAcceptDefault();
                }
            }
            else {
                if (nCmd == APPCOMMAND_MEDIA_STOP) {
                    messagesDlg->OnBnClickedEnd();
                }
                else {
                    // MySimpleSIP: GetWindowInfo(WS_VISIBLE) → isVisibleTo (a janela pode estar
                    // na bandeja)
                    QWidget* callButton = pageDialer->findChild<QWidget*>(QStringLiteral("IDC_CALL"));
                    bool isButtonVisisble = callButton && callButton->isVisibleTo(pageDialer);
                    if (isButtonVisisble && callButton->isEnabled()) {
                        pageDialer->OnBnClickedCall();
                    }
                    else {
                        messagesDlg->OnBnClickedHold();
                    }
                }
            }
        }
        else if (nCmd == APPCOMMAND_MEDIA_PAUSE) {
            messagesDlg->OnBnClickedHold();
        }
    }
    return 0;
}

LRESULT CmainDlg::onRefreshLevels(WPARAM, LPARAM)
{
    pageDialer->RefreshLevels();
    return 0;
}

// gethostbyaddrThread: nome reverso do IP do contato remoto, com espera máxima de 500 ms
// MySimpleSIP: CreateThread/WaitForSingleObject → std::thread destacada + condition_variable
struct GetHostByAddrContext
{
    CString data;
    bool done = false;
    std::mutex m;
    std::condition_variable cv;
};

static CString GetHostByAddr(const CString& ip, int timeoutMs)
{
    auto ctx = std::make_shared<GetHostByAddrContext>();
    ctx->data = ip;
    std::thread([ctx] {
        sockaddr_in sa = {};
        sa.sin_family = AF_INET;
        CString host;
        if (inet_pton(AF_INET, CStringA(ctx->data), &sa.sin_addr) == 1) {
            char buf[NI_MAXHOST] = { 0 };
            if (getnameinfo((sockaddr*)&sa, sizeof(sa), buf, sizeof(buf), nullptr, 0, NI_NAMEREQD) == 0) {
                host = CString(CStringA(buf));
            }
        }
        std::lock_guard<std::mutex> lock(ctx->m);
        if (!host.IsEmpty()) {
            ctx->data = host;
        }
        ctx->done = true;
        ctx->cv.notify_all();
    }).detach();
    std::unique_lock<std::mutex> lock(ctx->m);
    if (ctx->cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] { return ctx->done; })) {
        return ctx->data;
    }
    return ip;
}

LRESULT CmainDlg::onCallState(WPARAM wParam, LPARAM lParam)
{
    statistics.postCallState0 = 1;
    pjsua_call_info* call_info = (pjsua_call_info*)wParam;
    call_user_data* user_data = (call_user_data*)lParam;

    SIPURI sipuri;
    ParseCallSIPURI(call_info, user_data, sipuri);
    CString number = (!sipuri.user.IsEmpty() ? sipuri.user + _T("@") : _T("")) + sipuri.domain;

    user_data->CS.Lock();

    CString* str = new CString();
    CString adder;

    statistics.postCallState1 = 1;

    if (!accountSettings.singleMode) {
        if (call_info->state != PJSIP_INV_STATE_DISCONNECTED && call_info->state != PJSIP_INV_STATE_CONNECTING && call_info->remote_contact.slen > 0) {
            SIPURI contactURI;
            ParseCallSIPURI(call_info, user_data, contactURI);
            CString contactDomain = MSIP::RemovePort(contactURI.domain);
            if (MSIP::IsIP(contactDomain)) {
                contactDomain = GetHostByAddr(contactDomain, 500);
            }
            adder.AppendFormat(_T("%s; "), contactDomain);
        }
    }

    if (call_info->state == PJSIP_INV_STATE_CONFIRMED
        || call_info->state == PJSIP_INV_STATE_CONNECTING) {
        if (autoAnswerTimerCallId != PJSUA_INVALID_ID) {
            KillTimer(IDT_TIMER_AUTOANSWER);
            autoAnswerTimerCallId = PJSUA_INVALID_ID;
        }
        if (forwardingTimerCallId != PJSUA_INVALID_ID) {
            KillTimer(IDT_TIMER_FORWARDING);
            forwardingTimerCallId = PJSUA_INVALID_ID;
        }
    }

    unsigned cnt = 0;
    unsigned cnt_srtp = 0;

    switch (call_info->state) {
    case PJSIP_INV_STATE_CALLING:
        *str = Translate(_T("Calling"));
        str->AppendFormat(_T(" %s "), !sipuri.user.IsEmpty() ? sipuri.user : sipuri.domain);
        str->Append(_T("..."));
        break;
    case PJSIP_INV_STATE_INCOMING:
        str->SetString(Translate(_T("Incoming Call")));
        break;
    case PJSIP_INV_STATE_EARLY:
        str->SetString(Translate(MSIP::PjToStr(&call_info->last_status_text)));
        break;
    case PJSIP_INV_STATE_CONNECTING:
        str->Format(_T("%s..."), Translate(_T("Connecting")));
        break;
    case PJSIP_INV_STATE_CONFIRMED:
        str->SetString(Translate(_T("Connected")));

        statistics.postCallState2 = 1;

        for (unsigned i = 0; i < call_info->media_cnt; i++) {
            if (call_info->media[i].dir != PJMEDIA_DIR_NONE &&
                (call_info->media[i].type == PJMEDIA_TYPE_AUDIO || call_info->media[i].type == PJMEDIA_TYPE_VIDEO)) {
                cnt++;
                pjsua_call_info call_info_stub;
                if (is_pjsua_running() && pjsua_call_get_info(call_info->id, &call_info_stub) == PJ_SUCCESS) {
                    bool srtp = false;
                    bool ice = false;
                    pjmedia_transport_info t;
                    if (pjsua_call_get_med_transport_info(call_info->id, call_info->media[i].index, &t) == PJ_SUCCESS) {
                        for (unsigned j = 0; j < t.specific_info_cnt; j++) {
                            if (t.spc_info[j].buffer[0]) {
                                switch (t.spc_info[j].type) {
                                case PJMEDIA_TRANSPORT_TYPE_SRTP:
                                    srtp = true;
                                    break;
                                case PJMEDIA_TRANSPORT_TYPE_ICE:
                                    ice = true;
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    }
                    pjsua_stream_info psi;
                    if (pjsua_call_get_stream_info(call_info->id, call_info->media[i].index, &psi) == PJ_SUCCESS) {
                        pjmedia_tp_proto proto = PJMEDIA_TP_PROTO_NONE;
                        if (psi.type == PJMEDIA_TYPE_AUDIO) {
                            proto = psi.info.aud.proto;
                            adder.AppendFormat(_T("%s@%dkHz %dkbit/s%s, "),
                                MSIP::PjToStr(&psi.info.aud.fmt.encoding_name),
                                psi.info.aud.fmt.clock_rate / 1000,
                                psi.info.aud.param->info.avg_bps / 1000,
                                psi.info.aud.fmt.channel_cnt == 2 ? _T(" Stereo") : _T("")
                            );
                        }
                        else if (psi.type == PJMEDIA_TYPE_VIDEO) {
                            proto = psi.info.vid.proto;
                            adder.AppendFormat(_T("%s %dkbit/s, "),
                                MSIP::PjToStr(&psi.info.vid.codec_info.encoding_name),
                                psi.info.vid.codec_param->enc_fmt.det.vid.max_bps / 1000
                            );
                        }
                        if (srtp) {
                            srtp = true;
                        }
                        if (proto & PJMEDIA_TP_PROTO_DTLS) {
                            cnt_srtp++;
                            adder.Append(_T("DTLS-SRTP, "));
                        }
                        else if (srtp || (proto & PJMEDIA_TP_PROFILE_SRTP)) {
                            bool secure = false;
                            if (transport_tls != -1) {
                                pj_pool_t* tmp_pool = pjsua_pool_create("msip_ocs", 256, 256);
                                if (tmp_pool) {
                                    pjsua_acc_config acc_cfg;
                                    pjsua_acc_config_default(&acc_cfg);
                                    if (pjsua_acc_get_config(call_info->acc_id, tmp_pool, &acc_cfg) == PJ_SUCCESS) {
                                        pj_str_t tls = pj_str((char*)";transport=tls"); // MySimpleSIP: &pj_str(...) no GCC
                                        secure = pj_strstr(&acc_cfg.id, &tls);
                                    }
                                    pj_pool_release(tmp_pool);
                                }
                            }
                            if (secure) {
                                cnt_srtp++;
                                adder.Append(_T("SRTP, "));
                            }
                            else {
                                adder.Append(_T("SRTP without TLS, "));
                            }
                        }
                        else {
                            adder.AppendFormat(_T("%s, "), Translate(_T("unencrypted")));
                        }
                        if (ice) {
                            adder.Append(_T("ICE, "));
                        }
                    }
                }
            }
        }
        if (cnt_srtp && cnt == cnt_srtp) {
            user_data->srtp = MSIP_SRTP;
        }
        else {
            user_data->srtp = MSIP_SRTP_DISABLED;
        }
        break;
    default:
        break;
    }
    if (!str->IsEmpty() && !adder.IsEmpty()) {
        str->AppendFormat(_T(" (%s)"), adder.Left(adder.GetLength() - 2));
    }
    if (call_info->state == PJSIP_INV_STATE_CALLING) {
        //--
        if (!accountSettings.cmdOutgoingCall.IsEmpty()) {
            CString params = sipuri.user;
            MSIP::RunCmd(URLMask(accountSettings.cmdOutgoingCall, &sipuri, call_info->acc_id, user_data), params);
        }
        //--
    }
    statistics.postCallState3 = 1;
    if (call_info->state == PJSIP_INV_STATE_CONFIRMED) {
        PostMessage(WM_TIMER, IDT_TIMER_CALL, 0);
        SetTimer(IDT_TIMER_CALL, 1000, NULL);
        if (call_info->role == PJSIP_ROLE_UAS) {
            //--
            if (!accountSettings.cmdCallAnswer.IsEmpty()
                ) {
                CString params = sipuri.user;
                MSIP::RunCmd(accountSettings.cmdCallAnswer, params);
            }
            if (call_info->rem_vid_cnt && !accountSettings.cmdCallAnswerVideo.IsEmpty()) {
                CString params = sipuri.user;
                MSIP::RunCmd(accountSettings.cmdCallAnswerVideo, params);
            }
            statistics.postCallState4 = 1;
            //--
        }
        //--
        statistics.postCallState5 = 1;
        if (!accountSettings.cmdCallStart.IsEmpty()) {
            CString params = sipuri.user;
            MSIP::RunCmd(accountSettings.cmdCallStart, params);
        }
        //--
        if (!user_data->commands.IsEmpty()) {
            SetTimer((UINT_PTR)call_info->id, 1000, (TIMERPROC)DTMFQueueTimerHandler);
        }
    }

    if (!accountSettings.singleMode) {
        if (call_info->state != PJSIP_INV_STATE_CONFIRMED) {
            if (call_info->state != PJSIP_INV_STATE_DISCONNECTED) {
                UpdateWindowText(*str, call_info->role == PJSIP_ROLE_UAS ? IDI_CALL_IN : IDI_CALL_OUT);
            }
        }
    }

    if (call_info->role == PJSIP_ROLE_UAC) {
        if (call_info->last_status == 180 && !call_info->media_cnt) {
            if (toneCalls.IsEmpty()) {
                PostMessage(WM_TIMER, IDT_TIMER_TONE, 0);
                SetTimer(IDT_TIMER_TONE, 4500, NULL);
                toneCalls.AddTail(call_info->id);
            }
            else if (toneCalls.Find(call_info->id) == NULL) {
                toneCalls.AddTail(call_info->id);
            }
        }
        else {
            POSITION position = toneCalls.Find(call_info->id);
            if (position != NULL) {
                toneCalls.RemoveAt(position);
                if (toneCalls.IsEmpty()) {
                    KillTimer(IDT_TIMER_TONE);
                    PostMessage(UM_ON_PLAYER_STOP, 0, 0);
                }
            }
        }
    }
    statistics.postCallState6 = 1;
    bool doNotShowMessagesWindow =
        call_info->state == PJSIP_INV_STATE_INCOMING ||
        call_info->state == PJSIP_INV_STATE_EARLY ||
        call_info->state == PJSIP_INV_STATE_DISCONNECTED ||
        accountSettings.singleMode;

    if (user_data->autoAnswer) {
        if (!accountSettings.bringToFrontOnIncoming) {
            doNotShowMessagesWindow = true;
        }
    }
    MessagesContact* messagesContact = messagesDlg->AddTab(number,
        (!accountSettings.singleMode &&
            (call_info->state == PJSIP_INV_STATE_CONFIRMED
                || call_info->state == PJSIP_INV_STATE_CONNECTING)
            )
        ||
        (accountSettings.singleMode
            &&
            (
                (call_info->role == PJSIP_ROLE_UAC && call_info->state != PJSIP_INV_STATE_DISCONNECTED)
                ||
                (call_info->role == PJSIP_ROLE_UAS &&
                    (call_info->state == PJSIP_INV_STATE_CONFIRMED
                        || call_info->state == PJSIP_INV_STATE_CONNECTING)
                    )
                ))
        ? TRUE : FALSE,
        call_info, user_data, doNotShowMessagesWindow, call_info->state == PJSIP_INV_STATE_DISCONNECTED
    );

    if (call_info->state == PJSIP_INV_STATE_CONFIRMED) {
        if (!accountSettings.singleMode && accountSettings.AC) {
            messagesDlg->OnMergeAll();
        }
    }

    if (call_info->state == PJSIP_INV_STATE_DISCONNECTED) {
        if (call_info->role == PJSIP_ROLE_UAS && call_info->connect_duration.sec == 0 && call_info->connect_duration.msec == 0 && call_info->last_status != 486) {
            //-- missed call
            missed = true;
        }
    }

    if (messagesContact) {
        CString name = messagesContact->name;
        CString address = messagesContact->aor + messagesContact->aorSuffix;
        if (call_info->state == PJSIP_INV_STATE_DISCONNECTED) {
            messagesContact->mediaStatus = PJSUA_CALL_MEDIA_ERROR;
            if (call_info->role == PJSIP_ROLE_UAS && call_info->last_status == 486) {
                pageCalls->Add(call_info->call_id, address, name, messagesContact->commands, MSIP_CALL_TYPE_MISSED, user_data);
            }
        }
        else {
            if (call_info->role == PJSIP_ROLE_UAS) {
                pageCalls->Add(call_info->call_id, address, name, messagesContact->commands, MSIP_CALL_TYPE_INCOMING, user_data);
            }
            else {
                pageCalls->Add(call_info->call_id, address, name, messagesContact->commands, MSIP_CALL_TYPE_OUTGOING, user_data);
            }
        }
    }
    if (accountSettings.singleMode) {
        if (call_info->state != PJSIP_INV_STATE_DISCONNECTED) {
            if (call_info->state != PJSIP_INV_STATE_CONFIRMED) {
                UpdateWindowText(*str, call_info->role == PJSIP_ROLE_UAS ? IDI_CALL_IN : IDI_CALL_OUT);
            }
            int tabN = 0;
            GotoTab(tabN);
            messagesDlg->OnChangeTab(call_info, user_data, false);
        }
    }
    statistics.postCallState7 = 1;

    if (call_info->state == PJSIP_INV_STATE_DISCONNECTED) {
        messagesDlg->OnEndCall(call_info, user_data);
        messagesContact = nullptr;
    }
    else {
        if (messagesContact && !str->IsEmpty()) {
            messagesDlg->AddMessage(messagesContact, *str, MSIP_MESSAGE_TYPE_SYSTEM,
                call_info->state == PJSIP_INV_STATE_INCOMING || call_info->state == PJSIP_INV_STATE_EARLY
            );
        }
    }

    bool hasCalls = messagesDlg->GetCallsCount();

    if (call_info->role == PJSIP_ROLE_UAS) {
        if (call_info->state != PJSIP_INV_STATE_INCOMING && call_info->state != PJSIP_INV_STATE_EARLY) {
            int count = ringinDlgs.GetCount();
            if (!count) {
                if (call_info->state != PJSIP_INV_STATE_DISCONNECTED || (call_info->state == PJSIP_INV_STATE_DISCONNECTED && call_info->connect_duration.sec == 0 && call_info->connect_duration.msec == 0)) {
                    PlayerStop();
                }
            }
            else {
                for (int i = 0; i < count; i++) {
                    RinginDlg* ringinDlg = ringinDlgs.GetAt(i);
                    if (call_info->id == ringinDlg->GetCallId()) {
                        if (count == 1) {
                            PlayerStop();
                        }
                        ringinDlgs.RemoveAt(i);
                        ringinDlg->DestroyWindow();
                        break;
                    }
                }
            }
        }
    }
    statistics.postCallState8 = 1;
    if (call_info->state != PJSIP_INV_STATE_INCOMING &&
        call_info->state != PJSIP_INV_STATE_EARLY
        ) {
        if (call_info->state != PJSIP_INV_STATE_DISCONNECTED) {
            if (messagesContact) {
                CString name = messagesContact->name;
                pageDialer->SetName(name);
            }
        }
    }

    user_data->CS.Unlock();

    // --delete user data
    if (call_info->state == PJSIP_INV_STATE_DISCONNECTED) {
        if (user_data) {
            delete user_data;
        }
    }
    // --
    delete call_info;
    delete str;

    if (pageDialer->m_ButtonRec) {
        pageDialer->m_ButtonRec->setEnabled(hasCalls);
    }
    // SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED...) durante as chamadas
    if (m_powerMonitor) {
        m_powerMonitor->setCallActive(hasCalls);
    }
    if (accountSettings.headsetSupport) {
        Hid::SetOffhookRing(hasCalls, ringinDlgs.GetCount());
    }
    // MySimpleSIP: teclas de mídia vão para este player enquanto houver chamada (MPRIS)
    if (m_mediaKeys) {
        m_mediaKeys->setCallActive(hasCalls || ringinDlgs.GetCount());
    }
    statistics.postCallState9 = 1;
    return 0;
}

LRESULT CmainDlg::onCallMediaState(WPARAM wParam, LPARAM lParam)
{
    pjsua_call_info* call_info = (pjsua_call_info*)wParam;
    call_user_data* user_data = (call_user_data*)lParam;

    messagesDlg->UpdateHoldButton(call_info);

    CString message;
    CString number = MSIP::PjToStr(&call_info->remote_info, TRUE);

    MessagesContact* messagesContact = messagesDlg->AddTab(number, FALSE, call_info, user_data, TRUE, TRUE);

    if (messagesContact) {
        if (call_info->media_status == PJSUA_CALL_MEDIA_REMOTE_HOLD) {
            message = _T("Call on Remote Hold");
        }
        if (call_info->media_status == PJSUA_CALL_MEDIA_LOCAL_HOLD) {
            message = _T("Call on Local Hold");
        }
        if (call_info->media_status == PJSUA_CALL_MEDIA_NONE) {
            message = _T("Call on Hold");
        }
        if (messagesContact->mediaStatus != PJSUA_CALL_MEDIA_ERROR && messagesContact->mediaStatus != call_info->media_status && call_info->media_status == PJSUA_CALL_MEDIA_ACTIVE) {
            message = _T("Call is Active");
        }
        if (!message.IsEmpty()) {
            messagesDlg->AddMessage(messagesContact, Translate(message), MSIP_MESSAGE_TYPE_SYSTEM, TRUE);
        }
        messagesContact->mediaStatus = call_info->media_status;
        pageDialer->SetName();
    }
    if (call_info->media_status == PJSUA_CALL_MEDIA_ACTIVE
        || call_info->media_status == PJSUA_CALL_MEDIA_REMOTE_HOLD
        ) {
        onRefreshLevels(0, 0);
    }

    delete call_info;

    return 0;
}

LRESULT CmainDlg::onIncomingCall(WPARAM wParam, LPARAM lParam)
{
    statistics.postIncomingCall0 = 1;
    pjsua_call_info* call_info = (pjsua_call_info*)wParam;
    call_user_data* user_data = (call_user_data*)lParam;

    user_data->CS.Lock();

    SIPURI sipuri;
    ParseCallSIPURI(call_info, user_data, sipuri);

    GetNameForCall(sipuri, user_data);

    accountSettings.lastCallNumber = sipuri.user;
    accountSettings.lastCallHasVideo = false;

    statistics.postIncomingCall1 = 1;

    bool autoAnswer = user_data->autoAnswer;
    user_data->autoAnswer = false;
    bool playBeep = false;

    if (user_data->forwarding && messagesDlg->CallAction(MSIP_ACTION_FORWARD, _T(""), call_info->id)) {
        statistics.postIncomingCall2 = 1;
    }
    else
        if (autoAnswer && AutoAnswer(call_info->id)) {
            statistics.postIncomingCall3 = 1;
        }
        else {
            bool createRinging = true;
            if (createRinging) {
                PostMessage(UM_CREATE_RINGING, (WPARAM)call_info->id, 0);
            }
            statistics.postIncomingCall4 = 1;
            pjsua_call_answer(call_info->id, 180, NULL, NULL);
            if (messagesDlg->GetCallsCount()) {
                statistics.postIncomingCall5 = 1;

                playBeep = true;
            }
            else {
                statistics.postIncomingCall6 = 1;
                    if (!accountSettings.ringtone.GetLength()) {
                        onPlayerPlay(MSIP_SOUND_RINGTONE, 0);
                    }
                    else {
                        onPlayerPlay(MSIP_SOUND_CUSTOM, (LPARAM)&accountSettings.ringtone);
                    }
            }
            if (accountSettings.headsetSupport) {
                Hid::SetRing(true);
            }
            // MySimpleSIP: teclas de mídia para atender (player MPRIS "tocando")
            if (m_mediaKeys) {
                m_mediaKeys->setCallActive(true);
            }
            statistics.postIncomingCall7 = 1;
            //--
            if (!accountSettings.cmdCallRing.IsEmpty()) {
                CString params = sipuri.user;
                MSIP::RunCmd(accountSettings.cmdCallRing, params);
            }
            //--
        }
    statistics.postIncomingCall8 = 1;
    if (accountSettings.localDTMF && playBeep) {
        onPlayerPlay(MSIP_SOUND_RINGIN2, 0);
    }

    user_data->CS.Unlock();
    delete call_info;
    statistics.postIncomingCall9 = 1;
    return 0;
}

LRESULT CmainDlg::onCreateRingingDlg(WPARAM wParam, LPARAM)
{
    pjsua_call_id call_id = wParam;
    pjsua_call_info call_info;

    if (!is_pjsua_running() || pjsua_call_get_info(call_id, &call_info) != PJ_SUCCESS) {
        int count = ringinDlgs.GetCount();
        if (!count) {
            PlayerStop();
        }
        return  0;
    }

    call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(call_info.id);
    if (!user_data) {
        return  0;
    }

    user_data->CS.Lock();

    RinginDlg* ringinDlg = new RinginDlg(this, call_info.rem_vid_cnt);
    ringinDlg->SetCallId(call_info.id);

    SIPURI sipuri;

    CString name = user_data->name;

    ringinDlg->SetCallerName(name); // IDC_CALLER_NAME e IDC_RINGIN_NAME_BLIND

    CString str;
    ParseCallSIPURI(&call_info, user_data, sipuri);
    CString info = (!sipuri.user.IsEmpty() ? sipuri.user + _T("@") : _T("")) + sipuri.domain;
    if (!sipuri.name.IsEmpty() && sipuri.name != name) {
        info = sipuri.name + _T(" <") + info + _T(">");
    }
    str.AppendFormat(_T("%s\r\n"), info);
    if (!user_data->userAgent.IsEmpty()) {
        str.AppendFormat(_T("%s\r\n"), user_data->userAgent);
    }
    str.Append(_T("\r\n"));
    info = MSIP::PjToStr(&call_info.local_info, TRUE);
    MSIP::ParseSIPURI(info, sipuri);
    info = (!sipuri.user.IsEmpty() ? sipuri.user + _T("@") : _T("")) + sipuri.domain;
    str.AppendFormat(_T("%s: %s\r\n"), Translate(_T("To")), info);

    if (!user_data->diversion.IsEmpty()) {
        str.AppendFormat(_T("%s: %s\r\n"), Translate(_T("Diversion")), user_data->diversion);
    }
    if (str == name) {
        str.Empty();
    }
    ringinDlg->SetCallerAddr(str); // vazio → IDC_CALLER_ADDR desabilitado

    ringinDlgs.Add(ringinDlg);
    if (name == sipuri.user) {
        str = sipuri.user;
    }
    else {
        str.Format(_T("%s – %s"), name, sipuri.user);
    }
    if (!accountSettings.bringToFrontOnIncoming) {
        if (!IsForegroundApp()) {
            BaloonPopup(Translate(_T("Incoming Call")), str, NIIF_INFO);
        }
    }
    user_data->CS.Unlock();
    return 0;
}

LRESULT CmainDlg::onPlayerPlay(WPARAM wParam, LPARAM lParam)
{
    CString filename;
    BOOL noLoop;
    BOOL inCall;
    if (wParam == MSIP_SOUND_CUSTOM) {
        filename = *(CString*)lParam;
        MSIP::ExpandEnvironmentStrings(filename);
        noLoop = FALSE;
        inCall = FALSE;
    }
    else if (wParam == MSIP_SOUND_CUSTOM_NOLOOP) {
        filename = *(CString*)lParam;
        MSIP::ExpandEnvironmentStrings(filename);
        noLoop = TRUE;
        inCall = FALSE;
    }
    else {
        switch (wParam) {
        case MSIP_SOUND_MESSAGE_IN:
            filename.Append(_T("msgin.wav"));
            noLoop = TRUE;
            inCall = FALSE;
            break;
        case MSIP_SOUND_MESSAGE_OUT:
            filename.Append(_T("msgout.wav"));
            noLoop = TRUE;
            inCall = FALSE;
            break;
        case MSIP_SOUND_HANGUP:
            filename.Append(_T("hangup.wav"));
            noLoop = TRUE;
            inCall = TRUE;
            break;
        case MSIP_SOUND_RINGTONE:
            filename.Append(_T("ringtone.wav"));
            noLoop = FALSE;
            inCall = FALSE;
            break;
        case MSIP_SOUND_RINGIN2:
            filename.Append(_T("ringing2.wav"));
            noLoop = TRUE;
            inCall = TRUE;
            break;
        case MSIP_SOUND_RINGING:
            filename.Append(_T("ringing.wav"));
            noLoop = TRUE;
            inCall = TRUE;
            break;
        default:
            noLoop = TRUE;
            inCall = FALSE;
        }
    }
    if (filename.Find('\\') == -1 && filename.Find('/') == -1) {
        // MySimpleSIP: sons ao lado do executável (build/portátil) ou em share/mysimplesip/sounds
        CString path = accountSettings.pathExe + _T("/") + filename;
        if (!QFileInfo::exists(QS(path))) {
            path = CS(QStringLiteral(MSIP_DATA_DIR "/sounds/")) + filename;
        }
        filename = path;
    }
    PlayerPlay(filename, noLoop, inCall);
    return 0;
}

void CmainDlg::onPlayerStop()
{
    PlayerStop();
    if (autoAnswerPlayCallId != PJSUA_INVALID_ID) {
        AutoAnswer(autoAnswerPlayCallId, true);
        autoAnswerPlayCallId = PJSUA_INVALID_ID;
    }
}

void CmainDlg::OnTimerCall()
{
    pjsua_call_id call_id;
    int duration = messagesDlg->GetCallDuration(&call_id);
    if (duration != -1) {
        CString str;
        unsigned icon = IDI_ACTIVE;
        if (call_id != PJSUA_INVALID_ID) {
            int holdFrom = -1;
            if (is_pjsua_running()) {
                call_user_data* user_data = (call_user_data*)pjsua_call_get_user_data(call_id);
                if (user_data) {
                    user_data->CS.Lock();
                    holdFrom = user_data->holdFrom;
                    user_data->CS.Unlock();
                }
            }
            if (holdFrom != -1) {
                icon = IDI_HOLD;
                str.Format(_T("%s %s / %s"), Translate(_T("Hold")), MSIP::GetDuration(duration - holdFrom, true), MSIP::GetDuration(duration, true));
            }
            else {
                str.Format(_T("%s %s"), Translate(_T("Connected")), MSIP::GetDuration(duration, true));
            }
        }
        else {
            str.Format(_T("%s (%d)"), Translate(_T("Connected")), duration);
        }
        if (call_id != PJSUA_INVALID_ID && icon != IDI_HOLD) {
            call_user_data* user_data;
            if (is_pjsua_running()) {
                user_data = (call_user_data*)pjsua_call_get_user_data(call_id);
            }
            else {
                user_data = NULL;
            }
            if (user_data) {
                user_data->CS.Lock();
                if (user_data->srtp == MSIP_SRTP) {
                    icon = IDI_ACTIVE_SECURE;
                }
                float MOS;
                if (duration > 0 && user_data && msip_call_statistics(user_data, &MOS)) {
                    if (MOS <= 2) {
                        icon = (icon == IDI_ACTIVE_SECURE ? IDI_ACTIVE_SECURE_RED : IDI_ACTIVE_RED);
                    }
                    else if (MOS <= 3) {
                        icon = (icon == IDI_ACTIVE_SECURE ? IDI_ACTIVE_SECURE_YELLOW : IDI_ACTIVE_YELLOW);
                    }
                }
                user_data->CS.Unlock();
            }
        }
        UpdateWindowText(str, icon);
    }
    else {
        KillTimer(IDT_TIMER_CALL);
    }
}

void CmainDlg::OnTimer(UINT_PTR TimerVal)
{
    // MySimpleSIP: IDT_TIMER_VERSION (verificação de atualizações) depende do servidor de
    // atualizações do projeto (decisão pendente)
    if (TimerVal == IDT_TIMER_NETWORK_CHANGED) {
        KillTimer(IDT_TIMER_NETWORK_CHANGED);
        OnTimerNetworkChange();
        return;
    }
    if (TimerVal == IDT_TIMER_SWITCH_DEVICES) {
        KillTimer(IDT_TIMER_SWITCH_DEVICES);
        if (is_pjsua_running()) {
            statistics.winAudioChanged++;
            statistics.winAudioChangedTime = CTime::GetCurrentTime().GetTime();
            PJ_LOG(3, (THIS_FILENAME, "Execute refresh devices"));
            bool is_active = msip_snd_is_active();
            bool is_ring = false;
            if (is_active) {
                int in, out;
                if (pjsua_get_snd_dev(&in, &out) == PJ_SUCCESS) {
                    is_ring = (out == msip_audio_ring);
                }
                else {
                    is_ring = false;
                }
                pjsua_set_null_snd_dev();
            }
            pjmedia_aud_dev_refresh();
            UpdateSoundDevicesIds();
            if (is_active) {
                msip_set_sound_device(is_ring ? msip_audio_ring : msip_audio_output, true);
            }
#ifdef _GLOBAL_VIDEO
            if (!accountSettings.disableVideo) {
                pjmedia_vid_subsys* vid_subsys = pjmedia_get_vid_subsys();
                if (vid_subsys->init_count) {
                    videoUsedTime = CTime::GetCurrentTime().GetTime();
                    pjmedia_vid_dev_refresh();
                }
            }
#endif
            if (accountSettings.headsetSupport) {
                Hid::OpenDevice();
            }
        }
        return;
    }
    if (TimerVal == IDT_TIMER_IDLE) {
        if (is_pjsua_running() && m_PresenceStatus != PJRPID_ACTIVITY_BUSY) {
            //--
            // MySimpleSIP: GetLastInputInfo → instante da última entrada = agora − ociosidade
            // da sessão (org.gnome.Mutter.IdleMonitor), em segundos para não oscilar
            const qint64 idle = SessionIdleTimeMs();
            if (idle >= 0) {
                const qint64 lastInput = (QDateTime::currentMSecsSinceEpoch() - idle) / 1000;
                if (m_lastInputTime < 0 || qAbs(lastInput - m_lastInputTime) > 1) {
                    m_lastInputTime = lastInput;
                    m_idleCounter = 0;
                    if (m_PresenceStatus == PJRPID_ACTIVITY_AWAY) {
                        PublishStatus();
                    }
                }
                else {
                    m_idleCounter++;
                    if (m_idleCounter == 120) {
                        PublishStatus(false);
                    }
                }
            }
            //--
        }
        return;
    }
    if (TimerVal == IDT_TIMER_AUTOANSWER) {
        KillTimer(IDT_TIMER_AUTOANSWER);
        if (autoAnswerTimerCallId != PJSUA_INVALID_ID) {
            AutoAnswer(autoAnswerTimerCallId);
            autoAnswerTimerCallId = PJSUA_INVALID_ID;
        }
    }
    else if (TimerVal == IDT_TIMER_FORWARDING) {
        KillTimer(IDT_TIMER_FORWARDING);
        if (forwardingTimerCallId != PJSUA_INVALID_ID) {
            messagesDlg->CallAction(MSIP_ACTION_FORWARD, _T(""), forwardingTimerCallId);
            forwardingTimerCallId = PJSUA_INVALID_ID;
        }
    }
    else if (TimerVal == IDT_TIMER_SAVE) {
        KillTimer(IDT_TIMER_SAVE);
        accountSettings.SettingsSave();
    }
    else if (TimerVal == IDT_TIMER_DIRECTORY) {
        UsersDirectoryLoad(true);
    }
    else if (TimerVal == IDT_TIMER_CALL) {
        OnTimerCall();
    }
    else if (TimerVal == IDT_TIMER_TONE) {
        onPlayerPlay(MSIP_SOUND_RINGING, 0);
    }
}

void CmainDlg::OnInstanceMessage(const QString& line)
{
    if (line == QLatin1String("exit")) {
        OnMenuExit(); // WM_COMMAND ID_EXIT
    }
    else if (line.startsWith(QLatin1String("cmd "))) {
        // OnCopyData (dwData = 1) e, se ele pedir, ShowWindow + SetForegroundWindow
        BOOL res = TRUE;
        if (is_pjsua_running()) {
            res = CommandLine(CS(line.mid(4)));
        }
        if (res) {
            onTrayNotify(0, 0);
        }
    }
    else {
        onTrayNotify(0, 0); // mostrar e trazer para a frente
    }
}

void CmainDlg::PlatformMonitorsStart()
{
    // NotifyUnicastIpAddressChange/NotifyRouteChange2 → netlink
    delete m_networkMonitor;
    m_networkMonitor = new NetworkMonitor(this);
    QObject::connect(m_networkMonitor, &NetworkMonitor::changed, this, [this] { PostMessage(UM_NETWORK_CHANGE, 0, 0); });
    // CMMNotificationClient → eventos do PulseAudio (com o backend ALSA não há aviso)
    if (msip_pulse_audio_active()) {
        msip_pulse_audio_monitor_start([](void* data) {
            static_cast<CmainDlg*>(data)->PostMessage(WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 1);
        }, this);
    }
    // fim do PJCreate original
    if (accountSettings.headsetSupport) {
        Hid::OpenDevice();
    }
    // MySimpleSIP: a opção das teclas de mídia pode ter mudado (Configurações recriam o pjsua)
    if (m_mediaKeys) {
        m_mediaKeys->setEnabled(accountSettings.enableMediaButtons);
    }
}

void CmainDlg::PlatformMonitorsStop()
{
    msip_pulse_audio_monitor_stop();
    delete m_networkMonitor;
    m_networkMonitor = nullptr;
    // PJDestroy: Hid::CloseDevice(true) com headsetSupport
    // MySimpleSIP: sem a condição, para fechar o fone também quando a opção foi desmarcada
    Hid::CloseDevice(true);
}

LRESULT CmainDlg::OnNetworkChange(WPARAM wParam, LPARAM lParam)
{
    KillTimer(IDT_TIMER_NETWORK_CHANGED);
    if (is_pjsua_running()) {
        PJ_LOG(3, (THIS_FILENAME, "UM_NETWORK_CHANGE(%d,%d)", (int)wParam, (int)lParam));
        SetTimer(IDT_TIMER_NETWORK_CHANGED, 1000, 0);
    }
    return 0;
}

void CmainDlg::OnTimerNetworkChange()
{
    statistics.winNetworkChanged++;
    statistics.winNetworkChangedTime = CTime::GetCurrentTime().GetTime();
    if (!is_pjsua_running()) {
        return;
    }
    statistics.winNetworkChanged0++;
    if (!MSIP::IsConnectedToNetworkCOM()) {
        return;
    }
    if (!is_pjsua_running()) {
        return;
    }
    //--
    if (!MSIP::IsNetworkChanged()) {
        return;
    }
    //--
    statistics.winNetworkChanged1++;
    MSIP::PortKnock();
    if (accountSettings.networkChanges) {
        statistics.winNetworkChanged2++;
        //PJ_LOG(3, (THIS_FILENAME, "NETWORK CHANGED, update transports, accounts and calls"));
        pjsua_ip_change_param param;
        pjsua_ip_change_param_default(&param);
        statistics.winNetworkChangedThreadId = (int)pj_getpid(); // MySimpleSIP: GetCurrentThreadId
        auto status = pjsua_handle_ip_change(&param);
        if (status == PJ_SUCCESS) {
            statistics.winNetworkChanged3++;
            statistics.winNetworkChangedTimeSuccess = CTime::GetCurrentTime().GetTime();
            pjsua_acc_id ids[PJSUA_MAX_ACC];
            unsigned count = PJSUA_MAX_ACC;
            if (pjsua_enum_accs(ids, &count) == PJ_SUCCESS) {
                for (unsigned i = 0; i < count; i++) {
                    pj_pool_t* tmp_pool = pjsua_pool_create("msip_ipch", 256, 256);
                    if (!tmp_pool) continue;
                    pjsua_acc_config acc_cfg;
                    pjsua_acc_config_default(&acc_cfg);
                    if (pjsua_acc_get_config(ids[i], tmp_pool, &acc_cfg) == PJ_SUCCESS) {
                        if (acc_cfg.rtp_cfg.public_addr.slen > 0) {
                            // update public address for account
                            Account accountTmp;
                            accountTmp.publicAddr = MSIP::PjToStr(&acc_cfg.rtp_cfg.public_addr);
                            CStringA str = CStringA(get_public_addr(&accountTmp));
                            pj_str_t new_pub_addr = pj_str((char*)str.GetString());
                            if (pj_strcmp(&acc_cfg.rtp_cfg.public_addr, &new_pub_addr) != 0) {
                                pj_strdup(tmp_pool, &acc_cfg.rtp_cfg.public_addr, &new_pub_addr);
                                pjsua_acc_modify(ids[i], &acc_cfg);
                            }
                        }
                    }
                    pj_pool_release(tmp_pool);
                    if (!acc_cfg.allow_contact_rewrite) {
                        // re-register even if allow_contact_rewrite is disabled
                        pjsua_acc_info info;
                        pjsua_acc_get_info(ids[i], &info);
                        if (info.has_registration) {
                            pjsua_acc_update_contact_on_ip_change(&pjsua_var.acc[ids[i]]);
                        }
                    }
                }
            }
        }
        else {
            // !! restart pjsip if we got here?
            statistics.winNetworkChanged4++;
            statistics.winNetworkChangedTimeError = CTime::GetCurrentTime().GetTime();
            statistics.winNetworkChanged4Error = status;
        }
    }
}

UINT CmainDlg::OnPowerBroadcast(UINT nPowerEvent, LPARAM)
{
    if (nPowerEvent == PBT_APMRESUMEAUTOMATIC) {
        statistics.winPowerBroadcastResume++;
        statistics.winPowerBroadcastResumeTime = CTime::GetCurrentTime().GetTime();
        PostMessage(UM_RECREATE, 0, 0);  // Callbacks must be processed before a new session starts
    }
    else if (nPowerEvent == PBT_APMSUSPEND) {
        statistics.winPowerBroadcastSuspend++;
        statistics.winPowerBroadcastSuspendTime = CTime::GetCurrentTime().GetTime();
        if (is_pjsua_running()) {
            PJ_LOG(3, (THIS_FILENAME, "WM_POWERBROADCAST received"));
            PJDestroy();
        }
    }
    return TRUE;
}

BOOL CmainDlg::OnDeviceChange(UINT nEventType, DWORD_PTR)
{
    if (nEventType == DBT_DEVNODES_CHANGED) {
        KillTimer(IDT_TIMER_SWITCH_DEVICES);
        if (is_pjsua_running()) {
            PJ_LOG(3, (THIS_FILENAME, "WM_DEVICECHANGE"));
            SetTimer(IDT_TIMER_SWITCH_DEVICES, 1500, NULL);
        }
    }
    return TRUE;
}

LRESULT CmainDlg::onCommandLine(WPARAM, LPARAM lParam)
{
    CString* str = (CString*)lParam;
    CommandLine(*str);
    delete str;
    return 0;
}

bool CmainDlg::CommandLine(CString params)
{
    bool activate = false;
    params.Trim();
    if (params.GetLength() >= 2 && params.GetAt(0) == '"' && params.GetAt(params.GetLength() - 1) == '"') { // MySimpleSIP: + GetLength (GetAt fora do limite)
        params = params.Mid(1, params.GetLength() - 2);
    }
    if (!params.IsEmpty()) {
        if (params.Find(_T("msip:")) == 0) {
            CString cmd = params.Mid(5);
            if (cmd == _T("minimize")) {
                hide(); // ShowWindow(SW_HIDE)
            }
            else if (cmd == _T("answer")) {
                msip_call_answer();
            }
            else if (cmd == _T("hangupall")) {
                msip_call_hangup_all_noincoming();
            }
            else if (cmd == _T("hold")) {
                messagesDlg->OnBnClickedHold();
            }
            else if (cmd.Find(_T("transfer_")) == 0) {
                messagesDlg->CallAction(MSIP_ACTION_TRANSFER, cmd.Mid(9));
            }
            else if (cmd == _T("micmute")) {
                pageDialer->MuteInput(true);
            }
            else if (cmd == _T("micunmute")) {
                pageDialer->MuteInput(false);
            }
            else if (cmd == _T("speakmute")) {
                pageDialer->MuteOutput(true);
            }
            else if (cmd == _T("speakunmute")) {
                pageDialer->MuteOutput(false);
            }
            else if (cmd == _T("micmuteclick")) {
                pageDialer->OnBnClickedMuteInput();
            }
            else if (cmd == _T("speakmuteclick")) {
                pageDialer->OnBnClickedMuteOutput();
            }
            else if (cmd == _T("micup")) {
                pageDialer->OnBnClickedPlusInput();
            }
            else if (cmd == _T("micdown")) {
                pageDialer->OnBnClickedMinusInput();
            }
            else if (cmd == _T("speakup")) {
                pageDialer->OnBnClickedPlusOutput();
            }
            else if (cmd == _T("speakdown")) {
                pageDialer->OnBnClickedMinusOutput();
            }
            else if (!cmd.IsEmpty()) {
                DialNumberFromCommandLine(cmd);
            }
            return activate;
        }
        DialNumberFromCommandLine(params);
    }
    return activate;
}

void CmainDlg::DialNumberFromCommandLine(CString params)
{
    pjsua_acc_info info;
    if (params.Mid(0, 4).CompareNoCase(_T("tel:")) == 0 || params.Mid(0, 4).CompareNoCase(_T("sip:")) == 0) {
        params = params.Mid(4);
    }
    else if (params.Mid(0, 7).CompareNoCase(_T("callto:")) == 0) {
        params = params.Mid(7);
    }
    else if (params.Mid(0, 8).CompareNoCase(_T("dialpad:")) == 0) {
        params = params.Mid(8);
    }
    else if (params.Mid(0, 5).CompareNoCase(_T("dial:")) == 0) {
        params = params.Mid(5);
    }
    if (params.Mid(0, 2) == _T("//")) {
        params = params.Mid(2);
        if (params.Right(1) == _T("/")) {
            params = params.Mid(0, params.GetLength() - 1);
        }
    }
    int pos = params.Find(_T("/account:"));
    if (pos != -1) {
        CString value = params.Mid(pos + 9);
        int pos2 = -1;
        if (!value.IsEmpty()) {
            pos2 = value.Find(_T(" "));
            if (pos2 != -1) {
                value = value.Left(pos2);
            }
            int accountId = _wtoi(value);
            if (accountId > 0) {
                Account account;
                if (accountSettings.AccountLoad(accountId, &account)) {
                    int pos = params.Find(_T("/password:"));
                    if (pos != -1) {
                        account.password = params.Mid(pos + 10);
                        accountSettings.AccountSave(accountId, &account);
                        if (accountSettings.accountId == accountId) {
                            PJAccountDelete();
                            accountSettings.AccountLoad(accountSettings.accountId, &accountSettings.account);
                            OnAccountChanged();
                            PJAccountAdd();
                        }
                        params.Empty();
                    }
                    else {
                        if (accountSettings.accountId != accountId) {
                            if (accountSettings.accountId) {
                                PJAccountDelete();
                            }
                            accountSettings.accountId = accountId;
                            accountSettings.AccountLoad(accountSettings.accountId, &accountSettings.account);
                            accountSettings.SettingsSave();
                            OnAccountChanged();
                            PJAccountAdd();
                        }
                    }

                }
                else {
                    params.Empty();
                }
            }
        }
        // MySimpleSIP: no 2º caso o original passa pos + 9 + pos2 + 1 como quantidade
        // (Delete(início, quantidade)) e, com texto antes de "/account:N ", apagava também o
        // começo do que vinha depois; aqui sai exatamente "/account:N "
        if (!params.IsEmpty()) {
            if (pos2 == -1) {
                params.Delete(pos - 1, params.GetLength());
            }
            else {
                params.Delete(pos, 9 + pos2 + 1);
            }
        }
    }
    if (params == _T("/answer")) {
        msip_call_answer();
    }
    else if (params == _T("/hangupall")) {
        msip_call_hangup_all_noincoming();
    }
    else if (params == _T("/hangupincoming")) {
        msip_call_hangup_incoming();
    }
    else if (params == _T("/hangupcalling")) {
        msip_call_hangup_calling();
    }
    else if (params.Find(_T("/dtmf:")) == 0) {
        CString value = params.Mid(6);
        if (!value.IsEmpty()) {
            pageDialer->DTMF(value);
        }
    }
    else if (params.Find(_T("/password:")) == 0) {
        CString value = params.Mid(10);
        password = value;
    }
    else if (params.Find(_T("/transfer:")) == 0) {
        CString value = params.Mid(10);
        if (!value.IsEmpty()) {
            messagesDlg->CallAction(MSIP_ACTION_TRANSFER, value);
        }
    }
    else {
        GotoTab(0);
        onTrayNotify(0, 0); // onTrayNotify(NULL, WM_LBUTTONUP)
        if (accountSettings.accountId > 0) {
            if (pjsua_acc_is_valid(account) &&
                (get_account_server().IsEmpty() ||
                    (pjsua_acc_get_info(account, &info) == PJ_SUCCESS && info.status == 200)
                    )
                ) {
                DialNumber(params);
            }
            else {
                dialNumberDelayed = params;
            }
        }
        else {
            if (pjsua_acc_is_valid(account_local)) {
                DialNumber(params);
            }
            else if (accountSettings.enableLocalAccount) {
                dialNumberDelayed = params;
            }
        }
    }
}

void CmainDlg::DialNumber(CString params)
{
    CString number;
    CString message;
    int i = params.Find(_T(" "));
    if (i != -1) {
        number = params.Mid(0, i);
        message = params.Mid(i + 1);
        message.Trim();
    }
    else {
        number = params;
    }
    number.Replace(_T("%20"), _T(" "));
    number.Replace(_T("%2B"), _T("+"));
    number.Trim();
    if (!number.IsEmpty()) {
        if (message.IsEmpty()) {
            CString numberAdd = number;
            pageDialer->DialedAdd(numberAdd);
            MakeCall(number, false, true);
        }
        else {
            messagesDlg->SendInstantMessage(NULL, message, number);
        }
    }
}

LRESULT CmainDlg::onPager(WPARAM wParam, LPARAM lParam)
{
    CString* number = (CString*)wParam;
    CString* message = (CString*)lParam;
    MessagesIncoming(number, message);
    delete number;
    delete message;
    return 0;
}

LRESULT CmainDlg::onPagerStatus(WPARAM wParam, LPARAM lParam)
{
    CString* number = (CString*)wParam;
    CString* message = (CString*)lParam;
    bool doNotShowMessagesWindow = !isVisible();
    MessagesContact* messagesContact = messagesDlg->AddTab(*number,
        FALSE, NULL, NULL,
        doNotShowMessagesWindow);
    if (messagesContact) {
        messagesDlg->AddMessage(messagesContact, *message);
    }
    delete number;
    delete message;
    return 0;
}

void CmainDlg::MessagesIncoming(CString* number, CString* message, CTime* pTime)
{
    bool doNotShowMessagesWindow = !isVisible();
    if (doNotShowMessagesWindow) {
        newMessages = true;
    }
    MessagesContact* messagesContact = messagesDlg->AddTab(*number,
        FALSE, NULL, NULL,
        doNotShowMessagesWindow
    );
    if (messagesContact) {
        messagesDlg->AddMessage(messagesContact, *message, MSIP_MESSAGE_TYPE_REMOTE, FALSE, pTime);
        onPlayerPlay(MSIP_SOUND_MESSAGE_IN, 0);
    }
}

LRESULT CmainDlg::WindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case UM_CREATED:
        return OnCreated(wParam, lParam);
    case UM_UPDATEWINDOWTEXT:
        return OnUpdateWindowText(wParam, lParam);
    case UM_ON_REG_STATE2:
        return onRegState2(wParam, lParam);
    case UM_ON_ACCOUNT:
        OnAccount(wParam, lParam);
        return 0;
    case UM_CALL_ANSWER:
        return onCallAnswer(wParam, lParam);
    case UM_CALL_HANGUP:
        return onCallHangup(wParam, lParam);
    case UM_ON_PLAYER_STOP:
        onPlayerStop();
        return 0;
    case UM_REFRESH_LEVELS:
        return onRefreshLevels(wParam, lParam);
    case UM_ON_INCOMING_CALL:
        return onIncomingCall(wParam, lParam);
    case UM_CREATE_RINGING:
        return onCreateRingingDlg(wParam, lParam);
    case UM_ON_CALL_STATE:
        return onCallState(wParam, lParam);
    case UM_ON_CALL_MEDIA_STATE:
        return onCallMediaState(wParam, lParam);
    case UM_NOTIFYICON:
        return onTrayNotify(wParam, lParam);
    case UM_ON_CALL_TRANSFER_STATUS:
        return onCallTransferStatus(wParam, lParam);
    case UM_ON_MWI_INFO:
        return onMWIInfo(wParam, lParam);
    case UM_ON_BUDDY_STATE:
        return onBuddyState(wParam, lParam);
    case UM_USERS_DIRECTORY:
        return onUsersDirectoryLoaded(wParam, lParam);
    case UM_RESTART:
        return OnRestart(wParam, lParam);
    case UM_NETWORK_CHANGE:
        return OnNetworkChange(wParam, lParam);
    case UM_RECREATE:
        return OnRecreate(wParam, lParam);
    case UM_UPDATE_CHECKER_LOADED:
        return OnUpdateCheckerLoaded(wParam, lParam);
    case WM_DEVICECHANGE:
        return OnDeviceChange(wParam, lParam);
    case UM_ON_COMMAND_LINE:
        return onCommandLine(wParam, lParam);
    case UM_ON_PAGER:
        return onPager(wParam, lParam);
    case UM_ON_PAGER_STATUS:
        return onPagerStatus(wParam, lParam);
    case WM_TIMER:
        OnTimer(wParam);
        return 0;
    default:
        return 0;
    }
}
