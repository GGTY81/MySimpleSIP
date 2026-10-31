#include "Collections.h"

#include <gtest/gtest.h>

TEST(CArray, OperacoesBasicas)
{
    CArray<int, int> a;
    EXPECT_TRUE(a.IsEmpty());
    EXPECT_EQ(a.Add(10), 0);
    EXPECT_EQ(a.Add(30), 1);
    a.InsertAt(1, 20);
    EXPECT_EQ(a.GetCount(), 3);
    EXPECT_EQ(a[1], 20);
    EXPECT_EQ(a.GetAt(2), 30);
    a.SetAt(0, 5);
    a.ElementAt(2) = 35;
    EXPECT_EQ(a[0], 5);
    EXPECT_EQ(a[2], 35);
    a.RemoveAt(0);
    EXPECT_EQ(a.GetSize(), 2);
    EXPECT_EQ(a.GetUpperBound(), 1);
    a.SetAtGrow(4, 99);
    EXPECT_EQ(a.GetSize(), 5);
    EXPECT_EQ(a[3], 0);
    a.RemoveAll();
    EXPECT_TRUE(a.IsEmpty());
    EXPECT_THROW(a.GetAt(0), std::out_of_range);
}

TEST(CStringArray, ComoNoCSV)
{
    CStringArray arr;
    arr.Add(_T("Nome"));
    arr.Add(_T("Número"));
    CStringArray outro;
    outro.Copy(arr);
    outro.Add(_T("E-mail"));
    EXPECT_EQ(arr.GetCount(), 2);
    EXPECT_EQ(outro.GetCount(), 3);
    EXPECT_EQ(outro[1], _T("Número"));
}

TEST(CList, PercursoComPosition)
{
    CStringList l;
    l.AddTail(_T("b"));
    l.AddTail(_T("c"));
    l.AddHead(_T("a"));
    EXPECT_EQ(l.GetCount(), 3);
    EXPECT_EQ(l.GetHead(), _T("a"));
    EXPECT_EQ(l.GetTail(), _T("c"));

    CString junto;
    POSITION pos = l.GetHeadPosition();
    while (pos)
        junto += l.GetNext(pos);
    EXPECT_EQ(junto, _T("abc"));

    CString reverso;
    pos = l.GetTailPosition();
    while (pos)
        reverso += l.GetPrev(pos);
    EXPECT_EQ(reverso, _T("cba"));
}

TEST(CList, RemocaoDuranteIteracao)
{
    // Padrão do MicroSIP: guarda a posição atual antes de GetNext e remove por ela
    CList<int> l;
    for (int i = 1; i <= 6; ++i)
        l.AddTail(i);
    POSITION pos = l.GetHeadPosition();
    while (pos) {
        POSITION atual = pos;
        int v = l.GetNext(pos);
        if (v % 2 == 0)
            l.RemoveAt(atual);
    }
    EXPECT_EQ(l.GetCount(), 3);
    EXPECT_EQ(l.GetHead(), 1);
    EXPECT_EQ(l.GetTail(), 5);
}

TEST(CList, FindInsertRemove)
{
    CList<int, int> l;
    l.AddTail(1);
    l.AddTail(2);
    l.AddTail(1);
    POSITION p1 = l.Find(1);
    ASSERT_NE(p1, nullptr);
    POSITION p2 = l.Find(1, p1);
    ASSERT_NE(p2, nullptr);
    EXPECT_NE(p1, p2);
    EXPECT_EQ(l.Find(7), nullptr);
    EXPECT_EQ(l.FindIndex(1), l.Find(2));
    EXPECT_EQ(l.FindIndex(3), nullptr);

    l.InsertAfter(l.Find(2), 3);
    l.InsertBefore(l.GetHeadPosition(), 0);
    CString s;
    POSITION pos = l.GetHeadPosition();
    while (pos)
        s.AppendFormat(_T("%d"), l.GetNext(pos));
    EXPECT_EQ(s, _T("01231"));

    EXPECT_EQ(l.RemoveHead(), 0);
    EXPECT_EQ(l.RemoveTail(), 1);
    EXPECT_EQ(l.GetCount(), 3);

    CList<int, int> copia(l);
    l.RemoveAll();
    EXPECT_TRUE(l.IsEmpty());
    EXPECT_EQ(copia.GetCount(), 3);
    EXPECT_THROW(l.GetHead(), std::out_of_range);
}

TEST(CList, CStringListFindComLiteral)
{
    CStringList proxies;
    proxies.AddTail(_T("sip:proxy1"));
    proxies.AddTail(_T("sip:proxy2"));
    EXPECT_NE(proxies.Find(_T("sip:proxy2")), nullptr);
    EXPECT_EQ(proxies.Find(_T("sip:proxy3")), nullptr);
}
