// Teste de interface (offscreen) das mensagens SIP (chat): onPager/MessagesIncoming abrem a
// aba do contato com a mensagem recebida, onPagerStatus mostra a falha de envio, e (com
// MSIP_TEST_SIP=1) uma troca real de MESSAGE entre a msip-cli e a janela.

#include "MessagesDlg.h"
#include "PasswordStore.h"
#include "UiCommon.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QFile>
#include <QProcess>
#include <QScopeGuard>
#include <QTest>
#include <QTextBrowser>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

static fs::path PrepareProfile(const char* name, int sourcePort)
{
    std::string tmpl = std::string("/tmp/msip-") + name + "-XXXXXX";
    const fs::path root = mkdtemp(tmpl.data());
    setenv("XDG_CONFIG_HOME", (root / "c").c_str(), 1);
    setenv("XDG_DATA_HOME", (root / "d").c_str(), 1);
    setenv("XDG_STATE_HOME", (root / "s").c_str(), 1);
    PrivateProfileFlushCache();
    SetPasswordStore(nullptr);
    ResetAccountSettings();
    accountSettings.Init();
    CString port;
    port.Format(_T("%d"), sourcePort);
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), port, accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();
    return root;
}

TEST(Chat, MensagemRecebidaEFalhaDeEnvio)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    PrepareProfile("chat", 5076);

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true); // 1ª execução: abre visível mesmo com "minimizado"
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        MessagesDlg* m = dlg.messagesDlg;
        auto* list = m->findChild<QTextBrowser*>(QStringLiteral("IDC_MESSAGES_LIST"));
        ASSERT_NE(list, nullptr);

        // UM_ON_PAGER (on_pager2): a janela é dona das duas strings
        dlg.PostMessage(UM_ON_PAGER, (WPARAM) new CString(_T("sip:1001@192.0.2.10")), (LPARAM) new CString(_T("Olá, tudo bem?")));
        ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCount() == 1; }, 2000));
        EXPECT_FALSE(dlg.newMessages); // janela principal visível: nada pendente
        EXPECT_TRUE(m->isVisible());
        MessagesContact* c = m->GetMessageContact();
        ASSERT_NE(c, nullptr);
        EXPECT_TRUE(list->toPlainText().contains(QStringLiteral("Olá, tudo bem?")));

        // segunda mensagem do mesmo contato: mesma aba
        dlg.PostMessage(UM_ON_PAGER, (WPARAM) new CString(_T("sip:1001@192.0.2.10")), (LPARAM) new CString(_T("Segunda")));
        ASSERT_TRUE(QTest::qWaitFor([&] { return list->toPlainText().contains(QStringLiteral("Segunda")); }, 2000));
        EXPECT_EQ(m->GetCount(), 1);

        // UM_ON_PAGER_STATUS (on_pager_status2, status ≥ 300): motivo como mensagem do sistema
        dlg.PostMessage(UM_ON_PAGER_STATUS, (WPARAM) new CString(_T("sip:1001@192.0.2.10")), (LPARAM) new CString(_T("Not Found")));
        ASSERT_TRUE(QTest::qWaitFor([&] { return list->toPlainText().contains(QStringLiteral("Not Found")); }, 2000));
        EXPECT_EQ(m->GetCount(), 1);

        // com a janela principal escondida (bandeja), a mensagem fica pendente e a janela de
        // mensagens não aparece
        m->hide();
        dlg.hide();
        dlg.PostMessage(UM_ON_PAGER, (WPARAM) new CString(_T("sip:1002@192.0.2.10")), (LPARAM) new CString(_T("Oi")));
        ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCount() == 2; }, 2000));
        EXPECT_TRUE(dlg.newMessages);
        EXPECT_FALSE(m->isVisible());
    }
}

TEST(Chat, TrocaRealDeMensagensComCli)
{
    if (!std::getenv("MSIP_TEST_SIP"))
        GTEST_SKIP() << "defina MSIP_TEST_SIP=1 (portas UDP 5060 e 5070 livres) para a troca real de mensagens";
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    const fs::path root = PrepareProfile("chat-sip", 5070);

    // a CLI (porta 5060) manda uma mensagem para a janela (5070) e espera a resposta
    QProcess cli;
    const QString script = QString::fromStdString((root / "cli.txt").string());
    {
        QFile f(script);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("wait 3\nmessage 127.0.0.1:5070 Olá da CLI\nwait 6\nquit\n");
    }
    cli.setProcessChannelMode(QProcess::MergedChannels);
    cli.start(QStringLiteral(MSIP_CLI_PATH), {QStringLiteral("--profile"), QString::fromStdString((root / "cli").string()),
                                              QStringLiteral("--null-audio"), QStringLiteral("--script"), script});
    ASSERT_TRUE(cli.waitForStarted());
    QTest::qWait(1000);

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        MessagesDlg* m = dlg.messagesDlg;
        auto* list = m->findChild<QTextBrowser*>(QStringLiteral("IDC_MESSAGES_LIST"));

        ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCount() == 1; }, 8000)) << cli.readAll().toStdString();
        MessagesContact* c = m->GetMessageContact();
        ASSERT_NE(c, nullptr);
        EXPECT_TRUE(list->toPlainText().contains(QStringLiteral("Olá da CLI")));

        // resposta pela janela de mensagens (SendInstantMessage → pjsua_im_send)
        EXPECT_TRUE(m->SendInstantMessage(c, _T("Resposta da janela")));
        ASSERT_TRUE(cli.waitForFinished(15000));
        const QString out = QString::fromUtf8(cli.readAll());
        EXPECT_TRUE(out.contains(QStringLiteral("[mensagem]")) && out.contains(QStringLiteral("Resposta da janela"))) << out.toStdString();
    }
}
