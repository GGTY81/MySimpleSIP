// Teste de interface (offscreen) de Exportar/Importar configuração (ExportDlg/ImportDlg):
// listas de contas (setas, Delete), arquivo exportado (contas renumeradas, senha ofuscada,
// configurações, atalhos), importação acrescentando contas e o reinício ao importar as
// configurações.

#include "ExportDlg.h"
#include "ImportDlg.h"
#include "PasswordStore.h"
#include "UiCommon.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QCheckBox>
#include <QListWidget>
#include <QPushButton>
#include <QScopeGuard>
#include <QTest>
#include <QToolButton>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

static CString IniValue(LPCTSTR section, LPCTSTR key, const CString& file)
{
    CString v;
    GetPrivateProfileString(section, key, _T("<ausente>"), v.GetBuffer(512), 512, file);
    v.ReleaseBuffer();
    return v;
}

static void SaveAccount(int id, LPCTSTR username, LPCTSTR password)
{
    Account a;
    a.domain = _T("pbx.example.com");
    a.server = _T("pbx.example.com");
    a.username = username;
    a.password = password;
    a.rememberPassword = true;
    accountSettings.AccountSave(id, &a);
}

TEST(ExportImport, ExportaReordenaEImporta)
{
    char tmpl[] = "/tmp/msip-exportimport-XXXXXX";
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
    accountSettings.enableLocalAccount = false;
    accountSettings.SettingsSave();
    SaveAccount(1, _T("1001"), _T("senha-1001"));
    SaveAccount(2, _T("1002"), _T("senha-1002"));
    shortcuts.RemoveAll();
    Shortcut sc;
    ShortcutDecode(_T("Suporte;300;call;;0;Suporte"), &sc);
    shortcuts.Add(sc);
    ShortcutsSave();
    shortcuts.RemoveAll();
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();
    msip_restart_requested = false;
    const CString exported = CString((root / "exportado.ini").wstring().c_str());

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;

        // --- exportar: todas as contas já vêm selecionadas ---
        dlg.OnMenuExport();
        ExportDlg* e = dlg.exportDlg;
        ASSERT_NE(e, nullptr);
        dlg.OnMenuExport(); // segunda vez: a mesma janela
        EXPECT_EQ(dlg.exportDlg, e);
        ASSERT_EQ(e->ListSelected()->count(), 2);
        EXPECT_EQ(e->ListAll()->count(), 0);
        EXPECT_EQ(e->ListSelected()->item(0)->text(), QStringLiteral("1: 1001@pbx.example.com"));
        EXPECT_TRUE(e->m_layout.Check(QStringLiteral("IDC_EXPORT_SETTINGS"))->isChecked());
        EXPECT_TRUE(e->m_layout.Check(QStringLiteral("IDC_EXPORT_SHORTCUTS"))->isChecked());

        // conta 2 para o topo (↑), conta 1 retirada com Delete e devolvida com Espaço
        e->ListSelected()->setCurrentRow(1);
        e->m_layout.Item(QStringLiteral("IDC_EXPORT_SPIN_ORDER"))->findChild<QToolButton*>(QStringLiteral("first"))->click();
        EXPECT_EQ(e->ListSelected()->item(0)->text(), QStringLiteral("2: 1002@pbx.example.com"));
        e->ListSelected()->setCurrentRow(1);
        QTest::keyClick(e->ListSelected(), Qt::Key_Delete);
        ASSERT_EQ(e->ListSelected()->count(), 1);
        ASSERT_EQ(e->ListAll()->count(), 1);
        e->ListAll()->setCurrentRow(0);
        QTest::keyClick(e->ListAll(), Qt::Key_Space);
        EXPECT_EQ(e->ListSelected()->count(), 2);
        // e retirada de novo pela seta ←
        e->ListSelected()->setCurrentRow(1);
        e->m_layout.Item(QStringLiteral("IDC_EXPORT_SPIN_MODIFY"))->findChild<QToolButton*>(QStringLiteral("first"))->click();
        ASSERT_EQ(e->ListSelected()->count(), 1);

        e->Export(exported);
        QApplication::processEvents();
        EXPECT_EQ(dlg.exportDlg, nullptr);

        EXPECT_EQ(IniValue(_T("Global"), _T("version"), exported), CString(_T(_GLOBAL_VERSION)));
        // a conta 2 do perfil vira a Account1 do arquivo, com a senha ofuscada no próprio arquivo
        EXPECT_EQ(IniValue(_T("Account1"), _T("username"), exported), CString(_T("1002")));
        EXPECT_EQ(IniValue(_T("Account1"), _T("password"), exported), PasswordObfuscate(_T("senha-1002")));
        EXPECT_EQ(IniValue(_T("Account2"), _T("username"), exported), CString(_T("<ausente>")));
        EXPECT_EQ(IniValue(_T("Shortcuts"), _T("0"), exported), CString(_T("Suporte;300;call;;0;Suporte")));
        EXPECT_NE(IniValue(_T("Settings"), _T("singleMode"), exported), CString(_T("<ausente>")));
        // o perfil continua igual
        Account check;
        ASSERT_TRUE(accountSettings.AccountLoad(1, &check));
        EXPECT_EQ(check.password, CString(_T("senha-1001")));

        // --- importar só a conta: entra depois das existentes, sem reiniciar ---
        dlg.importDlg = new ImportDlg(&dlg);
        ImportDlg* im = dlg.importDlg;
        im->Init(exported);
        ASSERT_EQ(im->ListSelected()->count(), 1);
        EXPECT_EQ(im->ListSelected()->item(0)->text(), QStringLiteral("1: 1002@pbx.example.com"));
        EXPECT_TRUE(im->m_layout.Check(QStringLiteral("IDC_IMPORT_SETTINGS"))->isChecked());
        im->m_layout.Check(QStringLiteral("IDC_IMPORT_SETTINGS"))->setChecked(false);
        im->m_layout.Check(QStringLiteral("IDC_IMPORT_SHORTCUTS"))->setChecked(false);
        im->m_layout.Button(QStringLiteral("IDOK"))->click();
        QApplication::processEvents();
        EXPECT_EQ(dlg.importDlg, nullptr);
        EXPECT_FALSE(msip_restart_requested);
        Account imported;
        ASSERT_TRUE(accountSettings.AccountLoad(3, &imported));
        EXPECT_EQ(imported.username, CString(_T("1002")));
        EXPECT_EQ(imported.password, CString(_T("senha-1002")));

        // --- importar as configurações reinicia o aplicativo (UM_RESTART) ---
        WritePrivateProfileString(_T("Settings"), _T("singleMode"), _T("0"), exported);
        dlg.importDlg = new ImportDlg(&dlg);
        dlg.importDlg->Init(exported);
        dlg.importDlg->ListSelected()->clear();
        dlg.importDlg->m_layout.Check(QStringLiteral("IDC_IMPORT_SHORTCUTS"))->setChecked(false);
        dlg.importDlg->m_layout.Button(QStringLiteral("IDOK"))->click();
        EXPECT_FALSE(accountSettings.singleMode);
        ASSERT_TRUE(QTest::qWaitFor([] { return msip_restart_requested; }, 3000));
    }
    msip_restart_requested = false;
}
