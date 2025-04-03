/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2025 David Edmundson <davidedmundson@kde.org>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_INHIBIT_H
#define XDG_DESKTOP_PORTAL_KDE_INHIBIT_H

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

#include "request.h"

class SessionStateMonitorSession;

class InhibitPortal : public QDBusAbstractAdaptor
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

    void queryCanEndSession(const QDBusMessage &message);
    void endSession();

public Q_SLOTS:
    void Inhibit(const QDBusObjectPath &handle, const QString &app_id, const QString &window, uint flags, const QVariantMap &options);
    uint CreateMonitor(const QDBusObjectPath &handle, const QDBusObjectPath &session_handle, const QString &app_id, const QString &window);
    void QueryEndResponse(const QDBusObjectPath &handle);

private Q_SLOTS:
    void lockScreenStateChanged(bool state);

Q_SIGNALS:
    void StateChanged(const QDBusObjectPath &path, const QVariantMap &state);

private:
    enum LockScreenState {
        LockScreenInactive,
        LockScreenActive,
    };
    enum SessionState {
        Running = 1,
        QueryEnd = 2,
        Ending = 3,
    };

    void updateState(SessionStateMonitorSession *session, LockScreenState, SessionState state);
    void queryCanShutDownComplete();
    QHash<QDBusObjectPath, SessionStateMonitorSession *> m_monitors;
    QHash<QDBusObjectPath, QString /*appId*/> m_appsBlockingLogout;
    QDBusMessage m_pendingCanShutDownReply;
    LockScreenState m_lockState = LockScreenInactive;
    SessionState m_sessionState = Running;
};

#endif // XDG_DESKTOP_PORTAL_KDE_INHIBIT_H
