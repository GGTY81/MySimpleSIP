// Testes do port de settings.cpp (AccountSettings, contas, senhas e atalhos).

#include "settings.h"
#include "PasswordStore.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>

namespace fs = std::filesystem;

namespace {

class MemoryPasswordStore : public IPasswordStore {
public:
    bool Load(const CString& account, CString& password) override
    {
        auto it = items.find(account);
        if (it == items.end())
            return false;
        password = it->second;
        return true;
    }
    bool Save(const CString& account, const CString& password) override
    {
        items[account] = password;
        return true;
    }
    void Remove(const CString& account) override { items.erase(account); }
    std::map<CString, CString> items;
};

class SettingsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        char tmpl[] = "/tmp/msip-settings-XXXXXX";
        root = mkdtemp(tmpl);
        setenv("XDG_CONFIG_HOME", (root / "config").c_str(), 1);
        setenv("XDG_DATA_HOME", (root / "data").c_str(), 1);
        setenv("XDG_STATE_HOME", (root / "state").c_str(), 1);
        PrivateProfileFlushCache();
        SetPasswordStore(nullptr);
        ResetAccountSettings();
        shortcuts.RemoveAll();
        msip_command_line.Empty();
    }
    void TearDown() override
    {
        SetPasswordStore(nullptr);
        unsetenv("XDG_CONFIG_HOME");
        unsetenv("XDG_DATA_HOME");
        unsetenv("XDG_STATE_HOME");
        fs::remove_all(root);
    }
    fs::path iniPath() const { return root / "config/MySimpleSIP/MySimpleSIP.ini"; }
    void installIni(const fs::path& from)
    {
        fs::create_directories(iniPath().parent_path());
        fs::copy_file(from, iniPath());
    }
    void writeIni(const std::string& text)
    {
        fs::create_directories(iniPath().parent_path());
        std::ofstream(iniPath(), std::ios::binary) << text;
    }
    std::string readIni() const
    {
        std::ifstream in(iniPath(), std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }
    CString ini(LPCTSTR section, LPCTSTR key)
    {
        CString s;
        GetPrivateProfileString(section, key, _T("<ausente>"), s.GetBuffer(1024), 1024, accountSettings.iniFile);
        s.ReleaseBuffer();
        return s;
    }

    fs::path root;
};

} // namespace

TEST_F(SettingsTest, PrimeiraExecucaoUsaPadroesDoMicroSIP)
{
    accountSettings.Init();
    EXPECT_TRUE(firstRun);
    EXPECT_FALSE(accountSettings.isPortable);
    EXPECT_EQ(accountSettings.iniFile, CString(iniPath().c_str()));
    EXPECT_EQ(accountSettings.accountId, 0);
    EXPECT_EQ(accountSettings.audioCodecs, _T("PCMA/8000/1 PCMU/8000/1"));
    EXPECT_EQ(accountSettings.singleMode, true);
    EXPECT_TRUE(accountSettings.ec);
    EXPECT_FALSE(accountSettings.vad);
    EXPECT_TRUE(accountSettings.localDTMF);
    EXPECT_TRUE(accountSettings.bringToFrontOnIncoming);
    EXPECT_TRUE(accountSettings.callWaiting);
    EXPECT_EQ(accountSettings.autoAnswer, _T("button"));
    EXPECT_EQ(accountSettings.denyIncoming, _T("button"));
    EXPECT_EQ(accountSettings.volumeRing, 100);
    EXPECT_TRUE(accountSettings.recordingPath.Right(11) == _T("/Recordings"));
    // [Global] version = versão do aplicativo
    EXPECT_EQ(ini(_T("Global"), _T("version")), _T(_GLOBAL_VERSION));
    // conta local (Account0) com rótulo padrão
    EXPECT_EQ(accountSettings.accountLocal.label, _T("Local (call by IP address)"));
}

TEST_F(SettingsTest, SalvarERecarregarPreservaValores)
{
    accountSettings.Init();
    accountSettings.audioCodecs = _T("opus/48000/2 G722/16000/1 PCMA/8000/1");
    accountSettings.autoAnswerDelay = 5;
    accountSettings.forwardingNumber = _T("+5511999990000");
    accountSettings.alwaysOnTop = true;
    accountSettings.mainX = 1635;
    accountSettings.cmdCallEnd = _T("notify-send \"fim da chamada\"");
    accountSettings.SettingsSave();

    ResetAccountSettings();
    PrivateProfileFlushCache();
    accountSettings.Init();
    EXPECT_FALSE(firstRun);
    EXPECT_EQ(accountSettings.audioCodecs, _T("opus/48000/2 G722/16000/1 PCMA/8000/1"));
    EXPECT_EQ(accountSettings.autoAnswerDelay, 5);
    EXPECT_EQ(accountSettings.forwardingNumber, _T("+5511999990000"));
    EXPECT_TRUE(accountSettings.alwaysOnTop);
    EXPECT_EQ(accountSettings.mainX, 1635);
    EXPECT_EQ(accountSettings.cmdCallEnd, _T("notify-send \"fim da chamada\""));
}

TEST_F(SettingsTest, ImportaMicrosipIniDoWindows)
{
    installIni(MSIP_TEST_DATA_DIR "/microsip_win.ini");
    accountSettings.Init();
    EXPECT_FALSE(firstRun);
    EXPECT_EQ(accountSettings.accountId, 1);
    const Account& a = accountSettings.account;
    EXPECT_EQ(a.label, _T("Conta Teste"));
    EXPECT_EQ(a.server, _T("sip.example.com"));
    EXPECT_EQ(a.domain, _T("sip.example.com"));
    EXPECT_EQ(a.username, _T("1001"));
    EXPECT_EQ(a.displayName, _T("Usuário Teste"));
    EXPECT_EQ(a.transport, _T("udp"));
    EXPECT_EQ(a.registerRefresh, 300);
    EXPECT_EQ(a.keepAlive, 15);
    EXPECT_TRUE(a.publish);
    EXPECT_TRUE(a.allowRewrite);
    EXPECT_FALSE(a.ice);
    EXPECT_EQ(accountSettings.recordingFormat, _T("mp3"));
    EXPECT_EQ(accountSettings.featureCodeCP, _T("**"));
    EXPECT_EQ(accountSettings.featureCodeBT, _T("##"));
    EXPECT_EQ(accountSettings.usersDirectory, _T("https://pbx.example.com/phonebook.xml"));
    EXPECT_EQ(accountSettings.videoBitrate, 256);
    // versão gravada atualizada para a do MySimpleSIP; INI agora em UTF-8
    EXPECT_EQ(ini(_T("Global"), _T("version")), _T(_GLOBAL_VERSION));
    EXPECT_NE(readIni().substr(0, 2), "\xFF\xFE");
}

TEST_F(SettingsTest, LimitesDaContaComoNoOriginal)
{
    writeIni("[Global]\n[Settings]\naccountId=1\n"
             "[Account1]\nserver=s\ndomain=d\nregisterRefresh=5\nkeepAlive=1\nhideCID=1\n");
    accountSettings.Init();
    EXPECT_EQ(accountSettings.account.registerRefresh, 10);
    EXPECT_EQ(accountSettings.account.keepAlive, 2);
    EXPECT_EQ(accountSettings.account.hideCID, _T("hard"));
    EXPECT_EQ(accountSettings.account.transport, _T("udp"));

    Account b;
    writeIni("[Account2]\ndomain=d\nregisterRefresh=0\nkeepAlive=-3\nhideCID=0\n");
    PrivateProfileFlushCache();
    ASSERT_TRUE(accountSettings.AccountLoad(2, &b));
    EXPECT_EQ(b.registerRefresh, 300);
    EXPECT_EQ(b.keepAlive, 15);
    EXPECT_TRUE(b.hideCID.IsEmpty());
    // conta sem domínio não é válida
    writeIni("[Account3]\nserver=s\n");
    PrivateProfileFlushCache();
    EXPECT_FALSE(accountSettings.AccountLoad(3, &b));
    EXPECT_FALSE(accountSettings.AccountLoad(9, &b));
}

TEST_F(SettingsTest, SenhaVaiParaOChaveiro)
{
    MemoryPasswordStore store;
    SetPasswordStore(&store);
    accountSettings.Init();
    Account a;
    a.label = _T("Teste");
    a.domain = _T("sip.example.com");
    a.username = _T("1001");
    a.password = _T("s3nh@ çã");
    a.rememberPassword = true;
    a.transport = _T("tls");
    accountSettings.AccountSave(1, &a);
    EXPECT_EQ(ini(_T("Account1"), _T("password")), _T(""));
    EXPECT_EQ(store.items[_T("Account1")], _T("s3nh@ çã"));

    Account b;
    ASSERT_TRUE(accountSettings.AccountLoad(1, &b));
    EXPECT_EQ(b.password, _T("s3nh@ çã"));
    EXPECT_EQ(b.transport, _T("tls"));
    EXPECT_TRUE(b.rememberPassword);

    accountSettings.AccountDelete(1);
    EXPECT_EQ(store.items.count(_T("Account1")), 0u);
    EXPECT_EQ(ini(_T("Account1"), _T("domain")), _T("<ausente>"));
}

TEST_F(SettingsTest, SemChaveiroSenhaFicaOfuscadaNoIni)
{
    accountSettings.Init();
    Account a;
    a.domain = _T("d");
    a.username = _T("u");
    a.password = _T("segredo");
    a.rememberPassword = true;
    accountSettings.AccountSave(1, &a);
    CString stored = ini(_T("Account1"), _T("password"));
    EXPECT_EQ(stored.Left(6), _T("msip1:"));
    EXPECT_EQ(stored.Find(_T("segredo")), -1);
    Account b;
    ASSERT_TRUE(accountSettings.AccountLoad(1, &b));
    EXPECT_EQ(b.password, _T("segredo"));
}

TEST_F(SettingsTest, SenhaEmTextoPuroEhMigrada)
{
    // como no MicroSIP: senha digitada em texto puro no INI é regravada protegida
    writeIni("[Account1]\ndomain=d\nusername=u\npassword=plain123\n");
    MemoryPasswordStore store;
    SetPasswordStore(&store);
    accountSettings.Init();
    Account a;
    ASSERT_TRUE(accountSettings.AccountLoad(1, &a));
    EXPECT_EQ(a.password, _T("plain123"));
    EXPECT_EQ(store.items[_T("Account1")], _T("plain123"));
    EXPECT_EQ(ini(_T("Account1"), _T("password")), _T(""));
}

TEST_F(SettingsTest, OutroArquivoNaoUsaOChaveiro)
{
    // exportar/importar configuração: a senha vai ofuscada no arquivo e o chaveiro do
    // perfil não é tocado (ele identifica a senha só pela seção)
    MemoryPasswordStore store;
    SetPasswordStore(&store);
    accountSettings.Init();
    store.items[_T("Account1")] = _T("senha-da-conta-1");
    Account a;
    a.domain = _T("d");
    a.username = _T("u");
    a.password = _T("senha-da-conta-3");
    a.rememberPassword = true;
    const CString other = CString((root / "export.ini").wstring().c_str());
    accountSettings.AccountSave(1, &a, other);
    EXPECT_EQ(store.items[_T("Account1")], _T("senha-da-conta-1"));
    CString stored;
    GetPrivateProfileString(_T("Account1"), _T("password"), NULL, stored.GetBuffer(256), 256, other);
    stored.ReleaseBuffer();
    EXPECT_EQ(stored, PasswordObfuscate(_T("senha-da-conta-3")));

    Account b;
    ASSERT_TRUE(accountSettings.AccountLoad(1, &b, other));
    EXPECT_EQ(b.password, _T("senha-da-conta-3"));
    EXPECT_EQ(store.items[_T("Account1")], _T("senha-da-conta-1"));
}

TEST(PasswordObfuscation, Roundtrip)
{
    for (const CString& p : {CString(), CString(_T("a")), CString(_T("ab")), CString(_T("abc")),
                             CString(_T("Ação 😀 ;=\"'"))}) {
        CString enc = PasswordObfuscate(p), dec;
        ASSERT_TRUE(PasswordDeobfuscate(enc, dec));
        EXPECT_EQ(dec, p);
    }
    CString out;
    EXPECT_EQ(PasswordObfuscate(_T("abc")), _T("msip1:YWJj"));
    EXPECT_FALSE(PasswordDeobfuscate(_T("abc"), out));
    EXPECT_FALSE(PasswordDeobfuscate(_T("msip1:YWJ"), out));
    EXPECT_FALSE(PasswordDeobfuscate(_T("msip1:Y=Jj"), out));
}

TEST_F(SettingsTest, AtalhosCodificacaoECarga)
{
    Shortcut s;
    ShortcutDecode(_T("Suporte;2000;call;;1;Plantão"), &s);
    EXPECT_EQ(s.label, _T("Suporte"));
    EXPECT_EQ(s.number, _T("2000"));
    EXPECT_EQ(s.type, MSIP_SHORTCUT_CALL);
    EXPECT_TRUE(s.number2.IsEmpty());
    EXPECT_TRUE(s.presence);
    EXPECT_EQ(s.label2, _T("Plantão"));
    EXPECT_EQ(ShortcutEncode(&s), _T("Suporte;2000;call;;1;Plantão"));

    // formato antigo: tipo numérico
    ShortcutDecode(_T("Vídeo;3000;1"), &s);
    EXPECT_EQ(s.type, MSIP_SHORTCUT_VIDEOCALL);
    ShortcutDecode(_T("Só nome;4000"), &s);
    EXPECT_EQ(s.number, _T("4000"));
    EXPECT_EQ(s.type, MSIP_SHORTCUT_DTMF); // padrão
    ShortcutDecode(_T("x;y"), &s, true);
    EXPECT_TRUE(s.type.IsEmpty());

    accountSettings.Init();
    shortcuts.RemoveAll();
    ShortcutDecode(_T("A;100;call;;0;"), &s);
    shortcuts.Add(s);
    ShortcutDecode(_T("B;200;transfer;;1;"), &s);
    shortcuts.Add(s);
    ShortcutsSave();
    EXPECT_EQ(ini(_T("Shortcuts"), _T("1")), _T("B;200;transfer;;1;"));
    shortcuts.RemoveAll();
    ShortcutsLoad();
    ASSERT_EQ(shortcuts.GetCount(), 2);
    EXPECT_EQ(shortcuts[1].type, MSIP_SHORTCUT_BT);
    EXPECT_TRUE(shortcuts[1].presence);
}
