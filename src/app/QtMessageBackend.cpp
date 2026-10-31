/*
 * MySimpleSIP — IMessageBackend sobre o laço de eventos do Qt.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "QtMessageBackend.h"

#include <QCoreApplication>
#include <QThread>
#include <QTimer>

QtMessageBackend::QtMessageBackend()
{
    SetMessageBackend(this);
}

QtMessageBackend::~QtMessageBackend()
{
    SetMessageBackend(nullptr);
    for (auto& [key, timer] : m_timers)
        delete timer;
}

HWND QtMessageBackend::Register(IMessageTarget* target)
{
    std::lock_guard lock(m_mutex);
    HWND h = reinterpret_cast<HWND>(m_nextHwnd);
    m_nextHwnd += 0x10;
    m_targets[h] = target;
    return h;
}

void QtMessageBackend::Unregister(HWND hWnd)
{
    std::lock_guard lock(m_mutex);
    m_targets.erase(hWnd);
}

LRESULT QtMessageBackend::Dispatch(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    IMessageTarget* target = nullptr;
    {
        std::lock_guard lock(m_mutex);
        auto it = m_targets.find(hWnd);
        if (it != m_targets.end())
            target = it->second;
    }
    return target ? target->WindowProc(msg, wParam, lParam) : 0;
}

BOOL QtMessageBackend::Post(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    {
        std::lock_guard lock(m_mutex);
        if (!m_targets.count(hWnd))
            return FALSE;
    }
    QMetaObject::invokeMethod(this, [=, this] { Dispatch(hWnd, msg, wParam, lParam); }, Qt::QueuedConnection);
    return TRUE;
}

LRESULT QtMessageBackend::Send(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (QThread::currentThread() == thread())
        return Dispatch(hWnd, msg, wParam, lParam);
    LRESULT result = 0;
    QMetaObject::invokeMethod(this, [&, this] { result = Dispatch(hWnd, msg, wParam, lParam); },
                              Qt::BlockingQueuedConnection);
    return result;
}

UINT_PTR QtMessageBackend::SetTimer(HWND hWnd, UINT_PTR id, UINT elapseMs, TIMERPROC proc)
{
    if (!hWnd && !id) {
        std::lock_guard lock(m_mutex);
        id = ++m_nextTimerId;
    }
    if (QThread::currentThread() == thread())
        StartTimer(hWnd, id, elapseMs, proc);
    else
        QMetaObject::invokeMethod(this, [=, this] { StartTimer(hWnd, id, elapseMs, proc); }, Qt::QueuedConnection);
    return id;
}

BOOL QtMessageBackend::KillTimer(HWND hWnd, UINT_PTR id)
{
    if (QThread::currentThread() == thread())
        StopTimer(hWnd, id);
    else
        QMetaObject::invokeMethod(this, [=, this] { StopTimer(hWnd, id); }, Qt::QueuedConnection);
    return TRUE;
}

void QtMessageBackend::StartTimer(HWND hWnd, UINT_PTR id, UINT elapseMs, TIMERPROC proc)
{
    QTimer*& timer = m_timers[{hWnd, id}];
    if (!timer) {
        timer = new QTimer(this);
    }
    timer->disconnect();
    QObject::connect(timer, &QTimer::timeout, this, [=, this] {
        if (proc)
            proc(hWnd, WM_TIMER, id, GetTickCount());
        else
            Dispatch(hWnd, WM_TIMER, id, 0);
    });
    timer->start(static_cast<int>(elapseMs));
}

void QtMessageBackend::StopTimer(HWND hWnd, UINT_PTR id)
{
    auto it = m_timers.find({hWnd, id});
    if (it != m_timers.end()) {
        it->second->stop();
        it->second->deleteLater();
        m_timers.erase(it);
    }
}
