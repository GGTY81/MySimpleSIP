/*
 * MySimpleSIP — camada de compatibilidade com tipos do Win32/MFC.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Apenas os tipos e funções que o código portado do MicroSIP realmente usa.
// Strings "TCHAR" são wchar_t (o MicroSIP é compilado em Unicode).

#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>

// Dois níveis, como no tchar.h, para que _T(MACRO) expanda a macro antes de colar o L
#ifndef _T
#   define __T(x) L##x
#   define _T(x) __T(x)
#endif

using TCHAR = wchar_t;
using LPTSTR = wchar_t*;
using LPCTSTR = const wchar_t*;
using LPSTR = char*;
using LPCSTR = const char*;
using LPCWSTR = const wchar_t*;

using BOOL = int;
using BYTE = std::uint8_t;
using WORD = std::uint16_t;
using DWORD = std::uint32_t;
using UINT = unsigned int;
using ULONGLONG = unsigned long long;
using LONG = std::int32_t;
using ULONG = std::uint32_t;
using WCHAR = wchar_t;
using HANDLE = void*;
using UCHAR = unsigned char;
using CHAR = char;
using PCHAR = char*;
using USHORT = unsigned short;

#ifndef CALLBACK
#   define CALLBACK
#endif
#ifndef ZeroMemory
#   define ZeroMemory(dst, len) std::memset((dst), 0, (len))
#endif

#ifndef TRUE
#   define TRUE 1
#endif
#ifndef FALSE
#   define FALSE 0
#endif

using COLORREF = DWORD;
#ifndef RGB
#   define RGB(r, g, b) (static_cast<COLORREF>((BYTE)(r) | ((WORD)(BYTE)(g) << 8) | ((DWORD)(BYTE)(b) << 16)))
#endif
#ifndef GetRValue
#   define GetRValue(rgb) ((BYTE)(rgb))
#   define GetGValue(rgb) ((BYTE)(((WORD)(rgb)) >> 8))
#   define GetBValue(rgb) ((BYTE)((rgb) >> 16))
#endif

#ifndef MAX_PATH
#   define MAX_PATH 260
#endif

// Inteiro a partir de string larga, como _wtoi do MSVC (0 em caso de erro, satura em int).
inline int _wtoi(const wchar_t* s)
{
    if (!s)
        return 0;
    long v = std::wcstol(s, nullptr, 10);
    if (v > INT32_MAX)
        return INT32_MAX;
    if (v < INT32_MIN)
        return INT32_MIN;
    return static_cast<int>(v);
}

inline int _wcsicmp(const wchar_t* a, const wchar_t* b)
{
    for (;; ++a, ++b) {
        wint_t ca = std::towlower(static_cast<wint_t>(*a));
        wint_t cb = std::towlower(static_cast<wint_t>(*b));
        if (ca != cb || !ca)
            return ca < cb ? -1 : (ca > cb ? 1 : 0);
    }
}

inline int lstrcmp(const wchar_t* a, const wchar_t* b)
{
    int r = std::wcscmp(a ? a : L"", b ? b : L"");
    return r < 0 ? -1 : (r > 0 ? 1 : 0);
}
