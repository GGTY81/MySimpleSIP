// Teste de interface (offscreen) do Dialer: teclado, campo de número, botões
// Chamar/Rediscar/Apagar, histórico [Dialed] e mudo.

#include "AccountDlg.h"
#include "Dialer.h"
#include "PasswordStore.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QTabBar>
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

// Enter na aba Telefone disca (IDOK → OnBnClickedOk) com o foco em qualquer controle da janela
// — campo de número, teclas/volumes do discador ou barra de abas —, uma única vez
TEST(Dialer, EnterDisca)
{
    char tmpl[] = "/tmp/msip-dialer-enter-XXXXXX";
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
    WritePrivateProfileString(_T("Settings"), _T("sourcePort"), _T("5084"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Settings"), _T("singleMode"), _T("0"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Dialed"), _T("0"), _T("1001"), accountSettings.iniFile);
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        ASSERT_TRUE(QTest::qWaitFor([] { return is_pjsua_running(); }, 5000));
        if (dlg.accountDlg) {
            dlg.accountDlg->close(); // sem conta: o diálogo Conta abre sozinho
        }
        Dialer* d = dlg.pageDialer;
        auto* number = d->findChild<QComboBox*>(QStringLiteral("IDC_NUMBER"));

        // a tecla vai para o controle com foco, como no teclado de verdade
        auto focusOn = [&](QWidget* w) {
            dlg.show();
            dlg.activateWindow();
            ASSERT_TRUE(QTest::qWaitForWindowActive(&dlg));
            // ao ser ativada, a janela põe o foco no campo de número (TabFocusSet, como no
            // original): espera isso passar e confirma que o foco ficou no controle pedido
            QTest::qWait(300);
            w->setFocus();
            QTest::qWait(100);
            ASSERT_EQ(QApplication::focusWidget(), w) << w->objectName().toStdString();
        };
        auto type = [](const QString& text) {
            for (QChar c : text)
                QTest::keyClick(QApplication::focusWidget(), c.toLatin1());
        };
        // as chamadas vão para portas sem ninguém (ficam chamando): conta-se quantas o Enter
        // criou, e o número discado vai para o topo do histórico (DialedAdd)
        auto enterCalls = [&](Qt::Key key, Qt::KeyboardModifiers mods, const char* what) {
            const QString dialed = number->currentText();
            const unsigned before = pjsua_call_get_count();
            QTest::keyClick(QApplication::focusWidget(), key, mods);
            EXPECT_TRUE(QTest::qWaitFor([&] { return pjsua_call_get_count() == before + 1; }, 2000)) << what;
            QTest::qWait(200);
            EXPECT_EQ(pjsua_call_get_count(), before + 1) << what << ": não discou uma única vez";
            EXPECT_EQ(number->itemText(0), dialed) << what;
            d->Clear();
        };

        // campo de número
        focusOn(number);
        type(QStringLiteral("127.0.0.1:5091"));
        enterCalls(Qt::Key_Return, Qt::NoModifier, "campo");

        // teclado físico com o foco num botão do discador (os dígitos vão para o campo)
        focusOn(d->findChild<QPushButton*>(QStringLiteral("IDC_KEY_5")));
        type(QStringLiteral("192.0.2.1")); // TEST-NET-1: nunca responde
        EXPECT_EQ(number->currentText(), QStringLiteral("192.0.2.1"));
        enterCalls(Qt::Key_Return, Qt::NoModifier, "botão");

        // foco fora do discador (barra de abas) e Enter do teclado numérico
        number->setEditText(QStringLiteral("127.0.0.1:5093"));
        focusOn(dlg.findChild<QTabBar*>());
        enterCalls(Qt::Key_Enter, Qt::KeypadModifier, "abas/teclado numérico");

        // campo vazio: Enter não faz nada
        const unsigned before = pjsua_call_get_count();
        focusOn(number);
        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        QTest::qWait(300);
        EXPECT_EQ(pjsua_call_get_count(), before);
        msip_call_hangup_all();
    }
    fs::remove_all(root);
}
