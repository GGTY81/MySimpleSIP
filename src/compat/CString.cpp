/*
 * MySimpleSIP — camada de compatibilidade: CString/CStringA no estilo MFC.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "CString.h"

#include <cctype>
#include <vector>

namespace msip_compat {

std::string toUtf8(const wchar_t* s, std::size_t len)
{
    std::string out;
    out.reserve(len);
    for (std::size_t i = 0; i < len; ++i) {
        auto c = static_cast<std::uint32_t>(s[i]);
        if (c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF))
            c = 0xFFFD;
        if (c < 0x80) {
            out.push_back(static_cast<char>(c));
        } else if (c < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (c >> 6)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else if (c < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (c >> 12)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (c >> 18)));
            out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }
    return out;
}

// Decodifica UTF-8; sequências inválidas viram U+FFFD (um por byte inválido).
std::wstring fromUtf8(const char* s, std::size_t len)
{
    std::wstring out;
    out.reserve(len);
    const auto* p = reinterpret_cast<const unsigned char*>(s);
    std::size_t i = 0;
    while (i < len) {
        unsigned char b = p[i];
        std::uint32_t c;
        int extra;
        if (b < 0x80) {
            out.push_back(static_cast<wchar_t>(b));
            ++i;
            continue;
        } else if ((b & 0xE0) == 0xC0) {
            c = b & 0x1F;
            extra = 1;
        } else if ((b & 0xF0) == 0xE0) {
            c = b & 0x0F;
            extra = 2;
        } else if ((b & 0xF8) == 0xF0) {
            c = b & 0x07;
            extra = 3;
        } else {
            out.push_back(L'\xFFFD');
            ++i;
            continue;
        }
        if (i + static_cast<std::size_t>(extra) >= len) { // sequência truncada no fim
            out.push_back(L'\xFFFD');
            ++i;
            continue;
        }
        bool ok = true;
        for (int k = 1; k <= extra; ++k) {
            if ((p[i + k] & 0xC0) != 0x80) {
                ok = false;
                break;
            }
            c = (c << 6) | (p[i + k] & 0x3F);
        }
        static const std::uint32_t minValue[4] = {0, 0x80, 0x800, 0x10000};
        if (!ok || c < minValue[extra] || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) {
            out.push_back(L'\xFFFD');
            ++i;
            continue;
        }
        out.push_back(static_cast<wchar_t>(c));
        i += static_cast<std::size_t>(extra) + 1;
    }
    return out;
}

namespace {

// Traduz um formato printf do MFC para a glibc.
// wideString = true: formato de CString (%s largo, %S estreito); false: CStringA.
template <typename Ch>
std::basic_string<Ch> translate(const Ch* fmt, bool wideString)
{
    std::basic_string<Ch> out;
    for (const Ch* p = fmt; *p; ++p) {
        out.push_back(*p);
        if (*p != Ch('%'))
            continue;
        ++p;
        if (*p == Ch('%')) {
            out.push_back(*p);
            continue;
        }
        // flags, largura e precisão: copiados como estão
        while (*p && std::basic_string<Ch>({Ch('-'), Ch('+'), Ch(' '), Ch('#'), Ch('0'), Ch('\'')})
                         .find(*p) != std::basic_string<Ch>::npos)
            out.push_back(*p++);
        while (*p == Ch('*') || (*p >= Ch('0') && *p <= Ch('9')))
            out.push_back(*p++);
        if (*p == Ch('.')) {
            out.push_back(*p++);
            while (*p == Ch('*') || (*p >= Ch('0') && *p <= Ch('9')))
                out.push_back(*p++);
        }
        // modificadores de tamanho (inclui os do MSVC: I64, I32, I)
        std::basic_string<Ch> length;
        if (p[0] == Ch('I') && p[1] == Ch('6') && p[2] == Ch('4')) {
            length = {Ch('l'), Ch('l')};
            p += 3;
        } else if (p[0] == Ch('I') && p[1] == Ch('3') && p[2] == Ch('2')) {
            p += 3;
        } else if (p[0] == Ch('I')) {
            length = {Ch('z')};
            p += 1;
        } else {
            while (*p == Ch('h') || *p == Ch('l') || *p == Ch('L') || *p == Ch('z') || *p == Ch('j')
                   || *p == Ch('t') || *p == Ch('q'))
                length.push_back(*p++);
        }
        if (!*p)
            break;
        const Ch conv = *p;
        const bool shortLen = length == std::basic_string<Ch>{Ch('h')};
        const bool longLen = length == std::basic_string<Ch>{Ch('l')};
        if (conv == Ch('s') || conv == Ch('c')) {
            // no CString, %s/%c sem modificador são largos; no CStringA, estreitos
            bool wide = longLen || (!shortLen && wideString);
            if (wide)
                out.push_back(Ch('l'));
            out.push_back(conv);
        } else if (conv == Ch('S') || conv == Ch('C')) {
            // %S/%C: o "outro" tipo
            bool wide = longLen || (!shortLen && !wideString);
            if (wide)
                out.push_back(Ch('l'));
            out.push_back(conv == Ch('S') ? Ch('s') : Ch('c'));
        } else {
            out += length;
            out.push_back(conv);
        }
    }
    return out;
}

} // namespace

std::wstring translateFormat(const wchar_t* fmt) { return translate(fmt, true); }
std::string translateFormat(const char* fmt) { return translate(fmt, false); }

std::wstring vformat(const wchar_t* fmt, va_list args)
{
    // vswprintf não informa o tamanho necessário: cresce o buffer até caber
    std::vector<wchar_t> buf(256);
    for (int attempt = 0; attempt < 16; ++attempt) {
        va_list copy;
        va_copy(copy, args);
        int n = std::vswprintf(buf.data(), buf.size(), fmt, copy);
        va_end(copy);
        if (n >= 0)
            return std::wstring(buf.data(), static_cast<std::size_t>(n));
        buf.resize(buf.size() * 2);
    }
    return std::wstring();
}

std::string vformat(const char* fmt, va_list args)
{
    va_list copy;
    va_copy(copy, args);
    int n = std::vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);
    if (n < 0)
        return std::string();
    std::string out(static_cast<std::size_t>(n) + 1, '\0');
    va_copy(copy, args);
    std::vsnprintf(out.data(), out.size(), fmt, copy);
    va_end(copy);
    out.resize(static_cast<std::size_t>(n));
    return out;
}

wchar_t toLower(wchar_t c) { return static_cast<wchar_t>(std::towlower(static_cast<wint_t>(c))); }
wchar_t toUpper(wchar_t c) { return static_cast<wchar_t>(std::towupper(static_cast<wint_t>(c))); }
bool isSpace(wchar_t c) { return std::iswspace(static_cast<wint_t>(c)) != 0; }
char toLower(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c; }
char toUpper(char c) { return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 32) : c; }
bool isSpace(char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }

} // namespace msip_compat
