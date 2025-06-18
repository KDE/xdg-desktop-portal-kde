/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
*/

#ifndef XDG_DESKTOP_PORTAL_KDE_LOCATION_H
#define XDG_DESKTOP_PORTAL_KDE_LOCATION_H

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

class QDBusMessage;

class LocationPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Location")
    Q_PROPERTY(uint version READ version CONSTANT)
public:
    explicit LocationPortal(QObject *parent);

    enum LocationAccuracy {
        None = 0x0,
        Country = 0x1,
        City = 0x2,
        Neighborhood = 0x3,
        Street = 0x4,
        Exact = 0x5,
    };
    Q_DECLARE_FLAGS(LocationAccuracies, LocationAccuracy)

    uint version() const
    {
        return 1;
    }

public Q_SLOTS:
    uint CreateSession(const QDBusObjectPath &handle,
                       const QDBusObjectPath &session_handle,
                       const QString &app_id,
                       const QVariantMap &options,
                       QVariantMap &results);

    void Start(const QDBusObjectPath &handle,
               const QDBusObjectPath &session_handle,
               const QString &app_id,
               const QString &parent_window,
               const QVariantMap &options,
               const QDBusMessage &message,
               uint &replyResponse,
               QVariantMap &replyResults);

Q_SIGNALS:
    void LocationUpdated(const QDBusObjectPath &session_handle, const QVariantMap &location);
};

#endif // XDG_DESKTOP_PORTAL_KDE_LOCATION_H
