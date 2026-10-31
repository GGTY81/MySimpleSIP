/*
 * MySimpleSIP — expressões regulares no dialeto da ATL (CAtlRegExp) para o plano de
 * discagem do MicroSIP.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// FormatNumber (global.cpp) monta, a partir do dialPlan da conta, uma expressão no
// dialeto da ATL e usa CAtlRegExp. O atlrx.h é código proprietário da Microsoft e não
// é usado no port; esta tradução converte o dialeto ATL para ECMAScript (std::wregex):
//   {…} grupo com captura → (…)          (…) grupo sem captura → (?:…)
//   \d [0-9]   \a [a-zA-Z0-9]   \b [ \t]   \c [a-zA-Z]   \h [0-9a-fA-F]
//   \w [a-zA-Z]+   \z [0-9]+   \n \r?\n   \q "…" ou '…'   \x (outro) → literal x
//   . * + ? ^ $ [ ] *? +? ?? | iguais.
// O operador de negação "!" da ATL não é suportado (a expressão é tratada como
// inválida, como um erro de parse no original — o MicroSIP ignora esse item do plano).

#pragma once

#include "afx.h"

#include <utility>
#include <vector>

struct DialPlanMatch {
    int start = 0;                          // início da correspondência
    int end = 0;                            // fim (exclusivo)
    std::vector<std::pair<int, int>> groups; // grupos {…}: [início, fim); -1 se não participou
};

// Traduz o dialeto ATL para ECMAScript; false se a expressão não for suportada.
bool AtlRegexToEcma(const CString& atl, std::wstring& ecma);

// Equivalente a CAtlRegExp::Parse(pattern, caseSensitive=true) + Match(text).
// Retorna false se a expressão for inválida ou não corresponder.
bool DialPlanRegexMatch(const CString& pattern, const CString& text, DialPlanMatch& match);
