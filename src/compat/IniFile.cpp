/*
 * MySimpleSIP — arquivo INI com a semântica dos "private profile" do Windows.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "IniFile.h"

#include "CString.h"
#include "WinTypes.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <unistd.h>

namespace {

std::wstring trim(const std::wstring& s)
{
    const wchar_t* ws = L" \t\r\n\v\f";
    std::size_t b = s.find_first_not_of(ws);
    if (b == std::wstring::npos)
        return std::wstring();
    std::size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

bool iequals(const std::wstring& a, const std::wstring& b)
{
    return _wcsicmp(a.c_str(), b.c_str()) == 0;
}

std::wstring decodeUtf16(const std::string& bytes, std::size_t start, bool bigEndian)
{
    std::wstring out;
    out.reserve((bytes.size() - start) / 2);
    auto unit = [&](std::size_t i) -> std::uint32_t {
        auto b0 = static_cast<unsigned char>(bytes[i]);
        auto b1 = static_cast<unsigned char>(bytes[i + 1]);
        return bigEndian ? (b0 << 8 | b1) : (b1 << 8 | b0);
    };
    for (std::size_t i = start; i + 1 < bytes.size(); i += 2) {
        std::uint32_t c = unit(i);
        if (c >= 0xD800 && c <= 0xDBFF && i + 3 < bytes.size()) {
            std::uint32_t lo = unit(i + 2);
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                c = 0x10000 + ((c - 0xD800) << 10) + (lo - 0xDC00);
                i += 2;
            } else {
                c = 0xFFFD;
            }
        } else if (c >= 0xD800 && c <= 0xDFFF) {
            c = 0xFFFD;
        }
        out.push_back(static_cast<wchar_t>(c));
    }
    return out;
}

} // namespace

std::wstring IniFile::decode(const std::string& bytes)
{
    if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFF
        && static_cast<unsigned char>(bytes[1]) == 0xFE)
        return decodeUtf16(bytes, 2, false);
    if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFE
        && static_cast<unsigned char>(bytes[1]) == 0xFF)
        return decodeUtf16(bytes, 2, true);
    std::size_t start = 0;
    if (bytes.size() >= 3 && bytes.compare(0, 3, "\xEF\xBB\xBF") == 0)
        start = 3;
    return msip_compat::fromUtf8(bytes.data() + start, bytes.size() - start);
}

bool IniFile::load(const std::string& path)
{
    m_lines.clear();
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    parse(decode(bytes));
    return true;
}

bool IniFile::save(const std::string& path) const
{
    const std::wstring text = serialize();
    const std::string bytes = msip_compat::toUtf8(text.data(), text.size());
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out)
            return false;
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        out.flush();
        if (!out)
            return false;
    }
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        ::unlink(tmp.c_str());
        return false;
    }
    return true;
}

IniFile::Line IniFile::parseLine(const std::wstring& raw)
{
    Line line;
    line.raw = raw;
    const std::wstring t = trim(raw);
    if (t.empty()) {
        line.kind = Line::Kind::Blank;
    } else if (t[0] == L';') {
        line.kind = Line::Kind::Comment;
    } else if (t[0] == L'[') {
        line.kind = Line::Kind::Section;
        std::size_t close = t.find(L']');
        line.name = trim(t.substr(1, close == std::wstring::npos ? std::wstring::npos : close - 1));
    } else if (std::size_t eq = t.find(L'='); eq != std::wstring::npos) {
        line.kind = Line::Kind::Key;
        line.name = trim(t.substr(0, eq));
        line.value = trim(t.substr(eq + 1));
    }
    return line;
}

void IniFile::parse(const std::wstring& text)
{
    m_lines.clear();
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t nl = text.find(L'\n', pos);
        std::wstring raw = text.substr(pos, nl == std::wstring::npos ? std::wstring::npos : nl - pos);
        if (!raw.empty() && raw.back() == L'\r')
            raw.pop_back();
        m_lines.push_back(parseLine(raw));
        if (nl == std::wstring::npos)
            break;
        pos = nl + 1;
    }
}

std::wstring IniFile::serialize() const
{
    std::wstring out;
    for (const Line& l : m_lines) {
        out += l.raw;
        out += L'\n';
    }
    return out;
}

int IniFile::findSection(const std::wstring& section) const
{
    for (std::size_t i = 0; i < m_lines.size(); ++i)
        if (m_lines[i].kind == Line::Kind::Section && iequals(m_lines[i].name, section))
            return static_cast<int>(i);
    return -1;
}

int IniFile::sectionEnd(int sectionLine) const
{
    std::size_t i = static_cast<std::size_t>(sectionLine) + 1;
    while (i < m_lines.size() && m_lines[i].kind != Line::Kind::Section)
        ++i;
    return static_cast<int>(i);
}

int IniFile::findKey(int sectionLine, const std::wstring& key) const
{
    int end = sectionEnd(sectionLine);
    for (int i = sectionLine + 1; i < end; ++i)
        if (m_lines[i].kind == Line::Kind::Key && iequals(m_lines[i].name, key))
            return i;
    return -1;
}

std::optional<std::wstring> IniFile::get(const std::wstring& section, const std::wstring& key) const
{
    int s = findSection(section);
    if (s < 0)
        return std::nullopt;
    int k = findKey(s, key);
    if (k < 0)
        return std::nullopt;
    std::wstring v = m_lines[k].value;
    if (v.size() >= 2 && (v.front() == L'"' || v.front() == L'\'') && v.back() == v.front())
        v = v.substr(1, v.size() - 2);
    return v;
}

void IniFile::set(const std::wstring& section, const std::wstring& key, const std::wstring& value)
{
    Line kv = parseLine(key + L"=" + value);
    kv.name = key;
    kv.kind = Line::Kind::Key;
    int s = findSection(section);
    if (s < 0) {
        m_lines.push_back(parseLine(L"[" + section + L"]"));
        m_lines.push_back(kv);
        return;
    }
    int k = findKey(s, key);
    if (k >= 0) {
        m_lines[k] = kv;
        return;
    }
    // depois da última linha não vazia da seção
    int insertAt = sectionEnd(s);
    while (insertAt - 1 > s && m_lines[insertAt - 1].kind == Line::Kind::Blank)
        --insertAt;
    m_lines.insert(m_lines.begin() + insertAt, kv);
}

bool IniFile::removeKey(const std::wstring& section, const std::wstring& key)
{
    int s = findSection(section);
    if (s < 0)
        return false;
    int k = findKey(s, key);
    if (k < 0)
        return false;
    m_lines.erase(m_lines.begin() + k);
    return true;
}

bool IniFile::removeSection(const std::wstring& section)
{
    int s = findSection(section);
    if (s < 0)
        return false;
    m_lines.erase(m_lines.begin() + s, m_lines.begin() + sectionEnd(s));
    return true;
}

std::vector<std::wstring> IniFile::sectionNames() const
{
    std::vector<std::wstring> names;
    for (const Line& l : m_lines)
        if (l.kind == Line::Kind::Section)
            names.push_back(l.name);
    return names;
}

std::vector<std::wstring> IniFile::keyNames(const std::wstring& section) const
{
    std::vector<std::wstring> names;
    int s = findSection(section);
    if (s < 0)
        return names;
    int end = sectionEnd(s);
    for (int i = s + 1; i < end; ++i)
        if (m_lines[i].kind == Line::Kind::Key)
            names.push_back(m_lines[i].name);
    return names;
}

bool IniFile::hasSection(const std::wstring& section) const
{
    return findSection(section) >= 0;
}
