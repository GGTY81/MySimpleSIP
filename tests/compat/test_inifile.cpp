// Testes do IniFile e das funções Get/WritePrivateProfileString*.

#include "IniFile.h"
#include "PrivateProfile.h"
#include "CString.h"

#include <gtest/gtest.h>

#include "TestPrint.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace fs = std::filesystem;

namespace {

class TempDir {
public:
    TempDir()
    {
        char tmpl[] = "/tmp/msip-test-XXXXXX";
        m_path = mkdtemp(tmpl);
    }
    ~TempDir() { fs::remove_all(m_path); }
    std::string file(const char* name) const { return (m_path / name).string(); }
    CString wfile(const char* name) const { return CString(file(name).c_str()); }

private:
    fs::path m_path;
};

std::string readAll(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

void writeAll(const std::string& path, const std::string& data)
{
    std::ofstream(path, std::ios::binary) << data;
}

CString getStr(LPCTSTR section, LPCTSTR key, LPCTSTR def, const CString& file, DWORD size = 256)
{
    CString s;
    DWORD n = GetPrivateProfileString(section, key, def, s.GetBuffer(static_cast<int>(size)), size, file);
    s.ReleaseBuffer();
    EXPECT_EQ(n, static_cast<DWORD>(s.GetLength()));
    return s;
}

} // namespace

TEST(IniFile, ParseRegrasDoWindows)
{
    IniFile ini;
    ini.parse(L"; comentário\n[Settings]\n  Ring = \"Alto falante (USB)\" \nEC=1\nvazio=\n"
              L"aspas='x'\nmeia=\"abc\n[settings]\nEC=2\n");
    EXPECT_EQ(ini.get(L"settings", L"ring").value(), L"Alto falante (USB)");
    EXPECT_EQ(ini.get(L"Settings", L"EC").value(), L"1"); // primeira seção/chave vence
    EXPECT_EQ(ini.get(L"Settings", L"vazio").value(), L"");
    EXPECT_EQ(ini.get(L"Settings", L"aspas").value(), L"x");
    EXPECT_EQ(ini.get(L"Settings", L"meia").value(), L"\"abc"); // aspas não pareadas ficam
    EXPECT_FALSE(ini.get(L"Settings", L"nada").has_value());
    EXPECT_FALSE(ini.get(L"Outra", L"EC").has_value());
}

TEST(IniFile, GravacaoPreservaOrdemEComentarios)
{
    IniFile ini;
    ini.parse(L"; topo\n[A]\nx=1\n\n[B]\ny=2\n");
    ini.set(L"A", L"novo", L"3");       // entra no fim de [A], antes da linha em branco
    ini.set(L"b", L"Y", L"20");         // seção/chave sem diferenciar caixa; grava a chave como recebida
    ini.set(L"C", L"z", L"\"com espaço\"");
    EXPECT_EQ(ini.serialize(), L"; topo\n[A]\nx=1\nnovo=3\n\n[B]\nY=20\n[C]\nz=\"com espaço\"\n");
    EXPECT_EQ(ini.get(L"C", L"z").value(), L"com espaço");

    EXPECT_TRUE(ini.removeKey(L"A", L"x"));
    EXPECT_FALSE(ini.removeKey(L"A", L"x"));
    EXPECT_TRUE(ini.removeSection(L"b"));
    EXPECT_EQ(ini.serialize(), L"; topo\n[A]\nnovo=3\n\n[C]\nz=\"com espaço\"\n");
    EXPECT_EQ(ini.sectionNames(), (std::vector<std::wstring>{L"A", L"C"}));
    EXPECT_EQ(ini.keyNames(L"a"), (std::vector<std::wstring>{L"novo"}));
}

TEST(IniFile, DecodificaUtf16Utf8EBom)
{
    std::string utf16le("\xFF\xFE[\0A\0]\0\r\0\n\0k\0=\0\xE7\0\x3D\xD8\x00\xDE", 22);
    EXPECT_EQ(IniFile::decode(utf16le), L"[A]\r\nk=ç😀");
    std::string utf16be("\xFE\xFF\0[\0A\0]", 8);
    EXPECT_EQ(IniFile::decode(utf16be), L"[A]");
    EXPECT_EQ(IniFile::decode("\xEF\xBB\xBF[ç]"), L"[ç]");
    EXPECT_EQ(IniFile::decode("[ç]"), L"[ç]");
}

TEST(PrivateProfile, GetComDefaultETruncamento)
{
    TempDir dir;
    writeAll(dir.file("a.ini"), "[Settings]\ntransport=udp\ncodecs=PCMA/8000/1 PCMU/8000/1\n");
    CString f = dir.wfile("a.ini");
    PrivateProfileFlushCache();

    EXPECT_EQ(getStr(L"Settings", L"transport", L"tcp", f), L"udp");
    EXPECT_EQ(getStr(L"Settings", L"ausente", L"padrão  ", f), L"padrão"); // espaços finais saem
    EXPECT_EQ(getStr(L"Settings", L"ausente", nullptr, f), L"");
    EXPECT_EQ(getStr(L"Nada", L"transport", L"x", f), L"x");

    wchar_t buf[5];
    EXPECT_EQ(GetPrivateProfileString(L"Settings", L"codecs", nullptr, buf, 5, f), 4u);
    EXPECT_STREQ(buf, L"PCMA");

    EXPECT_EQ(GetPrivateProfileInt(L"Settings", L"transport", 7, f), 0u);
    EXPECT_EQ(GetPrivateProfileInt(L"Settings", L"ausente", 7, f), 7u);
}

TEST(PrivateProfile, ListasDeChavesESecoes)
{
    TempDir dir;
    writeAll(dir.file("a.ini"), "[Account1]\nserver=s\nport=5060\n[Vazia]\n");
    CString f = dir.wfile("a.ini");
    PrivateProfileFlushCache();

    wchar_t buf[64];
    DWORD n = GetPrivateProfileString(L"Account1", nullptr, nullptr, buf, 64, f);
    EXPECT_EQ(n, 12u); // "server\0port\0"
    EXPECT_EQ(std::wstring(buf, 13), std::wstring(L"server\0port\0\0", 13));

    n = GetPrivateProfileString(nullptr, nullptr, nullptr, buf, 64, f);
    EXPECT_EQ(std::wstring(buf, n), std::wstring(L"Account1\0Vazia\0", 15));

    // Uso do MicroSIP (MSIP::IniSectionExists/ExportDlg): buffer de 3 só para saber se há chaves
    wchar_t small[3];
    EXPECT_EQ(GetPrivateProfileString(L"Account1", nullptr, nullptr, small, 3, f), 1u);
    EXPECT_EQ(GetPrivateProfileString(L"Vazia", nullptr, nullptr, small, 3, f), 0u);
    EXPECT_EQ(GetPrivateProfileString(L"Account9", nullptr, nullptr, small, 3, f), 0u);
}

TEST(PrivateProfile, GravarApagarESecao)
{
    TempDir dir;
    CString f = dir.wfile("novo.ini");
    PrivateProfileFlushCache();

    EXPECT_TRUE(WritePrivateProfileString(L"Account1", L"server", L"sip.example.com", f));
    EXPECT_TRUE(WritePrivateProfileString(L"Account1", L"port", L"5060", f));
    EXPECT_TRUE(WritePrivateProfileString(L"Calls", L"0", L"x", f));
    EXPECT_EQ(readAll(dir.file("novo.ini")), "[Account1]\nserver=sip.example.com\nport=5060\n[Calls]\n0=x\n");

    EXPECT_TRUE(WritePrivateProfileString(L"Account1", L"port", nullptr, f)); // apaga chave
    EXPECT_TRUE(WritePrivateProfileSection(L"Calls", nullptr, f));             // apaga seção
    EXPECT_EQ(readAll(dir.file("novo.ini")), "[Account1]\nserver=sip.example.com\n");

    EXPECT_TRUE(WritePrivateProfileSection(L"Shortcuts", L"0=a;b\0" L"1=c\0", f));
    EXPECT_EQ(getStr(L"Shortcuts", L"1", nullptr, f), L"c");

    EXPECT_TRUE(WritePrivateProfileString(L"Account1", nullptr, nullptr, f)); // apaga seção
    EXPECT_EQ(readAll(dir.file("novo.ini")), "[Shortcuts]\n0=a;b\n1=c\n");
}

TEST(PrivateProfile, RecarregaSeArquivoMudarPorFora)
{
    TempDir dir;
    writeAll(dir.file("a.ini"), "[S]\nk=1\n");
    CString f = dir.wfile("a.ini");
    PrivateProfileFlushCache();
    EXPECT_EQ(getStr(L"S", L"k", nullptr, f), L"1");
    writeAll(dir.file("a.ini"), "[S]\nk=22\n"); // tamanho diferente garante a detecção
    EXPECT_EQ(getStr(L"S", L"k", nullptr, f), L"22");
}

TEST(PrivateProfile, ImportaMicrosipIniDoWindows)
{
    // Cópia anonimizada de um MicroSIP.ini real (UTF-16 LE com BOM, CRLF)
    TempDir dir;
    fs::copy_file(MSIP_TEST_DATA_DIR "/microsip_win.ini", dir.file("m.ini"));
    CString f = dir.wfile("m.ini");
    PrivateProfileFlushCache();

    EXPECT_EQ(getStr(L"Global", L"version", nullptr, f), L"3.22.12");
    EXPECT_EQ(getStr(L"Settings", L"audioCodecs", nullptr, f), L"PCMA/8000/1 PCMU/8000/1");
    EXPECT_EQ(getStr(L"Settings", L"audioOutputDevice", L"x", f), L""); // ""
    EXPECT_EQ(getStr(L"Settings", L"featureCodeCP", nullptr, f), L"**");
    EXPECT_EQ(getStr(L"Account1", L"transport", nullptr, f), L"udp");
    EXPECT_EQ(getStr(L"Account1", L"displayName", nullptr, f), L"Usuário Teste");

    // gravar converte para UTF-8 e mantém todas as chaves
    IniFile antes;
    ASSERT_TRUE(antes.load(dir.file("m.ini")));
    EXPECT_TRUE(WritePrivateProfileString(L"Settings", L"volumeRing", L"80", f));
    const std::string bytes = readAll(dir.file("m.ini"));
    EXPECT_NE(bytes.substr(0, 2), "\xFF\xFE");
    EXPECT_EQ(bytes.find('\r'), std::string::npos);
    IniFile depois;
    ASSERT_TRUE(depois.load(dir.file("m.ini")));
    for (const auto& sec : antes.sectionNames())
        for (const auto& key : antes.keyNames(sec)) {
            if (sec == L"Settings" && key == L"volumeRing")
                continue;
            EXPECT_EQ(antes.get(sec, key), depois.get(sec, key)) << CStringA(CString(key.c_str())).GetString();
        }
    EXPECT_EQ(depois.get(L"Settings", L"volumeRing").value(), L"80");
}
