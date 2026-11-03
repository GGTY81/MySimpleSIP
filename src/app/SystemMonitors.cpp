/*
 * MySimpleSIP — monitores do sistema (rede, energia, ociosidade) para a janela principal.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "SystemMonitors.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QSocketNotifier>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <unistd.h>

static const char* kLogindService = "org.freedesktop.login1";
static const char* kLogindPath = "/org/freedesktop/login1";
static const char* kLogindManager = "org.freedesktop.login1.Manager";

// chamada D-Bus direta (sem a introspecção do QDBusInterface), com tempo-limite curto
static QDBusMessage Call(QDBusConnection bus, const QString& service, const QString& path, const QString& iface,
                         const QString& method, const QVariantList& args = {})
{
    QDBusMessage m = QDBusMessage::createMethodCall(service, path, iface, method);
    m.setArguments(args);
    return bus.call(m, QDBus::Block, 2000);
}

static QDBusMessage Logind(const QString& method, const QVariantList& args)
{
    return Call(QDBusConnection::systemBus(), QLatin1String(kLogindService), QLatin1String(kLogindPath),
                QLatin1String(kLogindManager), method, args);
}

static QDBusMessage GnomeSession(const QString& method, const QVariantList& args)
{
    return Call(QDBusConnection::sessionBus(), QStringLiteral("org.gnome.SessionManager"),
                QStringLiteral("/org/gnome/SessionManager"), QStringLiteral("org.gnome.SessionManager"), method, args);
}

// ---------------------------------------------------------------- rede

NetworkMonitor::NetworkMonitor(QObject* parent) : QObject(parent)
{
    m_fd = ::socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, NETLINK_ROUTE);
    if (m_fd < 0) {
        return;
    }
    sockaddr_nl addr = {};
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR | RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_ROUTE;
    if (::bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::close(m_fd);
        m_fd = -1;
        return;
    }
    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &NetworkMonitor::onReadable);
}

NetworkMonitor::~NetworkMonitor()
{
    delete m_notifier;
    if (m_fd >= 0) {
        ::close(m_fd);
    }
}

void NetworkMonitor::onReadable()
{
    // esvazia o socket; uma rajada de mensagens vira um aviso (a janela ainda espera 1 s,
    // como no OnNetworkChange original)
    char buf[8192];
    bool any = false;
    while (::recv(m_fd, buf, sizeof(buf), 0) > 0) {
        any = true;
    }
    if (any) {
        emit changed();
    }
}

// ---------------------------------------------------------------- energia

PowerMonitor::PowerMonitor(QObject* parent) : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        return;
    }
    m_connected = bus.connect(QLatin1String(kLogindService), QLatin1String(kLogindPath), QLatin1String(kLogindManager),
                              QStringLiteral("PrepareForSleep"), this, SLOT(onPrepareForSleep(bool)));
    if (m_connected) {
        takeSleepDelay();
    }
    // WM_WTSSESSION_CHANGE (WTS_CONSOLE_CONNECT): a sessão deste processo volta a ser a ativa
    // (troca de usuário, retorno ao terminal) — propriedade Active da sessão no logind
    QDBusReply<QDBusObjectPath> session = Logind(QStringLiteral("GetSessionByPID"), {(uint) ::getpid()});
    if (session.isValid()) {
        m_sessionPath = session.value().path();
        bus.connect(QLatin1String(kLogindService), m_sessionPath, QStringLiteral("org.freedesktop.DBus.Properties"),
                    QStringLiteral("PropertiesChanged"), this, SLOT(onSessionPropertiesChanged(QString, QVariantMap, QStringList)));
    }
}

void PowerMonitor::onSessionPropertiesChanged(const QString& iface, const QVariantMap& changed, const QStringList&)
{
    if (iface == QLatin1String("org.freedesktop.login1.Session") && changed.contains(QStringLiteral("Active")) &&
        changed.value(QStringLiteral("Active")).toBool()) {
        emit sessionActivated();
    }
}

PowerMonitor::~PowerMonitor()
{
    setCallActive(false);
}

void PowerMonitor::takeSleepDelay()
{
    QDBusReply<QDBusUnixFileDescriptor> reply =
        Logind(QStringLiteral("Inhibit"), {QStringLiteral("sleep"), QStringLiteral("MySimpleSIP"),
                                           QStringLiteral("Encerrar a sessão SIP antes de suspender"), QStringLiteral("delay")});
    m_sleepDelay = reply.isValid() ? reply.value() : QDBusUnixFileDescriptor();
}

void PowerMonitor::onPrepareForSleep(bool start)
{
    if (start) {
        emit aboutToSleep();
        m_sleepDelay = QDBusUnixFileDescriptor(); // libera a suspensão
    }
    else {
        emit resumed();
        takeSleepDelay();
    }
}

void PowerMonitor::setCallActive(bool active)
{
    if (active == callInhibited()) {
        return;
    }
    if (active) {
        // gnome-session: 4 = suspender, 8 = ociosidade (tela e suspensão automática)
        QDBusReply<uint> cookie = GnomeSession(QStringLiteral("Inhibit"), {QStringLiteral("mysimplesip"), uint(0),
                                                                            QStringLiteral("Chamada em andamento"), uint(4 | 8)});
        if (cookie.isValid() && cookie.value()) {
            m_gnomeCookie = cookie.value();
            return;
        }
        QDBusReply<QDBusUnixFileDescriptor> lock =
            Logind(QStringLiteral("Inhibit"), {QStringLiteral("idle:sleep"), QStringLiteral("MySimpleSIP"),
                                               QStringLiteral("Chamada em andamento"), QStringLiteral("block")});
        if (lock.isValid()) {
            m_callLock = lock.value();
        }
    }
    else {
        if (m_gnomeCookie) {
            GnomeSession(QStringLiteral("Uninhibit"), {m_gnomeCookie});
            m_gnomeCookie = 0;
        }
        m_callLock = QDBusUnixFileDescriptor();
    }
}

// ---------------------------------------------------------------- ociosidade

qint64 SessionIdleTimeMs()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return -1;
    }
    QDBusReply<qulonglong> reply = Call(bus, QStringLiteral("org.gnome.Mutter.IdleMonitor"),
                                        QStringLiteral("/org/gnome/Mutter/IdleMonitor/Core"),
                                        QStringLiteral("org.gnome.Mutter.IdleMonitor"), QStringLiteral("GetIdletime"));
    return reply.isValid() ? qint64(reply.value()) : -1;
}
