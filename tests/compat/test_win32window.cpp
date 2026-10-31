// Testes da emulação de PostMessage/SetTimer/CWnd/CCriticalSection.

#include "afx.h"

#include <gtest/gtest.h>

#include <thread>
#include <vector>

namespace {

struct Posted {
    HWND hWnd;
    UINT msg;
    WPARAM w;
    LPARAM l;
};

class FakeBackend : public IMessageBackend {
public:
    BOOL Post(HWND h, UINT m, WPARAM w, LPARAM l) override
    {
        posted.push_back({h, m, w, l});
        return TRUE;
    }
    LRESULT Send(HWND, UINT m, WPARAM w, LPARAM) override { return m + static_cast<LRESULT>(w); }
    UINT_PTR SetTimer(HWND, UINT_PTR id, UINT ms, TIMERPROC proc) override
    {
        lastMs = ms;
        lastProc = proc;
        return id ? id : ++nextId;
    }
    BOOL KillTimer(HWND, UINT_PTR id) override
    {
        killed.push_back(id);
        return TRUE;
    }
    std::vector<Posted> posted;
    std::vector<UINT_PTR> killed;
    UINT lastMs = 0;
    TIMERPROC lastProc = nullptr;
    UINT_PTR nextId = 100;
};

int g_timerCalls = 0;
void timerProc(HWND, UINT, UINT_PTR, DWORD) { ++g_timerCalls; }

} // namespace

TEST(Win32Window, SemBackendNadaAcontece)
{
    SetMessageBackend(nullptr);
    EXPECT_FALSE(PostMessage(nullptr, WM_USER + 1, 0, 0));
    EXPECT_EQ(SetTimer(nullptr, 0, 100, timerProc), 0u);
    CWnd* main = AfxGetApp()->m_pMainWnd;
    EXPECT_EQ(main, nullptr);
}

TEST(Win32Window, BackendRecebeMensagensETimers)
{
    FakeBackend backend;
    SetMessageBackend(&backend);
    HWND h = reinterpret_cast<HWND>(0x1234);
    CWnd wnd(h);
    AfxGetApp()->m_pMainWnd = &wnd;

    // padrão do global.cpp: AfxGetApp()->m_pMainWnd->PostMessage(UM_..., ...)
    EXPECT_TRUE(AfxGetApp()->m_pMainWnd->PostMessage(WM_USER + 7, 3, 4));
    ASSERT_EQ(backend.posted.size(), 1u);
    EXPECT_EQ(backend.posted[0].hWnd, h);
    EXPECT_EQ(backend.posted[0].msg, WM_USER + 7u);
    EXPECT_EQ(backend.posted[0].w, 3u);
    EXPECT_EQ(wnd.SendMessage(10, 5), 15);

    EXPECT_EQ(SetTimer(nullptr, 0, 800, timerProc), 101u);
    EXPECT_EQ(backend.lastMs, 800u);
    backend.lastProc(nullptr, WM_TIMER, 101, 0);
    EXPECT_EQ(g_timerCalls, 1);
    EXPECT_EQ(wnd.SetTimer(42, 1000, nullptr), 42u);
    EXPECT_TRUE(KillTimer(nullptr, 101));
    EXPECT_EQ(backend.killed, (std::vector<UINT_PTR>{101}));

    AfxGetApp()->m_pMainWnd = nullptr;
    SetMessageBackend(nullptr);
}

TEST(Sync, CriticalSectionReentranteEEntreThreads)
{
    CCriticalSection cs;
    cs.Lock();
    cs.Lock(); // reentrante, como no Windows
    cs.Unlock();
    cs.Unlock();

    int counter = 0;
    auto work = [&] {
        for (int i = 0; i < 10000; ++i) {
            CSingleLock lock(&cs, TRUE);
            ++counter;
        }
    };
    std::thread t1(work), t2(work);
    t1.join();
    t2.join();
    EXPECT_EQ(counter, 20000);
}

TEST(WinTypes, RGBEComponentes)
{
    const COLORREF c = RGB(76, 217, 100);
    EXPECT_EQ(c, 0x0064D94Cu);
    EXPECT_EQ(GetRValue(c), 76);
    EXPECT_EQ(GetGValue(c), 217);
    EXPECT_EQ(GetBValue(c), 100);
}
