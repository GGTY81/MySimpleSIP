// Teste de interface (offscreen) do AccountDlg: preenche os campos pelos IDs do .rc,
// salva e confere o INI. Sem pjsua em execução e sem chaveiro (senha ofuscada no INI).

#include "AccountDlg.h"
#include "PasswordStore.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

CString IniValue(LPCTSTR section, LPCTSTR key)
{
    CString s;
    GetPrivateProfileString(section, key, _T("<ausente>"), s.GetBuffer(512), 512, accountSettings.iniFile);
    s.ReleaseBuffer();
    return s;
}

} // namespace

TEST(AccountDlg, NovaContaSalvaNoIni)
{
    char tmpl[] = "/tmp/msip-ui-XXXXXX";
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

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        dlg.OnMenuAccountAdd();
        AccountDlg* acc = dlg.accountDlg;
        ASSERT_NE(acc, nullptr);
        // novo: "Excluir conta" oculto, asteriscos de obrigatório visíveis
        EXPECT_TRUE(acc->findChild<QLabel*>(QStringLiteral("IDC_SYSLINK_ACCOUNT_DELETE"))->isHidden());
        EXPECT_FALSE(acc->findChild<QLabel*>(QStringLiteral("IDC_ACCOUNT_REQUIRED_USERNAME"))->isHidden());

        auto set = [acc](const char* id, const char* text) {
            acc->findChild<QLineEdit*>(QString::fromLatin1(id))->setText(QString::fromUtf8(text));
        };
        // sem usuário/domínio: aviso e nada salvo (o QMessageBox é modal; não clicamos Salvar)
        set("IDC_ACCOUNT_LABEL", "Conta Teste");
        set("IDC_EDIT_SERVER", "sip.example.com");
        set("IDC_EDIT_USERNAME", "1001");
        set("IDC_EDIT_DOMAIN", "sip.example.com");
        set("IDC_EDIT_PASSWORD", "  s3nh@  ");
        set("IDC_EDIT_DISPLAYNAME", "Usuário Teste");
        set("IDC_ACCOUNT_REGISTER_REFRESH", "0"); // ≤ 0 → PJSUA_REG_INTERVAL
        acc->findChild<QComboBox*>(QStringLiteral("IDC_TRANSPORT"))->setCurrentIndex(3); // TLS
        acc->findChild<QComboBox*>(QStringLiteral("IDC_SRTP"))->setCurrentIndex(2);      // mandatory
        acc->findChild<QComboBox*>(QStringLiteral("IDC_ACCOUNT_HIDE_CID"))->setCurrentIndex(2); // hard
        acc->findChild<QCheckBox*>(QStringLiteral("IDC_PUBLISH"))->setChecked(true);
        acc->findChild<QPushButton*>(QStringLiteral("IDOK"))->click();
        QApplication::processEvents();

        EXPECT_EQ(dlg.accountDlg, nullptr);
        EXPECT_EQ(accountSettings.accountId, 1);
        EXPECT_EQ(IniValue(_T("Account1"), _T("label")), _T("Conta Teste"));
        EXPECT_EQ(IniValue(_T("Account1"), _T("username")), _T("1001"));
        EXPECT_EQ(IniValue(_T("Account1"), _T("displayName")), _T("Usuário Teste"));
        EXPECT_EQ(IniValue(_T("Account1"), _T("transport")), _T("tls"));
        EXPECT_EQ(IniValue(_T("Account1"), _T("SRTP")), _T("mandatory"));
        EXPECT_EQ(IniValue(_T("Account1"), _T("hideCID")), _T("hard"));
        EXPECT_EQ(IniValue(_T("Account1"), _T("publish")), _T("1"));
        EXPECT_EQ(IniValue(_T("Account1"), _T("registerRefresh")), _T("300"));
        EXPECT_EQ(IniValue(_T("Settings"), _T("accountId")), _T("1"));
        // senha aparada (Trim) e, sem chaveiro, ofuscada
        EXPECT_EQ(IniValue(_T("Account1"), _T("password")).Left(6), _T("msip1:"));
        EXPECT_EQ(accountSettings.account.password, _T("s3nh@"));

        // editar: carrega os valores e mostra "Excluir conta"
        dlg.OnMenuAccountEdit(0);
        acc = dlg.accountDlg;
        ASSERT_NE(acc, nullptr);
        EXPECT_EQ(acc->findChild<QLineEdit*>(QStringLiteral("IDC_EDIT_USERNAME"))->text(), QStringLiteral("1001"));
        EXPECT_EQ(acc->findChild<QComboBox*>(QStringLiteral("IDC_TRANSPORT"))->currentIndex(), 3);
        EXPECT_FALSE(acc->findChild<QLabel*>(QStringLiteral("IDC_SYSLINK_ACCOUNT_DELETE"))->isHidden());
        acc->close();
        QApplication::processEvents();
        dlg.OnClose();
    }
    fs::remove_all(root);
}
