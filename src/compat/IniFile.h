/*
 * MySimpleSIP — arquivo INI com a semântica dos "private profile" do Windows.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Regras reproduzidas do Windows (GetPrivateProfileString/WritePrivateProfileString):
//  - seções e chaves sem diferenciar maiúsculas/minúsculas; vale a primeira ocorrência;
//  - na leitura, chave e valor são aparados e aspas externas iguais ("..." ou '...')
//    são removidas; na gravação o valor é escrito como recebido;
//  - comentários (;), linhas em branco e a ordem do arquivo são preservados;
//  - chave nova entra no fim da sua seção; seção nova entra no fim do arquivo.
// Leitura aceita UTF-8 (com/sem BOM) e UTF-16 LE/BE com BOM (microsip.ini do Windows);
// a gravação é sempre em UTF-8 sem BOM, com quebras de linha "\n".

#pragma once

#include <optional>
#include <string>
#include <vector>

class IniFile {
public:
    // Carrega o arquivo; arquivo inexistente resulta em INI vazio (e retorna false).
    bool load(const std::string& path);
    // Grava de forma atômica (arquivo temporário + rename).
    bool save(const std::string& path) const;

    void parse(const std::wstring& text);
    std::wstring serialize() const;

    std::optional<std::wstring> get(const std::wstring& section, const std::wstring& key) const;
    void set(const std::wstring& section, const std::wstring& key, const std::wstring& value);
    bool removeKey(const std::wstring& section, const std::wstring& key);
    bool removeSection(const std::wstring& section);

    std::vector<std::wstring> sectionNames() const;
    std::vector<std::wstring> keyNames(const std::wstring& section) const;
    bool hasSection(const std::wstring& section) const;

    // Decodifica bytes de arquivo INI (detecta BOM); exposto para testes.
    static std::wstring decode(const std::string& bytes);

private:
    struct Line {
        enum class Kind { Blank, Comment, Section, Key, Other } kind = Kind::Other;
        std::wstring raw;   // texto original da linha (sem quebra)
        std::wstring name;  // nome da seção ou da chave (aparado)
        std::wstring value; // valor bruto (aparado), apenas para Key
    };

    static Line parseLine(const std::wstring& raw);
    // Índice da linha [section] ou -1; a seção vai até a próxima linha Section.
    int findSection(const std::wstring& section) const;
    int sectionEnd(int sectionLine) const;
    int findKey(int sectionLine, const std::wstring& key) const;

    std::vector<Line> m_lines;
};
