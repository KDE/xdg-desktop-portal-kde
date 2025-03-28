/*
 * SPDX-FileCopyrightText: 2016 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016 Jan Grulich <jgrulich@redhat.com>
 */

#include "request.h"
#include "request_debug.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

Request::Request(const QDBusObjectPath &handle, QObject *parent, const QString &portalName, const QVariant &data)
    : QDBusVirtualObject(parent)
    , m_data(data)
    , m_portalName(portalName)
{
    auto sessionBus = QDBusConnection::sessionBus();
    if (sessionBus.registerVirtualObject(handle.path(), this, QDBusConnection::VirtualObjectRegisterOption::SubPath)) {
        connect(this, &Request::closeRequested, this, [this, handle]() {
            QDBusConnection::sessionBus().unregisterObject(handle.path());
            deleteLater();
        });
    } else {
        qCDebug(XdgRequestKdeRequest) << sessionBus.lastError().message();
        qCDebug(XdgRequestKdeRequest) << "Failed to register request object for" << handle.path();
        deleteLater();
    }
}

bool Request::handleMessage(const QDBusMessage &message, const QDBusConnection &connection)
{
    Q_UNUSED(connection);

    /* Check to make sure we're getting properties on our interface */
    if (message.type() != QDBusMessage::MessageType::MethodCallMessage) {
        return false;
    }

    qCDebug(XdgRequestKdeRequest) << message.interface();
    qCDebug(XdgRequestKdeRequest) << message.member();
    qCDebug(XdgRequestKdeRequest) << message.path();

    if (message.interface() == QLatin1String("org.freedesktop.impl.portal.Request")) {
        if (message.member() == QLatin1String("Close")) {
            Q_EMIT closeRequested();
            QDBusMessage reply = message.createReply();
            return connection.send(reply);
        }
    }

    return true;
}

QString Request::introspect(const QString &path) const
{
    QString nodes;

    if (path.startsWith(QLatin1String("/org/freedesktop/portal/desktop/request/"))) {
        nodes = QStringLiteral(
            "<interface name=\"org.freedesktop.impl.portal.Request\">"
            "    <method name=\"Close\">"
            "    </method>"
            "</interface>");
    }

    qCDebug(XdgRequestKdeRequest) << nodes;

    return nodes;
}
