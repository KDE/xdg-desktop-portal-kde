/*
 * Copyright © 2018 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_SESSION_H
#define XDG_DESKTOP_PORTAL_KDE_SESSION_H

#include <QObject>
#include <QDBusVirtualObject>

#include "remotedesktop.h"
#include "screencast.h"

class Session : public QDBusVirtualObject
{
    Q_OBJECT
public:
    explicit Session(QObject *parent = nullptr, const QString &appId = QString(), const QString &path = QString());
    ~Session();

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
    ~ScreenCastSession();

    void setOptions(const QVariantMap &options);

    ScreenCastPortal::CursorModes cursorMode() const;
    bool multipleSources() const;
    ScreenCastPortal::SourceType types() const;

    SessionType type() const override { return SessionType::ScreenCast; }

private:
    bool m_multipleSources;
    ScreenCastPortal::CursorModes m_cursorMode;
    ScreenCastPortal::SourceType m_types;
};

class RemoteDesktopSession : public ScreenCastSession
{
    Q_OBJECT
public:
    explicit RemoteDesktopSession(QObject *parent = nullptr, const QString &appId = QString(), const QString &path = QString());
    ~RemoteDesktopSession();

    RemoteDesktopPortal::DeviceTypes deviceTypes() const;
    void setDeviceTypes(RemoteDesktopPortal::DeviceTypes deviceTypes);

    bool screenSharingEnabled() const;
    void setScreenSharingEnabled(bool enabled);

    SessionType type() const override { return SessionType::RemoteDesktop; }

private:
    bool m_screenSharingEnabled;
    RemoteDesktopPortal::DeviceTypes m_deviceTypes;
};

#endif // XDG_DESKTOP_PORTAL_KDE_SESSION_H

