// Fone USB HID (Hid.cpp portado, com um fone falso no lugar do hidapi) e teclas de mídia
// (player MPRIS → onShellHookMessage): escolha da coleção de telefonia, LEDs, mudo pelo fone,
// fone desconectado e, com MSIP_TEST_SIP=1, atender/desligar pelo gancho do fone e pelas
// teclas de mídia numa chamada real vinda da msip-cli.

#include "Dialer.h"
#include "Hid.h"
#include "MediaKeys.h"
#include "MessagesDlg.h"
#include "PasswordStore.h"
#include "RinginDlg.h"
#include "UiCommon.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusVariant>
#include <QFile>
#include <QProcess>
#include <QPushButton>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTest>

#include <cstdlib>
#include <deque>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace {

// mesmo descritor do teste do HidParser: volume (report 1) + telefonia (entrada 2, LEDs 3)
const std::vector<unsigned char> kHeadset = {
    0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x01, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x02,
    0x09, 0xE9, 0x09, 0xEA, 0x81, 0x02, 0x95, 0x06, 0x81, 0x03, 0xC0,
    0x05, 0x0B, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x02, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x01,
    0x09, 0x20, 0x81, 0x22, 0x09, 0x2F, 0x81, 0x06, 0x09, 0x24, 0x81, 0x22, 0x95, 0x05, 0x81, 0x03,
    0x85, 0x03, 0x05, 0x08, 0x95, 0x03, 0x09, 0x17, 0x09, 0x18, 0x09, 0x09, 0x91, 0x22, 0x95, 0x05,
    0x91, 0x03, 0xC0,
};

class FakeHeadset : public HidIo {
public:
    FakeHeadset()
    {
        // como o hidapi no Linux: uma entrada por coleção de nível superior, mesmo caminho
        m_infos[0] = Info("mouse", 0x045e, 0x0001, 0x0002, L"Mouse");
        m_infos[1] = Info("headset", 0x0b0e, 0x000C, 0x0001, L"Fone");
        m_infos[2] = Info("headset", 0x0b0e, 0x000B, 0x0005, L"Fone");
        m_infos[0].next = &m_infos[1];
        m_infos[1].next = &m_infos[2];
    }
    hid_device_info* enumerate() override { return present ? &m_infos[0] : nullptr; }
    void free_enumeration(hid_device_info*) override {}
    void* open_path(const char* path) override
    {
        if (!present || std::string(path) != "headset")
            return nullptr; // o mouse não abre (sem permissão no /dev/hidraw*)
        opened++;
        open = true;
        return this;
    }
    int get_report_descriptor(void*, unsigned char* buf, size_t size) override
    {
        const size_t n = std::min(size, kHeadset.size());
        std::copy(kHeadset.begin(), kHeadset.begin() + n, buf);
        return (int) n;
    }
    int read(void*, unsigned char* data, size_t length) override
    {
        if (!present)
            return -1; // desconectado
        if (reports.empty())
            return 0;
        const std::vector<unsigned char> r = reports.front();
        reports.pop_front();
        const size_t n = std::min(length, r.size());
        std::copy(r.begin(), r.begin() + n, data);
        return (int) n;
    }
    int write(void*, const unsigned char* data, size_t length) override
    {
        writes.emplace_back(data, data + length);
        return (int) length;
    }
    int set_nonblocking(void*, int) override { return 0; }
    const wchar_t* error(void*) override { return L"erro"; }
    void close(void*) override { open = false; }
    void exit() override {}

    // último estado dos LEDs enviado (byte de dados do report 3)
    int leds() const
    {
        for (auto it = writes.rbegin(); it != writes.rend(); ++it) {
            if (it->size() == 2 && (*it)[0] == 0x03)
                return (*it)[1];
        }
        return -1;
    }

    bool present = true;
    bool open = false;
    int opened = 0;
    std::deque<std::vector<unsigned char>> reports;
    std::vector<std::vector<unsigned char>> writes;

private:
    static hid_device_info Info(const char* path, unsigned short vid, unsigned short page, unsigned short usage, const wchar_t* product)
    {
        hid_device_info i{};
        i.path = const_cast<char*>(path);
        i.vendor_id = vid;
        i.usage_page = page;
        i.usage = usage;
        i.manufacturer_string = const_cast<wchar_t*>(L"Teste");
        i.product_string = const_cast<wchar_t*>(product);
        return i;
    }
    hid_device_info m_infos[3];
};

constexpr int LED_OFF_HOOK = 0x01, LED_RING = 0x02, LED_MUTE = 0x04;

fs::path PrepareProfile(const char* name, int sourcePort, bool mediaButtons)
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
    WritePrivateProfileString(_T("Settings"), _T("headsetSupport"), _T("1"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("enableMediaButtons"), mediaButtons ? _T("1") : _T("0"), accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();
    return root;
}

QVariant MprisProperty(const QString& service, const QString& name)
{
    QDBusMessage m = QDBusMessage::createMethodCall(service, QStringLiteral("/org/mpris/MediaPlayer2"),
                                                    QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    m << QStringLiteral("org.mpris.MediaPlayer2.Player") << name;
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(m, 3000);
    (void) QTest::qWaitFor([&] { return call.isFinished(); }, 3000);
    const QDBusMessage reply = call.reply();
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty())
        return {};
    return reply.arguments().first().value<QDBusVariant>().variant();
}

void MprisCall(const QString& service, const QString& method)
{
    QDBusMessage m = QDBusMessage::createMethodCall(service, QStringLiteral("/org/mpris/MediaPlayer2"),
                                                    QStringLiteral("org.mpris.MediaPlayer2.Player"), method);
    QDBusConnection::sessionBus().asyncCall(m, 3000);
}

} // namespace

TEST(Headset, FoneHidLedsMudoEDesconexao)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    PrepareProfile("headset", 5077, false);
    FakeHeadset fake;
    Hid::SetIo(&fake);
    auto restoreIo = qScopeGuard([] { Hid::SetIo(nullptr); });

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));

        // PJCreate → Hid::OpenDevice: a coleção de telefonia do fone (não o mouse nem o volume)
        ASSERT_TRUE(Hid::IsOpen());
        EXPECT_EQ(fake.opened, 1);
        EXPECT_EQ(fake.leds(), 0); // UpdateLED inicial: tudo apagado
        Hid::OpenDevice();         // já aberto: nada muda
        EXPECT_EQ(fake.opened, 1);

        // LEDs: toque, fora do gancho
        Hid::SetRing(true);
        EXPECT_EQ(fake.leds(), LED_RING);
        Hid::SetOffhookRing(true, false);
        EXPECT_EQ(fake.leds(), LED_OFF_HOOK);
        Hid::SetOffhookRing(false, false);
        EXPECT_EQ(fake.leds(), 0);

        // botão de mudo do fone (relativo: cada aperto alterna) → MuteInput na janela
        Dialer* dialer = dlg.pageDialer;
        ASSERT_FALSE(dialer->muteInput);
        fake.reports.push_back({0x02, 0x02});
        ASSERT_TRUE(QTest::qWaitFor([&] { return dialer->muteInput; }, 2000)); // temporizador de 200 ms
        fake.reports.push_back({0x02, 0x00}); // soltar não muda (relativo)
        QTest::qWait(500);
        EXPECT_TRUE(dialer->muteInput);
        // o volume (outra coleção, report 1) não interfere
        fake.reports.push_back({0x01, 0x01});
        QTest::qWait(500);
        EXPECT_TRUE(dialer->muteInput);
        // mudo pelo botão da janela → LED do mudo segue
        dialer->findChild<QPushButton*>(QStringLiteral("IDC_BUTTON_MUTE_INPUT"))->click();
        EXPECT_FALSE(dialer->muteInput);
        dialer->findChild<QPushButton*>(QStringLiteral("IDC_BUTTON_MUTE_INPUT"))->click();
        EXPECT_TRUE(dialer->muteInput);
        EXPECT_EQ(fake.leds(), LED_MUTE);
        fake.reports.push_back({0x02, 0x02});
        ASSERT_TRUE(QTest::qWaitFor([&] { return !dialer->muteInput; }, 2000));
        EXPECT_EQ(fake.leds(), 0);

        // fone desconectado: hid_read com erro fecha o dispositivo
        fake.present = false;
        ASSERT_TRUE(QTest::qWaitFor([] { return !Hid::IsOpen(); }, 2000));
        EXPECT_FALSE(fake.open);
        // reconectado: a troca de dispositivos (IDT_TIMER_SWITCH_DEVICES) abre de novo
        fake.present = true;
        dlg.OnTimer(IDT_TIMER_SWITCH_DEVICES);
        EXPECT_TRUE(Hid::IsOpen());
        EXPECT_EQ(fake.opened, 2);
    }
    // PJDestroy fecha o fone
    EXPECT_FALSE(Hid::IsOpen());
    EXPECT_FALSE(fake.open);
}

TEST(Headset, TeclasDeMidiaPeloMpris)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);
    if (!QDBusConnection::sessionBus().isConnected())
        GTEST_SKIP() << "sem barramento D-Bus da sessão";
    PrepareProfile("mediakeys", 5079, true);
    FakeHeadset fake;
    fake.present = false;
    Hid::SetIo(&fake);
    auto restoreIo = qScopeGuard([] { Hid::SetIo(nullptr); });

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        ASSERT_NE(dlg.m_mediaKeys, nullptr);
        ASSERT_TRUE(dlg.m_mediaKeys->isEnabled());
        const QString service = dlg.m_mediaKeys->serviceName();

        // fora de chamada: parado e sem "tocar" (o GNOME não mostra o player)
        EXPECT_EQ(MprisProperty(service, QStringLiteral("PlaybackStatus")).toString(), QStringLiteral("Stopped"));
        EXPECT_EQ(MprisProperty(service, QStringLiteral("CanPlay")).toBool(), false);
        dlg.m_mediaKeys->setCallActive(true);
        EXPECT_EQ(MprisProperty(service, QStringLiteral("PlaybackStatus")).toString(), QStringLiteral("Playing"));
        dlg.m_mediaKeys->setCallActive(false);

        // tecla (comando MPRIS) → APPCOMMAND
        QSignalSpy spy(dlg.m_mediaKeys, &MediaKeys::appCommand);
        MprisCall(service, QStringLiteral("Stop"));
        ASSERT_TRUE(QTest::qWaitFor([&] { return spy.count() == 1; }, 3000));
        EXPECT_EQ(spy.at(0).at(0).toInt(), APPCOMMAND_MEDIA_STOP);
        MprisCall(service, QStringLiteral("PlayPause"));
        ASSERT_TRUE(QTest::qWaitFor([&] { return spy.count() == 2; }, 3000));
        EXPECT_EQ(spy.at(1).at(0).toInt(), APPCOMMAND_MEDIA_PLAY_PAUSE);

        // Configurações: desmarcar recria o pjsua e retira o player
        accountSettings.enableMediaButtons = false;
        dlg.PJDestroy();
        dlg.OnRecreate(0, 0);
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        EXPECT_FALSE(dlg.m_mediaKeys->isEnabled());
    }
}

TEST(Headset, AtenderEDesligarPeloFoneEPelasTeclas)
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
    const fs::path root = PrepareProfile("headset-sip", 5070, true);
    FakeHeadset fake;
    Hid::SetIo(&fake);
    auto restoreIo = qScopeGuard([] { Hid::SetIo(nullptr); });

    QtMessageBackend backend;
    CmainDlg dlg(&backend, true);
    auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
    dlg.nullSoundDevice = true;
    ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
    ASSERT_TRUE(Hid::IsOpen());
    MessagesDlg* m = dlg.messagesDlg;

    QProcess cli;
    const QString script = QString::fromStdString((root / "cli.txt").string());
    {
        QFile f(script);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("wait 1\ncall 127.0.0.1:5070\nwait 7\ncall 127.0.0.1:5070\nwait 7\nquit\n");
    }
    cli.setProcessChannelMode(QProcess::MergedChannels);
    cli.start(QStringLiteral(MSIP_CLI_PATH), {QStringLiteral("--profile"), QString::fromStdString((root / "cli").string()),
                                              QStringLiteral("--null-audio"), QStringLiteral("--script"), script});
    ASSERT_TRUE(cli.waitForStarted());

    // 1ª chamada: tocando → LED de toque e player MPRIS "tocando"; tocar/pausar atende, parar desliga
    ASSERT_TRUE(QTest::qWaitFor([&] { return dlg.ringinDlgs.GetCount() == 1; }, 8000)) << cli.readAll().toStdString();
    EXPECT_TRUE(fake.leds() & LED_RING);
    EXPECT_TRUE(dlg.m_mediaKeys->callActive());
    dlg.onShellHookMessage(HSHELL_APPCOMMAND, (LPARAM) (APPCOMMAND_MEDIA_PLAY_PAUSE << 16));
    ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(false, true) == 1; }, 5000));
    EXPECT_EQ(dlg.ringinDlgs.GetCount(), 0);
    EXPECT_EQ(fake.leds(), LED_OFF_HOOK);
    dlg.onShellHookMessage(HSHELL_APPCOMMAND, (LPARAM) (APPCOMMAND_MEDIA_STOP << 16));
    ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(true) == 0; }, 5000));
    ASSERT_TRUE(QTest::qWaitFor([&] { return fake.leds() == 0; }, 3000));
    EXPECT_FALSE(dlg.m_mediaKeys->callActive());

    // 2ª chamada: tirar o fone do gancho atende; pôr de volta desliga
    ASSERT_TRUE(QTest::qWaitFor([&] { return dlg.ringinDlgs.GetCount() == 1; }, 10000)) << cli.readAll().toStdString();
    fake.reports.push_back({0x02, 0x01});
    ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(false, true) == 1; }, 5000));
    EXPECT_EQ(fake.leds(), LED_OFF_HOOK);
    fake.reports.push_back({0x02, 0x00});
    ASSERT_TRUE(QTest::qWaitFor([&] { return m->GetCallsCount(true) == 0; }, 5000));

    cli.waitForFinished(20000);
    const QString log = QString::fromUtf8(cli.readAll());
    EXPECT_EQ(log.count(QStringLiteral("CONFIRMED")), 2) << log.toStdString();
}
