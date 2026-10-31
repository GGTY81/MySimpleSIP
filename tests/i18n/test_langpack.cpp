// Testes do port de lib/langpack (Translate e carga de langpack_*.txt).

#include "langpack.h"

#include <gtest/gtest.h>

#include "TestPrint.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {

class LangPackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        char tmpl[] = "/tmp/msip-lang-XXXXXX";
        dir = mkdtemp(tmpl);
        UnloadLangPackModule();
    }
    void TearDown() override
    {
        UnloadLangPackModule();
        unsetenv("LC_ALL");
        fs::remove_all(dir);
    }
    CString write(const char* name, const std::string& data)
    {
        std::ofstream(dir / name, std::ios::binary) << data;
        return CString((dir / name).string().c_str());
    }
    fs::path dir;
};

} // namespace

TEST_F(LangPackTest, LangpackPortuguesBrasilReal)
{
    ASSERT_EQ(LoadLangPack(CString(MSIP_RESOURCES_DIR "/langpacks/langpack_portuguesebr.txt")), 0);
    EXPECT_EQ(langPack.language, _T("PortugueseBR"));
    EXPECT_FALSE(langPack.rtl);
    EXPECT_GT(langPack.entryCount, 290);
    EXPECT_STREQ(Translate(_T("Call")), L"Chamar");
    EXPECT_STREQ(Translate(_T("End Call")), L"Encerrar chamada");
    EXPECT_STREQ(Translate(_T("Contacts")), L"Contatos");
    LPCTSTR unknown = _T("Texto que não existe");
    EXPECT_EQ(Translate(unknown), unknown); // mesmo ponteiro, como no original
    EXPECT_EQ(Translate(nullptr), nullptr);
}

TEST_F(LangPackTest, SemLangpackDevolveOriginal)
{
    LPCTSTR s = _T("Call");
    EXPECT_EQ(Translate(s), s);
}

TEST_F(LangPackTest, RegrasDoFormato)
{
    CString f = write("langpack_teste.txt",
        "\xEF\xBB\xBF" "Language Pack\r\n"
        "Language: Teste\r\n"
        "Authors: Fulano\r\n"
        "RTL: 1\r\n"
        "\r\n"
        "; comentário\r\n"
        "[Call]\r\n"
        "Ligar\r\n"
        "[Call]\r\n"              // repetida: vale a primeira
        "Outra\r\n"
        "[Sem tradução]\r\n"
        "\r\n"
        "[Two\\nLines]\r\n"       // escape no texto original
        "Linha 1\r\n"
        "Linha 2\\tcom tab\r\n"   // tradução em várias linhas
        "; comentário no corpo é ignorado\r\n"
        "[Path]\r\n"
        "C:\\\\pasta\r\n");
    ASSERT_EQ(LoadLangPack(f), 0);
    EXPECT_EQ(langPack.language, _T("Teste"));
    EXPECT_EQ(langPack.authors, _T("Fulano"));
    EXPECT_TRUE(langPack.rtl);
    EXPECT_STREQ(Translate(_T("Call")), L"Ligar");
    LPCTSTR semTraducao = _T("Sem tradução");
    EXPECT_EQ(Translate(semTraducao), semTraducao);
    EXPECT_STREQ(Translate(_T("Two\nLines")), L"Linha 1\nLinha 2\tcom tab");
    EXPECT_STREQ(Translate(_T("Path")), L"C:\\pasta");
    EXPECT_EQ(langPack.entryCount, 3);
}

TEST_F(LangPackTest, CodigosDeErroEWindows1252)
{
    EXPECT_EQ(LoadLangPack(CString((dir / "nao-existe.txt").c_str())), 1);
    EXPECT_EQ(LoadLangPack(write("a.txt", "Outro arquivo\n[x]\ny\n")), 2);
    EXPECT_EQ(LoadLangPack(write("b.txt", "Language Pack\ncabecalho sem dois pontos\n")), 3);

    // arquivo antigo em Windows-1252, sem BOM
    ASSERT_EQ(LoadLangPack(write("c.txt", "Language Pack\nLanguage: Fran\xE7" "ais\n[Exit]\nQuitter \x96 fin\n")), 0);
    EXPECT_EQ(langPack.language, _T("Français"));
    EXPECT_STREQ(Translate(_T("Exit")), L"Quitter – fin");
}

TEST(LangPackLocale, MapeamentoDeIdioma)
{
    EXPECT_EQ(LangPackFileForLocale(_T("pt_BR.UTF-8")), _T("langpack_portuguesebr.txt"));
    EXPECT_EQ(LangPackFileForLocale(_T("pt_PT.UTF-8")), _T("langpack_portuguese.txt"));
    EXPECT_EQ(LangPackFileForLocale(_T("es_AR")), _T("langpack_spanish.txt"));
    EXPECT_EQ(LangPackFileForLocale(_T("de_DE@euro")), _T("langpack_german.txt"));
    EXPECT_TRUE(LangPackFileForLocale(_T("en_US.UTF-8")).IsEmpty());
    EXPECT_TRUE(LangPackFileForLocale(_T("C")).IsEmpty());
}

TEST_F(LangPackTest, ModuloEscolheArquivoDaPasta)
{
    const std::string pt = "Language Pack\nLanguage: PortugueseBR\n[Call]\nChamar\n";
    const std::string es = "Language Pack\nLanguage: Spanish\n[Call]\nLlamar\n";
    write("langpack_portuguesebr.txt", pt);
    const CString exeDir(dir.c_str());

    // um único arquivo: usado independentemente do idioma
    setenv("LC_ALL", "en_US.UTF-8", 1);
    LoadLangPackModule(exeDir);
    EXPECT_STREQ(Translate(_T("Call")), L"Chamar");

    // vários: escolhe pelo idioma do sistema
    write("langpack_spanish.txt", es);
    setenv("LC_ALL", "es_ES.UTF-8", 1);
    LoadLangPackModule(exeDir);
    EXPECT_STREQ(Translate(_T("Call")), L"Llamar");

    // vários e nenhum do idioma: inglês
    UnloadLangPackModule();
    setenv("LC_ALL", "en_US.UTF-8", 1);
    LoadLangPackModule(exeDir);
    EXPECT_STREQ(Translate(_T("Call")), L"Call");
}
