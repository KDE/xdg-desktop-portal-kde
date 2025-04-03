/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_INHIBIT_H
#define XDG_DESKTOP_PORTAL_KDE_INHIBIT_H

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

#include "request.h"

class SessionStateMonitorSession;

class InhibitPortal : public QDBusAbstractAdaptor, public QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Inhibit")
    Q_PROPERTY(uint version READ version CONSTANT)

public:
    explicit InhibitPortal(QObject *parent);

    uint version() const
    {
        return 3;
    }

public Q_SLOTS:
    void Inhibit(const QDBusObjectPath &handle, const QString &app_id, const QString &window, uint flags, const QVariantMap &options);
    uint CreateMonitor(const QDBusObjectPath &handle, const QDBusObjectPath &session_handle, const QString &app_id, const QString &window);
    void QueryEndResponse(const QDBusObjectPath &handle);

    // called by PlasmaShutdown. Should clearly be on a different iface, but for testing.. yolo!
    void queryCanShutDown(const QDBusMessage &message, bool &reply);
    void endSession();

private:
    void queryCanShutDownComplete();
    QHash<QDBusObjectPath, SessionStateMonitorSession *> m_monitors;
    QHash<QDBusObjectPath, QString /*appId*/> m_appsBlockingLogout;
    QDBusMessage m_pendingCanShutDownReply;
};

#endif // XDG_DESKTOP_PORTAL_KDE_INHIBIT_H
