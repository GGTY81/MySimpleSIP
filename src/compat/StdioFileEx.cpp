/*
 * MySimpleSIP — CStdioFileEx (ver StdioFileEx.h). Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>. GPL-2.0+.
 */

#include "StdioFileEx.h"

#include <string>

CStdioFileEx::~CStdioFileEx()
{
    Close();
}

BOOL CStdioFileEx::Open(LPCTSTR fileName, unsigned openFlags)
{
    Close();
    const bool write = (openFlags & (modeWrite | modeReadWrite)) != 0;
    const char* mode;
    if (write) {
        mode = (openFlags & modeCreate) && !(openFlags & modeNoTruncate) ? "wb" : "r+b";
    }
    else {
        mode = "rb";
    }
    const CStringA path(fileName); // UTF-8
    m_file = std::fopen(path.GetString(), mode);
    if (!m_file && write && (openFlags & modeCreate)) {
        m_file = std::fopen(path.GetString(), "w+b");
    }
    m_first = true;
    return m_file != nullptr;
}

void CStdioFileEx::Close()
{
    if (m_file) {
        std::fclose(m_file);
        m_file = nullptr;
    }
}

static CString DecodeBytes(const std::string& bytes, UINT codePage)
{
    if (codePage == CP_UTF8) {
        return CString(CStringA(bytes.c_str()));
    }
    CString out;
    for (unsigned char c : bytes) {
        out.AppendChar(static_cast<wchar_t>(c)); // Latin-1
    }
    return out;
}

BOOL CStdioFileEx::ReadString(CString& line)
{
    line.Empty();
    if (!m_file) {
        return FALSE;
    }
    std::string bytes;
    int c;
    bool any = false;
    while ((c = std::fgetc(m_file)) != EOF) {
        any = true;
        if (c == '\n') {
            break;
        }
        bytes.push_back(static_cast<char>(c));
    }
    if (!any) {
        return FALSE;
    }
    if (!bytes.empty() && bytes.back() == '\r') {
        bytes.pop_back();
    }
    if (m_first) {
        m_first = false;
        if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEF
            && static_cast<unsigned char>(bytes[1]) == 0xBB && static_cast<unsigned char>(bytes[2]) == 0xBF) {
            bytes.erase(0, 3);
        }
    }
    line = DecodeBytes(bytes, m_codePage);
    return TRUE;
}

void CStdioFileEx::WriteString(LPCTSTR str)
{
    if (!m_file || !str) {
        return;
    }
    if (m_first) {
        m_first = false;
        if (m_codePage == CP_UTF8 && std::ftell(m_file) == 0) {
            std::fputs("\xEF\xBB\xBF", m_file);
        }
    }
    // modo texto do Windows grava "\r\n" para "\n"
    CString s(str);
    s.Replace(_T("\r\n"), _T("\n"));
    s.Replace(_T("\n"), _T("\r\n"));
    if (m_codePage == CP_UTF8) {
        const CStringA a(s);
        std::fwrite(a.GetString(), 1, a.GetLength(), m_file);
    }
    else {
        for (int i = 0; i < s.GetLength(); i++) {
            wchar_t ch = s.GetAt(i);
            std::fputc(ch < 256 ? static_cast<int>(ch) : '?', m_file);
        }
    }
}

ULONGLONG CStdioFileEx::GetPosition() const
{
    return m_file ? static_cast<ULONGLONG>(std::ftell(m_file)) : 0;
}
