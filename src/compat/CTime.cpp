/*
 * MySimpleSIP — camada de compatibilidade: CTime/CTimeSpan no estilo MFC.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "CTime.h"

#include <cwchar>

CTime::CTime(int year, int month, int day, int hour, int min, int sec)
{
    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = -1;
    m_time = std::mktime(&tm);
}

std::tm CTime::local() const
{
    std::tm tm{};
    localtime_r(&m_time, &tm);
    return tm;
}

namespace {

CString formatTm(const std::tm& tm, LPCTSTR format)
{
    if (!format || !*format)
        return CString();
    wchar_t buf[512];
    std::size_t n = std::wcsftime(buf, sizeof(buf) / sizeof(buf[0]), format, &tm);
    return CString(buf, static_cast<int>(n));
}

} // namespace

CString CTime::Format(LPCTSTR format) const
{
    return formatTm(local(), format);
}

CString CTime::FormatGmt(LPCTSTR format) const
{
    std::tm tm{};
    gmtime_r(&m_time, &tm);
    return formatTm(tm, format);
}
