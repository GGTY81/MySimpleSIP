/*
 * MySimpleSIP — teclas de mídia pelo MPRIS (equivalente ao gancho de shell do Windows).
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// MySimpleSIP: no Windows o MicroSIP recebe as teclas de mídia (tocar/pausar/parar do
// teclado e de fones Bluetooth) pelo gancho de shell (RegisterShellHookWindow →
// HSHELL_APPCOMMAND). No GNOME as teclas vão para o player MPRIS ativo (o
// gnome-settings-daemon escolhe o último que passou a "tocar"): o aplicativo publica um
// player MPRIS que fica "tocando" durante as chamadas e repassa os comandos como
// APPCOMMAND_MEDIA_* (sinal appCommand).

#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QObject>
#include <QVariantMap>

class MediaKeys : public QObject {
    Q_OBJECT
public:
    explicit MediaKeys(QObject* parent = nullptr);
    ~MediaKeys() override;
    // publica (ou retira) o player no barramento da sessão
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_registered; }
    // chamada tocando ou em andamento: o player passa a "tocar" e vira o destino das teclas
    void setCallActive(bool active);
    bool callActive() const { return m_callActive; }
    QString serviceName() const { return m_service; }

signals:
    void appCommand(int nCmd); // APPCOMMAND_MEDIA_PLAY, _PLAY_PAUSE, _PAUSE, _STOP...
    void raiseRequested();

private:
    void emitPlayerChanged();
    bool m_registered = false;
    bool m_callActive = false;
    QString m_service;
    QObject* m_object = nullptr; // /org/mpris/MediaPlayer2 com os dois adaptadores
    friend class MprisPlayerAdaptor;
};

// org.mpris.MediaPlayer2
class MprisRootAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ canQuit)
    Q_PROPERTY(bool CanRaise READ canRaise)
    Q_PROPERTY(bool HasTrackList READ hasTrackList)
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes)
public:
    MprisRootAdaptor(QObject* object, MediaKeys* keys) : QDBusAbstractAdaptor(object), m_keys(keys) {}
    bool canQuit() const { return false; }
    bool canRaise() const { return true; }
    bool hasTrackList() const { return false; }
    QString identity() const;
    QString desktopEntry() const { return QStringLiteral("mysimplesip"); }
    QStringList supportedUriSchemes() const { return {}; }
    QStringList supportedMimeTypes() const { return {}; }

public slots:
    void Raise();
    void Quit() {}

private:
    MediaKeys* m_keys;
};

// org.mpris.MediaPlayer2.Player
class MprisPlayerAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(double Rate READ rate)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(double Volume READ volume)
    Q_PROPERTY(qlonglong Position READ position)
    Q_PROPERTY(double MinimumRate READ rate)
    Q_PROPERTY(double MaximumRate READ rate)
    Q_PROPERTY(bool CanGoNext READ no)
    Q_PROPERTY(bool CanGoPrevious READ no)
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanPause READ canPlay)
    Q_PROPERTY(bool CanSeek READ no)
    Q_PROPERTY(bool CanControl READ yes)
public:
    MprisPlayerAdaptor(QObject* object, MediaKeys* keys) : QDBusAbstractAdaptor(object), m_keys(keys) {}
    QString playbackStatus() const;
    double rate() const { return 1.0; }
    QVariantMap metadata() const { return {}; }
    double volume() const { return 1.0; }
    qlonglong position() const { return 0; }
    bool no() const { return false; }
    bool yes() const { return true; }
    // fora de chamada não se apresenta como player (o GNOME só mostra o que pode tocar)
    bool canPlay() const;

public slots:
    void Next() {}
    void Previous() {}
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong) {}
    void SetPosition(const QDBusObjectPath&, qlonglong) {}
    void OpenUri(const QString&) {}

private:
    MediaKeys* m_keys;
};
