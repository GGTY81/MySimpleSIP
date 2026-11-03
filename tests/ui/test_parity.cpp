// Fase 10 (paridade): verificação de atualizações pela última Release do repositório (API do
// GitHub, resposta simulada — sem rede), "Visitar site"/"Ajuda" ativos no menu e
// WM_WTSSESSION_CHANGE (sessão reativada no logind) refazendo os dispositivos.

#include "PasswordStore.h"
#include "SystemMonitors.h"
#include "UiCommon.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QMenu>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTest>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

TEST(Parity, ComparacaoDeVersoes)
{
    EXPECT_TRUE(IsNewerVersion(_T("v1.0.0"), _T("0.9.0")));
    EXPECT_TRUE(IsNewerVersion(_T("1.0.1"), _T("1.0.0")));
    EXPECT_TRUE(IsNewerVersion(_T("1.1"), _T("1.0.9")));
    EXPECT_TRUE(IsNewerVersion(_T("0.10.0"), _T("0.9.9"))); // numérico, não texto
    EXPECT_FALSE(IsNewerVersion(_T("1.0.0"), _T("1.0.0")));
    EXPECT_FALSE(IsNewerVersion(_T("v0.9.0"), _T("1.0.0")));
    EXPECT_FALSE(IsNewerVersion(_T("1.0"), _T("1.0.0")));
    EXPECT_FALSE(IsNewerVersion(_T("v2.0.0-beta"), _T("1.0.0"))); // pré-versões não contam
    EXPECT_FALSE(IsNewerVersion(_T(""), _T("1.0.0")));
}

static URLGetAsyncData* Response(DWORD status, const char* body)
{
    auto* r = new URLGetAsyncData();
    r->statusCode = status;
    r->body = body;
    return r;
}

TEST(Parity, AtualizacoesSiteESessao)
{
    char tmpl[] = "/tmp/msip-parity-XXXXXX";
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
    // sem verificação automática na criação (nada de rede no teste)
    WritePrivateProfileString(_T("Settings"), _T("updatesInterval"), _T("never"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5082"), accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        dlg.updateCheckerSilent = true;

        // Release mais nova, da página do projeto: atualização encontrada
        dlg.OnUpdateCheckerLoaded((WPARAM) Response(200, R"({"tag_name":"v99.0.0",
            "html_url":"https://github.com/GGTY81/MySimpleSIP/releases/tag/v99.0.0","body":"Novidades"})"), 0);
        EXPECT_TRUE(dlg.updateCheckerDone);
        EXPECT_EQ(dlg.lastUpdateFound, CString(_T("v99.0.0")));

        // mesma versão, ou endereço fora do projeto: nada
        dlg.lastUpdateFound.Empty();
        const std::string same = std::string(R"({"tag_name":"v)") + MYSIMPLESIP_VERSION +
                                 R"(","html_url":"https://github.com/GGTY81/MySimpleSIP/releases/tag/x"})";
        dlg.OnUpdateCheckerLoaded((WPARAM) Response(200, same.c_str()), 0);
        EXPECT_TRUE(dlg.lastUpdateFound.IsEmpty());
        dlg.OnUpdateCheckerLoaded((WPARAM) Response(200, R"({"tag_name":"v99.0.0","html_url":"https://example.com/x"})"), 0);
        EXPECT_TRUE(dlg.lastUpdateFound.IsEmpty());
        // ainda sem Release (404) ou resposta quebrada: nada, sem travar
        dlg.OnUpdateCheckerLoaded((WPARAM) Response(404, R"({"message":"Not Found"})"), 0);
        dlg.OnUpdateCheckerLoaded((WPARAM) Response(200, "{quebrado"), 0);
        EXPECT_TRUE(dlg.lastUpdateFound.IsEmpty());

        // menu: "Visitar site" e "Ajuda" ativos (antes desativados)
        dlg.MainPopupMenu(true);
        QMenu* menu = nullptr;
        for (QWidget* w : QApplication::topLevelWidgets()) {
            if (auto* m = qobject_cast<QMenu*>(w); m && m->isVisible())
                menu = m;
        }
        ASSERT_NE(menu, nullptr);
        int found = 0;
        for (QAction* a : menu->actions()) {
            if (a->text().startsWith(TR(_T("Visit Website"))) || a->text().startsWith(TR(_T("Help")))) {
                EXPECT_TRUE(a->isEnabled()) << a->text().toStdString();
                found++;
            }
        }
        EXPECT_EQ(found, 2);
        menu->close();

        // sessão reativada no logind → WM_WTSSESSION_CHANGE (só "Active" = true conta)
        ASSERT_NE(dlg.m_powerMonitor, nullptr);
        QSignalSpy spy(dlg.m_powerMonitor, &PowerMonitor::sessionActivated);
        dlg.m_powerMonitor->onSessionPropertiesChanged(QStringLiteral("org.freedesktop.login1.Session"),
                                                       {{QStringLiteral("Active"), false}}, {});
        dlg.m_powerMonitor->onSessionPropertiesChanged(QStringLiteral("org.freedesktop.login1.Session"),
                                                       {{QStringLiteral("IdleHint"), true}}, {});
        EXPECT_EQ(spy.count(), 0);
        dlg.m_powerMonitor->onSessionPropertiesChanged(QStringLiteral("org.freedesktop.login1.Session"),
                                                       {{QStringLiteral("Active"), true}}, {});
        EXPECT_EQ(spy.count(), 1);
        // o temporizador de troca de dispositivos (1,5 s) refaz a lista sem derrubar o pjsua
        QTest::qWait(2000);
        EXPECT_TRUE(is_pjsua_running());
    }
}
