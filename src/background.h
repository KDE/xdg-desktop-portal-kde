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

namespace KWayland {
    namespace Client {
        class PlasmaWindow;
    }
}

class BackgroundPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Background")
public:
    explicit BackgroundPortal(QObject *parent);
    ~BackgroundPortal();

    enum ApplicationState {
        Background = 0,
        Running = 1,
        Active = 2,
    };

    enum AutostartFlag {
        None = 0x0,
        Activatable = 0x1,
    };
    Q_DECLARE_FLAGS(AutostartFlags, AutostartFlag)

    enum NotifyResult {
        Forbid = 0,
        Allow = 1,
        Ignore = 2,
    };

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

private:
    void addWindow(KWayland::Client::PlasmaWindow *window);
    void setActiveWindow(const QString &appId, bool active);

    uint m_notificationCounter = 0;
    QList<KWayland::Client::PlasmaWindow*> m_windows;
    QVariantMap m_appStates;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(BackgroundPortal::AutostartFlags)

#endif // XDG_DESKTOP_PORTAL_KDE_BACKGROUND_H


