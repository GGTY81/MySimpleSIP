// Impressão legível de CString/CStringA nas mensagens de falha do GoogleTest.

#pragma once

#include "CString.h"

#include <ostream>

inline void PrintTo(const CStringT<wchar_t>& s, std::ostream* os)
{
    *os << '"' << CStringA(s).GetString() << '"';
}

inline void PrintTo(const CStringT<char>& s, std::ostream* os)
{
    *os << '"' << s.GetString() << '"';
}
