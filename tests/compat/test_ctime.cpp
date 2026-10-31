#include "CTime.h"

#include <gtest/gtest.h>

// O main dos testes fixa TZ=America/Sao_Paulo (UTC-3, sem horário de verão desde 2019).

TEST(CTime, ConstrucaoECampos)
{
    CTime t(2026, 10, 30, 14, 5, 9);
    EXPECT_EQ(t.GetYear(), 2026);
    EXPECT_EQ(t.GetMonth(), 10);
    EXPECT_EQ(t.GetDay(), 30);
    EXPECT_EQ(t.GetHour(), 14);
    EXPECT_EQ(t.GetMinute(), 5);
    EXPECT_EQ(t.GetSecond(), 9);
    EXPECT_EQ(t.GetDayOfWeek(), 6); // sexta-feira (1 = domingo)
    EXPECT_EQ(t.GetTime(), 1793379909);
}

TEST(CTime, Format)
{
    CTime t(static_cast<std::time_t>(1793379909));
    EXPECT_EQ(t.Format(_T("%Y-%m-%d %H:%M:%S")), _T("2026-10-30 14:05:09"));
    // mesmo formato usado pelo MicroSIP em FormatGmt
    EXPECT_EQ(t.FormatGmt(_T("%Y-%m-%dT%H:%M:%SZ")), _T("2026-10-30T17:05:09Z"));
    EXPECT_TRUE(t.Format(_T("")).IsEmpty());
}

TEST(CTime, AritmeticaEComparacao)
{
    CTime a(static_cast<std::time_t>(1000));
    CTime b(static_cast<std::time_t>(1090));
    CTimeSpan d = b - a;
    EXPECT_EQ(d.GetTotalSeconds(), 90);
    EXPECT_EQ(d.GetTotalMinutes(), 1);
    EXPECT_TRUE(a < b);
    EXPECT_EQ(a + CTimeSpan(0, 0, 1, 30), b);
    EXPECT_EQ(b - d, a);
    EXPECT_GT(CTime::GetCurrentTime().GetTime(), 1700000000);
}
