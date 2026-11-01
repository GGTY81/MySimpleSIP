/*
 * MySimpleSIP — softphone SIP para Linux baseado no MicroSIP.
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

// Ponto de entrada (equivale a CmicrosipApp::InitInstance do MicroSIP, em port).

#include "version.h"

#include "KeychainPasswordStore.h"
#include "QtMessageBackend.h"
#include "UiCommon.h"
#include "langpack.h"
#include "AccountDlg.h"
#include "MessagesDlg.h"
#include "RinginDlg.h"
#include "Contacts.h"
#include "Transfer.h"
#include "SettingsDlg.h"
#include "ShortcutsDlg.h"
#include "ExportDlg.h"
#include "mainDlg.h"

#include "Paths.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QLocalServer>
#include <QMessageBox>
#include <QProcess>
#include <QLocalSocket>
#include <QCommandLineParser>
#include <QTimer>

#include <cstdio>
#include <stdexcept>
#include <unistd.h>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral(MYSIMPLESIP_NAME));
    QApplication::setApplicationVersion(QStringLiteral(MYSIMPLESIP_VERSION));
    QApplication::setDesktopFileName(QStringLiteral("mysimplesip"));
    // a janela principal fecha para a bandeja; o app só termina em "Sair"
    QApplication::setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Softphone SIP para Linux baseado no MicroSIP"));
    parser.addHelpOption();
    QCommandLineOption versionOption(QStringList{QStringLiteral("v"), QStringLiteral("version")},
                                     QStringLiteral("Mostra as versões e sai."));
    QCommandLineOption minimizedOption(QStringLiteral("minimized"), QStringLiteral("Inicia minimizado na bandeja (/minimized)."));
    QCommandLineOption quitOption(QStringLiteral("quit-after"),
                                  QStringLiteral("Encerra após <ms> milissegundos (testes)."), QStringLiteral("ms"));
    QCommandLineOption screenshotOption(QStringLiteral("screenshot"),
                                        QStringLiteral("Salva uma imagem da janela em <arquivo> e encerra (testes)."),
                                        QStringLiteral("arquivo"));
    parser.addOptions({versionOption, minimizedOption, quitOption, screenshotOption});
    parser.addPositionalArgument(QStringLiteral("comando"),
                                 QStringLiteral("Como no MicroSIP: número ou URI (sip:, tel:, callto:) para discar, "
                                                "/minimized, /exit, /answer, /hangupall, /hangupincoming, /hangupcalling, "
                                                "/dtmf:N, /transfer:N, /account:N, msip:answer, msip:hold, msip:micmute..., "
                                                "/reset, /resetnoask."),
                                 QStringLiteral("[comando...]"));
    parser.process(app);

    if (parser.isSet(versionOption)) {
        std::printf("%s %s (base MicroSIP %s)\npjproject %s\nQt %s\n", MYSIMPLESIP_NAME, MYSIMPLESIP_VERSION,
                    MSIP_BASE_VERSION, pj_get_version(), qVersion());
        return 0;
    }
    // theApp.m_lpCmdLine: os argumentos (sem as opções do Qt) unidos por espaço
    QString cmdLine = parser.positionalArguments().join(QLatin1Char(' ')).trimmed();
    cmdLine.replace(QLatin1Char('\n'), QLatin1Char(' '));
    if (parser.isSet(minimizedOption) && cmdLine.isEmpty())
        cmdLine = QStringLiteral("/minimized");
    msip_command_line = CS(cmdLine);

    // Instância única (no MicroSIP: EnumWindows + WM_COPYDATA para a janela já aberta).
    // Um servidor local por usuário e por arquivo INI; a segunda execução manda uma linha à
    // primeira e termina: "show" (mostrar), "exit" (/exit → ID_EXIT) ou "cmd <linha de
    // comando>" (WM_COPYDATA: CmainDlg::CommandLine, que decide se mostra a janela).
    const AppPaths paths = ResolveAppPaths(_T(_GLOBAL_NAME_NICE));
    const QString serverName = QStringLiteral("mysimplesip-%1-%2")
                                   .arg(getuid())
                                   .arg(QString::fromLatin1(QCryptographicHash::hash(QS(paths.iniFile).toUtf8(), QCryptographicHash::Md5).toHex().left(12)));
    auto sendToRunning = [&serverName](const QString& message) -> bool {
        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (!socket.waitForConnected(500))
            return false;
        if (!message.isEmpty()) {
            socket.write(message.toUtf8() + '\n');
            socket.waitForBytesWritten(1000);
        }
        socket.disconnectFromServer();
        if (socket.state() != QLocalSocket::UnconnectedState)
            socket.waitForDisconnected(1000);
        return true;
    };
    const bool cmdReset = cmdLine == QLatin1String("/reset");
    const bool cmdResetNoAsk = cmdLine == QLatin1String("/resetnoask");
    if (cmdReset || cmdResetNoAsk) {
        LoadLangPackModule();
        KeychainPasswordStore passwordStore;
        SetPasswordStore(&passwordStore);
        sendToRunning(QStringLiteral("exit")); // ::SendMessage(hWndRunning, WM_CLOSE)
        accountSettings.Init(); // com "/reset" só resolve os caminhos
        if (cmdResetNoAsk
            || QMessageBox::question(nullptr, QStringLiteral(_GLOBAL_NAME_NICE),
                                     TR(_T("Do you want to delete user data and program settings? This action cannot be undone.")),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
            if (cmdResetNoAsk
                || QMessageBox::question(nullptr, QStringLiteral(_GLOBAL_NAME_NICE), TR(_T("Are you sure you want to delete?")),
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
                msip_reset_user_data();
            }
        }
        SetPasswordStore(nullptr);
        return 0;
    }
    if (cmdLine == QLatin1String("/exit")) {
        sendToRunning(QStringLiteral("exit"));
        return 0;
    }
    if (cmdLine == QLatin1String("/minimized") ? sendToRunning(QString())
                                                : sendToRunning(cmdLine.isEmpty() ? QStringLiteral("show") : QStringLiteral("cmd ") + cmdLine)) {
        return 0;
    }
    if (cmdLine == QLatin1String("/answer") || cmdLine == QLatin1String("/hangupall")) {
        return 0;
    }
    QLocalServer::removeServer(serverName);
    QLocalServer instanceServer;
    instanceServer.listen(serverName);

    LoadLangPackModule();

    // senhas das contas no chaveiro do sistema (deve vir antes de Init, que lê as contas)
    KeychainPasswordStore passwordStore;
    SetPasswordStore(&passwordStore);
    try {
        accountSettings.Init();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    QtMessageBackend backend;
    auto* dlg = new CmainDlg(&backend, parser.isSet(minimizedOption));
    QObject::connect(&instanceServer, &QLocalServer::newConnection, dlg, [&instanceServer, dlg] {
        while (QLocalSocket* client = instanceServer.nextPendingConnection()) {
            // uma linha por conexão (ver sendToRunning); conexão sem texto: nada a fazer
            auto readLine = [client, dlg] {
                while (client->canReadLine()) {
                    const QString line = QString::fromUtf8(client->readLine()).trimmed();
                    dlg->OnInstanceMessage(line);
                }
            };
            QObject::connect(client, &QLocalSocket::readyRead, dlg, readLine);
            QObject::connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
            readLine();
        }
    });

    if (parser.isSet(screenshotOption)) {
        const QString file = parser.value(screenshotOption);
        // MSIP_SCREENSHOT_DIALOG=account captura o diálogo de conta (nova conta);
        // =messages, a janela de chamadas com uma aba de exemplo; =ringin, a de chamada recebida;
        // =contact, o diálogo de contato; =transfer, o de transferência;
        // =settings, as Configurações; =shortcuts, os Atalhos; =export, Exportar configuração
        const QByteArray which = qgetenv("MSIP_SCREENSHOT_DIALOG");
        QTimer::singleShot(2500, dlg, [dlg, file, which] {
            QWidget* target = dlg;
            if (which == "account") {
                dlg->OnMenuAccountAdd();
                target = dlg->accountDlg;
            }
            else if (which == "messages") {
                MessagesContact* c = dlg->messagesDlg->AddTab(_T("sip:1001@example.com"), TRUE, NULL, NULL, FALSE, FALSE, _T("1001"), _T("Alice"));
                dlg->messagesDlg->AddMessage(c, Translate(_T("Call Ended")), MSIP_MESSAGE_TYPE_SYSTEM, TRUE);
                dlg->messagesDlg->AddMessage(c, _T("Olá! Veja https://example.com"), MSIP_MESSAGE_TYPE_REMOTE, TRUE);
                dlg->messagesDlg->AddTab(_T("sip:1002@example.com"), FALSE, NULL, NULL, TRUE, FALSE, _T("1002"));
                target = dlg->messagesDlg;
            }
            else if (which == "contact") {
                dlg->pageContacts->OnMenuAdd();
                Contact c;
                c.name = _T("Maria Souza");
                c.number = _T("1004");
                c.firstname = _T("Maria");
                c.lastname = _T("Souza");
                c.mobile = _T("5511999990000");
                c.starred = true;
                dlg->pageContacts->addDlg->Load(&c);
                target = dlg->pageContacts->addDlg;
            }
            else if (which == "transfer") {
                Contact c;
                c.name = _T("Suporte");
                c.number = _T("300");
                c.starred = true;
                dlg->pageContacts->ContactAdd(c);
                c.name = _T("Vendas");
                c.number = _T("301");
                c.mobile = _T("5511999990000");
                dlg->pageContacts->ContactAdd(c);
                dlg->OpenTransferDlg(dlg, MSIP_ACTION_TRANSFER);
                target = dlg->transferDlg;
            }
            else if (which == "settings") {
                dlg->OnMenuSettings();
                target = dlg->settingsDlg;
            }
            else if (which == "shortcuts") {
                dlg->OnMenuShortcuts();
                target = dlg->shortcutsDlg;
            }
            else if (which == "export") {
                dlg->OnMenuExport();
                target = dlg->exportDlg;
            }
            else if (which == "ringin") {
                auto* r = new RinginDlg(dlg, false);
                r->SetCallerName(_T("Alice"));
                r->SetCallerAddr(CString(_T("1001@example.com\r\nMicroSIP/3.22\r\n\r\n")) + Translate(_T("To")) + _T(": 2000@example.com\r\n"));
                target = r;
            }
            QTimer::singleShot(500, dlg, [dlg, file, target] {
                target->grab().save(file);
                dlg->OnClose();
            });
        });
    }
    if (parser.isSet(quitOption))
        QTimer::singleShot(parser.value(quitOption).toInt(), dlg, [dlg] { dlg->OnClose(); });

    int rc = app.exec();
    delete dlg;
    SetPasswordStore(nullptr);
    UnloadLangPackModule();
    if (msip_restart_requested) {
        // OnRestart (mudança na posição ou na quantidade de atalhos): libera a instância
        // única e abre o aplicativo de novo, com os mesmos argumentos
        instanceServer.close();
        QProcess::startDetached(QCoreApplication::applicationFilePath(), app.arguments().mid(1));
    }
    return rc;
}
