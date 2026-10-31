/*
 * MySimpleSIP — CMarkup: navegação e geração de XML simples.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// MySimpleSIP: o MicroSIP usa o CMarkup Lite (First Objective Software), cuja licença não
// permite redistribuir o código-fonte. Esta é uma implementação própria (sobre o
// QXmlStreamReader) só da interface usada pelo código portado (Contacts.xml, exportação
// XML, diretório de usuários), com a mesma semântica de posições do CMarkup:
// posição principal (FindElem/AddElem/GetAttrib) e posição filha (FindChildElem/
// GetChildData), IntoElem desce (o filho vira a posição principal) e OutOfElem sobe.

#pragma once

#include "afx.h"

#include <memory>
#include <vector>

class CMarkup {
public:
    CMarkup();
    ~CMarkup();

    BOOL SetDoc(LPCTSTR szDoc);
    CString GetDoc() const;

    BOOL FindElem(LPCTSTR szName = NULL);
    BOOL FindChildElem(LPCTSTR szName = NULL);
    BOOL IntoElem();
    BOOL OutOfElem();
    void ResetPos();
    void ResetChildPos() { m_child = -1; }
    void ResetMainPos();

    CString GetTagName() const;
    CString GetChildTagName() const;
    CString GetAttrib(LPCTSTR szAttrib) const;
    BOOL FindAttrib(LPCTSTR szAttrib) const;
    CString GetData() const;
    CString GetChildData() const;

    BOOL AddElem(LPCTSTR szName, LPCTSTR szData = NULL);
    BOOL AddChildElem(LPCTSTR szName, LPCTSTR szData = NULL);
    BOOL AddAttrib(LPCTSTR szAttrib, LPCTSTR szValue);
    BOOL AddChildAttrib(LPCTSTR szAttrib, LPCTSTR szValue);

    struct Node;

private:
    Node* Main() const;
    Node* Child() const;
    int Find(Node* parent, int from, LPCTSTR szName) const;

    std::unique_ptr<Node> m_root; // documento (filhos = elementos de nível superior)
    Node* m_parent;               // elemento cujos filhos são o nível principal
    int m_main = -1;              // índice da posição principal em m_parent
    int m_child = -1;             // índice da posição filha no elemento principal
};
