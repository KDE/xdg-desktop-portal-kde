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

using namespace Qt::StringLiterals;

Request::Request(const QDBusObjectPath &handle, QObject *parent, const QString &portalName, const QVariant &data)
    : QObject(parent)
    , m_data(data)
    , m_portalName(portalName)
    , m_path(handle)
{
    auto sessionBus = QDBusConnection::sessionBus();
    if (!sessionBus.registerObject(handle.path(), u"org.freedesktop.impl.portal.Request"_s, this, QDBusConnection::ExportAllSlots)) {
        qCDebug(XdgRequestKdeRequest) << sessionBus.lastError().message();
        qCDebug(XdgRequestKdeRequest) << "Failed to register request object for" << handle.path();
        deleteLater();
    }
}

void Request::close()
{
    Q_EMIT closeRequested();
    QDBusConnection::sessionBus().unregisterObject(m_path.path());
    deleteLater();
}
