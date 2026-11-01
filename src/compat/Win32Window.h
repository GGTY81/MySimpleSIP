/*
 * MySimpleSIP — camada de compatibilidade: mensagens, timers e janelas do Win32/MFC.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// O núcleo portado do MicroSIP conversa com a interface como no Windows:
// PostMessage(janela, UM_..., wParam, lParam), SetTimer/KillTimer com TIMERPROC e
// AfxGetApp()->m_pMainWnd->PostMessage(...). Aqui esses pontos viram chamadas a um
// IMessageBackend instalado pela aplicação (na UI Qt: fila de eventos da thread
// principal e QTimer; nos testes: fila manual). Sem backend, as chamadas não fazem
// nada e retornam FALSE/0 — o núcleo continua utilizável sem interface.

#pragma once

#include "WinTypes.h"

#include <cstdint>

using HWND = struct HWND__*;
using WPARAM = std::uintptr_t;
using LPARAM = std::intptr_t;
using LRESULT = std::intptr_t;
using UINT_PTR = std::uintptr_t;
using DWORD_PTR = std::uintptr_t;
using TIMERPROC = void (*)(HWND, UINT, UINT_PTR, DWORD);

#ifndef WM_USER
#   define WM_USER 0x0400
#endif
#ifndef WM_LBUTTONUP
#   define WM_LBUTTONUP 0x0202
#   define WM_RBUTTONUP 0x0205
#endif
#ifndef WM_TIMER
#   define WM_TIMER 0x0113
#endif
#ifndef WM_DEVICECHANGE
#   define WM_DEVICECHANGE 0x0219
#   define DBT_DEVNODES_CHANGED 0x0007
#endif
#ifndef PBT_APMSUSPEND
#   define PBT_APMSUSPEND 0x0004
#   define PBT_APMRESUMEAUTOMATIC 0x0012
#endif

// Ícones de balão da bandeja (Shell_NotifyIcon), usados como "flags" de BaloonPopup
#ifndef NIIF_NONE
#   define NIIF_NONE 0x0
#   define NIIF_INFO 0x1
#   define NIIF_WARNING 0x2
#   define NIIF_ERROR 0x3
#endif

class IMessageBackend {
public:
    virtual ~IMessageBackend() = default;
    // Enfileira para a thread da UI (pode ser chamado de qualquer thread).
    virtual BOOL Post(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
    // Executa na thread da UI e espera o resultado.
    virtual LRESULT Send(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
    // Timer na thread da UI. Semântica do Win32: hWnd nulo e id 0 → id novo;
    // mesmo (hWnd, id) → reinicia. proc nulo → mensagem WM_TIMER para hWnd.
    virtual UINT_PTR SetTimer(HWND hWnd, UINT_PTR id, UINT elapseMs, TIMERPROC proc) = 0;
    virtual BOOL KillTimer(HWND hWnd, UINT_PTR id) = 0;
};

void SetMessageBackend(IMessageBackend* backend);
IMessageBackend* GetMessageBackend();

BOOL PostMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT SendMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
UINT_PTR SetTimer(HWND hWnd, UINT_PTR id, UINT elapseMs, TIMERPROC proc);
BOOL KillTimer(HWND hWnd, UINT_PTR id);
// IsWindow: no port, uma janela existe enquanto tem identificador.
inline BOOL IsWindow(HWND hWnd) { return hWnd != nullptr; }
void Sleep(DWORD ms);
DWORD GetTickCount();

// Representação mínima de uma janela MFC: só o identificador e o envio de mensagens.
class CWnd {
public:
    CWnd() = default;
    explicit CWnd(HWND h) : m_hWnd(h) {}
    virtual ~CWnd() = default;

    HWND GetSafeHwnd() const { return m_hWnd; }
    BOOL PostMessage(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const
    {
        return ::PostMessage(m_hWnd, msg, wParam, lParam);
    }
    LRESULT SendMessage(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const
    {
        return ::SendMessage(m_hWnd, msg, wParam, lParam);
    }
    UINT_PTR SetTimer(UINT_PTR id, UINT elapseMs, TIMERPROC proc) const
    {
        return ::SetTimer(m_hWnd, id, elapseMs, proc);
    }
    BOOL KillTimer(UINT_PTR id) const { return ::KillTimer(m_hWnd, id); }

    HWND m_hWnd = nullptr;
};

// AfxGetApp()->m_pMainWnd: a UI registra aqui a janela principal.
struct CWinApp {
    CWnd* m_pMainWnd = nullptr;
};
CWinApp* AfxGetApp();

struct CPoint {
    LONG x = 0;
    LONG y = 0;
};

struct CSize {
    LONG cx = 0;
    LONG cy = 0;
};

struct CRect {
    LONG left = 0;
    LONG top = 0;
    LONG right = 0;
    LONG bottom = 0;
    LONG Width() const { return right - left; }
    LONG Height() const { return bottom - top; }
};
