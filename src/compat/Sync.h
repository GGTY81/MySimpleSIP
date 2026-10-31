/*
 * MySimpleSIP — camada de compatibilidade: CCriticalSection/CSingleLock (afxmt.h).
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "WinTypes.h"

#include <mutex>

// Seção crítica do Windows é reentrante na mesma thread: recursive_mutex.
class CCriticalSection {
public:
    BOOL Lock()
    {
        m_mutex.lock();
        return TRUE;
    }
    BOOL Unlock()
    {
        m_mutex.unlock();
        return TRUE;
    }

private:
    std::recursive_mutex m_mutex;
};

class CSingleLock {
public:
    explicit CSingleLock(CCriticalSection* cs, BOOL initialLock = FALSE) : m_cs(cs)
    {
        if (initialLock)
            Lock();
    }
    ~CSingleLock()
    {
        if (m_locked)
            Unlock();
    }
    CSingleLock(const CSingleLock&) = delete;
    CSingleLock& operator=(const CSingleLock&) = delete;

    BOOL Lock()
    {
        if (!m_locked && m_cs) {
            m_cs->Lock();
            m_locked = true;
        }
        return TRUE;
    }
    BOOL Unlock()
    {
        if (m_locked && m_cs) {
            m_cs->Unlock();
            m_locked = false;
        }
        return TRUE;
    }
    BOOL IsLocked() const { return m_locked; }

private:
    CCriticalSection* m_cs;
    bool m_locked = false;
};
