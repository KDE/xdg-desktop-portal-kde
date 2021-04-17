/*
 * Copyright (c) 2020 Kai Uwe Broulik <kde@broulik.de>
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
 */

#include "notificationinhibition.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QLoggingCategory>
#include <QPointer>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeNotificationInhibition, "xdp-kde-notificationinhibition")

static const auto s_notificationService = QStringLiteral("org.freedesktop.Notifications");
static const auto s_notificationPath = QStringLiteral("/org/freedesktop/Notifications");
static const auto s_notificationInterface = QStringLiteral("org.freedesktop.Notifications");

NotificationInhibition::NotificationInhibition(const QString &appId, const QString &reason, QObject *parent)
    : QObject(parent)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(s_notificationService, s_notificationPath, s_notificationInterface, QStringLiteral("Inhibit"));
    msg.setArguments({appId, reason, QVariantMap()});

    QPointer<NotificationInhibition> guardedThis(this);

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(watcher, &QDBusPendingCallWatcher::finished, [guardedThis, appId, reason](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<uint> reply = *watcher;
        watcher->deleteLater();

        if (reply.isError()) {
            qCDebug(XdgDesktopPortalKdeNotificationInhibition) << "Failed to inhibit: " << reply.error().message();
            return;
        }

        const auto cookie = reply.value();

        // In case the inhibition was revoked again before the async DBus reply arrived
        if (guardedThis) {
            qCDebug(XdgDesktopPortalKdeNotificationInhibition) << "Inhibiting notifications for" << appId << "with reason" << reason << "and cookie" << cookie;
            guardedThis->m_cookie = cookie;
        } else {
            uninhibit(cookie);
        }
    });
}

NotificationInhibition::~NotificationInhibition()
{
    if (m_cookie) {
        uninhibit(m_cookie);
    }
}

void NotificationInhibition::uninhibit(uint cookie)
{
    qCDebug(XdgDesktopPortalKdeNotificationInhibition) << "Removing inhibition with cookie" << cookie;
    QDBusMessage msg = QDBusMessage::createMethodCall(s_notificationService, s_notificationPath, s_notificationInterface, QStringLiteral("UnInhibit"));
    msg.setArguments({cookie});
    QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
}
