// Diálogo Transfer (offscreen) e transferência cega de ponta a ponta: a janela liga para
// o msip-cli A (5060), abre o Transfer e transfere para o msip-cli B (5080); A recebe o
// REFER e liga para B; o NOTIFY 200 encerra a chamada da janela (onCallTransferStatus).

#include "Contacts.h"
#include "MessagesDlg.h"
#include "PasswordStore.h"
#include "Transfer.h"
#include "UiCommon.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QLineEdit>
#include <QProcess>
#include <QScopeGuard>
#include <QPointer>
#include <QPushButton>
#include <QTest>
#include <QTextBrowser>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace {
struct Env {
    fs::path root;
    explicit Env(const char* tag)
    {
        std::string t = std::string("/tmp/msip-") + tag + "-XXXXXX";
        root = mkdtemp(t.data());
        setenv("XDG_CONFIG_HOME", (root / "c").c_str(), 1);
        setenv("XDG_DATA_HOME", (root / "d").c_str(), 1);
        setenv("XDG_STATE_HOME", (root / "s").c_str(), 1);
        qputenv("QT_QPA_PLATFORM", "offscreen");
        PrivateProfileFlushCache();
        SetPasswordStore(nullptr);
        ResetAccountSettings();
        accountSettings.Init();
    }
    ~Env()
    {
        if (!qEnvironmentVariableIsSet("MSIP_TEST_KEEP"))
            fs::remove_all(root);
    }
};
} // namespace

TEST(Transfer, TitulosBotoesListaETeclado)
{
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    Env env("transfer-ui");
    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        for (auto [n, name, starred, mobile] : {std::tuple{_T("1002"), _T("Ana"), true, _T("5511988887777")},
                                               std::tuple{_T("1003"), _T("Bruno"), false, _T("")},
                                               std::tuple{_T("1004"), _T("Carla"), true, _T("")}}) {
            Contact c;
            c.number = n;
            c.name = name;
            c.starred = starred;
            c.mobile = mobile;
            dlg.pageContacts->ContactAdd(c, TRUE);
        }

        // transferência cega sem código de função: só OK, título "Transferência cega"
        dlg.OpenTransferDlg(&dlg, MSIP_ACTION_TRANSFER);
        Transfer* t = dlg.transferDlg;
        ASSERT_NE(t, nullptr);
        EXPECT_EQ(t->windowTitle(), TR(_T("Blind Transfer")));
        EXPECT_FALSE(t->m_layout.Button(QStringLiteral("IDOK"))->isHidden());
        EXPECT_TRUE(t->m_layout.Button(QStringLiteral("IDC_TRANSFER_BLIND"))->isHidden());
        // lista: só favoritos (há favoritos), com celular como item extra
        QComboBox* combo = t->m_layout.Combo(QStringLiteral("IDC_NUMBER"));
        QStringList items;
        for (int i = 0; i < combo->count(); i++)
            items << combo->itemData(i).toString();
        items.sort();
        EXPECT_EQ(items, (QStringList{QStringLiteral("1002"), QStringLiteral("1004"), QStringLiteral("5511988887777")}));
        EXPECT_TRUE(combo->currentText().isEmpty());
        // teclado acrescenta dígitos
        for (const char* id : {"IDC_KEY_1", "IDC_KEY_0", "IDC_KEY_GRATE"})
            t->m_layout.Button(QString::fromLatin1(id))->click();
        EXPECT_EQ(combo->currentText(), QStringLiteral("10#"));

        // abrir de novo substitui o anterior; com código de função AT: botões assistida/cega
        accountSettings.enableFeatureCodeAT = true;
        accountSettings.featureCodeAT = _T("*2");
        QPointer<Transfer> old = t;
        dlg.OpenTransferDlg(&dlg, MSIP_ACTION_TRANSFER);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        EXPECT_TRUE(old.isNull());
        t = dlg.transferDlg;
        EXPECT_EQ(t->windowTitle(), TR(_T("Call Transfer")));
        EXPECT_TRUE(t->m_layout.Button(QStringLiteral("IDOK"))->isHidden());
        EXPECT_FALSE(t->m_layout.Button(QStringLiteral("IDC_TRANSFER_ATTENDED"))->isHidden());
        EXPECT_FALSE(t->m_layout.Button(QStringLiteral("IDC_TRANSFER_BLIND"))->isHidden());

        // conferência e desvio
        dlg.OpenTransferDlg(&dlg, MSIP_ACTION_INVITE);
        EXPECT_EQ(dlg.transferDlg->windowTitle(), TR(_T("Invite to Conference")));
        dlg.OpenTransferDlg(&dlg, MSIP_ACTION_FORWARD, 3);
        EXPECT_EQ(dlg.transferDlg->windowTitle(), TR(_T("Call Transfer")));
        EXPECT_EQ(dlg.transferDlg->callId, 3);

        // Cancelar fecha e zera mainDlg->transferDlg
        dlg.transferDlg->m_layout.Button(QStringLiteral("IDCANCEL"))->click();
        EXPECT_EQ(dlg.transferDlg, nullptr);
        dlg.OnClose();
    }
}

static void RunBlindTransfer(bool targetBusy, const char* target = "127.0.0.1:5080")
{
    if (!std::getenv("MSIP_TEST_SIP"))
        GTEST_SKIP() << "defina MSIP_TEST_SIP=1 (portas UDP 5060, 5070 e 5080 livres)";
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    Env env("transfer-sip");
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5070"), accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();

    auto startCli = [&](QProcess& p, const char* tag, const char* port, const char* extraIni = "") {
        const fs::path prof = env.root / tag;
        fs::create_directories(prof / "config" / "MySimpleSIP");
        QFile ini(QString::fromStdString((prof / "config" / "MySimpleSIP" / "MySimpleSIP.ini").string()));
        EXPECT_TRUE(ini.open(QIODevice::WriteOnly));
        ini.write(QByteArray("[Settings]\nsourcePort=") + port + "\n" + extraIni
                  + (qEnvironmentVariableIsSet("MSIP_TEST_CLI_LOG") ? "enableLog=1\n" : ""));
        ini.close();
        const QString script = QString::fromStdString((prof / "s.txt").string());
        QFile f(script);
        EXPECT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("wait 18\nstatus\nquit\n");
        f.close();
        p.setProcessChannelMode(QProcess::MergedChannels);
        QStringList args{QStringLiteral("--profile"), QString::fromStdString(prof.string()), QStringLiteral("--null-audio"),
                         QStringLiteral("--auto-answer"), QStringLiteral("--script"), script};
        if (qEnvironmentVariableIsSet("MSIP_TEST_CLI_LOG"))
            args << QStringLiteral("--log"); // log do pjsip das CLIs (diagnóstico)
        p.start(QStringLiteral(MSIP_CLI_PATH), args);
        EXPECT_TRUE(p.waitForStarted());
    };
    QProcess cliA, cliB;
    auto stopClis = qScopeGuard([&] {
        for (QProcess* p : {&cliA, &cliB}) {
            if (qEnvironmentVariableIsSet("MSIP_TEST_KEEP"))
                p->waitForFinished(20000); // diagnóstico: deixa a CLI gravar o log
            if (p->state() != QProcess::NotRunning) {
                p->kill();
                p->waitForFinished(3000);
            }
        }
    });
    // A primeiro: com sourcePort ≠ 5060 o MicroSIP também abre a 5060 (conta local), então
    // B e a janela só sobem depois de A ter a 5060
    if (qEnvironmentVariableIsSet("MSIP_TEST_RACE")) {
        // diagnóstico: B primeiro (B fica com a 5060 da conta local)
        startCli(cliB, "B", "5080", targetBusy ? "denyIncoming=all\n" : "");
        QTest::qWait(1200);
        startCli(cliA, "A", "5060");
        QTest::qWait(1200);
    }
    else {
        startCli(cliA, "A", "5060");
        QTest::qWait(1200);
        startCli(cliB, "B", "5080", targetBusy ? "denyIncoming=all\n" : "");
        QTest::qWait(1200);
    }

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); }); // ASSERT falho: encerra o pjsua antes de destruir a janela
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        MessagesDlg* m = dlg.messagesDlg;

        ASSERT_TRUE(dlg.MakeCall(_T("127.0.0.1")));
        ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(false, true) == 1; }, 8000)) << cliA.readAll().toStdString();

        QTest::qWait(1000); // como um usuário: transfere com a chamada já estabelecida
        // Transferir (Dialer) → Transfer → digita o destino → OK → CallAction (pjsua_call_xfer)
        dlg.OpenTransferDlg(&dlg, MSIP_ACTION_TRANSFER);
        Transfer* t = dlg.transferDlg;
        t->m_layout.Combo(QStringLiteral("IDC_NUMBER"))->setEditText(QString::fromLatin1(target));
        t->m_layout.Button(QStringLiteral("IDOK"))->click();
        EXPECT_EQ(dlg.transferDlg, nullptr);

        if (!targetBusy) {
            // NOTIFY 200 do A → onCallTransferStatus encerra a chamada da janela
            ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(true) == 0; }, 10000))
                << cliA.readAll().toStdString() << cliB.readAll().toStdString();
        }
        else {
            // B recusa (486): o NOTIFY final informa a falha e a chamada da janela continua
            auto* list = m->findChild<QTextBrowser*>(QStringLiteral("IDC_MESSAGES_LIST"));
            ASSERT_TRUE(QTest::qWaitFor([&] { return list->toPlainText().contains(QStringLiteral("Busy Here")); }, 10000))
                << list->toPlainText().toStdString() << cliA.readAll().toStdString();
            QTest::qWait(500);
            EXPECT_EQ(m->GetCallsCount(true), 1);
            m->OnBnClickedEnd();
            ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(true) == 0; }, 5000));
        }

        dlg.OnClose();
    }
    cliA.waitForFinished(20000);
    cliB.waitForFinished(20000);
    // as CLIs não podem sobrar ocupando as portas para o próximo teste
    for (QProcess* p : {&cliA, &cliB}) {
        if (p->state() != QProcess::NotRunning) {
            p->kill();
            p->waitForFinished(3000);
        }
    }
    if (!targetBusy) {
        const QString logB = QString::fromUtf8(cliB.readAll());
        EXPECT_TRUE(logB.contains(QStringLiteral("atendendo automaticamente"))) << logB.toStdString();
        EXPECT_TRUE(logB.contains(QStringLiteral("CONFIRMED"))) << logB.toStdString();
    }
}

TEST(Transfer, TransferenciaCegaRealComDuasClis)
{
    RunBlindTransfer(false);
}

// Destino recusa (denyIncoming=all → 486): a falha chega pelo NOTIFY e nada quebra
TEST(Transfer, TransferenciaCegaRecusadaPeloDestino)
{
    RunBlindTransfer(true);
}

// Destino = a própria ponta transferida (A recebe REFER para si mesma): situação que
// aparecia quando a CLI B ocupava a 5060; o MicroSIP recusa/encerra sem quebrar
TEST(Transfer, TransferenciaParaAPropriaPonta)
{
    RunBlindTransfer(true, "127.0.0.1:5060");
}

// Conferência real: a janela (modo de várias chamadas) liga para a CLI A e para a CLI B,
// "Mesclar todas" junta as duas no bridge (A ouve B e B ouve A) e "Separar todas" desfaz.
static bool Listens(pjsua_conf_port_id from, pjsua_conf_port_id to)
{
    pjsua_conf_port_info info;
    if (pjsua_conf_get_port_info(from, &info) != PJ_SUCCESS)
        return false;
    for (unsigned i = 0; i < info.listener_cnt; i++)
        if (info.listeners[i] == to)
            return true;
    return false;
}

TEST(Conference, MesclarESepararComDuasClis)
{
    if (!std::getenv("MSIP_TEST_SIP"))
        GTEST_SKIP() << "defina MSIP_TEST_SIP=1 (portas UDP 5060, 5070 e 5080 livres)";
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    Env env("conference-sip");
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5070"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("singleMode"), _T("0"), accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();
    ASSERT_FALSE(accountSettings.singleMode);

    auto startCli = [&](QProcess& p, const char* tag, const char* port) {
        const fs::path prof = env.root / tag;
        fs::create_directories(prof / "config" / "MySimpleSIP");
        QFile ini(QString::fromStdString((prof / "config" / "MySimpleSIP" / "MySimpleSIP.ini").string()));
        EXPECT_TRUE(ini.open(QIODevice::WriteOnly));
        ini.write(QByteArray("[Settings]\nsourcePort=") + port + "\n");
        ini.close();
        const QString script = QString::fromStdString((prof / "s.txt").string());
        QFile f(script);
        EXPECT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("wait 16\nquit\n");
        f.close();
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.start(QStringLiteral(MSIP_CLI_PATH), {QStringLiteral("--profile"), QString::fromStdString(prof.string()), QStringLiteral("--null-audio"),
                                               QStringLiteral("--auto-answer"), QStringLiteral("--script"), script});
        EXPECT_TRUE(p.waitForStarted());
    };
    QProcess cliA, cliB;
    auto stopClis = qScopeGuard([&] {
        for (QProcess* p : {&cliA, &cliB}) {
            if (p->state() != QProcess::NotRunning) {
                p->kill();
                p->waitForFinished(3000);
            }
        }
    });
    startCli(cliA, "A", "5060");
    QTest::qWait(1200);
    startCli(cliB, "B", "5080");
    QTest::qWait(1200);

    QtMessageBackend backend;
    CmainDlg dlg(&backend, true);
    auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
    dlg.nullSoundDevice = true;
    ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
    MessagesDlg* m = dlg.messagesDlg;

    ASSERT_TRUE(dlg.MakeCall(_T("127.0.0.1")));
    ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(false, true) == 1; }, 8000));
    const pjsua_call_id callA = m->CurrentCallId();
    ASSERT_TRUE(dlg.MakeCall(_T("127.0.0.1:5080")));
    // ao ligar a segunda, a primeira fica em espera (várias chamadas)
    ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount() == 2 && m->GetCallsCount(false, true) == 1; }, 8000))
        << m->GetCallsCount() << " " << m->GetCallsCount(false, true);
    const pjsua_call_id callB = m->CurrentCallId();
    ASSERT_NE(callA, callB);

    // Conferência → Mesclar todas (aba ativa = B)
    m->OnMergeAll();
    ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(false, true) == 2; }, 8000)); // A sai da espera
    pjsua_call_info a, b;
    ASSERT_EQ(pjsua_call_get_info(callA, &a), PJ_SUCCESS);
    ASSERT_EQ(pjsua_call_get_info(callB, &b), PJ_SUCCESS);
    auto inConf = [](pjsua_call_id id) {
        auto* ud = (call_user_data*)pjsua_call_get_user_data(id);
        CSingleLock lock(&ud->CS, TRUE);
        return ud->inConference;
    };
    EXPECT_TRUE(inConf(callA));
    EXPECT_TRUE(inConf(callB));
    ASSERT_TRUE(QTest::qWaitFor([&] { return Listens(a.conf_slot, b.conf_slot) && Listens(b.conf_slot, a.conf_slot); }, 3000));

    // Separar todas: a ativa (B) vai para espera e A sai da conferência
    m->OnSeparateAll();
    ASSERT_TRUE(QTest::qWaitFor([&] { return !inConf(callA) && !Listens(b.conf_slot, a.conf_slot); }, 5000));

    m->OnCloseAllTabs(); // só fecha abas sem chamada
    msip_call_hangup_all();
    ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(true) == 0; }, 5000));
}
