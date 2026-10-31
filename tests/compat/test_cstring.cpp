// Testes da CString de compatibilidade. Os resultados esperados seguem o
// comportamento documentado do CStringT do MFC.

#include "CString.h"

#include <gtest/gtest.h>

#include "TestPrint.h"

#include <cstring>

TEST(CString, ConstrucaoBasica)
{
    CString vazio;
    EXPECT_TRUE(vazio.IsEmpty());
    EXPECT_EQ(vazio.GetLength(), 0);
    EXPECT_STREQ(vazio.GetString(), L"");

    CString s(_T("MicroSIP"));
    EXPECT_EQ(s.GetLength(), 8);
    EXPECT_EQ(s, _T("MicroSIP"));
    EXPECT_EQ(CString(L'x', 3), _T("xxx"));
    EXPECT_EQ(CString(_T("abcdef"), 3), _T("abc"));

    const wchar_t* nulo = nullptr;
    EXPECT_TRUE(CString(nulo).IsEmpty());
}

TEST(CString, ConversaoUtf8)
{
    CString s(_T("Ligação ☎ 😀"));
    CStringA a(s);
    EXPECT_STREQ(a.GetString(), "Liga\xC3\xA7\xC3\xA3o \xE2\x98\x8E \xF0\x9F\x98\x80");
    CString volta(a);
    EXPECT_EQ(volta, s);

    CString deLiteral("caf\xC3\xA9");
    EXPECT_EQ(deLiteral, _T("café"));

    // byte inválido e sequência truncada viram U+FFFD
    CString invalido("a\xFF" "b\xC3");
    EXPECT_EQ(invalido, _T("a\xFFFD" L"b\xFFFD"));
}

TEST(CString, FormatComCStringEInteiros)
{
    CString nome(_T("MicroSIP"));
    CString s;
    s.Format(_T("%s/%s"), nome, _T("3.22.16"));
    EXPECT_EQ(s, _T("MicroSIP/3.22.16"));

    s.Format(_T("%d: %s"), 42, nome);
    EXPECT_EQ(s, _T("42: MicroSIP"));

    s.Format(_T("%05d|%-4s|%x|%.2f"), 7, _T("ab"), 255, 1.5);
    EXPECT_EQ(s, _T("00007|ab  |ff|1.50"));

    s.Format(_T("%I64d %c %%"), static_cast<long long>(1) << 40, L'Z');
    EXPECT_EQ(s, _T("1099511627776 Z %"));

    // %S em CString = texto estreito (char*)
    s.Format(_T("[%S]"), "pjsip");
    EXPECT_EQ(s, _T("[pjsip]"));

    s.AppendFormat(_T(" %s"), CString(_T("ção")));
    EXPECT_EQ(s, _T("[pjsip] ção"));
}

TEST(CString, FormatLongo)
{
    CString grande(L'a', 5000);
    CString s;
    s.Format(_T("<%s>"), grande);
    EXPECT_EQ(s.GetLength(), 5002);
}

TEST(CStringA, FormatInverteSeS)
{
    CStringA a;
    a.Format("%s=%S", "user", L"José");
    EXPECT_STREQ(a.GetString(), "user=Jos\xC3\xA9");
}

TEST(CString, Tokenize)
{
    // Mesmo uso de CmainDlg::PJCreateRaw ao ler a lista de codecs
    CString codecs(_T("PCMA/8000/1 PCMU/8000/1  opus/48000/2"));
    int pos = 0;
    CString t = codecs.Tokenize(_T(" "), pos);
    EXPECT_EQ(t, _T("PCMA/8000/1"));
    EXPECT_EQ(pos, 12);
    EXPECT_EQ(codecs.Tokenize(_T(" "), pos), _T("PCMU/8000/1"));
    EXPECT_EQ(codecs.Tokenize(_T(" "), pos), _T("opus/48000/2"));
    EXPECT_EQ(pos, codecs.GetLength() + 1);
    EXPECT_TRUE(codecs.Tokenize(_T(" "), pos).IsEmpty());
    EXPECT_EQ(pos, -1);

    CString partes(_T("opus/48000/2"));
    int p = 0;
    EXPECT_EQ(partes.Tokenize(_T("/"), p), _T("opus"));
    EXPECT_EQ(partes.Tokenize(_T("/"), p), _T("48000"));
    EXPECT_EQ(partes.Tokenize(_T("/"), p), _T("2"));
    EXPECT_NE(p, -1); // o MFC só devolve -1 na chamada seguinte
    partes.Tokenize(_T("/"), p);
    EXPECT_EQ(p, -1);

    // delimitadores múltiplos e iniciais (lista de STUN ";,")
    CString stun(_T(";,stun1.example.com; stun2.example.com,"));
    int q = 0;
    EXPECT_EQ(stun.Tokenize(_T(";,"), q), _T("stun1.example.com"));
    CString t2 = stun.Tokenize(_T(";,"), q);
    EXPECT_EQ(t2.Trim(), _T("stun2.example.com"));
    EXPECT_TRUE(stun.Tokenize(_T(";,"), q).IsEmpty());
    EXPECT_EQ(q, -1);

    CString vazio;
    int r = 0;
    EXPECT_TRUE(vazio.Tokenize(_T(" "), r).IsEmpty());
    EXPECT_EQ(r, -1);
}

TEST(CString, Find)
{
    CString s(_T("sip:100@pbx.local;transport=tcp"));
    EXPECT_EQ(s.Find(_T("@")), 7);
    EXPECT_EQ(s.Find(L'@'), 7);
    EXPECT_EQ(s.Find(_T(";"), 8), 17);
    EXPECT_EQ(s.Find(_T("xyz")), -1);
    EXPECT_EQ(s.Find(L':', 4), -1);
    EXPECT_EQ(s.Find(L'a', 100), -1);
    EXPECT_EQ(s.Find(_T("tcp"), s.GetLength()), -1);
    EXPECT_EQ(s.ReverseFind(L'p'), 30);
    EXPECT_EQ(s.FindOneOf(_T("@;")), 7);
}

TEST(CString, MidLeftRightComLimites)
{
    CString s(_T("abcdef"));
    EXPECT_EQ(s.Mid(2), _T("cdef"));
    EXPECT_EQ(s.Mid(2, 2), _T("cd"));
    EXPECT_EQ(s.Mid(4, 100), _T("ef"));
    EXPECT_EQ(s.Mid(-3, 2), _T("ab"));
    EXPECT_EQ(s.Mid(10), _T(""));
    EXPECT_EQ(s.Mid(1, -1), _T(""));
    EXPECT_EQ(s.Left(3), _T("abc"));
    EXPECT_EQ(s.Left(99), _T("abcdef"));
    EXPECT_EQ(s.Left(-1), _T(""));
    EXPECT_EQ(s.Right(2), _T("ef"));
    EXPECT_EQ(s.Right(99), _T("abcdef"));
    EXPECT_EQ(s.SpanIncluding(_T("abc")), _T("abc"));
    EXPECT_EQ(s.SpanExcluding(_T("d")), _T("abc"));
}

TEST(CString, TrimReplaceRemoveInsertDelete)
{
    CString s(_T("  \t valor \r\n"));
    EXPECT_EQ(s.Trim(), _T("valor"));
    CString t(_T("xxabcxx"));
    EXPECT_EQ(t.Trim(L'x'), _T("abc"));
    CString u(_T("\"aspas\""));
    EXPECT_EQ(u.Trim(_T("\"")), _T("aspas"));

    CString r(_T("a-b-c"));
    EXPECT_EQ(r.Replace(_T("-"), _T("--")), 2);
    EXPECT_EQ(r, _T("a--b--c"));
    EXPECT_EQ(r.Replace(L'-', L'+'), 4);
    EXPECT_EQ(r, _T("a++b++c"));
    EXPECT_EQ(r.Replace(_T(""), _T("x")), 0);
    EXPECT_EQ(r.Replace(_T("++"), _T("")), 2);
    EXPECT_EQ(r, _T("abc"));

    CString n(_T("(11) 9999-0000"));
    EXPECT_EQ(n.Remove(L' '), 1);
    EXPECT_EQ(n, _T("(11)9999-0000"));

    CString f(_T("call.log"));
    f.Insert(f.Find(L'.'), _T("_1"));
    EXPECT_EQ(f, _T("call_1.log"));
    EXPECT_EQ(f.Insert(100, L'!'), 11);
    EXPECT_EQ(f, _T("call_1.log!"));
    EXPECT_EQ(f.Delete(4, 2), 9);
    EXPECT_EQ(f, _T("call.log!"));
    EXPECT_EQ(f.Delete(8, 50), 8);
    EXPECT_EQ(f, _T("call.log"));
}

TEST(CString, CaixaEComparacao)
{
    CString s(_T("Ação SIP"));
    CString lower(s);
    lower.MakeLower();
    EXPECT_EQ(lower, _T("ação sip"));
    CString upper(s);
    upper.MakeUpper();
    EXPECT_EQ(upper, _T("AÇÃO SIP"));

    EXPECT_EQ(CString(_T("abc")).Compare(_T("abd")), -1);
    EXPECT_EQ(CString(_T("abc")).Compare(_T("abc")), 0);
    EXPECT_EQ(CString(_T("b")).Compare(_T("a")), 1);
    EXPECT_EQ(CString(_T("UDP")).CompareNoCase(_T("udp")), 0);
    EXPECT_EQ(CString(_T("AÇÃO")).CompareNoCase(_T("ação")), 0);
    EXPECT_NE(CString(_T("tcp")).CompareNoCase(_T("tls")), 0);

    EXPECT_TRUE(CString(_T("a")) < CString(_T("b")));
    EXPECT_TRUE(_T("x") == CString(_T("x")));
    EXPECT_TRUE(CString(_T("x")) != _T("y"));
}

TEST(CString, ConcatenacaoEAcesso)
{
    CString s = CString(_T("sip:")) + _T("100") + L'@' + CString(_T("pbx"));
    EXPECT_EQ(s, _T("sip:100@pbx"));
    s += _T(";t=1");
    s.AppendChar(L'!');
    EXPECT_EQ(s, _T("sip:100@pbx;t=1!"));
    EXPECT_EQ(s.GetAt(4), L'1');
    EXPECT_EQ(s[0], L's');
    s.SetAt(0, L'S');
    EXPECT_EQ(s.Left(3), _T("Sip"));
    LPCTSTR p = s;
    EXPECT_EQ(std::wcscmp(p, _T("Sip:100@pbx;t=1!")), 0);
    s.Empty();
    EXPECT_TRUE(s.IsEmpty());
}

TEST(CString, GetBufferReleaseBuffer)
{
    // Padrão usado com GetPrivateProfileString em settings.cpp
    CString s;
    wchar_t* buf = s.GetBuffer(255);
    std::wcscpy(buf, L"udp");
    s.ReleaseBuffer();
    EXPECT_EQ(s, _T("udp"));
    EXPECT_EQ(s.GetLength(), 3);

    CString t(_T("abcdef"));
    t.GetBuffer(2); // não encolhe
    t.ReleaseBuffer(4);
    EXPECT_EQ(t, _T("abcd"));

    CString u;
    wchar_t* b = u.GetBufferSetLength(3);
    b[0] = L'x';
    b[1] = L'y';
    b[2] = L'z';
    u.ReleaseBuffer();
    EXPECT_EQ(u, _T("xyz"));
}
