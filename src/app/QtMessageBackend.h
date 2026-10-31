/*
 * MySimpleSIP — IMessageBackend sobre o laço de eventos do Qt.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Faz o PostMessage/SendMessage/SetTimer emulados (compat/Win32Window.h) funcionarem
// como no Windows: as mensagens e os timers são entregues na thread principal do Qt
// (a "thread da UI"), mesmo quando postados das threads do pjsua. Cada janela que
// recebe mensagens implementa IMessageTarget e é registrada aqui, recebendo um HWND.
// Usado pela CLI de teste e pela janela Qt.

#pragma once

#include "Win32Window.h"

#include <QObject>

#include <map>
#include <mutex>

class QTimer;

class IMessageTarget {
public:
    virtual ~IMessageTarget() = default;
    // Equivalente ao WindowProc: recebe as mensagens UM_* e WM_TIMER (wParam = id).
    virtual LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
};

class QtMessageBackend : public QObject, public IMessageBackend {
public:
    // Deve ser criado na thread principal, depois da QCoreApplication.
    QtMessageBackend();
    ~QtMessageBackend() override;

    // Registra um destino e devolve o HWND que o representa.
    HWND Register(IMessageTarget* target);
    void Unregister(HWND hWnd);

    BOOL Post(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
    LRESULT Send(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
    UINT_PTR SetTimer(HWND hWnd, UINT_PTR id, UINT elapseMs, TIMERPROC proc) override;
    BOOL KillTimer(HWND hWnd, UINT_PTR id) override;

private:
    LRESULT Dispatch(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void StartTimer(HWND hWnd, UINT_PTR id, UINT elapseMs, TIMERPROC proc);
    void StopTimer(HWND hWnd, UINT_PTR id);

    std::mutex m_mutex;
    std::map<HWND, IMessageTarget*> m_targets;
    std::map<std::pair<HWND, UINT_PTR>, QTimer*> m_timers; // só na thread principal
    std::uintptr_t m_nextHwnd = 0x1000;
    UINT_PTR m_nextTimerId = 0x8000;
};
