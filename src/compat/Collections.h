/*
 * MySimpleSIP — camada de compatibilidade: coleções do MFC (CArray, CList...).
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// CArray sobre std::vector; CList como lista duplamente ligada própria, porque o
// código do MicroSIP percorre listas com POSITION (ponteiro opaco para o nó) e
// remove/insere durante a iteração.

#pragma once

#include "CString.h"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

struct __POSITION {};
using POSITION = __POSITION*;

using INT_PTR = std::ptrdiff_t;

template <typename TYPE, typename ARG_TYPE = const TYPE&>
class CArray {
public:
    INT_PTR GetSize() const { return static_cast<INT_PTR>(m_data.size()); }
    INT_PTR GetCount() const { return GetSize(); }
    INT_PTR GetUpperBound() const { return GetSize() - 1; }
    bool IsEmpty() const { return m_data.empty(); }
    void SetSize(INT_PTR newSize) { m_data.resize(static_cast<std::size_t>(newSize)); }
    void RemoveAll() { m_data.clear(); }

    const TYPE& GetAt(INT_PTR i) const { return m_data.at(static_cast<std::size_t>(i)); }
    TYPE& GetAt(INT_PTR i) { return m_data.at(static_cast<std::size_t>(i)); }
    void SetAt(INT_PTR i, ARG_TYPE v) { m_data.at(static_cast<std::size_t>(i)) = v; }
    TYPE& ElementAt(INT_PTR i) { return GetAt(i); }
    const TYPE& operator[](INT_PTR i) const { return GetAt(i); }
    TYPE& operator[](INT_PTR i) { return GetAt(i); }
    TYPE* GetData() { return m_data.data(); }
    const TYPE* GetData() const { return m_data.data(); }

    INT_PTR Add(ARG_TYPE v)
    {
        m_data.push_back(v);
        return GetSize() - 1;
    }
    void SetAtGrow(INT_PTR i, ARG_TYPE v)
    {
        if (i >= GetSize())
            SetSize(i + 1);
        SetAt(i, v);
    }
    void InsertAt(INT_PTR i, ARG_TYPE v, INT_PTR count = 1)
    {
        if (i > GetSize())
            SetSize(i);
        m_data.insert(m_data.begin() + i, static_cast<std::size_t>(count), v);
    }
    void RemoveAt(INT_PTR i, INT_PTR count = 1)
    {
        m_data.erase(m_data.begin() + i, m_data.begin() + i + count);
    }
    INT_PTR Append(const CArray& other)
    {
        INT_PTR old = GetSize();
        m_data.insert(m_data.end(), other.m_data.begin(), other.m_data.end());
        return old;
    }
    void Copy(const CArray& other) { m_data = other.m_data; }

    // Iteração estilo C++ (para código novo)
    auto begin() { return m_data.begin(); }
    auto end() { return m_data.end(); }
    auto begin() const { return m_data.begin(); }
    auto end() const { return m_data.end(); }

private:
    std::vector<TYPE> m_data;
};

template <typename TYPE, typename ARG_TYPE = const TYPE&>
class CList {
    struct Node : __POSITION {
        Node* prev = nullptr;
        Node* next = nullptr;
        TYPE data;
        explicit Node(ARG_TYPE v) : data(v) {}
    };

public:
    CList() = default;
    CList(const CList& other) { copyFrom(other); }
    CList& operator=(const CList& other)
    {
        if (this != &other) {
            RemoveAll();
            copyFrom(other);
        }
        return *this;
    }
    ~CList() { RemoveAll(); }

    INT_PTR GetCount() const { return m_count; }
    INT_PTR GetSize() const { return m_count; }
    bool IsEmpty() const { return m_count == 0; }

    TYPE& GetHead() { return check(m_head)->data; }
    const TYPE& GetHead() const { return check(m_head)->data; }
    TYPE& GetTail() { return check(m_tail)->data; }
    const TYPE& GetTail() const { return check(m_tail)->data; }

    POSITION AddHead(ARG_TYPE v) { return InsertBefore(m_head, v); }
    POSITION AddTail(ARG_TYPE v) { return InsertAfter(m_tail, v); }
    void AddTail(const CList& other)
    {
        for (Node* n = other.m_head; n; n = n->next)
            AddTail(n->data);
    }

    TYPE RemoveHead()
    {
        TYPE v = check(m_head)->data;
        RemoveAt(m_head);
        return v;
    }
    TYPE RemoveTail()
    {
        TYPE v = check(m_tail)->data;
        RemoveAt(m_tail);
        return v;
    }
    void RemoveAll()
    {
        Node* n = m_head;
        while (n) {
            Node* next = n->next;
            delete n;
            n = next;
        }
        m_head = m_tail = nullptr;
        m_count = 0;
    }

    POSITION GetHeadPosition() const { return m_head; }
    POSITION GetTailPosition() const { return m_tail; }
    TYPE& GetNext(POSITION& pos)
    {
        Node* n = node(pos);
        pos = n->next;
        return n->data;
    }
    const TYPE& GetNext(POSITION& pos) const
    {
        Node* n = node(pos);
        pos = n->next;
        return n->data;
    }
    TYPE& GetPrev(POSITION& pos)
    {
        Node* n = node(pos);
        pos = n->prev;
        return n->data;
    }
    TYPE& GetAt(POSITION pos) { return node(pos)->data; }
    const TYPE& GetAt(POSITION pos) const { return node(pos)->data; }
    void SetAt(POSITION pos, ARG_TYPE v) { node(pos)->data = v; }

    void RemoveAt(POSITION pos)
    {
        Node* n = node(pos);
        (n->prev ? n->prev->next : m_head) = n->next;
        (n->next ? n->next->prev : m_tail) = n->prev;
        delete n;
        --m_count;
    }

    POSITION InsertBefore(POSITION pos, ARG_TYPE v)
    {
        Node* at = static_cast<Node*>(pos);
        Node* n = new Node(v);
        if (!at) { // lista vazia ou pos nulo: insere no início
            n->next = m_head;
            if (m_head)
                m_head->prev = n;
            m_head = n;
            if (!m_tail)
                m_tail = n;
        } else {
            n->prev = at->prev;
            n->next = at;
            (at->prev ? at->prev->next : m_head) = n;
            at->prev = n;
        }
        ++m_count;
        return n;
    }
    POSITION InsertAfter(POSITION pos, ARG_TYPE v)
    {
        Node* at = static_cast<Node*>(pos);
        Node* n = new Node(v);
        if (!at) { // lista vazia ou pos nulo: insere no fim
            n->prev = m_tail;
            if (m_tail)
                m_tail->next = n;
            m_tail = n;
            if (!m_head)
                m_head = n;
        } else {
            n->next = at->next;
            n->prev = at;
            (at->next ? at->next->prev : m_tail) = n;
            at->next = n;
        }
        ++m_count;
        return n;
    }

    // Procura a partir do elemento seguinte a startAfter (ou do início).
    POSITION Find(ARG_TYPE v, POSITION startAfter = nullptr) const
    {
        Node* n = startAfter ? static_cast<Node*>(startAfter)->next : m_head;
        for (; n; n = n->next)
            if (n->data == v)
                return n;
        return nullptr;
    }
    POSITION FindIndex(INT_PTR index) const
    {
        if (index < 0 || index >= m_count)
            return nullptr;
        Node* n = m_head;
        while (index-- > 0)
            n = n->next;
        return n;
    }

private:
    static Node* node(POSITION pos)
    {
        if (!pos)
            throw std::out_of_range("CList: POSITION nula");
        return static_cast<Node*>(pos);
    }
    static Node* check(Node* n)
    {
        if (!n)
            throw std::out_of_range("CList: lista vazia");
        return n;
    }
    void copyFrom(const CList& other)
    {
        for (Node* n = other.m_head; n; n = n->next)
            AddTail(n->data);
    }

    Node* m_head = nullptr;
    Node* m_tail = nullptr;
    INT_PTR m_count = 0;
};

using CStringArray = CArray<CString>;
using CStringList = CList<CString>;
using CByteArray = CArray<BYTE, BYTE>;
