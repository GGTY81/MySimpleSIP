/*
 * MySimpleSIP — diálogo de conta SIP (port Qt do AccountDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de AccountDlg.cpp (MicroSIP 3.22.16). Os controles vêm do IDD_ACCOUNT original
// (CDialogLayout); a lógica de Load/OnBnClickedOk/excluir segue o original.
// Diferenças marcadas com "MySimpleSIP:".

#include "AccountDlg.h"

#include "UiCommon.h"
#include "langpack.h"
#include "mainDlg.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

#include <netdb.h>
#include <unistd.h>

static CString transportItems[] = {
    _T("udp"),
    _T("tcp"),
    _T(""),
    _T("tls"),
};
static CString transportValues[] = {
    _T("UDP"),
    _T("TCP"),
    _T("UDP+TCP"),
    _T("TLS"),
};

static CString cidItems[] = {
    _T(""),
    _T("soft"),
    _T("hard"),
};
static CString cidValues[] = {
    _T("Disabled"),
    _T("Soft"),
    _T("Hard"),
};

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

AccountDlg::AccountDlg(QWidget* parent) : QWidget(parent, Qt::Dialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    accountId = -1;
    m_layout.Build(this, QStringLiteral("IDD_ACCOUNT"));
    OnInitDialog();
}

AccountDlg::~AccountDlg()
{
    // como OnDestroy: mainDlg->accountDlg = nullptr
    if (MainDlg() && MainDlg()->accountDlg == this)
        MainDlg()->accountDlg = nullptr;
}

void AccountDlg::OnInitDialog()
{
    CString str;
    str.Format(_T("<a href=\"#\">%s</a>"), Translate(_T("Delete Account")));
    m_layout.Label(QStringLiteral("IDC_SYSLINK_ACCOUNT_DELETE"))->setText(QS(str));

    str.Format(_T("<a href=\"#\">%s</a>"), Translate(_T("display password")));
    m_layout.Label(QStringLiteral("IDC_SYSLINK_DISPLAY_PASSWORD"))->setText(QS(str));

    if (langPack.rtl)
        setLayoutDirection(Qt::RightToLeft);

    m_layout.TranslateDialog(this);

    QComboBox* combobox;
    int n;

    combobox = m_layout.Combo(QStringLiteral("IDC_ACCOUNT_HIDE_CID"));
    n = sizeof(cidItems) / sizeof(cidItems[0]);
    for (int i = 0; i < n; i++) {
        combobox->addItem(QS(Translate(cidValues[i])));
    }
    combobox->setCurrentIndex(0);

    combobox = m_layout.Combo(QStringLiteral("IDC_SRTP"));
    combobox->addItem(QS(Translate(_T("Disabled"))));
    str.Format(_T("%s SRTP (RTP/AVP)"), Translate(_T("Optional")));
    combobox->addItem(QS(str));
    str.Format(_T("%s SRTP (RTP/SAVP)"), Translate(_T("Mandatory")));
    combobox->addItem(QS(str));
    combobox->addItem(QStringLiteral("DTLS-SRTP/SRTP"));
    combobox->addItem(QStringLiteral("DTLS-SRTP"));
    combobox->setCurrentIndex(0);

    combobox = m_layout.Combo(QStringLiteral("IDC_TRANSPORT"));
    n = sizeof(transportItems) / sizeof(transportItems[0]);
    for (int i = 0; i < n; i++) {
        combobox->addItem(QS(Translate(transportValues[i])));
    }
    combobox->setCurrentIndex(0);

    combobox = m_layout.Combo(QStringLiteral("IDC_PUBLIC_ADDR"));
    combobox->addItem(QS(Translate(_T("Auto"))));
    char hostname[256] = {};
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        addrinfo hints = {};
        hints.ai_family = AF_INET;
        addrinfo* result = nullptr;
        if (getaddrinfo(hostname, nullptr, &hints, &result) == 0) {
            char ip[NI_MAXHOST];
            for (addrinfo* ai = result; ai; ai = ai->ai_next) {
                if (getnameinfo(ai->ai_addr, static_cast<socklen_t>(ai->ai_addrlen), ip, sizeof(ip), nullptr, 0,
                                NI_NUMERICHOST) == 0) {
                    combobox->addItem(QString::fromLatin1(ip));
                }
            }
            freeaddrinfo(result);
        }
    }
    combobox->setCurrentIndex(0);
    if (accountSettings.enableSTUN && !accountSettings.stun.IsEmpty()) {
        combobox->setEnabled(false);
    }

    // --- ligações (mapa de mensagens do original) ---
    QObject::connect(m_layout.Button(QStringLiteral("IDOK")), &QPushButton::clicked, this, [this] { OnBnClickedOk(); });
    QObject::connect(m_layout.Button(QStringLiteral("IDCANCEL")), &QPushButton::clicked, this, [this] { close(); });
    QObject::connect(m_layout.Label(QStringLiteral("IDC_SYSLINK_ACCOUNT_DELETE")), &QLabel::linkActivated, this,
                     [this] { OnNMClickSyslinkDelete(); });
    QObject::connect(m_layout.Label(QStringLiteral("IDC_SYSLINK_DISPLAY_PASSWORD")), &QLabel::linkActivated, this,
                     [this] { OnNMClickSyslinkDisplayPasswod(); });
    QObject::connect(m_layout.Edit(QStringLiteral("IDC_EDIT_PASSWORD")), &QLineEdit::textChanged, this,
                     [this] { OnChangePassword(); });

    // links "?" de ajuda (OnNMClickSyslink*): abrem o manual na seção do campo
    static const struct {
        const char* id;
        LPCTSTR code;
    } helpLinks[] = {
        {"IDC_SYSLINK_SIP_SERVER", _T("sipServer")}, {"IDC_SYSLINK_SIP_PROXY", _T("sipProxy")},
        {"IDC_SYSLINK_USERNAME", _T("username")}, {"IDC_SYSLINK_DOMAIN", _T("domain")},
        {"IDC_SYSLINK_AUTHID", _T("login")}, {"IDC_SYSLINK_PASSWORD", _T("password")},
        {"IDC_SYSLINK_NAME", _T("name")}, {"IDC_SYSLINK_VOICEMAIL", _T("voicemail")},
        {"IDC_ACCOUNT_HELP_DIALING_PREFIX", _T("dialingPrefix")}, {"IDC_ACCOUNT_HELP_DIAL_PLAN", _T("dialPlan")},
        {"IDC_ACCOUNT_HELP_HIDE_CID", _T("hideCID")}, {"IDC_SYSLINK_ENCRYPTION", _T("encryption")},
        {"IDC_SYSLINK_TRANSPORT", _T("transport")}, {"IDC_SYSLINK_PUBLIC_ADDRESS", _T("publicAddress")},
        {"IDC_SYSLINK_PUBLISH_PRESENCE", _T("publishPresence")}, {"IDC_SYSLINK_ICE", _T("ice")},
        {"IDC_SYSLINK_REWRITE", _T("allowRewrite")}, {"IDC_SYSLINK_SESSION_TIMER", _T("sessionTimers")},
    };
    for (const auto& h : helpLinks) {
        QLabel* link = m_layout.Label(QString::fromLatin1(h.id));
        if (!link)
            continue;
        LPCTSTR code = h.code;
        link->setProperty("msipHelpCode", QS(code)); // MySimpleSIP: conferido nos testes
        QObject::connect(link, &QLabel::linkActivated, this, [code] { OpenHelp(code); });
    }
}

void AccountDlg::FocusPassword()
{
    QLineEdit* edit = m_layout.Edit(QStringLiteral("IDC_EDIT_PASSWORD"));
    edit->setFocus();
    edit->setCursorPosition(edit->text().size());
}

void AccountDlg::Load(int id)
{
    QComboBox* combobox;
    CString str;
    int i;
    int n;
    bool found;

    accountId = id;
    if (accountSettings.AccountLoad(id, &m_Account) || id == 0) {
        accountId = id;
        if (accountId && accountSettings.accountId == accountId && !accountSettings.account.rememberPassword) {
            m_Account.username = accountSettings.account.username;
            m_Account.password = accountSettings.account.password;
            m_Account.rememberPassword = false;
        }
    }
    else {
        accountId = -1;
    }

    bool isEdit = (accountId > 0 && (!m_Account.username.IsEmpty() || accountId > 1));

    const bool show = id != 0;
    DLG_ITEM(m_layout, IDC_ACCOUNT_REQUIRED_USERNAME)->setVisible(show);
    DLG_ITEM(m_layout, IDC_ACCOUNT_REQUIRED_DOMAIN)->setVisible(show);
    DLG_ITEM(m_layout, IDC_EDIT_SERVER)->setEnabled(id);
    DLG_ITEM(m_layout, IDC_PUBLISH)->setEnabled(id);
    DLG_ITEM(m_layout, IDC_ACCOUNT_REGISTER_REFRESH)->setEnabled(id);
    DLG_ITEM(m_layout, IDC_ACCOUNT_KEEP_ALIVE)->setEnabled(id);
    DLG_ITEM(m_layout, IDC_REWRITE)->setEnabled(id);

    m_layout.Edit(QStringLiteral("IDC_ACCOUNT_LABEL"))->setText(QS(m_Account.label));
    m_layout.Edit(QStringLiteral("IDC_EDIT_SERVER"))->setText(QS(m_Account.server));
    m_layout.Edit(QStringLiteral("IDC_EDIT_PROXY"))->setText(QS(m_Account.proxy));
    m_layout.Edit(QStringLiteral("IDC_EDIT_DOMAIN"))->setText(QS(m_Account.domain));
    m_layout.Edit(QStringLiteral("IDC_EDIT_AUTHID"))->setText(QS(m_Account.authID));
    m_layout.Edit(QStringLiteral("IDC_EDIT_USERNAME"))->setText(QS(m_Account.username));

    QLineEdit* edit = m_layout.Edit(QStringLiteral("IDC_EDIT_PASSWORD"));
    DLG_ITEM(m_layout, IDC_SYSLINK_DISPLAY_PASSWORD)->setVisible(accountId == -1 || m_Account.password.IsEmpty());
    edit->setEchoMode(QLineEdit::Password);
    edit->setText(QS(m_Account.password));

    m_layout.Edit(QStringLiteral("IDC_EDIT_DISPLAYNAME"))->setText(QS(m_Account.displayName));
    m_layout.Edit(QStringLiteral("IDC_EDIT_VOICEMAIL"))->setText(QS(m_Account.voicemailNumber));
    m_layout.Edit(QStringLiteral("IDC_ACCOUNT_DIALING_PREFIX"))->setText(QS(m_Account.dialingPrefix));
    m_layout.Edit(QStringLiteral("IDC_ACCOUNT_DIAL_PLAN"))->setText(QS(m_Account.dialPlan));

    combobox = m_layout.Combo(QStringLiteral("IDC_ACCOUNT_HIDE_CID"));
    n = sizeof(cidItems) / sizeof(cidItems[0]);
    found = false;
    for (int i = 0; i < n; i++) {
        if (m_Account.hideCID == cidItems[i]) {
            combobox->setCurrentIndex(i);
            found = true;
        }
    }
    if (!found) {
        combobox->setCurrentIndex(0);
    }

    combobox = m_layout.Combo(QStringLiteral("IDC_SRTP"));
    if (m_Account.srtp == _T("optional")) {
        i = 1;
    }
    else if (m_Account.srtp == _T("mandatory")) {
        i = 2;
    }
    else if (m_Account.srtp == _T("dtls-sdes")) {
        i = 3;
    }
    else if (m_Account.srtp == _T("dtls")) {
        i = 4;
    }
    else {
        i = 0;
    }
    if (i > 0) {
        combobox->setCurrentIndex(i);
    }

    combobox = m_layout.Combo(QStringLiteral("IDC_TRANSPORT"));
    n = sizeof(transportItems) / sizeof(transportItems[0]);
    found = false;
    for (int i = 0; i < n; i++) {
        if (m_Account.transport == transportItems[i]) {
            combobox->setCurrentIndex(i);
            found = true;
        }
    }
    if (!found) {
        combobox->setCurrentIndex(0);
    }

    combobox = m_layout.Combo(QStringLiteral("IDC_PUBLIC_ADDR"));
    if (combobox->isEnabled()) {
        str = get_public_addr(&m_Account);
        if (!str.IsEmpty()) {
            combobox->setEditText(QS(str));
        }
    }

    str.Format(_T("%d"), m_Account.registerRefresh);
    m_layout.Edit(QStringLiteral("IDC_ACCOUNT_REGISTER_REFRESH"))->setText(QS(str));
    str.Format(_T("%d"), m_Account.keepAlive);
    m_layout.Edit(QStringLiteral("IDC_ACCOUNT_KEEP_ALIVE"))->setText(QS(str));

    m_layout.Check(QStringLiteral("IDC_PUBLISH"))->setChecked(m_Account.publish);
    m_layout.Check(QStringLiteral("IDC_REWRITE"))->setChecked(m_Account.allowRewrite);
    m_layout.Check(QStringLiteral("IDC_ICE"))->setChecked(m_Account.ice);
    m_layout.Check(QStringLiteral("IDC_SESSION_TIMER"))->setChecked(m_Account.disableSessionTimer);
    DLG_ITEM(m_layout, IDC_SYSLINK_ACCOUNT_DELETE)->setVisible(isEdit);
}

void AccountDlg::OnBnClickedOk()
{
    CString str;
    QComboBox* combobox;
    int i;

    auto text = [this](const char* id) { return CS(m_layout.Edit(QString::fromLatin1(id))->text()).Trim(); };

    m_Account.label = text("IDC_ACCOUNT_LABEL");
    m_Account.server = text("IDC_EDIT_SERVER");
    m_Account.proxy = text("IDC_EDIT_PROXY");
    m_Account.domain = text("IDC_EDIT_DOMAIN");
    m_Account.authID = text("IDC_EDIT_AUTHID");
    m_Account.username = text("IDC_EDIT_USERNAME");
    m_Account.password = text("IDC_EDIT_PASSWORD");
    m_Account.displayName = text("IDC_EDIT_DISPLAYNAME");
    m_Account.voicemailNumber = text("IDC_EDIT_VOICEMAIL");
    m_Account.dialingPrefix = text("IDC_ACCOUNT_DIALING_PREFIX");
    m_Account.dialPlan = text("IDC_ACCOUNT_DIAL_PLAN");

    combobox = m_layout.Combo(QStringLiteral("IDC_ACCOUNT_HIDE_CID"));
    m_Account.hideCID = cidItems[combobox->currentIndex()];

    combobox = m_layout.Combo(QStringLiteral("IDC_SRTP"));
    i = combobox->currentIndex();
    switch (i) {
    case 1:
        m_Account.srtp = _T("optional");
        break;
    case 2:
        m_Account.srtp = _T("mandatory");
        break;
    case 3:
        m_Account.srtp = _T("dtls-sdes");
        break;
    case 4:
        m_Account.srtp = _T("dtls");
        break;
    default:
        m_Account.srtp = _T("");
    }

    combobox = m_layout.Combo(QStringLiteral("IDC_TRANSPORT"));
    m_Account.transport = transportItems[combobox->currentIndex()];

    combobox = m_layout.Combo(QStringLiteral("IDC_PUBLIC_ADDR"));
    if (combobox->isEnabled()) {
        m_Account.publicAddr = CS(combobox->currentText());
        if (m_Account.publicAddr == Translate(_T("Auto"))) {
            m_Account.publicAddr = _T("");
        }
    }

    m_Account.rememberPassword = 1;

    m_Account.registerRefresh = _wtoi(CS(m_layout.Edit(QStringLiteral("IDC_ACCOUNT_REGISTER_REFRESH"))->text()));
    if (m_Account.registerRefresh <= 0) {
        m_Account.registerRefresh = PJSUA_REG_INTERVAL;
    }

    m_Account.keepAlive = _wtoi(CS(m_layout.Edit(QStringLiteral("IDC_ACCOUNT_KEEP_ALIVE"))->text()));
    if (m_Account.keepAlive < 0) {
        m_Account.keepAlive = 15;
    }

    m_Account.publish = m_layout.Check(QStringLiteral("IDC_PUBLISH"))->isChecked();
    m_Account.allowRewrite = m_layout.Check(QStringLiteral("IDC_REWRITE"))->isChecked();
    m_Account.ice = m_layout.Check(QStringLiteral("IDC_ICE"))->isChecked();
    m_Account.disableSessionTimer = m_layout.Check(QStringLiteral("IDC_SESSION_TIMER"))->isChecked();

    if (m_Account.domain.IsEmpty() || m_Account.username.IsEmpty()) {
        if (accountId != 0) {
            CString str;
            str.Append(Translate(_T("Please fill out at least the required fields marked with *.")));
            str.AppendFormat(_T(" %s"), Translate(_T("Ask your SIP provider how to configure the account correctly.")));
            QMessageBox::warning(this, QStringLiteral(_GLOBAL_NAME_NICE), QS(str));
            return;
        }
    }

    hide();
    MainDlg()->accountDlg = nullptr;

    if (accountId == -1) { // find id for new account
        Account dummy;
        int i = 1;
        while (true) {
            if (!accountSettings.AccountLoad(i, &dummy)) {
                break;
            }
            i++;
        }
        accountId = i;
    }

    accountSettings.AccountSave(accountId, &m_Account);

    if (accountId) {
        MainDlg()->PJAccountDelete(true);

        accountSettings.accountId = accountId;
        accountSettings.account = m_Account;
        accountSettings.AccountLoad(accountSettings.accountId, &accountSettings.account);
        if (!m_Account.rememberPassword) {
            accountSettings.account.username = m_Account.username;
            accountSettings.account.password = m_Account.password;
            accountSettings.account.rememberPassword = false;
        }
        MainDlg()->OnAccountChanged();
        MainDlg()->InitUI();
        accountSettings.SettingsSave();
        MainDlg()->PJAccountAdd();
    }
    else {
        // local account
        MainDlg()->PJAccountDeleteLocal();
        accountSettings.AccountLoad(0, &accountSettings.accountLocal);
        MainDlg()->PJAccountAddLocal();
    }
    close();
}

void AccountDlg::OnNMClickSyslinkDisplayPasswod()
{
    DLG_ITEM(m_layout, IDC_SYSLINK_DISPLAY_PASSWORD)->hide();
    QLineEdit* edit = m_layout.Edit(QStringLiteral("IDC_EDIT_PASSWORD"));
    edit->setEchoMode(QLineEdit::Normal);
    FocusPassword();
}

void AccountDlg::OnNMClickSyslinkDelete()
{
    if (accountId > 0
        && QMessageBox::question(this, QStringLiteral(_GLOBAL_NAME_NICE), TR(_T("Are you sure you want to delete?")),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        hide();
        MainDlg()->accountDlg = nullptr;

        Account account;
        int i = accountId;
        while (true) {
            if (!accountSettings.AccountLoad(i + 1, &account)) {
                break;
            }
            accountSettings.AccountSave(i, &account);
            i++;
        }
        accountSettings.AccountDelete(i);
        if (accountId && accountId == accountSettings.accountId) {
            MainDlg()->PJAccountDelete(true);
            if (i > 1) {
                accountSettings.accountId = 1;
                accountSettings.AccountLoad(accountSettings.accountId, &accountSettings.account);
                MainDlg()->OnAccountChanged();
                MainDlg()->InitUI();
                MainDlg()->PJAccountAdd();
            }
            else {
                accountSettings.accountId = 0;
                MainDlg()->OnAccountChanged();
                MainDlg()->InitUI();
            }
            accountSettings.SettingsSave();
        }
        close();
    }
}

void AccountDlg::OnChangePassword()
{
    QLineEdit* edit = m_layout.Edit(QStringLiteral("IDC_EDIT_PASSWORD"));
    if (edit->text().isEmpty()) {
        DLG_ITEM(m_layout, IDC_SYSLINK_DISPLAY_PASSWORD)->show();
        edit->setEchoMode(QLineEdit::Password);
    }
}
