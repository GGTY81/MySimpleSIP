/*
 * MySimpleSIP — armazenamento das senhas das contas SIP.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "PasswordStore.h"

namespace {

IPasswordStore* g_store = nullptr;

const char kB64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int b64Value(wchar_t c)
{
    if (c >= L'A' && c <= L'Z')
        return c - L'A';
    if (c >= L'a' && c <= L'z')
        return c - L'a' + 26;
    if (c >= L'0' && c <= L'9')
        return c - L'0' + 52;
    if (c == L'+')
        return 62;
    if (c == L'/')
        return 63;
    return -1;
}

} // namespace

void SetPasswordStore(IPasswordStore* store)
{
    g_store = store;
}

IPasswordStore* GetPasswordStore()
{
    return g_store;
}

CString PasswordObfuscate(const CString& password)
{
    const CStringA utf8(password);
    const auto* p = reinterpret_cast<const unsigned char*>(utf8.GetString());
    const int n = utf8.GetLength();
    CString out(_T("msip1:"));
    for (int i = 0; i < n; i += 3) {
        unsigned v = p[i] << 16;
        if (i + 1 < n)
            v |= p[i + 1] << 8;
        if (i + 2 < n)
            v |= p[i + 2];
        out.AppendChar(kB64[(v >> 18) & 63]);
        out.AppendChar(kB64[(v >> 12) & 63]);
        out.AppendChar(i + 1 < n ? kB64[(v >> 6) & 63] : L'=');
        out.AppendChar(i + 2 < n ? kB64[v & 63] : L'=');
    }
    return out;
}

bool PasswordDeobfuscate(const CString& value, CString& password)
{
    if (value.Left(6) != _T("msip1:"))
        return false;
    const CString data = value.Mid(6);
    if (data.GetLength() % 4)
        return false;
    CStringA bytes;
    for (int i = 0; i < data.GetLength(); i += 4) {
        int v[4];
        int pad = 0;
        for (int k = 0; k < 4; ++k) {
            wchar_t c = data[i + k];
            if (c == L'=' && k >= 2) {
                v[k] = 0;
                ++pad;
            } else if ((v[k] = b64Value(c)) < 0 || pad) {
                return false;
            }
        }
        unsigned triple = (v[0] << 18) | (v[1] << 12) | (v[2] << 6) | v[3];
        bytes.AppendChar(static_cast<char>((triple >> 16) & 0xFF));
        if (pad < 2)
            bytes.AppendChar(static_cast<char>((triple >> 8) & 0xFF));
        if (pad < 1)
            bytes.AppendChar(static_cast<char>(triple & 0xFF));
    }
    password = CString(bytes);
    return true;
}
