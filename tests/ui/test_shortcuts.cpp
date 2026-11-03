// Teste de interface (offscreen) dos atalhos do discador: botões na coluna lateral ou
// embaixo, alternância, ShortcutsDlg (carga, edição, salvar no INI), ShortcutAction,
// teclas de atalho da janela principal (Ctrl+S, Ctrl+Tab, Ctrl+O) e o pedido de reinício.

#include "Dialer.h"
#include "PasswordStore.h"
#include "ShortcutsDlg.h"
#include "Transfer.h"
#include "UiCommon.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QTabBar>
#include <QTest>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

static Shortcut MakeShortcut(LPCTSTR label, LPCTSTR number, LPCTSTR type, LPCTSTR number2 = _T(""), LPCTSTR label2 = _T(""))
{
    Shortcut s;
    s.label = label;
    s.number = number;
    s.type = type;
    s.number2 = number2;
    s.label2 = *label2 ? label2 : label;
    return s;
}

// Perfil temporário com os atalhos dados; devolve a pasta raiz.
static fs::path PrepareProfile(const char* name, bool bottom, const CArray<Shortcut, Shortcut>& list)
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
    accountSettings.enableShortcuts = true;
    accountSettings.shortcutsBottom = bottom;
    accountSettings.SettingsSave();
    shortcuts.RemoveAll();
    for (int i = 0; i < list.GetCount(); i++) {
        shortcuts.Add(list.GetAt(i));
    }
    ShortcutsSave();
    shortcuts.RemoveAll(); // o CmainDlg carrega do INI (ShortcutsLoad)
    PrivateProfileFlushCache();
    ResetAccountSettings();
    accountSettings.Init();
    return root;
}

TEST(Shortcuts, ColunaLateralDialogoEAcoes)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);

    CArray<Shortcut, Shortcut> list;
    list.Add(MakeShortcut(_T("Suporte"), _T("300"), MSIP_SHORTCUT_CALL));
    list.Add(MakeShortcut(_T("Tom 1"), _T("1"), MSIP_SHORTCUT_DTMF, _T("2"), _T("Tom 2")));
    list.Add(MakeShortcut(_T("Transferir"), _T(""), MSIP_SHORTCUT_BT));
    PrepareProfile("shortcuts", false, list);
    msip_restart_requested = false;

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        dlg.show(); // com a janela visível, resize() entrega o resizeEvent na hora
        QApplication::processEvents();

        // coluna lateral de 180 px à direita do discador
        EXPECT_TRUE(dlg.shortcutsEnabled);
        EXPECT_FALSE(dlg.shortcutsBottom);
        EXPECT_EQ(dlg.shortcutsCount, 3);
        EXPECT_EQ(dlg.widthAdd, 180);
        EXPECT_EQ(dlg.heightAdd, 0);
        Dialer* dialer = dlg.pageDialer;
        ASSERT_EQ(dialer->shortcutButtons.size(), 3);
        EXPECT_EQ(dialer->shortcutButtons[0]->text(), QStringLiteral("Suporte"));
        EXPECT_FALSE(dialer->shortcutButtons[0]->isCheckable());
        EXPECT_TRUE(dialer->shortcutButtons[1]->isCheckable());
        dialer->resize(400, 380);
        for (QPushButton* b : dialer->shortcutButtons) {
            EXPECT_GE(b->x(), 400 - 180);
            EXPECT_LE(b->geometry().right(), 400);
            EXPECT_GT(b->height(), 25);
        }
        EXPECT_LT(dialer->shortcutButtons[0]->y(), dialer->shortcutButtons[1]->y());
        // o teclado ocupa só a área à esquerda da coluna
        auto* key3 = dialer->findChild<QWidget*>(QStringLiteral("IDC_KEY_3"));
        ASSERT_NE(key3, nullptr);
        EXPECT_LE(key3->geometry().right(), 400 - 180);

        // alternância: o rótulo troca a cada clique (DTMF sem chamada não faz nada)
        dialer->shortcutButtons[1]->click();
        EXPECT_EQ(dialer->shortcutButtons[1]->text(), QStringLiteral("Tom 2"));
        dialer->shortcutButtons[1]->click();
        EXPECT_EQ(dialer->shortcutButtons[1]->text(), QStringLiteral("Tom 1"));

        // transferência cega sem número abre a janela de transferência
        dialer->shortcutButtons[2]->click();
        ASSERT_NE(dlg.transferDlg, nullptr);
        dlg.transferDlg->OnClose();
        QApplication::processEvents();

        // Ctrl+S (CBaseDialog::PreTranslateMessage → ID_SHORTCUTS) abre os Atalhos
        QTest::keyClick(&dlg, Qt::Key_S, Qt::ControlModifier);
        ASSERT_TRUE(QTest::qWaitFor([&] { return dlg.shortcutsDlg != nullptr; }, 2000));
        ShortcutsDlg* s = dlg.shortcutsDlg;
        dlg.OnMenuShortcuts(); // segunda vez: a mesma janela
        EXPECT_EQ(dlg.shortcutsDlg, s);
        auto& L = s->m_layout;
        EXPECT_TRUE(L.Check(QStringLiteral("IDC_SHORTCUTS_ENABLE"))->isChecked());
        EXPECT_FALSE(L.Check(QStringLiteral("IDC_SHORTCUTS_BOTTOM"))->isChecked());
        EXPECT_EQ(L.Edit(QStringLiteral("IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL+0"))->text(), QStringLiteral("Suporte"));
        EXPECT_EQ(L.Edit(QStringLiteral("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER+0"))->text(), QStringLiteral("300"));
        EXPECT_EQ(L.Combo(QStringLiteral("IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+0"))->count(), 10);
        EXPECT_EQ(L.Combo(QStringLiteral("IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+1"))->currentIndex(), 3); // DTMF
        EXPECT_EQ(L.Combo(QStringLiteral("IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+2"))->currentIndex(), 4); // transferência
        EXPECT_EQ(L.Combo(QStringLiteral("IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+7"))->itemText(7),
                  TR(_T("Call")) + QStringLiteral(" / ") + TR(_T("Blind Transfer")));
        // linha com alternância: número estreito e segundo rótulo/número visíveis
        EXPECT_TRUE(L.Check(QStringLiteral("IDC_SHORTCUTS_TOGGLE+1"))->isChecked());
        EXPECT_EQ(L.Item(QStringLiteral("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER+1"))->width(),
                  L.Item(QStringLiteral("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2+1"))->width());
        EXPECT_FALSE(L.Item(QStringLiteral("IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2+1"))->isHidden());
        EXPECT_FALSE(L.Item(QStringLiteral("IDC_SHORTCUTS_LABEL2"))->isHidden());
        EXPECT_TRUE(L.Item(QStringLiteral("IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL2+0"))->isHidden());
        EXPECT_FALSE(L.Item(QStringLiteral("IDC_SHORTCUTS_SYSLINK_TYPE"))->isHidden()); // "?" → manual

        // edição: renomeia o 1º, desliga a alternância do 2º e muda o 3º para mensagem
        // sem número (descartado ao salvar)
        L.Edit(QStringLiteral("IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL+0"))->setText(QStringLiteral("Vendas"));
        L.Check(QStringLiteral("IDC_SHORTCUTS_TOGGLE+1"))->click();
        EXPECT_GT(L.Item(QStringLiteral("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER+1"))->width(),
                  L.Item(QStringLiteral("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER2+1"))->width());
        EXPECT_TRUE(L.Item(QStringLiteral("IDC_SHORTCUTS_LABEL2"))->isHidden());
        L.Combo(QStringLiteral("IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+2"))->setCurrentIndex(2); // mensagem
        L.Edit(QStringLiteral("IDC_SHORTCUTS_EDIT_SHORTCUT_LABEL+3"))->setText(QStringLiteral("Conferência"));
        L.Edit(QStringLiteral("IDC_SHORTCUTS_EDIT_SHORTCUT_NUMBER+3"))->setText(QStringLiteral("400"));
        L.Combo(QStringLiteral("IDC_SHORTCUTS_COMBO_SHORTCUT_TYPE+3"))->setCurrentIndex(6);
        L.Button(QStringLiteral("IDOK"))->click();
        QApplication::processEvents();
        EXPECT_EQ(dlg.shortcutsDlg, nullptr);
        EXPECT_FALSE(msip_restart_requested); // mesma posição e mesma coluna: só reconstrói

        ASSERT_EQ(shortcuts.GetCount(), 3);
        EXPECT_EQ(shortcuts[0].label, CString(_T("Vendas")));
        EXPECT_EQ(shortcuts[0].label2, CString(_T("Vendas")));
        EXPECT_TRUE(shortcuts[1].number2.IsEmpty());
        EXPECT_EQ(shortcuts[2].type, CString(MSIP_SHORTCUT_CONF));
        ASSERT_EQ(dialer->shortcutButtons.size(), 3);
        EXPECT_EQ(dialer->shortcutButtons[0]->text(), QStringLiteral("Vendas"));
        EXPECT_FALSE(dialer->shortcutButtons[1]->isCheckable());
        CString saved;
        GetPrivateProfileString(_T("Shortcuts"), _T("2"), NULL, saved.GetBuffer(256), 256, accountSettings.iniFile);
        saved.ReleaseBuffer();
        EXPECT_EQ(saved, CString(_T("Conferência;400;conference;;0;Conferência")));

        // Ctrl+Tab / Ctrl+Shift+Tab percorrem as abas; Ctrl+O silencia a saída
        auto* tabs = dlg.findChild<QTabBar*>();
        ASSERT_NE(tabs, nullptr);
        tabs->setCurrentIndex(0);
        QTest::keyClick(&dlg, Qt::Key_Tab, Qt::ControlModifier);
        EXPECT_EQ(tabs->currentIndex(), 1);
        QTest::keyClick(&dlg, Qt::Key_Backtab, Qt::ControlModifier | Qt::ShiftModifier);
        EXPECT_EQ(tabs->currentIndex(), 0);
        const BOOL muted = dialer->muteOutput;
        QTest::keyClick(&dlg, Qt::Key_O, Qt::ControlModifier);
        ASSERT_TRUE(QTest::qWaitFor([&] { return dialer->muteOutput != muted; }, 2000));
        QTest::keyClick(&dlg, Qt::Key_O, Qt::ControlModifier);
        ASSERT_TRUE(QTest::qWaitFor([&] { return dialer->muteOutput == muted; }, 2000));

        // mudar a posição para embaixo pede o reinício (UM_RESTART → OnRestart)
        dlg.OnMenuShortcuts();
        ASSERT_NE(dlg.shortcutsDlg, nullptr);
        dlg.shortcutsDlg->m_layout.Check(QStringLiteral("IDC_SHORTCUTS_BOTTOM"))->setChecked(true);
        dlg.shortcutsDlg->m_layout.Button(QStringLiteral("IDOK"))->click();
        ASSERT_TRUE(QTest::qWaitFor([] { return msip_restart_requested; }, 3000));
        CString bottom;
        GetPrivateProfileString(_T("Settings"), _T("shortcutsBottom"), NULL, bottom.GetBuffer(16), 16, accountSettings.iniFile);
        bottom.ReleaseBuffer();
        EXPECT_EQ(bottom, CString(_T("1")));
    }
    msip_restart_requested = false;
    shortcuts.RemoveAll();
}

TEST(Shortcuts, EmbaixoEmDuasColunas)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(layout);

    CArray<Shortcut, Shortcut> list;
    for (int i = 0; i < 7; i++) {
        CString label;
        label.Format(_T("A%d"), i);
        CString number;
        number.Format(_T("%d"), 200 + i);
        list.Add(MakeShortcut(label, number, MSIP_SHORTCUT_CALL));
    }
    PrepareProfile("shortcuts-bottom", true, list);

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        auto closeOnExit = qScopeGuard([&] { dlg.OnClose(); });
        dlg.nullSoundDevice = true;
        dlg.show();
        QApplication::processEvents();

        // 7 atalhos (> _GLOBAL_SHORTCUTS_QTY / 2): duas colunas, 4 linhas de 25 px
        EXPECT_EQ(dlg.widthAdd, 0);
        EXPECT_EQ(dlg.heightAdd, 10 + (7 + 1) * 25 / 2);
        Dialer* dialer = dlg.pageDialer;
        ASSERT_EQ(dialer->shortcutButtons.size(), 7);
        dialer->resize(300, 500);
        const auto& b = dialer->shortcutButtons;
        EXPECT_EQ(b[0]->y(), b[1]->y());
        EXPECT_LT(b[0]->x(), b[1]->x());
        EXPECT_EQ(b[2]->y(), b[0]->y() + 25);
        EXPECT_EQ(b[6]->y(), b[0]->y() + 3 * 25);
        EXPECT_EQ(b[0]->height(), 25);
        EXPECT_LE(b[6]->geometry().bottom(), 500);
        EXPECT_GE(b[0]->y(), 500 - dlg.heightAdd);
        // os controles de volume (AutoMove y = 100) ficam acima dos atalhos
        auto* volume = dialer->findChild<QWidget*>(QStringLiteral("IDC_VOLUME_OUTPUT"));
        ASSERT_NE(volume, nullptr);
        EXPECT_LE(volume->geometry().bottom(), b[0]->y());
    }
    shortcuts.RemoveAll();
}
