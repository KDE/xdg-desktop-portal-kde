/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_SESSION_H
#define XDG_DESKTOP_PORTAL_KDE_SESSION_H

#include <QDBusVirtualObject>
#include <QObject>

#include "remotedesktop.h"
#include "screencast.h"
#include "waylandintegration.h"

class Session : public QDBusVirtualObject
{
    Q_OBJECT
public:
    explicit Session(QObject *parent = nullptr, const QString &appId = QString(), const QString &path = QString());
    ~Session() override;

    enum SessionType {
        ScreenCast = 0,
        RemoteDesktop = 1,
    };

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override;
    QString introspect(const QString &path) const override;

    bool close();
    virtual SessionType type() const = 0;

    static Session *createSession(QObject *parent, SessionType type, const QString &appId, const QString &path);
    static Session *getSession(const QString &sessionHandle);

Q_SIGNALS:
    void closed();

private:
    const QString m_appId;
    const QString m_path;
};

class ScreenCastSession : public Session
{
    Q_OBJECT
public:
    explicit ScreenCastSession(QObject *parent = nullptr, const QString &appId = QString(), const QString &path = QString());
    ~ScreenCastSession() override;

    void setOptions(const QVariantMap &options);

    ScreenCastPortal::CursorModes cursorMode() const;
    bool multipleSources() const;
    ScreenCastPortal::SourceType types() const;

    SessionType type() const override
    {
        return SessionType::ScreenCast;
    }

    WaylandIntegration::Streams streams() const
    {
        return m_streams;
    }
    void setStreams(const WaylandIntegration::Streams &streams)
    {
        m_streams = streams;
    }

private:
    bool m_multipleSources;
    ScreenCastPortal::CursorModes m_cursorMode;
    ScreenCastPortal::SourceType m_types;

    WaylandIntegration::Streams m_streams;
};

class RemoteDesktopSession : public ScreenCastSession
{
    Q_OBJECT
public:
    explicit RemoteDesktopSession(QObject *parent = nullptr, const QString &appId = QString(), const QString &path = QString());
    ~RemoteDesktopSession() override;

    RemoteDesktopPortal::DeviceTypes deviceTypes() const;
    void setDeviceTypes(RemoteDesktopPortal::DeviceTypes deviceTypes);

    bool screenSharingEnabled() const;
    void setScreenSharingEnabled(bool enabled);

    SessionType type() const override
    {
        return SessionType::RemoteDesktop;
    }

private:
    bool m_screenSharingEnabled;
    RemoteDesktopPortal::DeviceTypes m_deviceTypes;
};

#endif // XDG_DESKTOP_PORTAL_KDE_SESSION_H
