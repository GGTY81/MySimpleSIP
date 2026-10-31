/*
 * MySimpleSIP — CFile/CStdioFileEx (arquivo de texto com página de código) para o código
 * portado do MicroSIP (lib/CSVFile e Service). Copyright (C) 2026 MySimpleSIP. GPL-2.0+.
 */

// No MicroSIP, CStdioFileEx (David Pritchard, lib/StdioFileEx) estende o CStdioFile do
// MFC para ler/gravar texto Unicode em várias páginas de código. Aqui só o que o código
// portado usa: Open/Close, SetCodePage, ReadString/WriteString e GetPosition.
// Semântica: ReadString devolve a linha sem "\n" (e sem "\r" final) e FALSE só no fim do
// arquivo sem nada lido; na gravação com CP_UTF8 o arquivo começa com o BOM UTF-8 (como o
// CStdioFileEx); na leitura o BOM UTF-8 é descartado se existir. Outras páginas de código:
// Latin-1 (CP_ACP no Windows ocidental) — suficiente para os CSV/XML do MicroSIP.

#pragma once

#include "CString.h"

#include <cstdio>

#ifndef CP_ACP
#   define CP_ACP 0
#endif
#ifndef CP_UTF8
#   define CP_UTF8 65001
#endif

class CFile {
public:
    enum OpenFlags : unsigned {
        modeRead = 0x0000,
        modeWrite = 0x0001,
        modeReadWrite = 0x0002,
        shareCompat = 0x0000,
        shareExclusive = 0x0010,
        shareDenyWrite = 0x0020,
        shareDenyRead = 0x0030,
        shareDenyNone = 0x0040,
        modeNoInherit = 0x0080,
        modeCreate = 0x1000,
        modeNoTruncate = 0x2000,
        typeText = 0x4000,
        typeBinary = 0x8000,
    };
};

class CStdioFileEx : public CFile {
public:
    CStdioFileEx() = default;
    ~CStdioFileEx();
    CStdioFileEx(const CStdioFileEx&) = delete;
    CStdioFileEx& operator=(const CStdioFileEx&) = delete;

    BOOL Open(LPCTSTR fileName, unsigned openFlags);
    void Close();
    void SetCodePage(UINT codePage) { m_codePage = codePage; }
    BOOL ReadString(CString& line);
    void WriteString(LPCTSTR str);
    ULONGLONG GetPosition() const;

private:
    std::FILE* m_file = nullptr;
    UINT m_codePage = CP_UTF8;
    bool m_first = true;
};
