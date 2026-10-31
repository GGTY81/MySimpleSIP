// Teste de interface (offscreen) do MessagesDlg: abas por contato, mensagens do sistema,
// botões sem chamada e fechamento das abas (sem pjsua rodando).

#include "Calls.h"
#include "Dialer.h"
#include "UiCommon.h"
#include "MessagesDlg.h"
#include "RinginDlg.h"
#include "PasswordStore.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QLabel>
#include <QPointer>
#include <QElapsedTimer>
#include <QFile>
#include <QProcess>
#include <QScopeGuard>
#include <QTest>
#include <QPushButton>
#include <QTabBar>
#include <QTextBrowser>
#include <QTreeWidget>

#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <fstream>

namespace fs = std::filesystem;

TEST(MessagesDlg, AbasMensagensEBotoes)
{
    char tmpl[] = "/tmp/msip-messages-XXXXXX";
    const fs::path root = mkdtemp(tmpl);
    setenv("XDG_CONFIG_HOME", (root / "c").c_str(), 1);
    setenv("XDG_DATA_HOME", (root / "d").c_str(), 1);
    setenv("XDG_STATE_HOME", (root / "s").c_str(), 1);
    qputenv("QT_QPA_PLATFORM", "offscreen");

    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);

    PrivateProfileFlushCache();
    SetPasswordStore(nullptr);
    ResetAccountSettings();
    accountSettings.Init();

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        MessagesDlg* m = dlg.messagesDlg;
        ASSERT_NE(m, nullptr);
        auto* tab = m->findChild<QTabBar*>(QStringLiteral("IDC_MESSAGES_TAB"));
        auto* list = m->findChild<QTextBrowser*>(QStringLiteral("IDC_MESSAGES_LIST"));
        auto* callEnd = m->findChild<QPushButton*>(QStringLiteral("IDC_CALL_END"));
        auto* end = m->findChild<QPushButton*>(QStringLiteral("IDC_END"));
        auto* closeAll = m->findChild<QPushButton*>(QStringLiteral("IDC_CLOSE_ALL"));
        ASSERT_TRUE(tab && list && callEnd && end && closeAll);

        // estado inicial do IDD_MESSAGES: sem abas, botões ocultos/desabilitados
        EXPECT_EQ(m->GetCount(), 0);
        EXPECT_TRUE(tab->isHidden());
        EXPECT_FALSE(closeAll->isEnabled());
        EXPECT_EQ(m->GetMessageContact(), nullptr);
        EXPECT_EQ(dlg.CurrentCallId(), -1);

        // AddTab (como MessagesOpen): nova aba ativa, título e botão "Chamar"
        MessagesContact* c1 = m->AddTab(_T("sip:1001@example.com"), TRUE, NULL, NULL, TRUE, FALSE, _T("1001"), _T("Alice"));
        ASSERT_NE(c1, nullptr);
        EXPECT_EQ(c1->aor, _T("1001@example.com"));
        EXPECT_EQ(c1->numberShort, _T("1001"));
        EXPECT_EQ(c1->callId, -1);
        EXPECT_EQ(m->GetCount(), 1);
        EXPECT_FALSE(tab->isHidden());
        EXPECT_TRUE(closeAll->isEnabled());
        EXPECT_EQ(m->GetMessageContact(), c1);
        EXPECT_EQ(m->windowTitle(), QString::fromUtf8("Alice – 1001"));
        EXPECT_FALSE(callEnd->isHidden());
        EXPECT_TRUE(end->isHidden());
        EXPECT_FALSE(m->isVisible()); // notShowWindow

        // mesmo AOR → mesma aba; outro contato → segunda aba ativada
        EXPECT_EQ(m->AddTab(_T("sip:1001@example.com"), TRUE), c1);
        MessagesContact* c2 = m->AddTab(_T("sip:1002@example.com"), TRUE, NULL, NULL, TRUE, FALSE, _T("1002"), _T("1002"));
        ASSERT_NE(c2, nullptr);
        EXPECT_EQ(m->GetCount(), 2);
        EXPECT_EQ(m->GetMessageContact(), c2);
        EXPECT_EQ(m->windowTitle(), QStringLiteral("1002"));
        EXPECT_EQ(m->GetCallsCount(), 0);

        // mensagem do sistema: no histórico da aba ativa; repetida em 2 s é ignorada
        m->AddMessage(c2, _T("Call Ended"));
        m->AddMessage(c2, _T("Call Ended"));
        EXPECT_EQ(list->toPlainText().count(QStringLiteral("Call Ended")), 1);
        // mensagem remota em aba inativa: ícone de nova mensagem, histórico separado
        m->AddMessage(c1, _T("veja https://example.com/x"), MSIP_MESSAGE_TYPE_REMOTE);
        EXPECT_TRUE(c1->hasNewMessages);
        EXPECT_TRUE(c1->messages.Find(_T("href=\"https://example.com/x\"")) != -1);
        EXPECT_FALSE(list->toPlainText().contains(QStringLiteral("example.com")));

        // trocar de aba (GotoTab): limpa o aviso e mostra o histórico do contato
        EXPECT_TRUE(m->GotoTab(0));
        EXPECT_EQ(m->GetMessageContact(), c1);
        EXPECT_FALSE(c1->hasNewMessages);
        EXPECT_TRUE(list->toPlainText().contains(QStringLiteral("https://example.com/x")));

        // fechar todas (sem chamadas): abas somem e os botões voltam ao estado inicial
        closeAll->click();
        EXPECT_EQ(m->GetCount(), 0);
        EXPECT_TRUE(tab->isHidden());
        EXPECT_FALSE(closeAll->isEnabled());
        EXPECT_TRUE(callEnd->isHidden());

        dlg.OnClose();
    }
    fs::remove_all(root);
}

// Chamada real pela janela, sem servidor: o msip-cli atende automaticamente em
// 127.0.0.1:5060 e o CmainDlg (porta 5070, sem placa de som) liga por IP, como no
// tests/sip-lab/loopback.sh. Opcional: MSIP_TEST_SIP=1 (precisa da porta UDP 5060 livre).
TEST(MessagesDlg, ChamadaPorIpComCli)
{
    if (!std::getenv("MSIP_TEST_SIP"))
        GTEST_SKIP() << "defina MSIP_TEST_SIP=1 (porta UDP 5060 livre) para a chamada de ponta a ponta";
    char tmpl[] = "/tmp/msip-messages-sip-XXXXXX";
    const fs::path root = mkdtemp(tmpl);
    setenv("XDG_CONFIG_HOME", (root / "c").c_str(), 1);
    setenv("XDG_DATA_HOME", (root / "d").c_str(), 1);
    setenv("XDG_STATE_HOME", (root / "s").c_str(), 1);
    qputenv("QT_QPA_PLATFORM", "offscreen");

    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);

    // atendedor: msip-cli com perfil próprio
    QProcess cli;
    const QString script = QString::fromStdString((root / "cli.txt").string());
    {
        QFile f(script);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("wait 12\nquit\n");
    }
    cli.setProcessChannelMode(QProcess::MergedChannels);
    cli.start(QStringLiteral(MSIP_CLI_PATH), {QStringLiteral("--profile"), QString::fromStdString((root / "cli").string()),
                                              QStringLiteral("--null-audio"), QStringLiteral("--auto-answer"),
                                              QStringLiteral("--script"), script});
    ASSERT_TRUE(cli.waitForStarted());
    QTest::qWait(1000);

    PrivateProfileFlushCache();
    SetPasswordStore(nullptr);
    ResetAccountSettings();
    accountSettings.Init();
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5070"), accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();
    ASSERT_EQ(accountSettings.sourcePort, 5070);

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); }); // ASSERT falho: encerra o pjsua antes de destruir a janela
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        MessagesDlg* m = dlg.messagesDlg;
        auto* end = m->findChild<QPushButton*>(QStringLiteral("IDC_END"));
        auto* list = m->findChild<QTextBrowser*>(QStringLiteral("IDC_MESSAGES_LIST"));

        // Discador → MakeCall → MessagesOpen/AddTab → Call → StartCall/PerformCall
        ASSERT_TRUE(dlg.MakeCall(_T("127.0.0.1")));
        MessagesContact* c = m->GetMessageContact();
        ASSERT_NE(c, nullptr);
        EXPECT_NE(c->callId, -1);
        EXPECT_EQ(accountSettings.lastCallNumber, _T("127.0.0.1"));

        // onCallState CONFIRMED + onCallMediaState: 1 chamada ativa, botão Encerrar visível
        ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(false, true) == 1; }, 8000))
            << cli.readAll().toStdString();
        EXPECT_EQ(m->GetCallsCount(), 1);
        EXPECT_FALSE(end->isHidden());
        EXPECT_EQ(dlg.CurrentCallId(), c->callId);
        EXPECT_TRUE(list->toPlainText().contains(QStringLiteral("Connected")) || list->toPlainText().contains(QString::fromUtf8("Conectado")));

        // espera pelo botão do MessagesDlg e retomada
        m->OnBnClickedHold();
        ASSERT_TRUE(QTest::qWaitFor([&] { return c->mediaStatus == PJSUA_CALL_MEDIA_LOCAL_HOLD; }, 5000));
        m->OnBnClickedHold();
        ASSERT_TRUE(QTest::qWaitFor([&] { return c->mediaStatus == PJSUA_CALL_MEDIA_ACTIVE; }, 5000));

        // Encerrar: OnEndCall encerra a aba da chamada; no modo de chamada única (padrão)
        // a aba é fechada (c deixa de existir)
        ASSERT_TRUE(accountSettings.singleMode);
        QTest::qWait(1500); // duração ≥ 1 s para aparecer no histórico
        end->click();
        ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCount() == 0; }, 5000));
        EXPECT_EQ(m->GetCallsCount(), 0);
        EXPECT_EQ(m->lastCall, nullptr);
        EXPECT_EQ(dlg.CurrentCallId(), -1);
        // msip_call_end → msip_call_hangup_fast já chama OnEndCall; o onCallState
        // (DISCONNECTED) chega depois e desabilita o REC
        ASSERT_NE(dlg.pageDialer->m_ButtonRec, nullptr);
        EXPECT_TRUE(QTest::qWaitFor([&] { return !dlg.pageDialer->m_ButtonRec->isEnabled(); }, 5000));

        // aba Histórico: chamada de saída gravada com duração e "Chamada encerrada"
        QTreeWidget* calls = dlg.pageCalls->List();
        ASSERT_EQ(calls->topLevelItemCount(), 1);
        EXPECT_EQ(calls->topLevelItem(0)->text(1), QStringLiteral("127.0.0.1"));
        EXPECT_EQ(calls->topLevelItem(0)->text(4), TR(_T("Call Ended")));
        EXPECT_FALSE(calls->topLevelItem(0)->text(3).isEmpty()) << calls->topLevelItem(0)->text(2).toStdString() << " / " << calls->topLevelItem(0)->text(4).toStdString();

        dlg.OnClose();
    }
    cli.waitForFinished(15000);
    fs::remove_all(root);
}

// Chamada recebida pela janela: o msip-cli (5060) liga duas vezes para o CmainDlg (5070,
// sem placa de som). A 1ª é atendida pelo botão Atender do RinginDlg e encerrada pela
// janela; a 2ª é recusada pelo botão Recusar. Opcional: MSIP_TEST_SIP=1.
TEST(RinginDlg, ChamadaRecebidaAtenderERecusar)
{
    if (!std::getenv("MSIP_TEST_SIP"))
        GTEST_SKIP() << "defina MSIP_TEST_SIP=1 (porta UDP 5060 livre) para a chamada de ponta a ponta";
    char tmpl[] = "/tmp/msip-ringin-sip-XXXXXX";
    const fs::path root = mkdtemp(tmpl);
    setenv("XDG_CONFIG_HOME", (root / "c").c_str(), 1);
    setenv("XDG_DATA_HOME", (root / "d").c_str(), 1);
    setenv("XDG_STATE_HOME", (root / "s").c_str(), 1);
    qputenv("QT_QPA_PLATFORM", "offscreen");

    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);

    PrivateProfileFlushCache();
    SetPasswordStore(nullptr);
    ResetAccountSettings();
    accountSettings.Init();
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5070"), accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();

    QtMessageBackend backend;
    CmainDlg dlg(&backend, true);
    auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); }); // ASSERT falho: encerra o pjsua antes de destruir a janela
    dlg.nullSoundDevice = true;
    ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));

    QProcess cli;
    const QString script = QString::fromStdString((root / "cli.txt").string());
    {
        QFile f(script);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("wait 1\ncall 127.0.0.1:5070\nwait 6\ncall 127.0.0.1:5070\nwait 5\nquit\n");
    }
    cli.setProcessChannelMode(QProcess::MergedChannels);
    cli.start(QStringLiteral(MSIP_CLI_PATH), {QStringLiteral("--profile"), QString::fromStdString((root / "cli").string()),
                                              QStringLiteral("--null-audio"), QStringLiteral("--script"), script});
    ASSERT_TRUE(cli.waitForStarted());

    // 1ª chamada: onIncomingCall → UM_CREATE_RINGING → RinginDlg com nome e endereço
    ASSERT_TRUE(QTest::qWaitFor([&] { return dlg.ringinDlgs.GetCount() == 1; }, 8000)) << cli.readAll().toStdString();
    RinginDlg* r = dlg.ringinDlgs.GetAt(0);
    EXPECT_NE(r->GetCallId(), PJSUA_INVALID_ID);
    EXPECT_FALSE(r->m_layout.Label(QStringLiteral("IDC_CALLER_NAME"))->text().isEmpty());
    EXPECT_TRUE(r->m_layout.Label(QStringLiteral("IDC_CALLER_ADDR"))->text().contains(QStringLiteral("127.0.0.1")));
    EXPECT_EQ(dlg.messagesDlg->GetCallsCount(), 0);      // tocando não conta
    EXPECT_EQ(dlg.messagesDlg->GetCallsCount(true), 1);  // withIncoming

    r->m_layout.Button(QStringLiteral("IDC_ANSWER"))->click();
    ASSERT_TRUE(QTest::qWaitFor([&] { return dlg.messagesDlg->GetCallsCount(false, true) == 1; }, 5000));
    EXPECT_EQ(dlg.ringinDlgs.GetCount(), 0); // onCallState (CONFIRMED) fecha o RinginDlg
    dlg.messagesDlg->OnBnClickedEnd();
    ASSERT_TRUE(QTest::qWaitFor([&] { return dlg.messagesDlg->GetCallsCount(true) == 0; }, 5000));

    // 2ª chamada: Recusar → 486, chamada perdida não marcada (recusada)
    ASSERT_TRUE(QTest::qWaitFor([&] { return dlg.ringinDlgs.GetCount() == 1; }, 8000)) << cli.readAll().toStdString();
    r = dlg.ringinDlgs.GetAt(0);
    r->m_layout.Button(QStringLiteral("IDC_DECLINE"))->click();
    EXPECT_EQ(dlg.ringinDlgs.GetCount(), 0);
    EXPECT_FALSE(dlg.callIdIncomingIgnore.IsEmpty());
    ASSERT_TRUE(QTest::qWaitFor([&] { return dlg.messagesDlg->GetCallsCount(true) == 0; }, 5000));

    // aba Histórico: 1ª recebida (atendida), 2ª recebida recusada ("Recusada"), mais nova em cima
    QTreeWidget* calls = dlg.pageCalls->List();
    ASSERT_TRUE(QTest::qWaitFor([&] { return calls->topLevelItemCount() == 2; }, 3000));
    EXPECT_EQ(calls->topLevelItem(0)->text(4), TR(_T("Declined")));
    EXPECT_EQ(calls->topLevelItem(1)->text(4), TR(_T("Call Ended")));

    cli.waitForFinished(15000);
    const QString log = QString::fromUtf8(cli.readAll());
    EXPECT_TRUE(log.contains(QStringLiteral("CONFIRMED"))) << log.toStdString();
    EXPECT_TRUE(log.contains(QStringLiteral("486"))) << log.toStdString();
    dlg.OnClose();
    fs::remove_all(root);
}

// RinginDlg sem pjsua: textos, estado dos botões e fechar = ignorar (sai de ringinDlgs).
TEST(RinginDlg, TextosEIgnorar)
{
    char tmpl[] = "/tmp/msip-ringin-XXXXXX";
    const fs::path root = mkdtemp(tmpl);
    setenv("XDG_CONFIG_HOME", (root / "c").c_str(), 1);
    setenv("XDG_DATA_HOME", (root / "d").c_str(), 1);
    setenv("XDG_STATE_HOME", (root / "s").c_str(), 1);
    qputenv("QT_QPA_PLATFORM", "offscreen");

    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);

    PrivateProfileFlushCache();
    SetPasswordStore(nullptr);
    ResetAccountSettings();
    accountSettings.Init();

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        QPointer<RinginDlg> r = new RinginDlg(&dlg, false);
        r->SetCallId(3);
        r->SetCallerName(_T("Alice"));
        r->SetCallerAddr(_T("1001@example.com\r\n\r\nPara: 2000@example.com\r\n"));
        dlg.ringinDlgs.Add(r);

        EXPECT_EQ(r->m_layout.Label(QStringLiteral("IDC_CALLER_NAME"))->text(), QStringLiteral("Alice"));
        EXPECT_TRUE(r->m_layout.Label(QStringLiteral("IDC_CALLER_ADDR"))->text().startsWith(QStringLiteral("1001@example.com\n")));
        EXPECT_TRUE(r->m_layout.Button(QStringLiteral("IDC_ANSWER"))->isEnabled());
        EXPECT_TRUE(r->m_layout.Button(QStringLiteral("IDC_DECLINE"))->isEnabled());
        EXPECT_FALSE(r->m_layout.Button(QStringLiteral("IDC_VIDEO"))->isEnabled()); // sem vídeo remoto
        EXPECT_FALSE(r->m_layout.Label(QStringLiteral("IDC_IGNORE"))->isHidden());   // noIgnoreCall = 0

        // segundo RinginDlg abre em cascata (+22, +22) em relação ao último
        RinginDlg* r2 = new RinginDlg(&dlg, true);
        EXPECT_EQ(r2->pos(), r->frameGeometry().topLeft() + QPoint(22, 22));
        EXPECT_TRUE(r2->m_layout.Button(QStringLiteral("IDC_VIDEO"))->isEnabled());
        r2->DestroyWindow();

        // fechar a janela (Esc / X) ignora a chamada: sai da lista e a janela é destruída
        r->OnClose();
        EXPECT_EQ(dlg.ringinDlgs.GetCount(), 0);
        EXPECT_TRUE(r->isHidden());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        EXPECT_TRUE(r.isNull());

        dlg.OnClose();
    }
    fs::remove_all(root);
}

// Gravação automática de uma chamada real (autoRecording=1, formato padrão = MP3): no
// fim fica só o .mp3 (convertido do WAV do pjsua). Opcional: MSIP_TEST_SIP=1.
TEST(Recording, ChamadaGravadaEmMp3)
{
    if (!std::getenv("MSIP_TEST_SIP"))
        GTEST_SKIP() << "defina MSIP_TEST_SIP=1 (porta UDP 5060 livre)";
    char tmpl[] = "/tmp/msip-recording-XXXXXX";
    const fs::path root = mkdtemp(tmpl);
    setenv("XDG_CONFIG_HOME", (root / "c").c_str(), 1);
    setenv("XDG_DATA_HOME", (root / "d").c_str(), 1);
    setenv("XDG_STATE_HOME", (root / "s").c_str(), 1);
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);

    QProcess cli;
    const QString script = QString::fromStdString((root / "cli.txt").string());
    {
        QFile f(script);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("wait 12\nquit\n");
    }
    cli.setProcessChannelMode(QProcess::MergedChannels);
    cli.start(QStringLiteral(MSIP_CLI_PATH), {QStringLiteral("--profile"), QString::fromStdString((root / "cli").string()),
                                              QStringLiteral("--null-audio"), QStringLiteral("--auto-answer"),
                                              QStringLiteral("--script"), script});
    ASSERT_TRUE(cli.waitForStarted());
    auto stopCli = qScopeGuard([&] {
        if (cli.state() != QProcess::NotRunning) {
            cli.kill();
            cli.waitForFinished(3000);
        }
    });
    QTest::qWait(1000);

    const fs::path rec = root / "rec";
    PrivateProfileFlushCache();
    SetPasswordStore(nullptr);
    ResetAccountSettings();
    accountSettings.Init();
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5070"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("autoRecording"), _T("1"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("recordingPath"), CString(rec.c_str()), accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();
    ASSERT_TRUE(accountSettings.autoRecording);
    ASSERT_NE(accountSettings.recordingFormat, _T("wav"));

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        ASSERT_TRUE(dlg.MakeCall(_T("127.0.0.1")));
        MessagesDlg* m = dlg.messagesDlg;
        ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(false, true) == 1; }, 8000));
        QTest::qWait(2000); // ~2 s de gravação
        // durante a chamada: grava em WAV
        bool wavWhileCalling = false;
        for (const auto& e : fs::directory_iterator(rec))
            wavWhileCalling |= e.path().extension() == ".wav";
        EXPECT_TRUE(wavWhileCalling);
        m->OnBnClickedEnd();
        ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(true) == 0; }, 5000));
    } // OnClose espera as conversões (msip_wav_to_mp3_wait)

    std::vector<fs::path> files;
    for (const auto& e : fs::directory_iterator(rec))
        files.push_back(e.path());
    ASSERT_EQ(files.size(), 1u);
    EXPECT_EQ(files[0].extension(), ".mp3");
    // data-hora-usuárioRemoto-direção-conta: chamada por IP não tem usuário (como no original)
    EXPECT_NE(files[0].filename().string().find("--outgoing-"), std::string::npos) << files[0];
    EXPECT_GT(fs::file_size(files[0]), 1000u);
    fs::remove_all(root);
}

// Comandos externos por evento (cmdOutgoingCall/cmdCallStart/cmdCallEnd) e desligamento
// automático (autoHangUpTime) numa chamada real. Opcional: MSIP_TEST_SIP=1.
TEST(Commands, ComandosPorEventoEAutoDesligar)
{
    if (!std::getenv("MSIP_TEST_SIP"))
        GTEST_SKIP() << "defina MSIP_TEST_SIP=1 (porta UDP 5060 livre)";
    char tmpl[] = "/tmp/msip-commands-XXXXXX";
    const fs::path root = mkdtemp(tmpl);
    setenv("XDG_CONFIG_HOME", (root / "c").c_str(), 1);
    setenv("XDG_DATA_HOME", (root / "d").c_str(), 1);
    setenv("XDG_STATE_HOME", (root / "s").c_str(), 1);
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);

    // script que registra o evento e os parâmetros recebidos
    const fs::path log = root / "events.txt";
    const fs::path hook = root / "hook.sh";
    {
        std::ofstream f(hook);
        f << "#!/bin/sh\necho \"$*\" >> '" << log.string() << "'\n";
    }
    fs::permissions(hook, fs::perms::owner_all);

    QProcess cli;
    const QString script = QString::fromStdString((root / "cli.txt").string());
    {
        QFile f(script);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("wait 12\nquit\n");
    }
    cli.setProcessChannelMode(QProcess::MergedChannels);
    cli.start(QStringLiteral(MSIP_CLI_PATH), {QStringLiteral("--profile"), QString::fromStdString((root / "cli").string()),
                                              QStringLiteral("--null-audio"), QStringLiteral("--auto-answer"),
                                              QStringLiteral("--script"), script});
    ASSERT_TRUE(cli.waitForStarted());
    auto stopCli = qScopeGuard([&] {
        if (cli.state() != QProcess::NotRunning) {
            cli.kill();
            cli.waitForFinished(3000);
        }
    });
    QTest::qWait(1000);

    PrivateProfileFlushCache();
    SetPasswordStore(nullptr);
    ResetAccountSettings();
    accountSettings.Init();
    const CString h(hook.c_str());
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5070"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("autoHangUpTime"), _T("2"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("cmdOutgoingCall"), h + _T(" outgoing"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("cmdCallStart"), h + _T(" start"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("cmdCallEnd"), h + _T(" end"), accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();
    ASSERT_EQ(accountSettings.autoHangUpTime, 2);

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        ASSERT_TRUE(dlg.MakeCall(_T("127.0.0.1")));
        MessagesDlg* m = dlg.messagesDlg;
        ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(false, true) == 1; }, 8000));
        // autoHangUpTime = 2 s: a chamada termina sozinha
        QElapsedTimer t;
        t.start();
        ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(true) == 0; }, 6000));
        EXPECT_GE(t.elapsed(), 1000);
        QTest::qWait(500);
    }
    std::ifstream in(log);
    std::string events((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(events.find("outgoing"), std::string::npos) << events;
    EXPECT_NE(events.find("start"), std::string::npos) << events;
    EXPECT_NE(events.find("end"), std::string::npos) << events;
    fs::remove_all(root);
}
