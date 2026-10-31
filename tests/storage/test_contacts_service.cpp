// Contatos: CMarkup próprio (Contacts.xml) e ContactsService (preparo, CSV, XML).

#include "CSVFile.h"
#include "ContactsRepository.h"
#include "ContactsService.h"
#include "Database.h"
#include "Markup.h"
#include "StringConverter.h"

#include <gtest/gtest.h>

#include "TestPrint.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

namespace fs = std::filesystem;

TEST(Markup, NavegacaoEGeracao)
{
    CMarkup xml;
    ASSERT_TRUE(xml.SetDoc(_T("<?xml version=\"1.0\"?>\r\n<contacts refresh=\"60\">\r\n"
                              "<contact name=\"José &amp; Cia\" number=\"1001\"/>\r\n"
                              "<group><x/></group>\r\n"
                              "<contact name=\"Ana\" number=\"1002\" presence=\"1\"><note>oi</note></contact>\r\n"
                              "</contacts>\r\n")));
    ASSERT_TRUE(xml.FindElem(_T("contacts")));
    EXPECT_TRUE(xml.FindAttrib(_T("refresh")));
    EXPECT_FALSE(xml.FindAttrib(_T("silent")));
    EXPECT_EQ(xml.GetAttrib(_T("refresh")), _T("60"));
    std::vector<CString> names;
    while (xml.FindChildElem(_T("contact"))) {
        xml.IntoElem();
        names.push_back(xml.GetAttrib(_T("name")));
        if (xml.GetAttrib(_T("number")) == _T("1002")) {
            EXPECT_TRUE(xml.FindChildElem(_T("note")));
            EXPECT_EQ(xml.GetChildData(), _T("oi"));
        }
        xml.OutOfElem();
    }
    ASSERT_EQ(names.size(), 2u);
    EXPECT_EQ(names[0], _T("José & Cia"));
    EXPECT_EQ(names[1], _T("Ana"));
    xml.ResetChildPos();
    EXPECT_TRUE(xml.FindChildElem()); // qualquer filho: o primeiro
    EXPECT_EQ(xml.GetChildTagName(), _T("contact"));

    EXPECT_FALSE(xml.SetDoc(_T("<a><b></a>"))); // XML inválido

    CMarkup out;
    out.AddElem(_T("contacts"));
    out.IntoElem();
    out.AddElem(_T("contact"));
    out.AddAttrib(_T("name"), _T("A \"B\" <C> & D"));
    out.AddAttrib(_T("number"), _T("1"));
    out.AddElem(_T("contact"));
    out.AddAttrib(_T("name"), _T("E"));
    EXPECT_EQ(out.GetDoc(), CString(_T("<contacts>\r\n<contact name=\"A &quot;B&quot; &lt;C&gt; &amp; D\" number=\"1\"/>\r\n"
                                       "<contact name=\"E\"/>\r\n</contacts>\r\n")));
}

class ContactsServiceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        char tmpl[] = "/tmp/msip-contacts-XXXXXX";
        root = mkdtemp(tmpl);
        db = std::make_unique<Database>((root / "contacts.db").string());
        db->InitContacts();
        service = std::make_unique<ContactsService>(std::make_unique<ContactsRepository>(db->Get()));
    }
    void TearDown() override { fs::remove_all(root); }
    fs::path root;
    std::unique_ptr<Database> db;
    std::unique_ptr<ContactsService> service;
};

TEST_F(ContactsServiceTest, PreparaEFiltra)
{
    Contact c;
    EXPECT_FALSE(service->ContactPrepare(c)); // sem número
    c.mobile = _T("5511999990000");
    c.firstname = _T("Maria");
    c.lastname = _T("Souza");
    ASSERT_TRUE(service->ContactPrepare(c));
    EXPECT_EQ(c.number, _T("5511999990000")); // número = telefone ou celular
    EXPECT_EQ(c.name, _T("Maria Souza"));     // nome = nome + sobrenome
    Contact d;
    d.number = _T("1003");
    ASSERT_TRUE(service->ContactPrepare(d));
    EXPECT_EQ(d.name, _T("1003"));
    EXPECT_FALSE(service->IsContactFiletered(c, _T("souza")));
    EXPECT_TRUE(service->IsContactFiletered(c, _T("bruno")));
}

TEST_F(ContactsServiceTest, ImportaCsvDoGoogleEExporta)
{
    // cabeçalhos alternativos (Google Contacts) reconhecidos pelo original
    const fs::path csv = root / "google.csv";
    {
        std::ofstream f(csv, std::ios::binary);
        f << "Given Name,Family Name,Phone 1 - Value,E-mail 1 - Value,Notes\r\n"
             "Ana,Lima,1002,ana@example.com,\"Cliente, VIP\"\r\n"
             "Sem,Numero,,x@example.com,\r\n";
    }
    CArray<ContactWithFields*> list;
    EXPECT_TRUE(service->ImportContactsCSV(CString(csv.c_str()), list).IsEmpty());
    ASSERT_EQ(list.GetCount(), 1); // a linha sem número é descartada
    const Contact& c = list[0]->contact;
    EXPECT_EQ(c.name, _T("Ana Lima"));
    EXPECT_EQ(c.number, _T("1002"));
    EXPECT_EQ(c.mobile, _T("1002"));
    EXPECT_EQ(c.email, _T("ana@example.com"));
    EXPECT_EQ(c.comment, _T("Cliente, VIP"));
    EXPECT_TRUE(list[0]->fields.Find(_T("mobile")) != NULL);

    // CSV sem colunas de número: dados não reconhecidos
    const fs::path bad = root / "bad.csv";
    {
        std::ofstream f(bad, std::ios::binary);
        f << "Foo,Bar\r\n1,2\r\n";
    }
    CArray<ContactWithFields*> none;
    EXPECT_EQ(service->ImportContactsCSV(CString(bad.c_str()), none), _T("The received data cannot be recognized"));

    // exportação CSV e XML (com filtro)
    Contact x = c;
    x.starred = true;
    Contact y;
    y.name = _T("Bruno");
    y.number = _T("1003");
    std::vector<Contact*> items{&x, &y};
    const CString outCsv = CString((root / "out.csv").c_str());
    service->ExportContactsCSV(outCsv, _T("lima"), items);
    CCSVFile f;
    ASSERT_TRUE(f.Open(outCsv, CFile::modeRead));
    CStringArray arr;
    ASSERT_TRUE(f.ReadData(arr));
    EXPECT_EQ(arr.GetCount(), 17);
    ASSERT_TRUE(f.ReadData(arr));
    EXPECT_EQ(arr[0], _T("Ana Lima"));
    EXPECT_EQ(arr[11], _T("Cliente, VIP"));
    EXPECT_EQ(arr[16], _T("1")); // Starred
    EXPECT_FALSE(f.ReadData(arr));
    f.Close();

    const fs::path outXml = root / "out.xml";
    service->ExportContactsXML(CString(outXml.c_str()), _T(""), items);
    std::ifstream in(outXml, std::ios::binary);
    std::string xml((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(xml.rfind("<?xml version=\"1.0\"?>\r\n<contacts>\r\n<contact name=\"Ana Lima\" number=\"1002\"", 0), 0u);
    EXPECT_NE(xml.find("<contact name=\"Bruno\" number=\"1003\""), std::string::npos);

    for (int i = 0; i < list.GetCount(); i++) {
        delete list[i];
    }
}
