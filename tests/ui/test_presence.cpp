// Diretório de usuários remoto (HTTP local no próprio teste: XML, JSON com presença e
// CSV) e presença/BLF de verdade contra o msip-cli (opcional, MSIP_TEST_SIP=1).

#include "Contacts.h"
#include "PasswordStore.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QFile>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>
#include <QScopeGuard>
#include <QTest>
#include <QTreeWidget>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

// Servidor HTTP mínimo: cada requisição recebe a próxima resposta da lista
class TinyHttp : public QObject {
public:
    explicit TinyHttp(QList<QByteArray> responses) : m_responses(std::move(responses))
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        QObject::connect(&m_server, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket* s = m_server.nextPendingConnection()) {
                QObject::connect(s, &QTcpSocket::readyRead, s, [this, s] {
                    m_buffer[s] += s->readAll();
                    if (!m_buffer[s].contains("\r\n\r\n"))
                        return;
                    requests << QString::fromLatin1(m_buffer[s].left(m_buffer[s].indexOf("\r\n")));
                    s->write(m_responses.isEmpty() ? QByteArray("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n") : m_responses.takeFirst());
                    s->disconnectFromHost();
                });
            }
        });
    }
    quint16 port() const { return m_server.serverPort(); }
    QStringList requests;

private:
    QTcpServer m_server;
    QList<QByteArray> m_responses;
    QHash<QTcpSocket*, QByteArray> m_buffer;
};

QByteArray Http(const QByteArray& type, const QByteArray& body, const QByteArray& extra = {})
{
    return "HTTP/1.1 200 OK\r\nContent-Type: " + type + "\r\n" + extra + "Content-Length: " + QByteArray::number(body.size())
        + "\r\nConnection: close\r\n\r\n" + body;
}

struct Env {
    fs::path root;
    Env(const char* tag)
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
    ~Env() { fs::remove_all(root); }
};

} // namespace

TEST(UsersDirectory, XmlJsonCsvEPresenca)
{
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    Env env("directory");

    TinyHttp http({
        Http("text/xml", "<?xml version=\"1.0\"?><contacts refresh=\"120\">"
                         "<contact name=\"Suporte\" number=\"300\" presence=\"1\"/>"
                         "<contact name=\"Vendas\" number=\"301\" starred=\"1\"/></contacts>"),
        Http("application/json", R"({"items":[{"name":"Suporte","number":"300"},{"name":"Financeiro","phone":"302"}],)"
                                 R"("presence":[{"number":"302","status":"busy","info":"Em reunião"}]})"),
        Http("text/csv", "Name,Number\r\nRH,303\r\n", "Cache-Control: max-age=30\r\n"),
    });
    accountSettings.usersDirectory.Format(_T("http://127.0.0.1:%d/dir"), (int)http.port());

    QtMessageBackend backend;
    CmainDlg dlg(&backend, true);
    Contacts* page = dlg.pageContacts;
    // contato manual (não é do diretório): nunca é apagado pelo diretório
    Contact manual;
    manual.name = _T("Ana");
    manual.number = _T("1002");
    page->ContactAdd(manual, TRUE);

    // 1) XML <contacts>: carregado sozinho ao iniciar (PJCreate → UsersDirectoryLoad);
    // contatos marcados como diretório, campos do XML
    ASSERT_TRUE(QTest::qWaitFor([&] { return page->FindContact(_T("301")) != nullptr; }, 5000)) << http.requests.join(QStringLiteral(" | ")).toStdString() << " contatos=" << page->contacts.GetCount();
    EXPECT_EQ(http.requests.value(0), QStringLiteral("GET /dir?sequence=0 HTTP/1.1"));
    EXPECT_TRUE(page->FindContact(_T("300"))->directory);
    EXPECT_TRUE(page->FindContact(_T("300"))->presence);
    EXPECT_TRUE(page->FindContact(_T("301"))->starred);
    EXPECT_TRUE(dlg.usersDirectoryLoaded);

    // 2) JSON: "phone" vira número, quem saiu do diretório é removido, presença aplicada
    dlg.UsersDirectoryLoad(true);
    ASSERT_TRUE(QTest::qWaitFor([&] { return page->FindContact(_T("302")) != nullptr; }, 5000));
    EXPECT_EQ(http.requests.value(1), QStringLiteral("GET /dir?sequence=1 HTTP/1.1"));
    EXPECT_EQ(page->FindContact(_T("301")), nullptr);
    ASSERT_NE(page->FindContact(_T("300")), nullptr);
    EXPECT_NE(page->FindContact(_T("1002")), nullptr);
    Contact* fin = page->FindContact(_T("302"));
    EXPECT_EQ(fin->name, _T("Financeiro"));
    EXPECT_EQ(fin->image, MSIP_CONTACT_ICON_BUSY);
    EXPECT_EQ(fin->info, _T("Em reunião"));

    // 3) CSV (Content-Type: text/csv)
    dlg.UsersDirectoryLoad(true);
    ASSERT_TRUE(QTest::qWaitFor([&] { return page->FindContact(_T("303")) != nullptr; }, 5000));
    EXPECT_EQ(page->FindContact(_T("303"))->name, _T("RH"));
    // no original, o CSV do diretório entra sem a marca "diretório" (Import sem directory)
    EXPECT_FALSE(page->FindContact(_T("303"))->directory);
    EXPECT_EQ(page->FindContact(_T("302")), nullptr);

    dlg.OnClose();
}

// Presença real: o CmainDlg (porta 5070) assina a presença do msip-cli (127.0.0.1:5060,
// comando "online" = PublishStatus); o NOTIFY muda o ícone do contato para "online";
// Unsubscribe volta ao ícone padrão.
TEST(Presence, AssinaturaRealComCli)
{
    if (!std::getenv("MSIP_TEST_SIP"))
        GTEST_SKIP() << "defina MSIP_TEST_SIP=1 (porta UDP 5060 livre) para a presença de ponta a ponta";
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    Env env("presence");
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5070"), accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();

    QProcess cli;
    const QString script = QString::fromStdString((env.root / "cli.txt").string());
    {
        QFile f(script);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("online\nwait 10\nquit\n");
    }
    cli.setProcessChannelMode(QProcess::MergedChannels);
    cli.start(QStringLiteral(MSIP_CLI_PATH), {QStringLiteral("--profile"), QString::fromStdString((env.root / "cli").string()),
                                              QStringLiteral("--null-audio"), QStringLiteral("--script"), script});
    ASSERT_TRUE(cli.waitForStarted());
    QTest::qWait(1000);

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); }); // ASSERT falho: encerra o pjsua antes de destruir a janela
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        dlg.Subscribe(); // no app: depois do registro (onRegState2) ou conta sem registro
        EXPECT_TRUE(dlg.isSubscribed);

        Contact c;
        c.name = _T("CLI");
        c.number = _T("127.0.0.1");
        c.presence = true;
        dlg.pageContacts->ContactAdd(c, TRUE);
        Contact* pc = dlg.pageContacts->FindContact(_T("127.0.0.1"));
        ASSERT_NE(pc, nullptr);
        ASSERT_TRUE(QTest::qWaitFor([&] { return pc->image == MSIP_CONTACT_ICON_ONLINE; }, 6000))
            << "imagem " << pc->image << "\n" << cli.readAll().toStdString();
        EXPECT_FALSE(dlg.pageContacts->List()->topLevelItem(0)->icon(0).isNull());

        dlg.Unsubscribe();
        EXPECT_FALSE(dlg.isSubscribed);
        EXPECT_EQ(pc->image, MSIP_CONTACT_ICON_DEFAULT);
        dlg.OnClose();
    }
    cli.waitForFinished(12000);
}
