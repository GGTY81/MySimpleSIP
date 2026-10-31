/*
 * MySimpleSIP — camada de compatibilidade: CTime/CTimeSpan no estilo MFC.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "CString.h"

#include <ctime>

class CTimeSpan {
public:
    CTimeSpan() = default;
    explicit CTimeSpan(std::time_t seconds) : m_span(seconds) {}
    CTimeSpan(long days, int hours, int mins, int secs)
        : m_span(((static_cast<std::time_t>(days) * 24 + hours) * 60 + mins) * 60 + secs) {}

    std::time_t GetTimeSpan() const { return m_span; }
    std::time_t GetTotalSeconds() const { return m_span; }
    std::time_t GetTotalMinutes() const { return m_span / 60; }
    std::time_t GetTotalHours() const { return m_span / 3600; }
    std::time_t GetDays() const { return m_span / 86400; }

    friend bool operator==(CTimeSpan a, CTimeSpan b) { return a.m_span == b.m_span; }
    friend auto operator<=>(CTimeSpan a, CTimeSpan b) { return a.m_span <=> b.m_span; }

private:
    std::time_t m_span = 0;
};

// Instante em segundos desde a época, interpretado na hora local (como o CTime do MFC).
class CTime {
public:
    CTime() = default;
    CTime(std::time_t t) : m_time(t) {}
    CTime(int year, int month, int day, int hour, int min, int sec);

    static CTime GetCurrentTime() { return CTime(std::time(nullptr)); }

    std::time_t GetTime() const { return m_time; }
    int GetYear() const { return local().tm_year + 1900; }
    int GetMonth() const { return local().tm_mon + 1; }
    int GetDay() const { return local().tm_mday; }
    int GetHour() const { return local().tm_hour; }
    int GetMinute() const { return local().tm_min; }
    int GetSecond() const { return local().tm_sec; }
    int GetDayOfWeek() const { return local().tm_wday + 1; } // 1 = domingo

    // Formato strftime (wcsftime); Format usa hora local, FormatGmt UTC.
    CString Format(LPCTSTR format) const;
    CString FormatGmt(LPCTSTR format) const;

    friend bool operator==(CTime a, CTime b) { return a.m_time == b.m_time; }
    friend auto operator<=>(CTime a, CTime b) { return a.m_time <=> b.m_time; }
    friend CTimeSpan operator-(CTime a, CTime b) { return CTimeSpan(a.m_time - b.m_time); }
    friend CTime operator+(CTime a, CTimeSpan s) { return CTime(a.m_time + s.GetTimeSpan()); }
    friend CTime operator-(CTime a, CTimeSpan s) { return CTime(a.m_time - s.GetTimeSpan()); }

private:
    std::tm local() const;
    std::time_t m_time = 0;
};
