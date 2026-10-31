/*
 * MySimpleSIP — camada de compatibilidade: CString/CStringA no estilo MFC.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Reproduz o subconjunto da API CStringT do MFC usado pelo MicroSIP, com a mesma
// semântica (índices, limites, retorno de Tokenize/Replace/Find...).
//   CString  = CStringT<wchar_t>  (texto da aplicação; equivale ao CString Unicode)
//   CStringA = CStringT<char>     (bytes; conversões com CString usam UTF-8,
//                                  não a code page ANSI do Windows)
// Format/AppendFormat aceitam CString diretamente nos argumentos e seguem a
// convenção do MFC: no CString, %s é texto largo e %S texto estreito; no CStringA
// é o contrário.

#pragma once

#include "WinTypes.h"

#include <cstdarg>
#include <cstdio>
#include <string>
#include <type_traits>
#include <utility>

namespace msip_compat {

std::string toUtf8(const wchar_t* s, std::size_t len);
std::wstring fromUtf8(const char* s, std::size_t len);

// Converte um formato printf no padrão MFC para o padrão glibc.
std::wstring translateFormat(const wchar_t* fmt);
std::string translateFormat(const char* fmt);

std::wstring vformat(const wchar_t* fmt, va_list args);
std::string vformat(const char* fmt, va_list args);

// Classificação/caixa por caractere (wchar_t usa o LC_CTYPE atual; char só ASCII,
// como o CStringA do MFC fora de code pages multibyte).
wchar_t toLower(wchar_t c);
wchar_t toUpper(wchar_t c);
bool isSpace(wchar_t c);
char toLower(char c);
char toUpper(char c);
bool isSpace(char c);

} // namespace msip_compat

template <typename Ch>
class CStringT {
public:
    using XCHAR = Ch;
    using YCHAR = std::conditional_t<std::is_same_v<Ch, wchar_t>, char, wchar_t>;
    using PCXSTR = const Ch*;
    using PXSTR = Ch*;
    using string_type = std::basic_string<Ch>;

    CStringT() = default;
    CStringT(const CStringT&) = default;
    CStringT(CStringT&&) noexcept = default;
    CStringT& operator=(const CStringT&) = default;
    CStringT& operator=(CStringT&&) noexcept = default;

    CStringT(PCXSTR s) : m_str(s ? s : empty()) {}
    CStringT(PCXSTR s, int len) : m_str(s ? string_type(s, len > 0 ? len : 0) : string_type()) {}
    CStringT(Ch ch, int repeat = 1) : m_str(repeat > 0 ? repeat : 0, ch) {}
    CStringT(const string_type& s) : m_str(s) {}
    CStringT(string_type&& s) : m_str(std::move(s)) {}
    CStringT(const YCHAR* s) { assignOther(s, s ? std::char_traits<YCHAR>::length(s) : 0); }
    CStringT(const CStringT<YCHAR>& s) { assignOther(s.GetString(), s.GetLength()); }

    CStringT& operator=(PCXSTR s) { m_str = s ? s : empty(); return *this; }
    void SetString(PCXSTR s) { m_str = s ? s : empty(); }
    void SetString(PCXSTR s, int len) { m_str = (s && len > 0) ? string_type(s, len) : string_type(); }
    CStringT& operator=(Ch ch) { m_str.assign(1, ch); return *this; }
    CStringT& operator=(const YCHAR* s)
    {
        assignOther(s, s ? std::char_traits<YCHAR>::length(s) : 0);
        return *this;
    }
    CStringT& operator=(const CStringT<YCHAR>& s)
    {
        assignOther(s.GetString(), s.GetLength());
        return *this;
    }

    // --- acesso ---
    int GetLength() const { return static_cast<int>(m_str.size()); }
    bool IsEmpty() const { return m_str.empty(); }
    void Empty() { m_str.clear(); }
    PCXSTR GetString() const { return m_str.c_str(); }
    operator PCXSTR() const { return m_str.c_str(); }
    const string_type& str() const { return m_str; }

    Ch GetAt(int i) const { return m_str.at(static_cast<std::size_t>(i)); }
    void SetAt(int i, Ch ch) { m_str.at(static_cast<std::size_t>(i)) = ch; }
    Ch operator[](int i) const { return GetAt(i); }

    // GetBuffer devolve área gravável com pelo menos minLen caracteres;
    // ReleaseBuffer(-1) ajusta o tamanho até o primeiro NUL, como no MFC.
    PXSTR GetBuffer() { return m_str.data(); }
    PXSTR GetBuffer(int minLen)
    {
        if (minLen > GetLength())
            m_str.resize(static_cast<std::size_t>(minLen), Ch(0));
        return m_str.data();
    }
    PXSTR GetBufferSetLength(int len)
    {
        m_str.resize(static_cast<std::size_t>(len > 0 ? len : 0), Ch(0));
        return m_str.data();
    }
    void ReleaseBuffer(int newLen = -1)
    {
        if (newLen < 0) {
            std::size_t n = m_str.find(Ch(0));
            if (n != string_type::npos)
                m_str.resize(n);
        } else if (static_cast<std::size_t>(newLen) <= m_str.size()) {
            m_str.resize(static_cast<std::size_t>(newLen));
        }
    }

    // --- concatenação ---
    CStringT& Append(PCXSTR s) { if (s) m_str.append(s); return *this; }
    CStringT& Append(PCXSTR s, int len) { if (s && len > 0) m_str.append(s, len); return *this; }
    CStringT& Append(const CStringT& s) { m_str.append(s.m_str); return *this; }
    void AppendChar(Ch ch) { m_str.push_back(ch); }
    CStringT& operator+=(const CStringT& s) { return Append(s); }
    CStringT& operator+=(PCXSTR s) { return Append(s); }
    CStringT& operator+=(Ch ch) { AppendChar(ch); return *this; }

    // --- formatação ---
    template <typename... Args>
    void Format(PCXSTR fmt, Args&&... args)
    {
        m_str = formatRaw(fmt, adapt(std::forward<Args>(args))...);
    }
    template <typename... Args>
    void AppendFormat(PCXSTR fmt, Args&&... args)
    {
        m_str += formatRaw(fmt, adapt(std::forward<Args>(args))...);
    }

    // --- busca ---
    int Find(PCXSTR sub, int start = 0) const
    {
        if (!sub || start < 0 || start > GetLength())
            return -1;
        return toIndex(m_str.find(sub, static_cast<std::size_t>(start)));
    }
    int Find(Ch ch, int start = 0) const
    {
        if (start < 0 || start >= GetLength())
            return -1;
        return toIndex(m_str.find(ch, static_cast<std::size_t>(start)));
    }
    int ReverseFind(Ch ch) const { return toIndex(m_str.rfind(ch)); }
    int FindOneOf(PCXSTR chars) const { return chars ? toIndex(m_str.find_first_of(chars)) : -1; }

    // --- subcadeias (limites ajustados como no MFC) ---
    CStringT Mid(int first) const { return Mid(first, GetLength()); }
    CStringT Mid(int first, int count) const
    {
        if (first < 0)
            first = 0;
        if (count < 0)
            count = 0;
        if (first > GetLength())
            return CStringT();
        if (count > GetLength() - first)
            count = GetLength() - first;
        return CStringT(m_str.substr(static_cast<std::size_t>(first), static_cast<std::size_t>(count)));
    }
    CStringT Left(int count) const { return Mid(0, count); }
    CStringT Right(int count) const
    {
        if (count < 0)
            count = 0;
        if (count > GetLength())
            count = GetLength();
        return Mid(GetLength() - count, count);
    }
    CStringT SpanIncluding(PCXSTR chars) const
    {
        return Left(chars ? toLength(m_str.find_first_not_of(chars)) : 0);
    }
    CStringT SpanExcluding(PCXSTR chars) const
    {
        return Left(chars ? toLength(m_str.find_first_of(chars)) : GetLength());
    }

    // Tokenize: ignora delimitadores iniciais, devolve o próximo token e avança start;
    // sem mais tokens devolve vazio e start = -1.
    CStringT Tokenize(PCXSTR tokens, int& start) const
    {
        if (start < 0)
            return CStringT();
        if (!tokens || !*tokens) {
            if (start < GetLength())
                return Mid(start);
        } else if (start < GetLength()) {
            std::size_t from = m_str.find_first_not_of(tokens, static_cast<std::size_t>(start));
            if (from != string_type::npos) {
                std::size_t until = m_str.find_first_of(tokens, from);
                if (until == string_type::npos)
                    until = m_str.size();
                start = static_cast<int>(until) + 1;
                return CStringT(m_str.substr(from, until - from));
            }
        }
        start = -1;
        return CStringT();
    }

    // --- modificação ---
    CStringT& Trim() { return TrimRight().TrimLeft(); }
    CStringT& Trim(Ch ch) { return TrimRight(ch).TrimLeft(ch); }
    CStringT& Trim(PCXSTR chars) { return TrimRight(chars).TrimLeft(chars); }
    CStringT& TrimLeft()
    {
        std::size_t i = 0;
        while (i < m_str.size() && isSpace(m_str[i]))
            ++i;
        m_str.erase(0, i);
        return *this;
    }
    CStringT& TrimRight()
    {
        std::size_t n = m_str.size();
        while (n > 0 && isSpace(m_str[n - 1]))
            --n;
        m_str.resize(n);
        return *this;
    }
    CStringT& TrimLeft(Ch ch) { m_str.erase(0, m_str.find_first_not_of(ch)); return *this; }
    CStringT& TrimRight(Ch ch) { m_str.erase(trimEnd(m_str.find_last_not_of(ch))); return *this; }
    CStringT& TrimLeft(PCXSTR chars)
    {
        if (chars)
            m_str.erase(0, m_str.find_first_not_of(chars));
        return *this;
    }
    CStringT& TrimRight(PCXSTR chars)
    {
        if (chars)
            m_str.erase(trimEnd(m_str.find_last_not_of(chars)));
        return *this;
    }

    CStringT& MakeLower()
    {
        for (Ch& c : m_str)
            c = msip_compat::toLower(c);
        return *this;
    }
    CStringT& MakeUpper()
    {
        for (Ch& c : m_str)
            c = msip_compat::toUpper(c);
        return *this;
    }
    CStringT& MakeReverse() { m_str.assign(m_str.rbegin(), m_str.rend()); return *this; }

    int Replace(Ch oldCh, Ch newCh)
    {
        int n = 0;
        for (Ch& c : m_str)
            if (c == oldCh) {
                c = newCh;
                ++n;
            }
        return n;
    }
    int Replace(PCXSTR oldStr, PCXSTR newStr)
    {
        if (!oldStr || !*oldStr)
            return 0;
        const string_type from(oldStr), to(newStr ? newStr : empty());
        int n = 0;
        for (std::size_t pos = m_str.find(from); pos != string_type::npos;
             pos = m_str.find(from, pos + to.size())) {
            m_str.replace(pos, from.size(), to);
            ++n;
        }
        return n;
    }
    int Remove(Ch ch)
    {
        std::size_t before = m_str.size();
        std::erase(m_str, ch);
        return static_cast<int>(before - m_str.size());
    }
    int Insert(int index, PCXSTR s)
    {
        index = clampIndex(index);
        if (s)
            m_str.insert(static_cast<std::size_t>(index), s);
        return GetLength();
    }
    int Insert(int index, Ch ch)
    {
        m_str.insert(static_cast<std::size_t>(clampIndex(index)), 1, ch);
        return GetLength();
    }
    int Delete(int index, int count = 1)
    {
        if (index < 0)
            index = 0;
        if (count > 0 && index < GetLength())
            m_str.erase(static_cast<std::size_t>(index), static_cast<std::size_t>(count));
        return GetLength();
    }
    void Truncate(int len)
    {
        if (len >= 0 && len < GetLength())
            m_str.resize(static_cast<std::size_t>(len));
    }

    // --- comparação ---
    int Compare(PCXSTR s) const
    {
        int r = m_str.compare(s ? s : empty());
        return r < 0 ? -1 : (r > 0 ? 1 : 0);
    }
    int CompareNoCase(PCXSTR s) const
    {
        if (!s)
            s = empty();
        for (const Ch* a = m_str.c_str();; ++a, ++s) {
            Ch ca = msip_compat::toLower(*a), cb = msip_compat::toLower(*s);
            if (ca != cb || !ca)
                return ca < cb ? -1 : (ca > cb ? 1 : 0);
        }
    }

    friend bool operator==(const CStringT& a, const CStringT& b) { return a.m_str == b.m_str; }
    friend bool operator==(const CStringT& a, PCXSTR b) { return a.Compare(b) == 0; }
    friend bool operator==(PCXSTR a, const CStringT& b) { return b.Compare(a) == 0; }
    friend bool operator!=(const CStringT& a, const CStringT& b) { return !(a == b); }
    friend bool operator!=(const CStringT& a, PCXSTR b) { return !(a == b); }
    friend bool operator!=(PCXSTR a, const CStringT& b) { return !(a == b); }
    friend bool operator<(const CStringT& a, const CStringT& b) { return a.m_str < b.m_str; }

    friend CStringT operator+(const CStringT& a, const CStringT& b) { return CStringT(a.m_str + b.m_str); }
    friend CStringT operator+(const CStringT& a, PCXSTR b) { CStringT r(a); r += b; return r; }
    friend CStringT operator+(PCXSTR a, const CStringT& b) { CStringT r(a); r += b; return r; }
    friend CStringT operator+(const CStringT& a, Ch b) { CStringT r(a); r += b; return r; }
    friend CStringT operator+(Ch a, const CStringT& b) { CStringT r(a); r += b; return r; }

private:
    static PCXSTR empty()
    {
        static const Ch e[1] = {Ch(0)};
        return e;
    }
    static int toIndex(std::size_t pos) { return pos == string_type::npos ? -1 : static_cast<int>(pos); }
    int toLength(std::size_t pos) const { return pos == string_type::npos ? GetLength() : static_cast<int>(pos); }
    static std::size_t trimEnd(std::size_t lastKept) { return lastKept == string_type::npos ? 0 : lastKept + 1; }
    int clampIndex(int i) const { return i < 0 ? 0 : (i > GetLength() ? GetLength() : i); }
    static bool isSpace(Ch c) { return msip_compat::isSpace(c); }

    void assignOther(const YCHAR* s, std::size_t len)
    {
        if constexpr (std::is_same_v<Ch, wchar_t>)
            m_str = msip_compat::fromUtf8(s, len);
        else
            m_str = msip_compat::toUtf8(s, len);
    }

    // CString/CStringA viram ponteiros; demais argumentos passam inalterados.
    template <typename T>
    static decltype(auto) adapt(T&& v)
    {
        using U = std::remove_cvref_t<T>;
        if constexpr (std::is_same_v<U, CStringT<wchar_t>> || std::is_same_v<U, CStringT<char>>)
            return v.GetString();
        else
            return std::forward<T>(v);
    }

    static string_type formatRaw(PCXSTR fmt, ...)
    {
        if (!fmt)
            return string_type();
        const auto translated = msip_compat::translateFormat(fmt);
        va_list args;
        va_start(args, fmt);
        string_type r = msip_compat::vformat(translated.c_str(), args);
        va_end(args);
        return r;
    }

    string_type m_str;
};

using CString = CStringT<wchar_t>;
using CStringA = CStringT<char>;
using CStringW = CString;
