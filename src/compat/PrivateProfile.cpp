/*
 * MySimpleSIP — Get/WritePrivateProfileString* do Win32 sobre IniFile.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "PrivateProfile.h"

#include "CString.h"
#include "IniFile.h"

#include <map>
#include <mutex>
#include <sys/stat.h>

namespace {

struct CachedIni {
    IniFile ini;
    struct timespec mtime {};
    off_t size = -1;
};

std::mutex g_mutex;
std::map<std::string, CachedIni> g_cache;

std::string pathOf(LPCTSTR fileName)
{
    return msip_compat::toUtf8(fileName, std::wcslen(fileName));
}

// Devolve o INI em cache, recarregando se o arquivo mudou no disco.
IniFile& acquire(const std::string& path)
{
    struct stat st {};
    bool exists = ::stat(path.c_str(), &st) == 0;
    CachedIni& c = g_cache[path];
    if (!exists) {
        if (c.size != -1) { // arquivo removido por fora
            c.ini = IniFile();
            c.size = -1;
        }
        return c.ini;
    }
    if (c.size != st.st_size || c.mtime.tv_sec != st.st_mtim.tv_sec
        || c.mtime.tv_nsec != st.st_mtim.tv_nsec) {
        c.ini.load(path);
        c.size = st.st_size;
        c.mtime = st.st_mtim;
    }
    return c.ini;
}

bool commit(const std::string& path)
{
    CachedIni& c = g_cache[path];
    if (!c.ini.save(path))
        return false;
    struct stat st {};
    if (::stat(path.c_str(), &st) == 0) {
        c.size = st.st_size;
        c.mtime = st.st_mtim;
    }
    return true;
}

// Copia uma lista de nomes no formato "a\0b\0\0".
DWORD copyList(const std::vector<std::wstring>& names, LPTSTR buffer, DWORD size)
{
    if (!buffer || size == 0)
        return 0;
    if (size == 1) {
        buffer[0] = 0;
        return 0;
    }
    DWORD pos = 0;
    for (const std::wstring& n : names) {
        if (pos + n.size() + 1 > size - 1) { // não cabe (precisa do NUL final extra)
            DWORD room = size - 2 > pos ? size - 2 - pos : 0;
            n.copy(buffer + pos, room);
            buffer[size - 2] = 0;
            buffer[size - 1] = 0;
            return size - 2;
        }
        n.copy(buffer + pos, n.size());
        pos += static_cast<DWORD>(n.size());
        buffer[pos++] = 0;
    }
    buffer[pos] = 0;
    if (pos == 0 && size > 1)
        buffer[1] = 0;
    return pos;
}

DWORD copyString(const std::wstring& s, LPTSTR buffer, DWORD size)
{
    if (!buffer || size == 0)
        return 0;
    DWORD n = static_cast<DWORD>(s.size());
    if (n > size - 1)
        n = size - 1;
    s.copy(buffer, n);
    buffer[n] = 0;
    return n;
}

} // namespace

DWORD GetPrivateProfileString(LPCTSTR section, LPCTSTR key, LPCTSTR defaultValue,
                              LPTSTR buffer, DWORD size, LPCTSTR fileName)
{
    if (!fileName)
        return copyString(L"", buffer, size);
    std::lock_guard lock(g_mutex);
    IniFile& ini = acquire(pathOf(fileName));
    if (!section)
        return copyList(ini.sectionNames(), buffer, size);
    if (!key)
        return copyList(ini.keyNames(section), buffer, size);
    if (auto v = ini.get(section, key))
        return copyString(*v, buffer, size);
    std::wstring def = defaultValue ? defaultValue : L"";
    while (!def.empty() && def.back() == L' ')
        def.pop_back();
    return copyString(def, buffer, size);
}

UINT GetPrivateProfileInt(LPCTSTR section, LPCTSTR key, int defaultValue, LPCTSTR fileName)
{
    wchar_t buf[64];
    if (!section || !key)
        return static_cast<UINT>(defaultValue);
    if (!GetPrivateProfileString(section, key, L"", buf, 64, fileName))
        return static_cast<UINT>(defaultValue);
    return static_cast<UINT>(_wtoi(buf));
}

BOOL WritePrivateProfileString(LPCTSTR section, LPCTSTR key, LPCTSTR value, LPCTSTR fileName)
{
    if (!section || !fileName)
        return FALSE;
    std::lock_guard lock(g_mutex);
    const std::string path = pathOf(fileName);
    IniFile& ini = acquire(path);
    if (!key)
        ini.removeSection(section);
    else if (!value)
        ini.removeKey(section, key);
    else
        ini.set(section, key, value);
    return commit(path) ? TRUE : FALSE;
}

BOOL WritePrivateProfileSection(LPCTSTR section, LPCTSTR data, LPCTSTR fileName)
{
    if (!section || !fileName)
        return FALSE;
    std::lock_guard lock(g_mutex);
    const std::string path = pathOf(fileName);
    IniFile& ini = acquire(path);
    ini.removeSection(section);
    if (data) {
        bool any = false;
        for (const wchar_t* p = data; *p; p += std::wcslen(p) + 1) {
            std::wstring entry(p);
            std::size_t eq = entry.find(L'=');
            if (eq == std::wstring::npos)
                continue;
            ini.set(section, entry.substr(0, eq), entry.substr(eq + 1));
            any = true;
        }
        if (!any) { // seção vazia continua existindo
            ini.set(section, L"", L"");
            ini.removeKey(section, L"");
        }
    }
    return commit(path) ? TRUE : FALSE;
}

void PrivateProfileFlushCache()
{
    std::lock_guard lock(g_mutex);
    g_cache.clear();
}
