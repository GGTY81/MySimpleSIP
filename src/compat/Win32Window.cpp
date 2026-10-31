/*
 * MySimpleSIP — camada de compatibilidade: mensagens, timers e janelas do Win32/MFC.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "Win32Window.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace {

std::atomic<IMessageBackend*> g_backend{nullptr};
CWinApp g_app;

} // namespace

void SetMessageBackend(IMessageBackend* backend)
{
    g_backend = backend;
}

IMessageBackend* GetMessageBackend()
{
    return g_backend;
}

BOOL PostMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    IMessageBackend* b = g_backend;
    return b ? b->Post(hWnd, msg, wParam, lParam) : FALSE;
}

LRESULT SendMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    IMessageBackend* b = g_backend;
    return b ? b->Send(hWnd, msg, wParam, lParam) : 0;
}

UINT_PTR SetTimer(HWND hWnd, UINT_PTR id, UINT elapseMs, TIMERPROC proc)
{
    IMessageBackend* b = g_backend;
    return b ? b->SetTimer(hWnd, id, elapseMs, proc) : 0;
}

BOOL KillTimer(HWND hWnd, UINT_PTR id)
{
    IMessageBackend* b = g_backend;
    return b ? b->KillTimer(hWnd, id) : FALSE;
}

void Sleep(DWORD ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

DWORD GetTickCount()
{
    using namespace std::chrono;
    return static_cast<DWORD>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

CWinApp* AfxGetApp()
{
    return &g_app;
}
