// Linha de comando do MicroSIP: CmainDlg::CommandLine/DialNumberFromCommandLine (msip:*,
// /dtmf:, /account:N /password:, discagem/mensagem) e, com o executável real, o repasse para
// a instância já aberta (cmd/exit), /answer sem instância e /resetnoask.

#include "Dialer.h"
#include "MessagesDlg.h"
#include "PasswordStore.h"
#include "UiCommon.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QProcess>
#include <QScopeGuard>
#include <QTest>

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static fs::path TempProfile(const char* name)
{
    std::string tmpl = std::string("/tmp/msip-") + name + "-XXXXXX";
    const fs::path root = mkdtemp(tmpl.data());
    setenv("XDG_CONFIG_HOME", (root / "c").c_str(), 1);
    setenv("XDG_DATA_HOME", (root / "d").c_str(), 1);
    setenv("XDG_STATE_HOME", (root / "s").c_str(), 1);
    return root;
}

TEST(CommandLine, ComandosMsipContaESenha)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    TempProfile("cmdline");
    PrivateProfileFlushCache();
    SetPasswordStore(nullptr);
    ResetAccountSettings();
    accountSettings.Init();
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5078"), accountSettings.iniFile);
    Account a;
    a.domain = _T("192.0.2.1");
    a.server = _T("192.0.2.1");
    a.username = _T("1001");
    a.password = _T("antiga");
    a.rememberPassword = true;
    accountSettings.AccountSave(1, &a);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();
    msip_command_line.Empty();

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        Dialer* dialer = dlg.pageDialer;

        // msip:* (com ou sem aspas em volta, como no WM_COPYDATA do Windows)
        EXPECT_FALSE(dlg.CommandLine(_T("msip:micmute")));
        EXPECT_TRUE(dialer->muteInput);
        dlg.CommandLine(_T("\"msip:micunmute\""));
        EXPECT_FALSE(dialer->muteInput);
        dlg.CommandLine(_T("msip:speakmuteclick"));
        EXPECT_TRUE(dialer->muteOutput);
        dlg.CommandLine(_T("msip:speakmuteclick"));
        EXPECT_FALSE(dialer->muteOutput);
        dlg.CommandLine(_T("msip:speakmute"));
        EXPECT_TRUE(dialer->muteOutput);
        dlg.CommandLine(_T("msip:speakunmute"));
        EXPECT_FALSE(dialer->muteOutput);
        dlg.CommandLine(_T("/dtmf:12")); // sem chamada: nada acontece
        dlg.CommandLine(_T("/hangupall"));
        dlg.CommandLine(_T("\""));       // aspas sozinhas não estouram o GetAt
        dlg.CommandLine(_T(""));

        dlg.show();
        dlg.CommandLine(_T("msip:minimize"));
        EXPECT_FALSE(dlg.isVisible());

        // pela instância única: "cmd" com /account:N /password: grava a senha da conta
        dlg.OnInstanceMessage(QStringLiteral("cmd /account:1 /password:nova"));
        Account b;
        ASSERT_TRUE(accountSettings.AccountLoad(1, &b));
        EXPECT_EQ(b.password, CString(_T("nova")));
        // conta inexistente: nada é discado
        dlg.OnInstanceMessage(QStringLiteral("cmd 1002 /account:9"));
        EXPECT_EQ(dlg.messagesDlg->GetCount(), 0);

        // /account:N troca a conta ativa; o número fica à espera do registro (servidor sem
        // resposta) — dialNumberDelayed
        dlg.CommandLine(_T("sip:1002 /account:1"));
        EXPECT_EQ(accountSettings.accountId, 1);
        EXPECT_EQ(dlg.dialNumberDelayed, CString(_T("1002")));
        EXPECT_TRUE(dlg.isVisible()); // discar mostra a janela (onTrayNotify)

        // "show" traz a janela de volta
        dlg.hide();
        dlg.OnInstanceMessage(QStringLiteral("show"));
        EXPECT_TRUE(dlg.isVisible());
    }
    shortcuts.RemoveAll();
}

// --- executável real: instância única ---

static QProcessEnvironment AppEnv(const fs::path& root)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("XDG_CONFIG_HOME"), QString::fromStdString((root / "c").string()));
    env.insert(QStringLiteral("XDG_DATA_HOME"), QString::fromStdString((root / "d").string()));
    env.insert(QStringLiteral("XDG_STATE_HOME"), QString::fromStdString((root / "s").string()));
    env.insert(QStringLiteral("HOME"), QString::fromStdString(root.string()));
    env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
    return env;
}

static int RunApp(const fs::path& root, const QStringList& args, int timeoutMs, qint64* elapsed = nullptr)
{
    QProcess p;
    p.setProcessEnvironment(AppEnv(root));
    p.setProcessChannelMode(QProcess::MergedChannels);
    QElapsedTimer t;
    t.start();
    p.start(QStringLiteral(MSIP_APP_PATH), args);
    if (!p.waitForFinished(timeoutMs)) {
        p.kill();
        p.waitForFinished();
        return -1;
    }
    if (elapsed)
        *elapsed = t.elapsed();
    return p.exitCode();
}

TEST(CommandLine, InstanciaUnicaRepassaESai)
{
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QCoreApplication app(argc, argv);
    char tmpl[] = "/tmp/msip-instance-XXXXXX";
    const fs::path root = mkdtemp(tmpl);
    fs::create_directories(root / "c/MySimpleSIP");
    std::ofstream(root / "c/MySimpleSIP/MySimpleSIP.ini") << "[Global]\n[Settings]\nsourcePort=5086\n";

    // sem instância aberta, /answer e /hangupall só terminam (não abrem o aplicativo)
    qint64 ms = 0;
    EXPECT_EQ(RunApp(root, {QStringLiteral("/answer")}, 5000, &ms), 0);
    EXPECT_LT(ms, 4000);

    QProcess first;
    first.setProcessEnvironment(AppEnv(root));
    first.setProcessChannelMode(QProcess::MergedChannels);
    first.start(QStringLiteral(MSIP_APP_PATH), {QStringLiteral("--quit-after"), QStringLiteral("30000")});
    ASSERT_TRUE(first.waitForStarted());
    QTest::qWait(3000);
    ASSERT_EQ(first.state(), QProcess::Running) << first.readAll().toStdString();

    // a segunda execução repassa o comando e termina logo (sem abrir outra janela)
    EXPECT_EQ(RunApp(root, {QStringLiteral("msip:micmute")}, 8000, &ms), 0);
    EXPECT_LT(ms, 4000);
    EXPECT_EQ(RunApp(root, {}, 8000, &ms), 0); // só mostrar
    EXPECT_EQ(first.state(), QProcess::Running);

    // /exit fecha a instância aberta
    EXPECT_EQ(RunApp(root, {QStringLiteral("/exit")}, 8000), 0);
    EXPECT_TRUE(first.waitForFinished(8000)) << "a instância aberta não fechou com /exit";
    EXPECT_EQ(first.exitCode(), 0);

    // /resetnoask apaga as pastas do aplicativo (config, data e state)
    EXPECT_TRUE(fs::exists(root / "c/MySimpleSIP"));
    EXPECT_EQ(RunApp(root, {QStringLiteral("/resetnoask")}, 8000), 0);
    EXPECT_FALSE(fs::exists(root / "c/MySimpleSIP"));
    EXPECT_FALSE(fs::exists(root / "d/MySimpleSIP"));
    EXPECT_FALSE(fs::exists(root / "s/MySimpleSIP"));
    EXPECT_TRUE(fs::exists(root / "c")); // só a pasta do aplicativo sai
}

TEST(CommandLine, DiscaEDesligaComCli)
{
    if (!std::getenv("MSIP_TEST_SIP"))
        GTEST_SKIP() << "defina MSIP_TEST_SIP=1 (portas UDP 5060 e 5070 livres) para a chamada real";
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    const fs::path root = TempProfile("cmdline-sip");

    // atendedor automático na 5060
    QProcess cli;
    const QString script = QString::fromStdString((root / "cli.txt").string());
    std::ofstream(root / "cli.txt") << "wait 12\nquit\n";
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
    msip_command_line.Empty();

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        MessagesDlg* m = dlg.messagesDlg;

        // URI da linha de comando (conta local) → DialNumber → MakeCall
        dlg.OnInstanceMessage(QStringLiteral("cmd sip:127.0.0.1"));
        ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(false, true) == 1; }, 8000)) << cli.readAll().toStdString();
        EXPECT_EQ(accountSettings.lastCallNumber, CString(_T("127.0.0.1")));

        // /hangupall pela instância única
        dlg.OnInstanceMessage(QStringLiteral("cmd /hangupall"));
        ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount() == 0; }, 5000));
    }
    cli.waitForFinished(15000);
}
