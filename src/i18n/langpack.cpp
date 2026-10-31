/*
 * MySimpleSIP — pacotes de idioma (langpack_*.txt), port de lib/langpack.* do MicroSIP.
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "langpack.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef MSIP_DATA_DIR
#   define MSIP_DATA_DIR "/usr/share/mysimplesip"
#endif

namespace fs = std::filesystem;

LangPackStruct langPack;

namespace {

std::unordered_map<std::wstring, std::wstring> g_entries;

// TrimString do original: remove caracteres <= ' ' nas pontas
std::wstring trimString(const std::wstring& s)
{
    std::size_t b = 0, e = s.size();
    while (b < e && s[b] <= L' ')
        ++b;
    while (e > b && s[e - 1] <= L' ')
        --e;
    return s.substr(b, e - b);
}

// IsEmpty do original: só espaços, CR ou LF
bool isBlank(const std::wstring& s)
{
    return std::all_of(s.begin(), s.end(), [](wchar_t c) { return c == L' ' || c == L'\r' || c == L'\n'; });
}

// ConvertBackslashes do original: \n \t \r; \x → x; "\" no fim corta a linha
std::wstring convertBackslashes(const std::wstring& s)
{
    std::wstring out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] != L'\\') {
            out.push_back(s[i]);
            continue;
        }
        if (i + 1 >= s.size())
            break;
        wchar_t c = s[++i];
        out.push_back(c == L'n' ? L'\n' : c == L't' ? L'\t' : c == L'r' ? L'\r' : c);
    }
    return out;
}

bool isValidUtf8(const std::string& s)
{
    const auto* p = reinterpret_cast<const unsigned char*>(s.data());
    std::size_t i = 0, n = s.size();
    while (i < n) {
        unsigned char c = p[i];
        int extra = c < 0x80 ? 0 : (c & 0xE0) == 0xC0 ? 1 : (c & 0xF0) == 0xE0 ? 2 : (c & 0xF8) == 0xF0 ? 3 : -1;
        if (extra < 0 || (extra > 0 && i + static_cast<std::size_t>(extra) >= n))
            return false;
        for (int k = 1; k <= extra; ++k)
            if ((p[i + k] & 0xC0) != 0x80)
                return false;
        i += static_cast<std::size_t>(extra) + 1;
    }
    return true;
}

std::wstring decodeCp1252(const std::string& s)
{
    static const wchar_t table[32] = {
        0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
        0x2039, 0x0152, 0x008D, 0x017D, 0x008F, 0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
        0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178};
    std::wstring out;
    out.reserve(s.size());
    for (unsigned char c : s)
        out.push_back(c >= 0x80 && c < 0xA0 ? table[c - 0x80] : static_cast<wchar_t>(c));
    return out;
}

std::vector<std::wstring> splitLines(const std::wstring& text)
{
    std::vector<std::wstring> lines;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        std::size_t nl = text.find(L'\n', pos);
        std::wstring line = text.substr(pos, nl == std::wstring::npos ? std::wstring::npos : nl - pos);
        if (!line.empty() && line.back() == L'\r')
            line.pop_back();
        lines.push_back(line);
        if (nl == std::wstring::npos)
            break;
        pos = nl + 1;
    }
    return lines;
}

std::vector<fs::path> langPackFiles(const fs::path& dir)
{
    std::vector<fs::path> files;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(dir, ec)) {
        const std::string name = e.path().filename().string();
        if (e.is_regular_file(ec) && name.rfind("langpack_", 0) == 0 && name.size() > 13
            && name.compare(name.size() - 4, 4, ".txt") == 0)
            files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::string systemLocale()
{
    for (const char* var : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
        const char* v = std::getenv(var);
        if (v && *v)
            return v;
    }
    return std::string();
}

} // namespace

CString LangPackFileForLocale(const CString& locale)
{
    // Nomes usados pelos langpacks do MicroSIP (lista parcial; ampliar quando houver
    // novos arquivos em resources/langpacks).
    static const std::map<std::wstring, std::wstring> byLanguage = {
        {L"pt_br", L"portuguesebr"}, {L"pt", L"portuguese"}, {L"es", L"spanish"},
        {L"fr", L"french"}, {L"de", L"german"}, {L"it", L"italian"}, {L"ru", L"russian"},
        {L"uk", L"ukrainian"}, {L"pl", L"polish"}, {L"nl", L"dutch"}, {L"cs", L"czech"},
        {L"sk", L"slovak"}, {L"hu", L"hungarian"}, {L"ro", L"romanian"}, {L"bg", L"bulgarian"},
        {L"el", L"greek"}, {L"tr", L"turkish"}, {L"he", L"hebrew"}, {L"ar", L"arabic"},
        {L"fa", L"persian"}, {L"ja", L"japanese"}, {L"ko", L"korean"}, {L"zh", L"chinese"},
        {L"sv", L"swedish"}, {L"da", L"danish"}, {L"fi", L"finnish"}, {L"no", L"norwegian"},
        {L"nb", L"norwegian"}, {L"sr", L"serbian"}, {L"hr", L"croatian"}, {L"sl", L"slovenian"},
        {L"vi", L"vietnamese"}, {L"th", L"thai"}, {L"id", L"indonesian"},
    };
    CString loc(locale);
    int cut = loc.FindOneOf(_T(".@"));
    if (cut >= 0)
        loc = loc.Left(cut);
    loc.MakeLower();
    loc.Replace(L'-', L'_');
    for (const CString& key : {loc, loc.Left(2)}) {
        auto it = byLanguage.find(key.str());
        if (it != byLanguage.end())
            return CString(_T("langpack_")) + it->second.c_str() + _T(".txt");
    }
    return CString();
}

int LoadLangPack(const CString& langPackFile)
{
    // MySimpleSIP: recarregar substitui o langpack anterior (o original acumulava)
    g_entries.clear();
    langPack = LangPackStruct();
    langPack.filename = langPackFile;
    std::ifstream in(CStringA(langPackFile).GetString(), std::ios::binary);
    if (!in)
        return 1;
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::wstring text;
    if (bytes.compare(0, 3, "\xEF\xBB\xBF") == 0)
        text = msip_compat::fromUtf8(bytes.data() + 3, bytes.size() - 3);
    else if (isValidUtf8(bytes))
        text = msip_compat::fromUtf8(bytes.data(), bytes.size());
    else
        text = decodeCp1252(bytes);

    const std::vector<std::wstring> lines = splitLines(text);
    if (lines.empty() || trimString(lines[0]) != L"Language Pack")
        return 2;

    // cabeçalhos
    std::size_t i = 1;
    for (; i < lines.size(); ++i) {
        std::wstring line = trimString(lines[i]);
        if (line.empty() || line[0] == L';')
            continue;
        if (line[0] == L'[')
            break;
        std::size_t colon = line.find(L':');
        if (colon == std::wstring::npos)
            return 3;
        const std::wstring name = line.substr(0, colon);
        const CString value(trimString(line.substr(colon + 1)).c_str());
        if (name == L"Language")
            langPack.language = value;
        else if (name == L"Last-Modified-Using")
            langPack.lastModifiedUsing = value;
        else if (name == L"Authors")
            langPack.authors = value;
        else if (name == L"Author-email")
            langPack.authorEmail = value;
        else if (name == L"RTL")
            langPack.rtl = value == _T("1");
        // "Locale" definia a code page ANSI no Windows; sem efeito aqui
    }

    // corpo
    std::wstring english, local;
    bool haveEntry = false, haveLocal = false;
    auto flush = [&] {
        if (haveEntry && haveLocal)
            g_entries.try_emplace(english, local); // primeira ocorrência vence
    };
    for (; i < lines.size(); ++i) {
        const std::wstring& raw = lines[i];
        if (raw.empty() || isBlank(raw) || raw[0] == L';')
            continue;
        const std::wstring line = convertBackslashes(raw);
        if (line.size() >= 2 && line.front() == L'[' && line.back() == L']') {
            flush();
            english = line.substr(1, line.size() - 2);
            local.clear();
            haveEntry = true;
            haveLocal = false;
        } else if (haveEntry) {
            if (haveLocal)
                local += L'\n';
            local += line;
            haveLocal = true;
        }
    }
    flush();
    langPack.entryCount = static_cast<int>(g_entries.size());
    return 0;
}

void LoadLangPackModule(const CString& exeDir)
{
    langPack.rtl = false;
    std::vector<fs::path> dirs;
    std::error_code ec;
    if (!exeDir.IsEmpty())
        dirs.emplace_back(CStringA(exeDir).GetString());
    else if (auto exe = fs::read_symlink("/proc/self/exe", ec); !ec)
        dirs.push_back(exe.parent_path());
    const char* dataHome = std::getenv("XDG_DATA_HOME");
    const char* home = std::getenv("HOME");
    if (dataHome && *dataHome == '/')
        dirs.push_back(fs::path(dataHome) / "MySimpleSIP/langpacks");
    else if (home && *home)
        dirs.push_back(fs::path(home) / ".local/share/MySimpleSIP/langpacks");
    dirs.push_back(fs::path(MSIP_DATA_DIR) / "langpacks");
    const char* dataDirs = std::getenv("XDG_DATA_DIRS");
    std::string list = dataDirs && *dataDirs ? dataDirs : "/usr/local/share:/usr/share";
    for (std::size_t pos = 0; pos <= list.size();) {
        std::size_t colon = list.find(':', pos);
        std::string d = list.substr(pos, colon == std::string::npos ? std::string::npos : colon - pos);
        if (!d.empty())
            dirs.push_back(fs::path(d) / "mysimplesip/langpacks");
        if (colon == std::string::npos)
            break;
        pos = colon + 1;
    }

    const CString wanted = LangPackFileForLocale(CString(systemLocale().c_str()));
    for (const fs::path& dir : dirs) {
        const std::vector<fs::path> files = langPackFiles(dir);
        if (files.empty())
            continue;
        if (files.size() == 1) {
            LoadLangPack(CString(files[0].string().c_str()));
            return;
        }
        for (const fs::path& f : files)
            if (!wanted.IsEmpty() && CString(f.filename().string().c_str()).CompareNoCase(wanted) == 0) {
                LoadLangPack(CString(f.string().c_str()));
                return;
            }
        return; // vários arquivos e nenhum do idioma do sistema: fica em inglês
    }
}

void UnloadLangPackModule()
{
    g_entries.clear();
    langPack = LangPackStruct();
}

LPTSTR Translate(LPCTSTR source)
{
    if (!source || g_entries.empty())
        return const_cast<LPTSTR>(source);
    auto it = g_entries.find(source);
    if (it == g_entries.end())
        return const_cast<LPTSTR>(source);
    return const_cast<LPTSTR>(it->second.c_str());
}
