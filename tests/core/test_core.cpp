// Testes das funções do núcleo portadas de global.cpp e lib/MSIP.cpp que não precisam
// do pjsua em execução (formatação de números, URIs SIP, plano de discagem, URLs).

#include "global.h"
#include "settings.h"
#include "DialPlanRegex.h"

#include <gtest/gtest.h>

#include "TestPrint.h"
#include "TestSettings.h"

#include <vector>

namespace {

class CoreTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ResetAccountSettings();
        accountSettings.accountId = 1;
        accountSettings.account.domain = _T("sip.example.com");
        accountSettings.account.transport = _T("udp");
        transport_tcp = -1;
        transport_tls = -1;
    }
};

} // namespace

TEST(DialPlanRegex, TraducaoDoDialetoAtl)
{
    std::wstring e;
    ASSERT_TRUE(AtlRegexToEcma(_T("^{0}..........$"), e));
    EXPECT_EQ(e, L"^(0)..........$");
    ASSERT_TRUE(AtlRegexToEcma(_T("(ab)+\\d\\a{\\z}"), e));
    EXPECT_EQ(e, L"(?:ab)+[0-9][a-zA-Z0-9]([0-9]+)");
    ASSERT_TRUE(AtlRegexToEcma(_T("[2-9\\]]\\+\\."), e));
    EXPECT_EQ(e, L"[2-9\\]]\\+\\.");
    EXPECT_FALSE(AtlRegexToEcma(_T("a!b"), e));   // negação ATL não suportada
    EXPECT_FALSE(AtlRegexToEcma(_T("[abc"), e));  // classe aberta
}

TEST(DialPlanRegex, CorrespondenciaEGrupos)
{
    DialPlanMatch m;
    ASSERT_TRUE(DialPlanRegexMatch(_T("^{0}{11}.*$"), _T("011999990000"), m));
    EXPECT_EQ(m.start, 0);
    EXPECT_EQ(m.end, 12);
    ASSERT_EQ(m.groups.size(), 2u);
    EXPECT_EQ(m.groups[0], std::make_pair(0, 1));
    EXPECT_EQ(m.groups[1], std::make_pair(1, 3));
    EXPECT_FALSE(DialPlanRegexMatch(_T("^1..$"), _T("2000"), m));
    EXPECT_FALSE(DialPlanRegexMatch(_T("^+55$"), _T("+55"), m)); // inválida em ATL e ECMAScript
}

TEST_F(CoreTest, SanitizeNumberEIsPSTN)
{
    EXPECT_EQ(SanitizeNumber(_T("(11) 9999-0000.")), _T("1199990000"));
    EXPECT_TRUE(MSIP::IsPSTNNnmber(_T("+55 (11) 9999-0000")));
    EXPECT_TRUE(MSIP::IsPSTNNnmber(_T("*97#")));
    EXPECT_FALSE(MSIP::IsPSTNNnmber(_T("100")));        // menos de 4 caracteres
    EXPECT_FALSE(MSIP::IsPSTNNnmber(_T("joao.silva")));
}

TEST_F(CoreTest, GetSIPURI)
{
    EXPECT_EQ(GetSIPURI(_T("1001")), _T("<sip:1001@sip.example.com>"));
    EXPECT_EQ(GetSIPURI(_T("sip:1001@outro.com")), _T("<sip:1001@outro.com>"));
    EXPECT_EQ(GetSIPURI(_T("1001"), true, false, _T("pbx.local")), _T("<sip:1001@pbx.local>"));
    EXPECT_EQ(GetSIPURI(_T("\"Fulano\" <sip:1001@pbx.local>")), _T("\"Fulano\" <sip:1001@pbx.local>"));

    // transporte TCP é acrescentado quando o transporte existe (não simples)
    accountSettings.account.transport = _T("tcp");
    transport_tcp = 1;
    EXPECT_EQ(GetSIPURI(_T("1001"), false), _T("<sip:1001@sip.example.com;transport=tcp>"));
    EXPECT_EQ(GetSIPURI(_T("1001"), true), _T("<sip:1001@sip.example.com>"));

    // sem conta (só conta local): não acrescenta domínio
    transport_tcp = -1;
    accountSettings.accountId = 0;
    EXPECT_EQ(GetSIPURI(_T("192.168.0.10")), _T("<sip:192.168.0.10>"));
}

TEST_F(CoreTest, AddTransportSuffix)
{
    CString a(_T("sip:1001@pbx.local;transport=udp;lr"));
    AddTransportSuffix(a, nullptr);
    EXPECT_EQ(a, _T("sip:1001@pbx.local;lr"));

    Account acc;
    acc.transport = _T("tls");
    transport_tls = 2;
    CString b(_T("sip:1001@pbx.local;transports=x?subject=oi"));
    AddTransportSuffix(b, &acc);
    EXPECT_EQ(b, _T("sip:1001@pbx.local;transports=x;transport=tls?subject=oi"));
}

TEST_F(CoreTest, FormatNumberComPrefixoEPlanoDeDiscagem)
{
    CString commands;
    CString n(_T("1001,123#"));
    EXPECT_EQ(FormatNumber(n, &commands), _T("<sip:1001@sip.example.com>"));
    EXPECT_EQ(n, _T("1001"));
    EXPECT_EQ(commands, _T(",123#"));

    accountSettings.account.dialingPrefix = _T("0");
    CString pstn(_T("+55 11 99999-0000"));
    EXPECT_EQ(FormatNumber(pstn), _T("<sip:05511999990000@sip.example.com>"));
    accountSettings.account.dialingPrefix.Empty();

    // "<0:>" remove o 0 inicial de números com 11 dígitos; ramais de 4 dígitos passam
    accountSettings.account.dialPlan = _T("(<0:>xxxxxxxxxx|xxxx)");
    CString a(_T("01133334444"));
    EXPECT_EQ(FormatNumber(a), _T("<sip:1133334444@sip.example.com>"));
    CString b(_T("1001"));
    EXPECT_EQ(FormatNumber(b), _T("<sip:1001@sip.example.com>"));
    CString c(_T("123"));
    EXPECT_EQ(FormatNumber(c), _T("<sip:@sip.example.com>")); // sem correspondência: número vazio

    // substituição de prefixo e "x." (qualquer sequência)
    accountSettings.account.dialPlan = _T("<00:+>x.|9x.");
    CString d(_T("005511999990000"));
    EXPECT_EQ(FormatNumber(d), _T("<sip:+5511999990000@sip.example.com>"));
    CString e(_T("90"));
    EXPECT_EQ(FormatNumber(e), _T("<sip:90@sip.example.com>"));

    // noTransform ignora prefixo e plano
    CString f(_T("123"));
    EXPECT_EQ(FormatNumber(f, nullptr, true), _T("<sip:123@sip.example.com>"));
}

TEST(MSIPUri, ParseEBuild)
{
    SIPURI u;
    ASSERT_TRUE(MSIP::ParseSIPURI(_T(" \"João Silva\" <sip:1001@pbx.local:5060;transport=tcp>"), u));
    EXPECT_EQ(u.name, _T("João Silva"));
    EXPECT_EQ(u.user, _T("1001"));
    EXPECT_EQ(u.domain, _T("pbx.local:5060"));
    EXPECT_EQ(u.suffix, _T(";transport=tcp"));
    EXPECT_EQ(MSIP::BuildSIPURI(u), _T("\"João Silva\" <sip:1001@pbx.local:5060;transport=tcp>"));
    EXPECT_EQ(MSIP::RemovePort(u.domain), _T("pbx.local"));

    ASSERT_TRUE(MSIP::ParseSIPURI(_T("unknown <sips:2000@x.com?subject=a>"), u));
    EXPECT_TRUE(u.name.IsEmpty());
    EXPECT_EQ(u.suffix, _T("?subject=a"));
    EXPECT_FALSE(MSIP::ParseSIPURI(_T("<sip:1001@>"), u));
    EXPECT_EQ(MSIP::AddrToNumber(_T("sip:3000@pbx")), _T("3000"));
    EXPECT_EQ(MSIP::AddrToNumber(_T("sip:pbx.local")), _T("pbx.local"));
}

TEST(MSIPUtil, ConversoesEFormatos)
{
    EXPECT_TRUE(MSIP::IsIP(_T("192.168.0.1")));
    EXPECT_TRUE(MSIP::IsIP(_T("fe80::1")));
    EXPECT_FALSE(MSIP::IsIP(_T("pbx.local")));
    EXPECT_FALSE(MSIP::IsIP(_T("")));

    EXPECT_EQ(MSIP::GetDuration(0), _T(""));
    EXPECT_EQ(MSIP::GetDuration(0, true), _T("0:00"));
    EXPECT_EQ(MSIP::GetDuration(75), _T("1:15"));
    EXPECT_EQ(MSIP::GetDuration(3725), _T("1:02:05"));

    pj_str_t p = pj_str(const_cast<char*>("Jos\xC3\xA9 <sip:1@x>"));
    EXPECT_EQ(MSIP::PjToStr(&p, TRUE), _T("José <sip:1@x>"));
    char* raw = MSIP::WideCharToPjStr(_T("ação"));
    EXPECT_STREQ(raw, "a\xC3\xA7\xC3\xA3o");
    free(raw);
    EXPECT_STREQ(MSIP::Utf8EncodeUni(_T("ç")).GetString(), "\xC3\xA7");

    EXPECT_STREQ(urlencode("a b&ç~").GetString(), "a%20b%26%c3%a7~");
    EXPECT_STREQ(urldecode("a%20b%26%c3%a7").GetString(), "a b&\xC3\xA7");
    EXPECT_STREQ(msip_md5sum(CStringA("abc")).GetString(), "900150983cd24fb0d6963f7d28e17f72");

    EXPECT_EQ(MSIP::MakeFilenameFromString(_T("  Minha   Conta ")), _T("minha-conta"));
    EXPECT_EQ(MSIP::GetNormalizedNumberOffset(_T("+5511999990000")), 1);
    EXPECT_EQ(MSIP::GetNormalizedNumberOffset(_T("005511999990000")), 2);
    EXPECT_EQ(MSIP::GetNormalizedNumberOffset(_T("1001")), 0);
}

TEST_F(CoreTest, CommandLineToShellEExpandEnv)
{
    CString cmd, params;
    MSIP::CommandLineToShell(_T("  notify-send \"Chamada de\" 1001 "), cmd, params);
    EXPECT_EQ(cmd, _T("notify-send"));
    EXPECT_EQ(params, _T("Chamada de 1001"));

    setenv("MSIP_TESTE", "valor", 1);
    CString s(_T("$MSIP_TESTE/pasta"));
    MSIP::ExpandEnvironmentStrings(s);
    EXPECT_EQ(s, _T("valor/pasta"));
}

TEST_F(CoreTest, UrlMaskEAccountLabel)
{
    accountSettings.account.label = _T("Minha Conta");
    accountSettings.account.username = _T("1001");
    accountSettings.account.password = _T("s&nha");
    CString u = msip_url_mask(_T("https://x/?u={username}&p={password}&l={label}&v={version}"));
    EXPECT_EQ(u, CString(_T("https://x/?u=1001&p=s%26nha&l=Minha%20Conta&v=")) + _T(_GLOBAL_VERSION));

    Account a;
    a.username = _T("1001");
    a.domain = _T("pbx");
    EXPECT_EQ(GetAccountLabel(&a, 1), _T("1001@pbx"));
    a.label = _T("Rótulo");
    EXPECT_EQ(GetAccountLabel(&a, 1), _T("Rótulo"));
}
