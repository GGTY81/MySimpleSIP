// Teste de interface (offscreen) da aba Histórico: carga do call_log.db (com migração do
// [Calls] do INI), ordenação, busca e as funções usadas pelo núcleo.

#include "Calls.h"
#include "PasswordStore.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QHeaderView>
#include <QLineEdit>
#include <QTreeWidget>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

TEST(Calls, CargaOrdenacaoBuscaEAtualizacao)
{
    char tmpl[] = "/tmp/msip-calls-ui-XXXXXX";
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
    WritePrivateProfileString(_T("Calls"), _T("0"), _T("1002;Maria;2;1700000100;0;Missed Call"), accountSettings.iniFile);
    WritePrivateProfileString(_T("Calls"), _T("1"), _T("1003;Bruno;0;1700000000;65;Call Ended"), accountSettings.iniFile);

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        QTreeWidget* list = dlg.pageCalls->List();
        auto* filter = dlg.pageCalls->findChild<QLineEdit*>(QStringLiteral("IDC_FILER_VALUE"));
        ASSERT_TRUE(list && filter);

        // colunas e ordem padrão: hora decrescente (OnCreated)
        EXPECT_EQ(list->columnCount(), 5);
        EXPECT_EQ(list->header()->sectionSize(0), 160);
        ASSERT_EQ(list->topLevelItemCount(), 2);
        EXPECT_EQ(list->topLevelItem(0)->text(0), QStringLiteral("Maria"));
        EXPECT_EQ(list->topLevelItem(1)->text(0), QStringLiteral("Bruno"));
        EXPECT_EQ(list->topLevelItem(1)->text(3), QStringLiteral("1:05"));
        EXPECT_FALSE(list->topLevelItem(0)->icon(0).isNull());

        // ordenação pela duração (valor, não texto)
        list->sortByColumn(3, Qt::DescendingOrder);
        EXPECT_EQ(list->topLevelItem(0)->text(0), QStringLiteral("Bruno"));
        list->sortByColumn(2, Qt::DescendingOrder);

        // busca por nome ou número
        filter->setText(QStringLiteral("100"));
        EXPECT_EQ(list->topLevelItemCount(), 2);
        filter->setText(QStringLiteral("mar"));
        ASSERT_EQ(list->topLevelItemCount(), 1);
        EXPECT_EQ(list->topLevelItem(0)->text(1), QStringLiteral("1002"));
        filter->clear();
        EXPECT_EQ(list->topLevelItemCount(), 2);

        // Add (nova chamada pelo núcleo) aparece no topo; SetDuration/SetInfo atualizam
        char cid[] = "call-xyz@host";
        pj_str_t id = pj_str(cid);
        dlg.pageCalls->Add(id, _T("sip:2000@example.com"), _T("Ana"), _T(""), MSIP_CALL_TYPE_OUTGOING, nullptr);
        ASSERT_EQ(list->topLevelItemCount(), 3);
        EXPECT_EQ(list->topLevelItem(0)->text(0), QStringLiteral("Ana"));
        EXPECT_EQ(list->topLevelItem(0)->text(1), QStringLiteral("2000@example.com"));
        dlg.pageCalls->SetDuration(id, 125, 130);
        dlg.pageCalls->SetInfo(id, _T("Call Ended"));
        EXPECT_EQ(list->topLevelItem(0)->text(3), QStringLiteral("2:05"));
        EXPECT_EQ(list->topLevelItem(0)->text(4), QStringLiteral("Call Ended"));
        // mesmo callId: atualiza em vez de duplicar
        dlg.pageCalls->Add(id, _T("sip:2000@example.com"), _T("Ana Paula"), _T(""), MSIP_CALL_TYPE_OUTGOING, nullptr);
        EXPECT_EQ(list->topLevelItemCount(), 3);
        EXPECT_EQ(list->topLevelItem(0)->text(0), QStringLiteral("Ana Paula"));

        // persistido: nova aba (outro CallsService) lê o mesmo banco
        Calls other;
        other.OnCreated();
        ASSERT_EQ(other.List()->topLevelItemCount(), 3);
        EXPECT_EQ(other.List()->topLevelItem(0)->text(3), QStringLiteral("2:05"));

        dlg.OnClose();
    }
    fs::remove_all(root);
}
