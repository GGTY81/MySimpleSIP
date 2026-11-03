// 1.0.0: links "?" dos diálogos Conta, Configurações e Atalhos visíveis e apontando para
// âncoras que existem no docs/MANUAL.md; diálogo "Sobre" com os créditos; botões de estado
// do discador (DND, AA, CONF, REC) com fundo colorido quando ligados.

#include "AboutDlg.h"
#include "AccountDlg.h"
#include "Dialer.h"
#include "PasswordStore.h"
#include "SettingsDlg.h"
#include "ShortcutsDlg.h"
#include "UiCommon.h"
#include "global.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSet>
#include <QTest>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

QSet<QString> ManualAnchors()
{
    QFile f(QStringLiteral(MSIP_SOURCE_DIR "/docs/MANUAL.md"));
    EXPECT_TRUE(f.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(f.readAll());
    QSet<QString> anchors;
    static const QRegularExpression rx(QStringLiteral("<a name=\"([^\"]+)\"></a>"));
    for (auto it = rx.globalMatch(text); it.hasNext();)
        anchors << it.next().captured(1);
    return anchors;
}

// links "?" de um diálogo: todos visíveis e com âncora no manual
int CheckHelpLinks(QWidget* dlg, const QSet<QString>& anchors)
{
    int count = 0;
    for (QLabel* l : dlg->findChildren<QLabel*>()) {
        const QString code = l->property("msipHelpCode").toString();
        if (code.isEmpty())
            continue;
        count++;
        EXPECT_TRUE(l->isVisibleTo(dlg)) << l->objectName().toStdString();
        const CString url = HelpURL(CS(code));
        const QString anchor = QS(url).section(QLatin1Char('#'), 1);
        EXPECT_TRUE(QS(url).startsWith(QStringLiteral("https://github.com/GGTY81/MySimpleSIP/blob/main/docs/MANUAL.md#")));
        EXPECT_TRUE(anchors.contains(anchor)) << "sem âncora no manual: " << anchor.toStdString();
    }
    return count;
}

QColor BackgroundColor(QPushButton* b)
{
    const QImage img = b->grab().toImage();
    return img.pixelColor(img.width() / 2, 4); // fundo acima do texto, dentro da borda
}

bool IsColored(const QColor& c)
{
    return c.saturation() > 120;
}

} // namespace

TEST(HelpAbout, LinksSobreEBotoes)
{
    char tmpl[] = "/tmp/msip-help-XXXXXX";
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
    WritePrivateProfileString(_T("Settings"), _T("updatesInterval"), _T("never"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5083"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("DND"), _T("1"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("AA"), _T("0"), accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();

    const QSet<QString> anchors = ManualAnchors();
    ASSERT_GT(anchors.size(), 40);

    QtMessageBackend backend;
    CmainDlg dlg(&backend, true);
    auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
    dlg.nullSoundDevice = true;
    ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));

    // --- links "?" ---
    dlg.OnMenuAccountAdd();
    ASSERT_NE(dlg.accountDlg, nullptr);
    EXPECT_EQ(CheckHelpLinks(dlg.accountDlg, anchors), 18);
    dlg.accountDlg->close();

    dlg.OnMenuSettings();
    ASSERT_NE(dlg.settingsDlg, nullptr);
    EXPECT_EQ(CheckHelpLinks(dlg.settingsDlg, anchors), 23);
    dlg.settingsDlg->OnClose();

    dlg.OnMenuShortcuts();
    ASSERT_NE(dlg.shortcutsDlg, nullptr);
    EXPECT_EQ(CheckHelpLinks(dlg.shortcutsDlg, anchors), 3);
    dlg.shortcutsDlg->close();

    // --- Sobre ---
    dlg.MainPopupMenu(true);
    QMenu* menu = nullptr;
    for (QWidget* w : QApplication::topLevelWidgets()) {
        if (auto* m = qobject_cast<QMenu*>(w); m && m->isVisible())
            menu = m;
    }
    ASSERT_NE(menu, nullptr);
    QAction* about = nullptr;
    for (QAction* a : menu->actions()) {
        if (a->text() == TR(_T("About")))
            about = a;
    }
    ASSERT_NE(about, nullptr);
    menu->close();
    about = nullptr;
    dlg.OnMenuAbout();
    ASSERT_FALSE(dlg.aboutDlg.isNull());
    AboutDlg* aboutDlg = dlg.aboutDlg;
    EXPECT_TRUE(aboutDlg->isVisible());
    const QString credits = aboutDlg->m_credits->text();
    EXPECT_TRUE(credits.contains(QStringLiteral("Giovanni Grimaldi Torelly")));
    EXPECT_TRUE(credits.contains(QStringLiteral("MicroSIP 3.22.16")));
    EXPECT_TRUE(credits.contains(QStringLiteral("2011-2026 MicroSIP")));
    EXPECT_TRUE(credits.contains(QStringLiteral("https://github.com/GGTY81/MySimpleSIP")));
    EXPECT_TRUE(credits.contains(QStringLiteral("docs/MANUAL.md")));
    dlg.OnMenuAbout(); // de novo: o mesmo diálogo
    EXPECT_EQ(dlg.aboutDlg.data(), aboutDlg);
    aboutDlg->close();
    ASSERT_TRUE(QTest::qWaitFor([&] { return dlg.aboutDlg.isNull(); }, 2000));

    // --- botões de estado: ligados ficam coloridos ---
    Dialer* d = dlg.pageDialer;
    ASSERT_NE(d->m_ButtonDND, nullptr);
    ASSERT_NE(d->m_ButtonAA, nullptr);
    ASSERT_NE(d->m_ButtonConf, nullptr);
    ASSERT_NE(d->m_ButtonRec, nullptr);
    EXPECT_TRUE(d->m_ButtonDND->isChecked());
    EXPECT_TRUE(IsColored(BackgroundColor(d->m_ButtonDND)));
    EXPECT_FALSE(IsColored(BackgroundColor(d->m_ButtonAA)));
    d->m_ButtonAA->click();
    EXPECT_TRUE(accountSettings.AA);
    EXPECT_TRUE(IsColored(BackgroundColor(d->m_ButtonAA)));
    dlg.SwitchDND(); // desliga
    EXPECT_FALSE(d->m_ButtonDND->isChecked());
    EXPECT_FALSE(IsColored(BackgroundColor(d->m_ButtonDND)));
    // REC gravando (o estado vem de UpdateRecButton → SetCheckREC)
    d->m_ButtonRec->setEnabled(true);
    d->SetCheckREC(true);
    const QColor rec = BackgroundColor(d->m_ButtonRec);
    EXPECT_TRUE(IsColored(rec));
    EXPECT_GT(rec.red(), rec.blue());
    d->SetCheckREC(false);
    EXPECT_FALSE(IsColored(BackgroundColor(d->m_ButtonRec)));
    // CONF: sem conferência não fica destacado; com a propriedade, fica
    d->m_ButtonConf->setEnabled(true);
    d->UpdateConfActive();
    EXPECT_FALSE(IsColored(BackgroundColor(d->m_ButtonConf)));
    SetStateButtonActive(d->m_ButtonConf, true);
    EXPECT_TRUE(IsColored(BackgroundColor(d->m_ButtonConf)));
    d->UpdateConfActive(); // sem chamadas: volta ao normal
    EXPECT_FALSE(IsColored(BackgroundColor(d->m_ButtonConf)));
}
