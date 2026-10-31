// Teste de interface (offscreen) do Dialer: teclado, campo de número, botões
// Chamar/Rediscar/Apagar, histórico [Dialed] e mudo.

#include "Dialer.h"
#include "PasswordStore.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QComboBox>
#include <QPushButton>
#include <QTest>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

TEST(Dialer, TecladoBotoesEHistorico)
{
    char tmpl[] = "/tmp/msip-dialer-XXXXXX";
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
    // [Dialed] pré-existente, como no MicroSIP.ini
    WritePrivateProfileString(_T("Dialed"), _T("0"), _T("1002"), accountSettings.iniFile);

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        Dialer* d = dlg.pageDialer;
        ASSERT_NE(d, nullptr);
        auto* number = d->findChild<QComboBox*>(QStringLiteral("IDC_NUMBER"));
        auto* call = d->findChild<QPushButton*>(QStringLiteral("IDC_CALL"));
        auto* redial = d->findChild<QPushButton*>(QStringLiteral("IDC_REDIAL"));
        auto* del = d->findChild<QPushButton*>(QStringLiteral("IDC_DELETE"));
        ASSERT_TRUE(number && call && redial && del);

        // histórico carregado, campo vazio: Chamar desabilitado e "R" visível
        EXPECT_EQ(number->count(), 1);
        EXPECT_EQ(number->itemText(0), QStringLiteral("1002"));
        EXPECT_TRUE(number->currentText().isEmpty());
        EXPECT_FALSE(call->isEnabled());
        EXPECT_FALSE(redial->isHidden());
        EXPECT_TRUE(del->isHidden());

        // teclas 1, 0, 0, 1 e "+" (sem DTMF)
        for (const char* id : {"IDC_KEY_1", "IDC_KEY_0", "IDC_KEY_0", "IDC_KEY_1"})
            d->findChild<QPushButton*>(QString::fromLatin1(id))->click();
        EXPECT_EQ(number->currentText(), QStringLiteral("1001"));
        EXPECT_TRUE(call->isEnabled());
        EXPECT_TRUE(redial->isHidden());
        EXPECT_FALSE(del->isHidden());

        // teclado físico fora do campo: dígitos vão para o número
        d->setFocus();
        QTest::keyClicks(d, QStringLiteral("23"));
        EXPECT_EQ(number->currentText(), QStringLiteral("100123"));

        // apagar (<) e limpar (C)
        del->click();
        EXPECT_EQ(number->currentText(), QStringLiteral("10012"));
        d->findChild<QPushButton*>(QStringLiteral("IDC_CLEAR"))->click();
        EXPECT_TRUE(number->currentText().isEmpty());
        EXPECT_FALSE(call->isEnabled());

        // DialedAdd: novo número vai para o topo e é gravado em [Dialed]
        d->DialedAdd(_T("5555"));
        EXPECT_EQ(number->itemText(0), QStringLiteral("5555"));
        EXPECT_EQ(number->count(), 2);
        CString v;
        GetPrivateProfileString(_T("Dialed"), _T("0"), NULL, v.GetBuffer(64), 64, accountSettings.iniFile);
        v.ReleaseBuffer();
        EXPECT_EQ(v, _T("5555"));
        // repetido existente sobe para o topo sem duplicar
        d->DialedAdd(_T("1002"));
        EXPECT_EQ(number->itemText(0), QStringLiteral("1002"));
        EXPECT_EQ(number->count(), 2);

        // mudo alterna o estado
        EXPECT_FALSE(d->muteOutput);
        d->findChild<QPushButton*>(QStringLiteral("IDC_BUTTON_MUTE_OUTPUT"))->click();
        EXPECT_TRUE(d->muteOutput);
        d->findChild<QPushButton*>(QStringLiteral("IDC_BUTTON_MUTE_OUTPUT"))->click();
        EXPECT_FALSE(d->muteOutput);

        dlg.OnClose();
    }
    fs::remove_all(root);
}
