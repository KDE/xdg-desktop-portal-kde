/*
 * SPDX-FileCopyrightText: 2016 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2026 David Redondo <kde@david-redondo.de>
 */

#include "request.h"
#include "request_debug.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

Request::Request(const QDBusObjectPath &handle, QObject *parent)
    : QObject(parent)
{
    auto sessionBus = QDBusConnection::sessionBus();
    if (sessionBus.registerObject(handle.path(), this, QDBusConnection::ExportAllProperties | QDBusConnection::ExportScriptableContents)) {
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

#include "moc_request.cpp"
