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

// Formato do arquivo (igual ao do MicroSIP, que herdou do Miranda IM):
//   Language Pack                      ← 1ª linha obrigatória (BOM UTF-8 opcional)
//   Language: PortugueseBR             ← cabeçalhos "Nome: valor" até o primeiro "["
//   RTL: 0
//   ; comentário
//   [Texto original em inglês]
//   Tradução (linhas seguintes são concatenadas com \n)
// Escapes \n, \t, \r e \x → x valem no texto original e na tradução. Entrada sem
// tradução é ignorada; com o mesmo texto original repetido, vale a primeira.
//
// MySimpleSIP: a busca usa o texto original exato (o MicroSIP compara só o hash
// MurmurHash2, sujeito a colisões). Arquivos sem BOM que não sejam UTF-8 válido são
// lidos como Windows-1252 (o original usava a code page do campo "Locale"/ANSI).
// TranslateDialog/TranslateMenu (Win32) serão reimplementados para Qt na camada de UI.

#pragma once

#include "afx.h"

struct LangPackStruct {
    CString filename;
    CString language;
    CString lastModifiedUsing;
    CString authors;
    CString authorEmail;
    bool rtl = false;
    int entryCount = 0;
};
extern LangPackStruct langPack;

// Retorna 0 = ok, 1 = arquivo não abre, 2 = 1ª linha não é "Language Pack",
// 3 = cabeçalho inválido (mesmos códigos do original).
int LoadLangPack(const CString& langPackFile);

// MicroSIP: carrega o primeiro langpack_*.txt da pasta do executável.
// MySimpleSIP: procura, em ordem, na pasta do executável (modo portátil), em
// $XDG_DATA_HOME/MySimpleSIP/langpacks e nas pastas de dados do sistema
// (<prefixo>/share/mysimplesip/langpacks, $XDG_DATA_DIRS). Na primeira pasta que tiver
// arquivos: se houver um só, usa-o; se houver vários, escolhe pelo idioma do sistema
// (LC_ALL/LC_MESSAGES/LANG, ex.: pt_BR → langpack_portuguesebr.txt).
void LoadLangPackModule(const CString& exeDir = CString());
void UnloadLangPackModule();

// Nome do arquivo de langpack correspondente a um locale POSIX ("pt_BR.UTF-8" →
// "langpack_portuguesebr.txt"); vazio se desconhecido ou inglês. Exposto para testes.
CString LangPackFileForLocale(const CString& locale);

// Tradução do texto em inglês; devolve o próprio ponteiro recebido se não houver.
// O ponteiro retornado permanece válido até UnloadLangPackModule().
LPTSTR Translate(LPCTSTR source);
