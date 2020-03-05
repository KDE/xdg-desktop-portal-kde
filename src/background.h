/*
 * Copyright © 2020 Red Hat, Inc
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

#ifndef XDG_DESKTOP_PORTAL_KDE_BACKGROUND_H
#define XDG_DESKTOP_PORTAL_KDE_BACKGROUND_H

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

class BackgroundPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Background")
public:
    explicit BackgroundPortal(QObject *parent);
    ~BackgroundPortal();

    enum AutostartFlag {
        None = 0x0,
        Activatable = 0x1
    };
    Q_DECLARE_FLAGS(AutostartFlags, AutostartFlag)

public Q_SLOTS:
    QVariantMap GetAppState();

    uint NotifyBackground(const QDBusObjectPath &handle,
                          const QString &app_id,
                          const QString &name,
                          QVariantMap &results);

    bool EnableAutostart(const QString &app_id,
                         bool enable,
                         const QStringList &commandline,
                         uint flags);
Q_SIGNALS:
    void RunningApplicationsChanged();
};
Q_DECLARE_OPERATORS_FOR_FLAGS(BackgroundPortal::AutostartFlags)

#endif // XDG_DESKTOP_PORTAL_KDE_BACKGROUND_H


