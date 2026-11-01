/*
 * MySimpleSIP — teclas de mídia pelo MPRIS (equivalente ao gancho de shell do Windows).
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "MediaKeys.h"

#include "Win32Window.h"

#include <QDBusConnection>
#include <QDBusMessage>

#include <unistd.h>

static const char* kMprisPath = "/org/mpris/MediaPlayer2";

MediaKeys::MediaKeys(QObject* parent) : QObject(parent)
{
    // uma instância por perfil: o PID distingue execuções com outros arquivos INI
    m_service = QStringLiteral("org.mpris.MediaPlayer2.mysimplesip.instance%1").arg(::getpid());
}

MediaKeys::~MediaKeys()
{
    setEnabled(false);
}

void MediaKeys::setEnabled(bool enabled)
{
    if (enabled == m_registered) {
        return;
    }
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (enabled) {
        if (!bus.isConnected()) {
            return;
        }
        m_object = new QObject(this);
        new MprisRootAdaptor(m_object, this);
        new MprisPlayerAdaptor(m_object, this);
        if (!bus.registerObject(QLatin1String(kMprisPath), m_object) || !bus.registerService(m_service)) {
            bus.unregisterObject(QLatin1String(kMprisPath));
            delete m_object;
            m_object = nullptr;
            return;
        }
        m_registered = true;
    }
    else {
        bus.unregisterService(m_service);
        bus.unregisterObject(QLatin1String(kMprisPath));
        delete m_object;
        m_object = nullptr;
        m_registered = false;
    }
}

void MediaKeys::setCallActive(bool active)
{
    if (active == m_callActive) {
        return;
    }
    m_callActive = active;
    emitPlayerChanged();
}

void MediaKeys::emitPlayerChanged()
{
    if (!m_registered) {
        return;
    }
    // os adaptadores do Qt não emitem PropertiesChanged sozinhos
    QDBusMessage signal = QDBusMessage::createSignal(QLatin1String(kMprisPath), QStringLiteral("org.freedesktop.DBus.Properties"),
                                                     QStringLiteral("PropertiesChanged"));
    QVariantMap changed;
    changed.insert(QStringLiteral("PlaybackStatus"), m_callActive ? QStringLiteral("Playing") : QStringLiteral("Stopped"));
    changed.insert(QStringLiteral("CanPlay"), m_callActive);
    changed.insert(QStringLiteral("CanPause"), m_callActive);
    signal << QStringLiteral("org.mpris.MediaPlayer2.Player") << changed << QStringList();
    QDBusConnection::sessionBus().send(signal);
}

QString MprisRootAdaptor::identity() const
{
    return QStringLiteral("MySimpleSIP");
}

void MprisRootAdaptor::Raise()
{
    emit m_keys->raiseRequested();
}

QString MprisPlayerAdaptor::playbackStatus() const
{
    return m_keys->callActive() ? QStringLiteral("Playing") : QStringLiteral("Stopped");
}

bool MprisPlayerAdaptor::canPlay() const
{
    return m_keys->callActive();
}

void MprisPlayerAdaptor::Pause()
{
    emit m_keys->appCommand(APPCOMMAND_MEDIA_PAUSE);
}

void MprisPlayerAdaptor::PlayPause()
{
    emit m_keys->appCommand(APPCOMMAND_MEDIA_PLAY_PAUSE);
}

void MprisPlayerAdaptor::Stop()
{
    emit m_keys->appCommand(APPCOMMAND_MEDIA_STOP);
}

void MprisPlayerAdaptor::Play()
{
    emit m_keys->appCommand(APPCOMMAND_MEDIA_PLAY);
}
