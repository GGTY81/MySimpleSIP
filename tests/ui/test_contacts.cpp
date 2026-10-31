// Teste de interface (offscreen) da aba Contatos e do AddDlg: carga do Contacts.xml,
// favoritos no topo, busca, adicionar/editar pelo diálogo, gravação, nome por número
// (GetNameForCall) e "Adicionar contato" a partir do Histórico.

#include "Calls.h"
#include "Contacts.h"
#include "PasswordStore.h"
#include "mainDlg.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <QApplication>
#include <QCheckBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
#include <QTreeWidget>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

static std::string ReadFile(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

TEST(Contacts, CargaFavoritosBuscaEAddDlg)
{
    char tmpl[] = "/tmp/msip-contacts-ui-XXXXXX";
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
    const fs::path xmlFile = fs::path(CStringA(accountSettings.pathRoaming).GetString()) / "Contacts.xml";
    fs::create_directories(xmlFile.parent_path());
    {
        std::ofstream f(xmlFile, std::ios::binary);
        f << "<?xml version=\"1.0\"?>\r\n<contacts>\r\n"
             "<contact name=\"Bruno\" number=\"1003\" mobile=\"5511988887777\"/>\r\n"
             "<contact name=\"Zeca\" number=\"1009\" starred=\"1\"/>\r\n"
             "<contact name=\"Ana\" number=\"1002\" info=\"ignorado sem presença\"/>\r\n"
             "</contacts>\r\n";
    }

    {
        QtMessageBackend backend;
        CmainDlg dlg(&backend, true);
        Contacts* page = dlg.pageContacts;
        QTreeWidget* list = page->List();
        ASSERT_EQ(list->columnCount(), 3);
        ASSERT_EQ(list->topLevelItemCount(), 3);
        // favorito no topo, depois ordem alfabética
        EXPECT_EQ(list->topLevelItem(0)->text(0), QStringLiteral("Zeca"));
        EXPECT_EQ(list->topLevelItem(1)->text(0), QStringLiteral("Ana"));
        EXPECT_EQ(list->topLevelItem(2)->text(0), QStringLiteral("Bruno"));
        EXPECT_TRUE(list->topLevelItem(1)->text(2).isEmpty()); // info só com presença/diretório
        // clique no cabeçalho: ordem decrescente, favorito continua no topo
        QHeaderView* header = list->header();
        const QPoint col0(header->sectionViewportPosition(0) + 10, header->height() / 2);
        QTest::mouseClick(header->viewport(), Qt::LeftButton, {}, col0);
        EXPECT_EQ(header->sortIndicatorOrder(), Qt::DescendingOrder);
        EXPECT_EQ(list->topLevelItem(0)->text(0), QStringLiteral("Zeca"));
        EXPECT_EQ(list->topLevelItem(1)->text(0), QStringLiteral("Bruno"));
        QTest::mouseClick(header->viewport(), Qt::LeftButton, {}, col0);
        EXPECT_EQ(list->topLevelItem(1)->text(0), QStringLiteral("Ana"));

        // busca
        auto* filter = page->findChild<QLineEdit*>(QStringLiteral("IDC_FILER_VALUE"));
        filter->setText(QStringLiteral("bru"));
        ASSERT_EQ(list->topLevelItemCount(), 1);
        filter->clear();
        EXPECT_EQ(list->topLevelItemCount(), 3);

        // nome por número: exato, pelo celular e por sufixo (GetNameAltByNumber)
        EXPECT_EQ(page->GetNameByNumber(_T("1003")), _T("Bruno"));
        EXPECT_EQ(page->GetNameByNumber(_T("5511988887777")), _T("Bruno"));
        // (por sufixo — GetNameAltByNumber — o original só acerta se o contato for o último
        // do laço: nameAlt é sobrescrito a cada contato; mantido por fidelidade)
        EXPECT_TRUE(page->GetNameByNumber(_T("4000")).IsEmpty());
        SIPURI sipuri;
        MSIP::ParseSIPURI(_T("sip:1009@example.com"), sipuri);
        EXPECT_EQ(dlg.GetNameForCall(sipuri, nullptr), _T("Zeca"));

        // novo contato pelo AddDlg: nome/sobrenome compõem o nome; telefone copia o número
        page->OnMenuAdd();
        AddDlg* add = page->addDlg;
        auto edit = [add](const char* id) { return add->m_layout.Edit(QString::fromLatin1(id)); };
        edit("IDC_ADD_FIRSTNAME")->setText(QStringLiteral("Maria"));
        edit("IDC_ADD_LASTNAME")->setText(QStringLiteral("Souza"));
        EXPECT_EQ(edit("IDC_ADD_NAME")->text(), QStringLiteral("Maria Souza"));
        edit("IDC_ADD_PHONE")->setText(QStringLiteral("1004"));
        EXPECT_EQ(edit("IDC_ADD_NUMBER")->text(), QStringLiteral("1004"));
        add->m_layout.Check(QStringLiteral("IDC_ADD_STARRED"))->setChecked(true);
        add->m_layout.Button(QStringLiteral("IDOK"))->click();
        EXPECT_FALSE(add->isVisible());
        ASSERT_EQ(list->topLevelItemCount(), 4);
        EXPECT_EQ(list->topLevelItem(0)->text(0), QStringLiteral("Maria Souza")); // favorito, "M" < "Z"
        std::string xml = ReadFile(xmlFile);
        EXPECT_NE(xml.find("name=\"Maria Souza\" number=\"1004\" firstname=\"Maria\" lastname=\"Souza\" phone=\"1004\""), std::string::npos);

        // editar: número antigo identifica o contato (ContactAdd com oldNumber)
        list->clearSelection();
        list->topLevelItem(2)->setSelected(true); // Ana
        page->OnMenuEdit();
        EXPECT_EQ(edit("IDC_ADD_NAME")->text(), QStringLiteral("Ana"));
        edit("IDC_ADD_NUMBER")->setText(QStringLiteral("1012"));
        add->m_layout.Button(QStringLiteral("IDOK"))->click();
        ASSERT_NE(page->FindContact(_T("1012")), nullptr);
        EXPECT_EQ(page->FindContact(_T("1002")), nullptr);
        EXPECT_EQ(list->topLevelItemCount(), 4);
        EXPECT_NE(ReadFile(xmlFile).find("name=\"Ana\" number=\"1012\""), std::string::npos);

        // "Adicionar contato" do Histórico abre o AddDlg com número e nome da chamada
        char cid[] = "c-1@h";
        dlg.pageCalls->Add(pj_str(cid), _T("sip:2000@example.com"), _T("Carlos"), _T(""), MSIP_CALL_TYPE_INCOMING, nullptr);
        dlg.pageCalls->List()->topLevelItem(0)->setSelected(true);
        dlg.pageCalls->OnMenuAdd();
        EXPECT_TRUE(add->isVisible());
        EXPECT_EQ(edit("IDC_ADD_NAME")->text(), QStringLiteral("Carlos"));
        EXPECT_EQ(edit("IDC_ADD_NUMBER")->text(), QStringLiteral("2000@example.com"));
        add->OnBnClickedCancel();
        EXPECT_FALSE(add->isVisible());
        EXPECT_EQ(list->topLevelItemCount(), 4);

        dlg.OnClose();
    }

    // migração do formato antigo ([Contacts] no INI) quando não há Contacts.xml
    fs::remove(xmlFile);
    PrivateProfileFlushCache();
    WritePrivateProfileString(_T("Contacts"), _T("0"), _T("1005;Pedro;1"), accountSettings.iniFile);
    {
        Contacts page;
        page.OnCreated();
        ASSERT_EQ(page.List()->topLevelItemCount(), 1);
        EXPECT_EQ(page.FindContact(_T("1005"))->name, _T("Pedro"));
        EXPECT_TRUE(page.FindContact(_T("1005"))->presence);
        EXPECT_TRUE(fs::exists(xmlFile));
    }
    fs::remove_all(root);
}
