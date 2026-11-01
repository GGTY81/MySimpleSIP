// Monitores do sistema (Fase 7): suspensão/retomada (WM_POWERBROADCAST via logind),
// mudança de rede (netlink → UM_NETWORK_CHANGE → OnTimerNetworkChange), troca de
// dispositivos de áudio (WM_DEVICECHANGE → IDT_TIMER_SWITCH_DEVICES) e a inibição de
// ociosidade/suspensão durante chamadas.

#include "PasswordStore.h"
#include "SystemMonitors.h"
#include "UiCommon.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QScopeGuard>
#include <QTest>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

TEST(SystemMonitors, SuspensaoRedeEDispositivos)
{
    char tmpl[] = "/tmp/msip-monitors-XXXXXX";
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

    // o monitor de rede abre o netlink do kernel sem privilégios
    {
        NetworkMonitor network;
        EXPECT_TRUE(network.isActive());
    }

    PrivateProfileFlushCache();
    SetPasswordStore(nullptr);
    ResetAccountSettings();
    accountSettings.Init();
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5082"), accountSettings.iniFile);
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
        ASSERT_NE(dlg.m_powerMonitor, nullptr);
        EXPECT_NE(dlg.m_networkMonitor, nullptr); // PlatformMonitorsStart no PJCreate

        // PrepareForSleep(true) → PBT_APMSUSPEND: encerra o pjsua (e os monitores dele)
        dlg.m_powerMonitor->onPrepareForSleep(true);
        EXPECT_FALSE(is_pjsua_running());
        EXPECT_EQ(dlg.m_networkMonitor, nullptr);
        // PrepareForSleep(false) → PBT_APMRESUMEAUTOMATIC: UM_RECREATE recria a sessão
        dlg.m_powerMonitor->onPrepareForSleep(false);
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        EXPECT_NE(dlg.m_networkMonitor, nullptr);

        // UM_NETWORK_CHANGE: espera 1 s e verifica a rota (OnTimerNetworkChange)
        const int network = statistics.winNetworkChanged;
        dlg.PostMessage(UM_NETWORK_CHANGE, 1, 0);
        ASSERT_TRUE(QTest::qWaitFor([&] { return statistics.winNetworkChanged == network + 1; }, 4000));
        EXPECT_TRUE(is_pjsua_running());

        // WM_DEVICECHANGE: refaz a lista de dispositivos depois de 1,5 s
        const int audio = statistics.winAudioChanged;
        dlg.PostMessage(WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 1);
        ASSERT_TRUE(QTest::qWaitFor([&] { return statistics.winAudioChanged == audio + 1; }, 5000));
        EXPECT_TRUE(is_pjsua_running());

        // inibição durante chamadas: liberada quando não há mais chamadas
        dlg.m_powerMonitor->setCallActive(true);
        dlg.m_powerMonitor->setCallActive(false);
        EXPECT_FALSE(dlg.m_powerMonitor->callInhibited());
    }
}
