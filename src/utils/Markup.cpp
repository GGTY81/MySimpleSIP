/*
 * MySimpleSIP — CMarkup (ver Markup.h).
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "Markup.h"

#include <QString>
#include <QXmlStreamReader>

#include <utility>

struct CMarkup::Node {
    CString name;
    std::vector<std::pair<CString, CString>> attribs;
    CString data;
    std::vector<std::unique_ptr<Node>> children;
    Node* parent = nullptr;
};

static CString CStr(QStringView s)
{
    return CString(s.toString().toStdWString());
}

CMarkup::CMarkup() : m_root(std::make_unique<Node>()), m_parent(m_root.get())
{
}

CMarkup::~CMarkup() = default;

BOOL CMarkup::SetDoc(LPCTSTR szDoc)
{
    m_root = std::make_unique<Node>();
    ResetPos();
    if (!szDoc) {
        return TRUE;
    }
    QXmlStreamReader reader(QString::fromWCharArray(szDoc));
    Node* current = m_root.get();
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            auto node = std::make_unique<Node>();
            node->name = CStr(reader.name());
            for (const QXmlStreamAttribute& a : reader.attributes()) {
                node->attribs.emplace_back(CStr(a.qualifiedName()), CStr(a.value()));
            }
            node->parent = current;
            Node* raw = node.get();
            current->children.push_back(std::move(node));
            current = raw;
        }
        else if (reader.isEndElement()) {
            if (current->parent) {
                current = current->parent;
            }
        }
        else if (reader.isCharacters() && current != m_root.get()) {
            current->data += CStr(reader.text());
        }
    }
    if (reader.hasError()) {
        m_root = std::make_unique<Node>();
        ResetPos();
        return FALSE;
    }
    return TRUE;
}

static void Escape(CString& out, const CString& s, bool attrib)
{
    for (int i = 0; i < s.GetLength(); i++) {
        wchar_t c = s.GetAt(i);
        switch (c) {
        case L'&': out += _T("&amp;"); break;
        case L'<': out += _T("&lt;"); break;
        case L'>': out += _T("&gt;"); break;
        case L'"':
            if (attrib) {
                out += _T("&quot;");
                break;
            }
            out.AppendChar(c);
            break;
        default: out.AppendChar(c); break;
        }
    }
}

static void Write(CString& out, const CMarkup::Node& node)
{
    out += _T("<");
    out += node.name;
    for (const auto& [name, value] : node.attribs) {
        out += _T(" ");
        out += name;
        out += _T("=\"");
        Escape(out, value, true);
        out += _T("\"");
    }
    if (node.children.empty() && node.data.IsEmpty()) {
        out += _T("/>\r\n");
        return;
    }
    out += _T(">");
    if (!node.children.empty()) {
        out += _T("\r\n");
        for (const auto& child : node.children) {
            Write(out, *child);
        }
    }
    else {
        Escape(out, node.data, false);
    }
    out += _T("</");
    out += node.name;
    out += _T(">\r\n");
}

CString CMarkup::GetDoc() const
{
    CString out;
    for (const auto& node : m_root->children) {
        Write(out, *node);
    }
    return out;
}

CMarkup::Node* CMarkup::Main() const
{
    if (m_main < 0 || m_main >= static_cast<int>(m_parent->children.size())) {
        return nullptr;
    }
    return m_parent->children[m_main].get();
}

CMarkup::Node* CMarkup::Child() const
{
    Node* main = Main();
    if (!main || m_child < 0 || m_child >= static_cast<int>(main->children.size())) {
        return nullptr;
    }
    return main->children[m_child].get();
}

int CMarkup::Find(Node* parent, int from, LPCTSTR szName) const
{
    for (int i = from; i < static_cast<int>(parent->children.size()); i++) {
        if (!szName || !*szName || parent->children[i]->name == szName) {
            return i;
        }
    }
    return -1;
}

BOOL CMarkup::FindElem(LPCTSTR szName)
{
    int i = Find(m_parent, m_main + 1, szName);
    if (i == -1) {
        return FALSE;
    }
    m_main = i;
    m_child = -1;
    return TRUE;
}

BOOL CMarkup::FindChildElem(LPCTSTR szName)
{
    // sem posição principal, o CMarkup usa o primeiro elemento
    if (!Main() && !FindElem()) {
        return FALSE;
    }
    int i = Find(Main(), m_child + 1, szName);
    if (i == -1) {
        return FALSE;
    }
    m_child = i;
    return TRUE;
}

BOOL CMarkup::IntoElem()
{
    // como no CMarkup: basta haver posição principal; a posição filha (talvez nenhuma)
    // vira a nova posição principal
    Node* main = Main();
    if (!main) {
        return FALSE;
    }
    m_parent = main;
    m_main = m_child;
    m_child = -1;
    return TRUE;
}

BOOL CMarkup::OutOfElem()
{
    Node* grand = m_parent->parent;
    if (!grand) {
        return FALSE;
    }
    int index = -1;
    for (int i = 0; i < static_cast<int>(grand->children.size()); i++) {
        if (grand->children[i].get() == m_parent) {
            index = i;
            break;
        }
    }
    m_child = m_main;
    m_main = index;
    m_parent = grand;
    return TRUE;
}

void CMarkup::ResetPos()
{
    m_parent = m_root.get();
    m_main = -1;
    m_child = -1;
}

void CMarkup::ResetMainPos()
{
    m_main = -1;
    m_child = -1;
}

CString CMarkup::GetTagName() const
{
    Node* main = Main();
    return main ? main->name : CString();
}

CString CMarkup::GetChildTagName() const
{
    Node* child = Child();
    return child ? child->name : CString();
}

CString CMarkup::GetAttrib(LPCTSTR szAttrib) const
{
    if (Node* main = Main()) {
        for (const auto& [name, value] : main->attribs) {
            if (name == szAttrib) {
                return value;
            }
        }
    }
    return CString();
}

BOOL CMarkup::FindAttrib(LPCTSTR szAttrib) const
{
    if (Node* main = Main()) {
        for (const auto& a : main->attribs) {
            if (a.first == szAttrib) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

CString CMarkup::GetData() const
{
    Node* main = Main();
    return main && main->children.empty() ? main->data : CString();
}

CString CMarkup::GetChildData() const
{
    Node* child = Child();
    return child && child->children.empty() ? child->data : CString();
}

BOOL CMarkup::AddElem(LPCTSTR szName, LPCTSTR szData)
{
    auto node = std::make_unique<Node>();
    node->name = szName;
    node->data = szData ? szData : _T("");
    node->parent = m_parent;
    // como no CMarkup: depois da posição principal (ou no fim, sem posição)
    int at = m_main >= 0 ? m_main + 1 : static_cast<int>(m_parent->children.size());
    m_parent->children.insert(m_parent->children.begin() + at, std::move(node));
    m_main = at;
    m_child = -1;
    return TRUE;
}

BOOL CMarkup::AddChildElem(LPCTSTR szName, LPCTSTR szData)
{
    Node* main = Main();
    if (!main) {
        return FALSE;
    }
    auto node = std::make_unique<Node>();
    node->name = szName;
    node->data = szData ? szData : _T("");
    node->parent = main;
    int at = m_child >= 0 ? m_child + 1 : static_cast<int>(main->children.size());
    main->children.insert(main->children.begin() + at, std::move(node));
    m_child = at;
    return TRUE;
}

static BOOL SetAttrib(CMarkup::Node* node, LPCTSTR szAttrib, LPCTSTR szValue)
{
    if (!node) {
        return FALSE;
    }
    for (auto& a : node->attribs) {
        if (a.first == szAttrib) {
            a.second = szValue ? szValue : _T("");
            return TRUE;
        }
    }
    node->attribs.emplace_back(szAttrib, szValue ? szValue : _T(""));
    return TRUE;
}

BOOL CMarkup::AddAttrib(LPCTSTR szAttrib, LPCTSTR szValue)
{
    return SetAttrib(Main(), szAttrib, szValue);
}

BOOL CMarkup::AddChildAttrib(LPCTSTR szAttrib, LPCTSTR szValue)
{
    return SetAttrib(Child(), szAttrib, szValue);
}
