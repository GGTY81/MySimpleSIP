/*
 * MySimpleSIP — monitores do sistema (rede, energia, ociosidade) para a janela principal.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// MySimpleSIP: equivalentes Linux das notificações do Windows usadas pelo MicroSIP:
//   NetworkMonitor — NotifyUnicastIpAddressChange/NotifyRouteChange2 (netlink do kernel:
//                    endereços, interfaces e rotas) → sinal changed();
//   PowerMonitor   — WM_POWERBROADCAST (logind PrepareForSleep, com trava "delay" para
//                    dar tempo de encerrar o pjsua antes de suspender) e
//                    SetThreadExecutionState durante chamadas (inibe ociosidade/suspensão
//                    pelo gnome-session, ou pelo logind sem GNOME);
//                    e WM_WTSSESSION_CHANGE (sessão reativada: propriedade Active do logind);
//   SessionIdleTimeMs — GetLastInputInfo (org.gnome.Mutter.IdleMonitor).

#pragma once

#include <QDBusUnixFileDescriptor>
#include <QObject>

class QSocketNotifier;

class NetworkMonitor : public QObject {
    Q_OBJECT
public:
    explicit NetworkMonitor(QObject* parent = nullptr);
    ~NetworkMonitor() override;
    bool isActive() const { return m_fd >= 0; }

signals:
    void changed();

private:
    void onReadable();
    int m_fd = -1;
    QSocketNotifier* m_notifier = nullptr;
};

class PowerMonitor : public QObject {
    Q_OBJECT
public:
    explicit PowerMonitor(QObject* parent = nullptr);
    ~PowerMonitor() override;
    bool isActive() const { return m_connected; }
    // ES_SYSTEM_REQUIRED (| ES_DISPLAY_REQUIRED) enquanto houver chamadas
    void setCallActive(bool active);
    bool callInhibited() const { return m_gnomeCookie || m_callLock.isValid(); }

signals:
    void aboutToSleep(); // PBT_APMSUSPEND: a trava "delay" é liberada logo depois
    void resumed();      // PBT_APMRESUMEAUTOMATIC
    void sessionActivated(); // WM_WTSSESSION_CHANGE (WTS_CONSOLE_CONNECT)

public slots:
    void onPrepareForSleep(bool start);
    void onSessionPropertiesChanged(const QString& iface, const QVariantMap& changed, const QStringList& invalidated);

private:
    void takeSleepDelay();
    bool m_connected = false;
    QDBusUnixFileDescriptor m_sleepDelay;
    QDBusUnixFileDescriptor m_callLock; // logind, sem gnome-session
    uint m_gnomeCookie = 0;
    QString m_sessionPath;
};

// Tempo sem entrada do usuário na sessão, em ms; -1 se não houver como saber.
qint64 SessionIdleTimeMs();
