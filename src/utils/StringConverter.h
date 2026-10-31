/*
 * MySimpleSIP — port de Utils/StringConverter.h (MicroSIP 3.22.16).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#pragma once
#include <string>
#include "afx.h" // MySimpleSIP: afxstr.h

inline std::string CStringToUtf8(const CString& str)
{
    // MySimpleSIP: CW2A(str, CP_UTF8) → CStringA da camada compat (UTF-8)
    return CStringA(str).str();
}

inline CString Utf8ToCString(const std::string& str)
{
    // MySimpleSIP: CA2W(str, CP_UTF8) → CString a partir de CStringA (UTF-8)
    return CString(CStringA(str));
}

inline void trim(std::string& s)
{
    const std::string whitespace = " \t\n\r\f\v";

    s.erase(0, s.find_first_not_of(whitespace));
    s.erase(s.find_last_not_of(whitespace) + 1);
}
