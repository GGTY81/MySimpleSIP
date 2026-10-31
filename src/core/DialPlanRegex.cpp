/*
 * MySimpleSIP — expressões regulares no dialeto da ATL para o plano de discagem.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "DialPlanRegex.h"

#include <regex>

bool AtlRegexToEcma(const CString& atl, std::wstring& ecma)
{
    ecma.clear();
    const std::wstring& s = atl.str();
    bool inClass = false;
    for (std::size_t i = 0; i < s.size(); ++i) {
        wchar_t c = s[i];
        if (inClass) {
            if (c == L'\\' && i + 1 < s.size()) {
                ecma += c;
                ecma += s[++i];
            } else {
                if (c == L']')
                    inClass = false;
                ecma += c;
            }
            continue;
        }
        switch (c) {
        case L'{':
            ecma += L'(';
            break;
        case L'}':
            ecma += L')';
            break;
        case L'(':
            ecma += L"(?:";
            break;
        case L'[':
            inClass = true;
            ecma += c;
            break;
        case L'!':
            return false;
        case L'\\': {
            if (i + 1 >= s.size())
                return false;
            wchar_t e = s[++i];
            switch (e) {
            case L'd': ecma += L"[0-9]"; break;
            case L'a': ecma += L"[a-zA-Z0-9]"; break;
            case L'b': ecma += L"[ \\t]"; break;
            case L'c': ecma += L"[a-zA-Z]"; break;
            case L'h': ecma += L"[0-9a-fA-F]"; break;
            case L'w': ecma += L"[a-zA-Z]+"; break;
            case L'z': ecma += L"[0-9]+"; break;
            case L'n': ecma += L"\\r?\\n"; break;
            case L'q': ecma += L"(?:\"[^\"]*\"|'[^']*')"; break;
            default:
                // escape de caractere literal
                if (std::wstring(L"\\^$.|?*+()[]{}/").find(e) != std::wstring::npos)
                    ecma += L'\\';
                ecma += e;
                break;
            }
            break;
        }
        default:
            ecma += c;
            break;
        }
    }
    return !inClass;
}

bool DialPlanRegexMatch(const CString& pattern, const CString& text, DialPlanMatch& match)
{
    match = DialPlanMatch();
    std::wstring ecma;
    if (!AtlRegexToEcma(pattern, ecma))
        return false;
    try {
        const std::wregex re(ecma, std::regex::ECMAScript);
        std::wsmatch m;
        const std::wstring& input = text.str();
        if (!std::regex_search(input, m, re))
            return false;
        match.start = static_cast<int>(m.position(0));
        match.end = match.start + static_cast<int>(m.length(0));
        for (std::size_t g = 1; g < m.size(); ++g) {
            if (m[g].matched) {
                int b = static_cast<int>(m.position(g));
                match.groups.emplace_back(b, b + static_cast<int>(m.length(g)));
            } else {
                match.groups.emplace_back(-1, -1);
            }
        }
        return true;
    } catch (const std::regex_error&) {
        return false;
    }
}
